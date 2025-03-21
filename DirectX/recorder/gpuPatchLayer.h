// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "gpuPatchDump.h"
#include "gpuAddressService.h"

#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace gits {
namespace DirectX {

class GpuPatchLayer : public Layer {
public:
  GpuPatchLayer(GpuAddressService& gpuAddressService);
  void post(ID3D12DeviceCreateCommandSignatureCommand& c) override;
  void post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;
  void post(IUnknownReleaseCommand& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void post(ID3D12Device10CreateCommittedResource3Command& c) override;

private:
  std::unordered_map<unsigned, D3D12_COMMAND_SIGNATURE_DESC> commandSignatures_;
  GpuPatchDump gpuPatchDump_;
  GpuAddressService& gpuAddressService_;
  std::unordered_set<unsigned> genericReadResources_;
  std::unordered_map<unsigned, ID3D12Resource*> resourcesByKey_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
