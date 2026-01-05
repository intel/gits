// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "directx.h"

#include <map>
#include <set>
#include <vector>
#include <memory>
#include <mutex>

namespace gits {
namespace DirectX {

struct DescriptorState {
  enum StateId {
    D3D12_RENDERTARGETVIEW,
    D3D12_DEPTHSTENCILVIEW,
    D3D12_SHADERRESOURCEVIEW,
    D3D12_UNORDEREDACCESSVIEW,
    D3D12_CONSTANTBUFFERVIEW,
    D3D12_SAMPLER
  };
  DescriptorState(StateId id_) : id(id_) {}
  StateId id{};
  unsigned deviceKey{};
  D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor{};
  unsigned destDescriptorKey{};
  unsigned destDescriptorIndex{};
  unsigned resourceKey{};
  unsigned auxiliaryResourceKey{};
};

struct D3D12RenderTargetViewState : public DescriptorState {
  D3D12RenderTargetViewState() : DescriptorState(D3D12_RENDERTARGETVIEW) {}
  bool isDesc{};
  D3D12_RENDER_TARGET_VIEW_DESC desc{};
};

struct D3D12DepthStencilViewState : public DescriptorState {
  D3D12DepthStencilViewState() : DescriptorState(D3D12_DEPTHSTENCILVIEW) {}
  bool isDesc{};
  D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
};

struct D3D12ShaderResourceViewState : public DescriptorState {
  D3D12ShaderResourceViewState() : DescriptorState(D3D12_SHADERRESOURCEVIEW) {}
  bool isDesc{};
  D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
  unsigned raytracingLocationOffset{};
};

struct D3D12UnorderedAccessViewState : public DescriptorState {
  D3D12UnorderedAccessViewState() : DescriptorState(D3D12_UNORDEREDACCESSVIEW) {}
  bool isDesc{};
  D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
};

struct D3D12ConstantBufferViewState : public DescriptorState {
  D3D12ConstantBufferViewState() : DescriptorState(D3D12_CONSTANTBUFFERVIEW) {}
  bool isDesc{};
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
  unsigned bufferLocationOffset{};
};

struct D3D12SamplerState : public DescriptorState {
  D3D12SamplerState() : DescriptorState(D3D12_SAMPLER) {}
  D3D12_SAMPLER_DESC desc{};
};

class StateTrackingService;
class ResourceForCBVRestoreService;

class DescriptorService {
public:
  DescriptorService() {}
  DescriptorService(StateTrackingService* stateService,
                    ResourceForCBVRestoreService* resourceForCBVRestoreService)
      : stateService_(stateService), resourceForCBVRestoreService_(resourceForCBVRestoreService) {}
  void storeState(DescriptorState* state);
  void removeState(unsigned key);
  void restoreState();
  void copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  DescriptorState* getDescriptorState(unsigned heapKey, unsigned descriptorIndex);

private:
  void restoreState(DescriptorState* state);
  DescriptorState* copyDescriptor(DescriptorState* state,
                                  unsigned destHeapKey,
                                  unsigned destHeapIndex);
  void restoreD3D12RenderTargetView(D3D12RenderTargetViewState* state);
  void restoreD3D12DepthStencilView(D3D12DepthStencilViewState* state);
  void restoreD3D12ShaderResourceView(D3D12ShaderResourceViewState* state);
  void restoreD3D12UnorderedAccessView(D3D12UnorderedAccessViewState* state);
  void restoreD3D12ConstantBufferView(D3D12ConstantBufferViewState* state);
  void restoreD3D12Sampler(D3D12SamplerState* state);

private:
  StateTrackingService* stateService_{};
  ResourceForCBVRestoreService* resourceForCBVRestoreService_{};
  std::map<unsigned, std::map<unsigned, std::unique_ptr<DescriptorState>>> statesByHeapIndex_;
  std::set<unsigned> resources_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
