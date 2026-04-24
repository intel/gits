// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchDumpService.h"
#include "configurationLib.h"

#include <sstream>

namespace gits {
namespace DirectX {

GpuPatchDumpService::GpuPatchDumpService(
    CapturePlayerGpuAddressService& addressService,
    CapturePlayerShaderIdentifierService& shaderIdentifierService,
    CapturePlayerDescriptorHandleService& descriptorHandleService)
    : m_ResourceDump(addressService, shaderIdentifierService, descriptorHandleService),
      m_ExecuteIndirectDump(addressService),
      m_RaytracingKeys(Configurator::Get().directx.features.raytracingDump.commandKeys),
      m_ExecuteIndirectKeys(Configurator::Get().directx.features.executeIndirectDump.commandKeys) {

  auto& configRaytracing = Configurator::Get().directx.features.raytracingDump;
  m_DumpInstancesPre = configRaytracing.instancesPre;
  m_DumpInstancesPost = configRaytracing.instancesPost;
  m_DumpBindingTablesPre = configRaytracing.bindingTablesPre;
  m_DumpBindingTablesPost = configRaytracing.bindingTablesPost;

  auto& configExecuteIndirect = Configurator::Get().directx.features.executeIndirectDump;
  m_DumpArgumentBufferPre = configExecuteIndirect.argumentBufferPre;
  m_DumpArgumentBufferPost = configExecuteIndirect.argumentBufferPost;

  if (m_DumpInstancesPre || m_DumpInstancesPost || m_DumpBindingTablesPre ||
      m_DumpBindingTablesPost || m_DumpArgumentBufferPre || m_DumpArgumentBufferPost) {
    addressService.EnablePlayerAddressLookup();
  } else {
    return;
  }

  if (m_DumpBindingTablesPre || m_DumpBindingTablesPost) {
    shaderIdentifierService.EnablePlayerIdentifierLookup();
    descriptorHandleService.EnablePlayerHandleLookup();
  }

  std::filesystem::path dumpPath = Configurator::Get().common.player.outputDir.empty()
                                       ? Configurator::Get().common.player.streamDir / "resources"
                                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  m_DumpPath = dumpPath;
}

void GpuPatchDumpService::DumpInstances(ID3D12GraphicsCommandList* commandList,
                                        ID3D12Resource* resource,
                                        unsigned ResourceKey,
                                        unsigned size,
                                        unsigned callKey,
                                        bool prePatch) {

  if (!(m_DumpInstancesPre && prePatch || m_DumpInstancesPost && !prePatch)) {
    return;
  }
  if (!m_RaytracingKeys.Empty() && !m_RaytracingKeys.Contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << m_DumpPath << L"/call_" << callKey << L"_instances_O" << ResourceKey << L"_"
           << (prePatch ? L"pre_patch" : L"post_patch") << L".txt";

  m_ResourceDump.DumpResource(commandList, resource, 0, size, 0,
                              D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dumpName.str(),
                              RaytracingResourceDump::DumpContentKind::Instances, prePatch);
}

void GpuPatchDumpService::DumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                                       ID3D12Resource* resource,
                                                       unsigned ResourceKey,
                                                       unsigned offset,
                                                       unsigned size,
                                                       D3D12_RESOURCE_STATES resourceState,
                                                       unsigned callKey,
                                                       bool prePatch) {
  if (!(m_DumpInstancesPre && prePatch || m_DumpInstancesPost && !prePatch)) {
    return;
  }
  if (!m_RaytracingKeys.Empty() && !m_RaytracingKeys.Contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << m_DumpPath << L"/call_" << callKey << L"_instances_array_of_pointers_O" << ResourceKey
           << L"_" << (prePatch ? L"pre_patch" : L"post_patch") << L".txt";

  m_ResourceDump.DumpResource(commandList, resource, offset, size, 0, resourceState, dumpName.str(),
                              RaytracingResourceDump::DumpContentKind::InstancesArrayOfPointers,
                              prePatch);
}

void GpuPatchDumpService::DumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                           ID3D12Resource* resource,
                                           unsigned offset,
                                           unsigned size,
                                           unsigned stride,
                                           unsigned callKey,
                                           BindingTableType bindingTableType,
                                           bool prePatch) {

  if (!(m_DumpBindingTablesPre && prePatch || m_DumpBindingTablesPost && !prePatch)) {
    return;
  }
  if (!m_RaytracingKeys.Empty() && !m_RaytracingKeys.Contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << m_DumpPath << L"/call_" << callKey << L"_binding_table_";
  switch (bindingTableType) {
  case RayGeneration:
    dumpName << L"ray_generation";
    break;
  case Miss:
    dumpName << L"miss";
    break;
  case HitGroup:
    dumpName << L"hit_group";
    break;
  case Callable:
    dumpName << L"callable";
    break;
  }
  dumpName << (prePatch ? L"_pre_patch" : L"_post_patch") << L".txt";

  m_ResourceDump.DumpResource(commandList, resource, offset, size, stride,
                              D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dumpName.str(),
                              RaytracingResourceDump::DumpContentKind::BindingTable, prePatch);
}

void GpuPatchDumpService::DumpExecuteIndirectArgumentBuffer(
    ID3D12GraphicsCommandList* commandList,
    const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
    unsigned maxCommandCount,
    ID3D12Resource* argumentBuffer,
    unsigned argumentBufferOffset,
    D3D12_RESOURCE_STATES argumentBufferState,
    ID3D12Resource* countBuffer,
    unsigned countBufferOffset,
    D3D12_RESOURCE_STATES countBufferState,
    unsigned callKey,
    bool prePatch) {

  if (!(m_DumpArgumentBufferPre && prePatch || m_DumpArgumentBufferPost && !prePatch)) {
    return;
  }
  if (!m_ExecuteIndirectKeys.Empty() && !m_ExecuteIndirectKeys.Contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << m_DumpPath << L"/call_" << callKey << L"_execute_indirect_"
           << (prePatch ? L"pre_patch" : L"post_patch") << L".txt";

  m_ExecuteIndirectDump.DumpArgumentBuffer(commandList, commandSignature, maxCommandCount,
                                           argumentBuffer, argumentBufferOffset,
                                           argumentBufferState, countBuffer, countBufferOffset,
                                           countBufferState, dumpName.str(), prePatch);
}

void GpuPatchDumpService::ExecuteCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum) {
  if (m_DumpInstancesPre || m_DumpInstancesPost || m_DumpBindingTablesPre ||
      m_DumpBindingTablesPost) {
    m_ResourceDump.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                       commandListNum);
  }
  if (m_DumpArgumentBufferPre || m_DumpArgumentBufferPost) {
    m_ExecuteIndirectDump.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                              commandListNum);
  }
}

void GpuPatchDumpService::CommandQueueWait(unsigned key,
                                           unsigned commandQueueKey,
                                           unsigned fenceKey,
                                           UINT64 fenceValue) {
  if (m_DumpInstancesPre || m_DumpInstancesPost || m_DumpBindingTablesPre ||
      m_DumpBindingTablesPost) {
    m_ResourceDump.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
  }
  if (m_DumpArgumentBufferPre || m_DumpArgumentBufferPost) {
    m_ExecuteIndirectDump.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
  }
}

void GpuPatchDumpService::CommandQueueSignal(unsigned key,
                                             unsigned commandQueueKey,
                                             unsigned fenceKey,
                                             UINT64 fenceValue) {
  if (m_DumpInstancesPre || m_DumpInstancesPost || m_DumpBindingTablesPre ||
      m_DumpBindingTablesPost) {
    m_ResourceDump.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
  }
  if (m_DumpArgumentBufferPre || m_DumpArgumentBufferPost) {
    m_ExecuteIndirectDump.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
  }
}

void GpuPatchDumpService::FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  if (m_DumpInstancesPre || m_DumpInstancesPost || m_DumpBindingTablesPre ||
      m_DumpBindingTablesPost) {
    m_ResourceDump.fenceSignal(key, fenceKey, fenceValue);
  }
  if (m_DumpArgumentBufferPre || m_DumpArgumentBufferPost) {
    m_ExecuteIndirectDump.fenceSignal(key, fenceKey, fenceValue);
  }
}

} // namespace DirectX
} // namespace gits
