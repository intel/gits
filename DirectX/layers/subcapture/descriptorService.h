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
  DescriptorState(StateId id_) : Id(id_) {}
  virtual ~DescriptorState() = default;
  StateId Id{};
  unsigned DeviceKey{};
  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{};
  unsigned DestDescriptorKey{};
  unsigned DestDescriptorIndex{};
  unsigned ResourceKey{};
  unsigned AuxiliaryResourceKey{};
};

struct D3D12RenderTargetViewState : public DescriptorState {
  D3D12RenderTargetViewState() : DescriptorState(D3D12_RENDERTARGETVIEW) {}
  bool IsDesc{};
  D3D12_RENDER_TARGET_VIEW_DESC Desc{};
};

struct D3D12DepthStencilViewState : public DescriptorState {
  D3D12DepthStencilViewState() : DescriptorState(D3D12_DEPTHSTENCILVIEW) {}
  bool IsDesc{};
  D3D12_DEPTH_STENCIL_VIEW_DESC Desc{};
};

struct D3D12ShaderResourceViewState : public DescriptorState {
  D3D12ShaderResourceViewState() : DescriptorState(D3D12_SHADERRESOURCEVIEW) {}
  bool IsDesc{};
  D3D12_SHADER_RESOURCE_VIEW_DESC Desc{};
  unsigned RaytracingLocationOffset{};
};

struct D3D12UnorderedAccessViewState : public DescriptorState {
  D3D12UnorderedAccessViewState() : DescriptorState(D3D12_UNORDEREDACCESSVIEW) {}
  bool IsDesc{};
  D3D12_UNORDERED_ACCESS_VIEW_DESC Desc{};
};

struct D3D12ConstantBufferViewState : public DescriptorState {
  D3D12ConstantBufferViewState() : DescriptorState(D3D12_CONSTANTBUFFERVIEW) {}
  bool IsDesc{};
  D3D12_CONSTANT_BUFFER_VIEW_DESC Desc{};
  unsigned BufferLocationOffset{};
};

struct D3D12SamplerState : public DescriptorState {
  D3D12SamplerState() : DescriptorState(D3D12_SAMPLER) {}
  D3D12_SAMPLER_DESC Desc{};
};

class StateTrackingService;
class ResourceForCBVRestoreService;

class DescriptorService {
public:
  DescriptorService() {}
  DescriptorService(StateTrackingService* stateService,
                    ResourceForCBVRestoreService* resourceForCBVRestoreService)
      : m_StateService(stateService),
        m_ResourceForCBVRestoreService(resourceForCBVRestoreService) {}
  void StoreState(DescriptorState* state);
  void RemoveState(unsigned key);
  void RestoreState();
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  DescriptorState* GetDescriptorState(unsigned heapKey, unsigned DescriptorIndex);

private:
  void RestoreState(DescriptorState* state);
  DescriptorState* CopyDescriptor(DescriptorState* state,
                                  unsigned destHeapKey,
                                  unsigned destHeapIndex);
  void RestoreD3D12RenderTargetView(D3D12RenderTargetViewState* state);
  void RestoreD3D12DepthStencilView(D3D12DepthStencilViewState* state);
  void RestoreD3D12ShaderResourceView(D3D12ShaderResourceViewState* state);
  void RestoreD3D12UnorderedAccessView(D3D12UnorderedAccessViewState* state);
  void RestoreD3D12ConstantBufferView(D3D12ConstantBufferViewState* state);
  void RestoreD3D12Sampler(D3D12SamplerState* state);

private:
  StateTrackingService* m_StateService{};
  ResourceForCBVRestoreService* m_ResourceForCBVRestoreService{};
  std::map<unsigned, std::map<unsigned, std::unique_ptr<DescriptorState>>> m_StatesByHeapIndex;
  std::set<unsigned> m_Resources;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
