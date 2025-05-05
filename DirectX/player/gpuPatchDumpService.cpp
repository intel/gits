// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchDumpService.h"
#include "gits.h"
#include "command.h"
#include "configurationLib.h"

#include <sstream>

namespace gits {
namespace DirectX {

GpuPatchDumpService::GpuPatchDumpService(GpuPatchAddressService& addressService,
                                         ShaderIdentifierService& shaderIdentifierService,
                                         GpuPatchDescriptorHandleService& descriptorHandleService)
    : resourceDump_(addressService, shaderIdentifierService, descriptorHandleService),
      executeIndirectDump_(addressService),
      raytracingKeys_(Configurator::Get().directx.features.raytracingDump.commandKeys),
      executeIndirectKeys_(Configurator::Get().directx.features.executeIndirectDump.commandKeys) {

  auto& configRaytracing = Configurator::Get().directx.features.raytracingDump;
  dumpInstancesPre_ = configRaytracing.instancesPre;
  dumpInstancesPost_ = configRaytracing.instancesPost;
  dumpBindingTablesPre_ = configRaytracing.bindingTablesPre;
  dumpBindingTablesPost_ = configRaytracing.bindingTablesPost;

  auto& configExecuteIndirect = Configurator::Get().directx.features.executeIndirectDump;
  dumpArgumentBufferPre_ = configExecuteIndirect.argumentBufferPre;
  dumpArgumentBufferPost_ = configExecuteIndirect.argumentBufferPost;

  if (dumpInstancesPre_ || dumpInstancesPost_ || dumpBindingTablesPre_ || dumpBindingTablesPost_ ||
      dumpArgumentBufferPre_ || dumpArgumentBufferPost_) {
    addressService.enableDumpLookup();
  } else {
    return;
  }

  if (dumpBindingTablesPre_ || dumpBindingTablesPost_) {
    shaderIdentifierService.enableDumpLookup();
    descriptorHandleService.enableDumpLookup();
  }

  std::filesystem::path dumpPath = Configurator::Get().common.player.outputDir.empty()
                                       ? Configurator::Get().common.player.streamDir / "resources"
                                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  dumpPath_ = dumpPath;
}

void GpuPatchDumpService::dumpInstances(ID3D12GraphicsCommandList* commandList,
                                        ID3D12Resource* resource,
                                        unsigned resourceKey,
                                        unsigned size,
                                        unsigned callKey,
                                        bool prePatch) {

  if (!(dumpInstancesPre_ && prePatch || dumpInstancesPost_ && !prePatch)) {
    return;
  }
  if (!raytracingKeys_.empty() && !raytracingKeys_.contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << dumpPath_ << L"/call_" << callKey << L"_instances_O" << resourceKey << L"_"
           << (prePatch ? L"pre_patch" : L"post_patch") << L".txt";

  resourceDump_.dumpResource(commandList, resource, 0, size, 0,
                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dumpName.str(),
                             RaytracingResourceDump::Instances, prePatch);
}

void GpuPatchDumpService::dumpInstancesArrayOfPointers(ID3D12GraphicsCommandList* commandList,
                                                       ID3D12Resource* resource,
                                                       unsigned resourceKey,
                                                       unsigned offset,
                                                       unsigned size,
                                                       D3D12_RESOURCE_STATES resourceState,
                                                       unsigned callKey,
                                                       bool prePatch) {
  if (!(dumpInstancesPre_ && prePatch || dumpInstancesPost_ && !prePatch)) {
    return;
  }
  if (!raytracingKeys_.empty() && !raytracingKeys_.contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << dumpPath_ << L"/call_" << callKey << L"_instances_array_of_pointers_O" << resourceKey
           << L"_" << (prePatch ? L"pre_patch" : L"post_patch") << L".txt";

  resourceDump_.dumpResource(commandList, resource, offset, size, 0, resourceState, dumpName.str(),
                             RaytracingResourceDump::InstancesArrayOfPointers, prePatch);
}

void GpuPatchDumpService::dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                           ID3D12Resource* resource,
                                           unsigned offset,
                                           unsigned size,
                                           unsigned stride,
                                           unsigned callKey,
                                           BindingTableType bindingTableType,
                                           bool prePatch) {

  if (!(dumpBindingTablesPre_ && prePatch || dumpBindingTablesPost_ && !prePatch)) {
    return;
  }
  if (!raytracingKeys_.empty() && !raytracingKeys_.contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << dumpPath_ << L"/call_" << callKey << L"_binding_table_";
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

  resourceDump_.dumpResource(commandList, resource, offset, size, stride,
                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dumpName.str(),
                             RaytracingResourceDump::BindingTable, prePatch);
}

void GpuPatchDumpService::dumpExecuteIndirectArgumentBuffer(
    ID3D12GraphicsCommandList* commandList,
    D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
    unsigned maxCommandCount,
    ID3D12Resource* argumentBuffer,
    unsigned argumentBufferOffset,
    D3D12_RESOURCE_STATES argumentBufferState,
    ID3D12Resource* countBuffer,
    unsigned countBufferOffset,
    D3D12_RESOURCE_STATES countBufferState,
    unsigned callKey,
    bool prePatch) {

  if (!(dumpArgumentBufferPre_ && prePatch || dumpArgumentBufferPost_ && !prePatch)) {
    return;
  }
  if (!executeIndirectKeys_.empty() && !executeIndirectKeys_.contains(callKey)) {
    return;
  }

  std::wstringstream dumpName;
  dumpName << dumpPath_ << L"/call_" << callKey << L"_execute_indirect_"
           << (prePatch ? L"pre_patch" : L"post_patch") << L".txt";

  executeIndirectDump_.dumpArgumentBuffer(commandList, commandSignature, maxCommandCount,
                                          argumentBuffer, argumentBufferOffset, argumentBufferState,
                                          countBuffer, countBufferOffset, countBufferState,
                                          dumpName.str(), prePatch);
}

void GpuPatchDumpService::executeCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum) {
  if (dumpInstancesPre_ || dumpInstancesPost_ || dumpBindingTablesPre_ || dumpBindingTablesPost_) {
    resourceDump_.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                      commandListNum);
  }
  if (dumpArgumentBufferPre_ || dumpArgumentBufferPost_) {
    executeIndirectDump_.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                             commandListNum);
  }
}

void GpuPatchDumpService::commandQueueWait(unsigned key,
                                           unsigned commandQueueKey,
                                           unsigned fenceKey,
                                           UINT64 fenceValue) {
  if (dumpInstancesPre_ || dumpInstancesPost_ || dumpBindingTablesPre_ || dumpBindingTablesPost_) {
    resourceDump_.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
  }
  if (dumpArgumentBufferPre_ || dumpArgumentBufferPost_) {
    executeIndirectDump_.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
  }
}

void GpuPatchDumpService::commandQueueSignal(unsigned key,
                                             unsigned commandQueueKey,
                                             unsigned fenceKey,
                                             UINT64 fenceValue) {
  if (dumpInstancesPre_ || dumpInstancesPost_ || dumpBindingTablesPre_ || dumpBindingTablesPost_) {
    resourceDump_.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
  }
  if (dumpArgumentBufferPre_ || dumpArgumentBufferPost_) {
    executeIndirectDump_.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
  }
}

void GpuPatchDumpService::fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  if (dumpInstancesPre_ || dumpInstancesPost_ || dumpBindingTablesPre_ || dumpBindingTablesPost_) {
    resourceDump_.fenceSignal(key, fenceKey, fenceValue);
  }
  if (dumpArgumentBufferPre_ || dumpArgumentBufferPost_) {
    executeIndirectDump_.fenceSignal(key, fenceKey, fenceValue);
  }
}

} // namespace DirectX
} // namespace gits
