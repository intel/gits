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
      m_GitsConfig(gitsConfig),
      m_ResourceStateTracker(resourceStateTracker),
      m_AddressService(addressService),
      m_ResourceByKey(resourceByKey) {
  if (gitsConfig.common.features.subcapture.enabled) {
    m_DumpDir = gitsConfig.common.player.subcapturePath;
    m_Frames = BitRange(gitsConfig.common.features.subcapture.frames);
    if (gitsConfig.common.features.subcapture.directx.commandListExecutions.empty()) {
      m_Executions = BitRange("all");
    } else {
      m_Executions = BitRange(gitsConfig.common.features.subcapture.directx.commandListExecutions);
    }
  } else {
    m_DumpDir = gitsConfig.common.player.streamDir;
    m_Frames = BitRange("all");
    m_Executions = BitRange("all");
  }
  m_DumpDir /= "cpu_patch";
  m_DumpDir /= "execute_indirect_dump";
  LOG_INFO << "CpuPatch - ExecuteIndirectDump location: " << m_DumpDir.string();
}

ExecuteIndirectDump::~ExecuteIndirectDump() {
  WaitUntilDumped();
  LOG_INFO << "CpuPatch - ExecuteIndirectDump - files dumped: " << std::to_string(m_NumFiles)
           << ", total size: " << FormatMemorySize(m_FilesTotalSize);
}

void ExecuteIndirectDump::ExecuteIndirect(
    ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  auto itCommandSignature = m_CommandSignatures.find(command.m_pCommandSignature.Key);
  GITS_ASSERT(itCommandSignature != m_CommandSignatures.end());
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

  Initialize();

  auto dumpBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS address, UINT64 size, UINT64 stride,
                              unsigned index, std::string type) {
    if (!address) {
      return;
    }
    CapturePlayerGpuAddressService::ResourceInfo* info =
        m_AddressService.GetResourceInfoByCaptureAddress(address);
    GITS_ASSERT(info);
    unsigned offset = address - info->CaptureStart;

    D3D12_RESOURCE_STATES state =
        GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, command.m_Object.Value,
                                info->Resource, offset, info->Key,
                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            .State;

    ExecuteIndirectDumpInfo* dumpInfo = new ExecuteIndirectDumpInfo();
    dumpInfo->Offset = offset;
    dumpInfo->Size = size;
    dumpInfo->DumpName = (m_DumpDir / "binding_tables" /
                          (std::to_string(command.Key) + "-" + std::to_string(index) + "-" + type))
                             .wstring();
    StageResource(command.m_Object.Value, info->Resource, state, *dumpInfo);
  };

  auto it = m_ExecuteIndirectDispatchRays.find(command.Key);
  if (it != m_ExecuteIndirectDispatchRays.end()) {
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
    info->Offset = command.m_ArgumentBufferOffset.Value;
    info->Size = commandSignature.ByteStride * command.m_MaxCommandCount.Value;
    info->DumpName = (m_DumpDir / (std::to_string(command.Key) + "-ArgumentBuffer")).wstring();

    ID3D12Resource* argumentBuffer = m_ResourceByKey[command.m_pArgumentBuffer.Key];
    GITS_ASSERT(argumentBuffer);
    StageResource(command.m_Object.Value, argumentBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                  *info);
  }

  if (command.m_pCountBuffer.Value) {
    ExecuteIndirectDumpInfo* info = new ExecuteIndirectDumpInfo();
    info->Offset = command.m_CountBufferOffset.Value;
    info->Size = sizeof(unsigned);
    info->DumpName = (m_DumpDir / (std::to_string(command.Key) + "-CountBuffer")).wstring();

    ID3D12Resource* countBuffer = m_ResourceByKey[command.m_pCountBuffer.Key];
    GITS_ASSERT(countBuffer);
    StageResource(command.m_Object.Value, countBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                  *info);
  }
}

void ExecuteIndirectDump::ExecuteCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum,
                                              unsigned frameCount,
                                              unsigned executeCount) {
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = m_StagedResources.find(commandLists[i]);
    if (it != m_StagedResources.end()) {
      for (DumpInfo* dumpInfo : it->second) {
        auto* info = static_cast<ExecuteIndirectDumpInfo*>(dumpInfo);
        info->FrameCount = frameCount;
        info->ExecuteCount = executeCount;
      }
    }
  }

  ResourceDump::ExecuteCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void ExecuteIndirectDump::CreateCommandSignature(
    ID3D12DeviceCreateCommandSignatureCommand& command) {
  m_CommandSignatures[command.m_ppvCommandSignature.Key].reset(
      new PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>(command.m_pDesc));
}

void ExecuteIndirectDump::DumpStagedResource(DumpInfo& dumpInfo) {
  {
    const auto& info = static_cast<ExecuteIndirectDumpInfo&>(dumpInfo);
    if (!m_Frames[info.FrameCount] || !m_Executions[info.ExecuteCount]) {
      return;
    }
  }

  void* data{};
  HRESULT hr = dumpInfo.StagingBuffer->Map(0, nullptr, &data);

  std::ofstream stream(dumpInfo.DumpName, std::ios_base::binary);
  stream.write(static_cast<char*>(data), dumpInfo.Size);

  dumpInfo.StagingBuffer->Unmap(0, nullptr);

  ++m_NumFiles;
  m_FilesTotalSize += dumpInfo.Size;
}

void ExecuteIndirectDump::Initialize() {
  if (m_Initialized) {
    return;
  }

  if (!m_DumpDir.empty() && !std::filesystem::exists(m_DumpDir)) {
    std::filesystem::create_directories(m_DumpDir);
    std::filesystem::create_directory(m_DumpDir / "binding_tables");
  }

  LoadExecuteIndirectDispatchRays();

  m_Initialized = true;
}

void ExecuteIndirectDump::LoadExecuteIndirectDispatchRays() {
  std::filesystem::path dumpPath = m_GitsConfig.common.player.streamDir;
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

    m_ExecuteIndirectDispatchRays[callKey].push_back(desc);
  }
}

} // namespace DirectX
} // namespace gits
