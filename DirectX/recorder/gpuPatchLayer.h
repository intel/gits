// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  void Post(ID3D12DeviceCreateCommandSignatureCommand& c) override;
  void Post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void Post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;
  void Post(IUnknownReleaseCommand& c) override;
  void Post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& c) override;

private:
  std::unordered_map<unsigned, D3D12_COMMAND_SIGNATURE_DESC> m_CommandSignatures;
  GpuPatchDump m_GpuPatchDump;
  GpuAddressService& m_GpuAddressService;
  std::unordered_set<unsigned> m_GenericReadResources;
  std::unordered_map<unsigned, ID3D12Resource*> m_ResourcesByKey;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
