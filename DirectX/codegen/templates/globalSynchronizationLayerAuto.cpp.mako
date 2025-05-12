// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "globalSynchronizationLayerAuto.h"

namespace gits {
namespace DirectX {

void GlobalSynchronizationLayer::pre(IUnknownQueryInterfaceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(IUnknownQueryInterfaceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(IUnknownAddRefCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(IUnknownAddRefCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(IUnknownReleaseCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(IUnknownReleaseCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::pre(INTC_D3D12_CreateHeapCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  mutex_.unlock();
}
%for function in functions:

void GlobalSynchronizationLayer::pre(${function.name}Command& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(${function.name}Command& command) {
  mutex_.unlock();
}
%endfor
%for interface in interfaces:
%for function in interface.functions:

void GlobalSynchronizationLayer::pre(${interface.name}${function.name}Command& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::post(${interface.name}${function.name}Command& command) {
  mutex_.unlock();
}
%endfor
%endfor

} // namespace DirectX
} // namespace gits
