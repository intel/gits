// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executeIndirectDump.h"
#include "pluginUtils.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace DirectX {

ExecuteIndirectDump::ExecuteIndirectDump(
    const Configuration& gitsConfig,
    ResourceStateTracker& resourceStateTracker,
    CapturePlayerGpuAddressService& addressService,
    std::unordered_map<unsigned, ID3D12Resource*>& resourceByKey)
    : ResourceDump(),
      gitsConfig_(gitsConfig),
      resourceStateTracker_(resourceStateTracker),
      addressService_(addressService),
      resourceByKey_(resourceByKey) {
  if (gitsConfig.directx.features.subcapture.enabled) {
    dumpDir_ = gitsConfig.common.player.subcapturePath;
    frames_ = BitRange(gitsConfig.directx.features.subcapture.frames);
    if (gitsConfig.directx.features.subcapture.commandListExecutions.empty()) {
      executions_ = BitRange("all");
    } else {
      executions_ = BitRange(gitsConfig.directx.features.subcapture.commandListExecutions);
    }
  } else {
    dumpDir_ = gitsConfig.common.player.streamDir;
    frames_ = BitRange("all");
    executions_ = BitRange("all");
  }
  dumpDir_ /= "cpu_patch";
  dumpDir_ /= "execute_indirect_dump";
  LOG_INFO << "CpuPatch - ExecuteIndirectDump location: " << dumpDir_.string();
}

ExecuteIndirectDump::~ExecuteIndirectDump() {
  waitUntilDumped();
  LOG_INFO << "CpuPatch - ExecuteIndirectDump - files dumped: " << std::to_string(numFiles_)
           << ", total size: " << FormatMemorySize(filesTotalSize_);
}

void ExecuteIndirectDump::executeIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  auto itCommandSignature = commandSignatures_.find(c.m_pCommandSignature.Key);
  GITS_ASSERT(itCommandSignature != commandSignatures_.end());
  const D3D12_COMMAND_SIGNATURE_DESC& commandSignature = *itCommandSignature->second->Value;

  bool view = false;
  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
    if (type == D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW) {
      view = true;
    } else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      raytracing = true;
    }
  }

  if (!view && !raytracing) {
    return;
  }

  initialize();

  auto dumpBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS address, UINT64 size, UINT64 stride,
                              unsigned index, std::string type) {
    if (!address) {
      return;
    }
    CapturePlayerGpuAddressService::ResourceInfo* info =
        addressService_.GetResourceInfoByCaptureAddress(address);
    GITS_ASSERT(info);
    unsigned offset = address - info->CaptureStart;

    D3D12_RESOURCE_STATES state =
        GetAdjustedCurrentState(resourceStateTracker_, addressService_, c.m_Object.Value,
                                info->Resource, offset, info->Key,
                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            .State;

    ExecuteIndirectDumpInfo* dumpInfo = new ExecuteIndirectDumpInfo();
    dumpInfo->offset = offset;
    dumpInfo->size = size;
    dumpInfo->dumpName = (dumpDir_ / "binding_tables" /
                          (std::to_string(c.Key) + "-" + std::to_string(index) + "-" + type))
                             .wstring();
    stageResource(c.m_Object.Value, info->Resource, state, *dumpInfo);
  };

  auto it = executeIndirectDispatchRays_.find(c.Key);
  if (it != executeIndirectDispatchRays_.end()) {
    for (unsigned i = 0; i < it->second.size(); ++i) {
      D3D12_DISPATCH_RAYS_DESC& desc = it->second.at(i);
      dumpBindingTable(desc.RayGenerationShaderRecord.StartAddress,
                       desc.RayGenerationShaderRecord.SizeInBytes,
                       desc.RayGenerationShaderRecord.SizeInBytes, i, "RayGeneration");
      dumpBindingTable(desc.MissShaderTable.StartAddress, desc.MissShaderTable.SizeInBytes,
                       desc.MissShaderTable.StrideInBytes, i, "Miss");
      dumpBindingTable(desc.HitGroupTable.StartAddress, desc.HitGroupTable.SizeInBytes,
                       desc.HitGroupTable.StrideInBytes, i, "HitGroup");
      dumpBindingTable(desc.CallableShaderTable.StartAddress, desc.CallableShaderTable.SizeInBytes,
                       desc.CallableShaderTable.StrideInBytes, i, "Callable");
    }
  }

  {
    ExecuteIndirectDumpInfo* info = new ExecuteIndirectDumpInfo();
    info->offset = c.m_ArgumentBufferOffset.Value;
    info->size = commandSignature.ByteStride * c.m_MaxCommandCount.Value;
    info->dumpName = (dumpDir_ / (std::to_string(c.Key) + "-ArgumentBuffer")).wstring();

    ID3D12Resource* argumentBuffer = resourceByKey_[c.m_pArgumentBuffer.Key];
    GITS_ASSERT(argumentBuffer);
    stageResource(c.m_Object.Value, argumentBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, *info);
  }

  if (c.m_pCountBuffer.Value) {
    ExecuteIndirectDumpInfo* info = new ExecuteIndirectDumpInfo();
    info->offset = c.m_CountBufferOffset.Value;
    info->size = sizeof(unsigned);
    info->dumpName = (dumpDir_ / (std::to_string(c.Key) + "-CountBuffer")).wstring();

    ID3D12Resource* countBuffer = resourceByKey_[c.m_pCountBuffer.Key];
    GITS_ASSERT(countBuffer);
    stageResource(c.m_Object.Value, countBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, *info);
  }
}

void ExecuteIndirectDump::executeCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum,
                                              unsigned frameCount,
                                              unsigned executeCount) {
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = stagedResources_.find(commandLists[i]);
    if (it != stagedResources_.end()) {
      for (DumpInfo* dumpInfo : it->second) {
        auto* info = static_cast<ExecuteIndirectDumpInfo*>(dumpInfo);
        info->frameCount = frameCount;
        info->executeCount = executeCount;
      }
    }
  }

  ResourceDump::executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void ExecuteIndirectDump::createCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& c) {
  commandSignatures_[c.m_ppvCommandSignature.Key].reset(
      new PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>(c.m_pDesc));
}

void ExecuteIndirectDump::dumpStagedResource(DumpInfo& dumpInfo) {
  {
    const auto& info = static_cast<ExecuteIndirectDumpInfo&>(dumpInfo);
    if (!frames_[info.frameCount] || !executions_[info.executeCount]) {
      return;
    }
  }

  void* data{};
  HRESULT hr = dumpInfo.stagingBuffer->Map(0, nullptr, &data);

  std::ofstream stream(dumpInfo.dumpName, std::ios_base::binary);
  stream.write(static_cast<char*>(data), dumpInfo.size);

  dumpInfo.stagingBuffer->Unmap(0, nullptr);

  ++numFiles_;
  filesTotalSize_ += dumpInfo.size;
}

void ExecuteIndirectDump::initialize() {
  if (initialized_) {
    return;
  }

  if (!dumpDir_.empty() && !std::filesystem::exists(dumpDir_)) {
    std::filesystem::create_directories(dumpDir_);
    std::filesystem::create_directory(dumpDir_ / "binding_tables");
  }

  loadExecuteIndirectDispatchRays();

  initialized_ = true;
}

void ExecuteIndirectDump::loadExecuteIndirectDispatchRays() {
  std::filesystem::path dumpPath = gitsConfig_.common.player.streamDir;
  std::ifstream stream(dumpPath / "executeIndirectRaytracing.txt");
  while (true) {
    unsigned callKey{};
    D3D12_DISPATCH_RAYS_DESC desc{};
    stream >> callKey;
    if (!stream) {
      break;
    }
    stream >> desc.RayGenerationShaderRecord.StartAddress >>
        desc.RayGenerationShaderRecord.SizeInBytes;
    stream >> desc.MissShaderTable.StartAddress >> desc.MissShaderTable.SizeInBytes >>
        desc.MissShaderTable.StrideInBytes;
    stream >> desc.HitGroupTable.StartAddress >> desc.HitGroupTable.SizeInBytes >>
        desc.HitGroupTable.StrideInBytes;
    stream >> desc.CallableShaderTable.StartAddress >> desc.CallableShaderTable.SizeInBytes >>
        desc.CallableShaderTable.StrideInBytes;
    stream >> desc.Width >> desc.Height >> desc.Depth;

    executeIndirectDispatchRays_[callKey].push_back(desc);
  }
}

} // namespace DirectX
} // namespace gits
