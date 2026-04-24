// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "stateTrackingService.h"
#include "resourceForCBVRestoreService.h"
#include "log.h"

namespace gits {
namespace DirectX {

void DescriptorService::StoreState(DescriptorState* state) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  m_StatesByHeapIndex[state->DestDescriptorKey][state->DestDescriptorIndex].reset(state);
  m_Resources.insert(state->ResourceKey);
}

void DescriptorService::RemoveState(unsigned key) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto itHeap = m_StatesByHeapIndex.find(key);
  if (itHeap != m_StatesByHeapIndex.end()) {
    m_StatesByHeapIndex.erase(itHeap);
    return;
  }
  m_Resources.erase(key);
}

void DescriptorService::RestoreState() {
  std::lock_guard<std::mutex> lock(m_Mutex);

  for (auto& itHeap : m_StatesByHeapIndex) {
    for (auto& it : itHeap.second) {
      DescriptorState* state = it.second.get();
      if (m_StateService->GetAnalyzerResults().RestoreDescriptor(state->DestDescriptorKey,
                                                                 state->DestDescriptorIndex)) {
        RestoreState(state);
      }
    }
  }

  m_ResourceForCBVRestoreService->ReleaseResources();
}

void DescriptorService::RestoreState(DescriptorState* state) {

  switch (state->Id) {
  case DescriptorState::D3D12_RENDERTARGETVIEW:
    RestoreD3D12RenderTargetView(static_cast<D3D12RenderTargetViewState*>(state));
    break;
  case DescriptorState::D3D12_DEPTHSTENCILVIEW:
    RestoreD3D12DepthStencilView(static_cast<D3D12DepthStencilViewState*>(state));
    break;
  case DescriptorState::D3D12_SHADERRESOURCEVIEW:
    RestoreD3D12ShaderResourceView(static_cast<D3D12ShaderResourceViewState*>(state));
    break;
  case DescriptorState::D3D12_UNORDEREDACCESSVIEW:
    RestoreD3D12UnorderedAccessView(static_cast<D3D12UnorderedAccessViewState*>(state));
    break;
  case DescriptorState::D3D12_CONSTANTBUFFERVIEW:
    RestoreD3D12ConstantBufferView(static_cast<D3D12ConstantBufferViewState*>(state));
    break;
  case DescriptorState::D3D12_SAMPLER:
    RestoreD3D12Sampler(static_cast<D3D12SamplerState*>(state));
    break;
  }
}

DescriptorState* DescriptorService::CopyDescriptor(DescriptorState* state,
                                                   unsigned destHeapKey,
                                                   unsigned destHeapIndex) {
  DescriptorState* destState = nullptr;
  switch (state->Id) {
  case DescriptorState::D3D12_RENDERTARGETVIEW:
    destState = new D3D12RenderTargetViewState(*static_cast<D3D12RenderTargetViewState*>(state));
    break;
  case DescriptorState::D3D12_DEPTHSTENCILVIEW:
    destState = new D3D12DepthStencilViewState(*static_cast<D3D12DepthStencilViewState*>(state));
    break;
  case DescriptorState::D3D12_SHADERRESOURCEVIEW:
    destState =
        new D3D12ShaderResourceViewState(*static_cast<D3D12ShaderResourceViewState*>(state));
    break;
  case DescriptorState::D3D12_UNORDEREDACCESSVIEW:
    destState =
        new D3D12UnorderedAccessViewState(*static_cast<D3D12UnorderedAccessViewState*>(state));
    break;
  case DescriptorState::D3D12_CONSTANTBUFFERVIEW:
    destState =
        new D3D12ConstantBufferViewState(*static_cast<D3D12ConstantBufferViewState*>(state));
    break;
  case DescriptorState::D3D12_SAMPLER:
    destState = new D3D12SamplerState(*static_cast<D3D12SamplerState*>(state));
    break;
  }
  destState->DestDescriptor.ptr = 0;
  destState->DestDescriptorKey = destHeapKey;
  destState->DestDescriptorIndex = destHeapIndex;
  return destState;
}

void DescriptorService::CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (!c.m_NumDescriptors.Value) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);

  auto srcHeapIt = m_StatesByHeapIndex.find(c.m_SrcDescriptorRangeStart.InterfaceKey);
  auto& destHeap = m_StatesByHeapIndex[c.m_DestDescriptorRangeStart.InterfaceKey];
  for (unsigned i = 0; i < c.m_NumDescriptors.Value; ++i) {
    unsigned destHeapIndex = c.m_DestDescriptorRangeStart.Index + i;
    if (srcHeapIt != m_StatesByHeapIndex.end()) {
      auto srcIt = srcHeapIt->second.find(c.m_SrcDescriptorRangeStart.Index + i);
      if (srcIt != srcHeapIt->second.end()) {
        destHeap[destHeapIndex].reset(CopyDescriptor(
            srcIt->second.get(), c.m_DestDescriptorRangeStart.InterfaceKey, destHeapIndex));
      } else {
        destHeap.erase(destHeapIndex);
      }
    } else {
      destHeap.erase(destHeapIndex);
    }
  }
}

void DescriptorService::CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (!c.m_NumDestDescriptorRanges.Value || !c.m_NumSrcDescriptorRanges.Value) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);

  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize =
      c.m_pDestDescriptorRangeSizes.Value ? c.m_pDestDescriptorRangeSizes.Value[destRangeIndex] : 1;

  unsigned destHeapKey = c.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < c.m_NumSrcDescriptorRanges.Value;
       ++srcRangeIndex) {
    unsigned srcRangeSize =
        c.m_pSrcDescriptorRangeSizes.Value ? c.m_pSrcDescriptorRangeSizes.Value[srcRangeIndex] : 1;
    auto srcHeapIt =
        m_StatesByHeapIndex.find(c.m_pSrcDescriptorRangeStarts.InterfaceKeys[srcRangeIndex]);
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      auto srcIt =
          srcHeapIt->second.find(c.m_pSrcDescriptorRangeStarts.Indexes[srcRangeIndex] + srcIndex);
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = c.m_pDestDescriptorRangeSizes.Value
                            ? c.m_pDestDescriptorRangeSizes.Value[destRangeIndex]
                            : 1;
        destHeapKey = c.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];
      }
      unsigned destHeapIndex = c.m_pDestDescriptorRangeStarts.Indexes[destRangeIndex] + destIndex;
      if (srcHeapIt != m_StatesByHeapIndex.end()) {
        if (srcIt != srcHeapIt->second.end()) {
          m_StatesByHeapIndex[destHeapKey][destHeapIndex].reset(
              CopyDescriptor(srcIt->second.get(), destHeapKey, destHeapIndex));
        } else {
          m_StatesByHeapIndex[destHeapKey].erase(destHeapIndex);
        }
      } else {
        m_StatesByHeapIndex[destHeapKey].erase(destHeapIndex);
      }
    }
  }
}

DescriptorState* DescriptorService::GetDescriptorState(unsigned heapKey, unsigned DescriptorIndex) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto heapIt = m_StatesByHeapIndex.find(heapKey);
  if (heapIt == m_StatesByHeapIndex.end()) {
    return nullptr;
  }
  auto stateIt = heapIt->second.find(DescriptorIndex);
  if (stateIt == heapIt->second.end()) {
    return nullptr;
  }
  return stateIt->second.get();
}

void DescriptorService::RestoreD3D12RenderTargetView(D3D12RenderTargetViewState* state) {
  if (m_Resources.find(state->ResourceKey) == m_Resources.end()) {
    return;
  }
  if (m_StateService->GetState(state->ResourceKey)) {
    m_StateService->RestoreState(state->ResourceKey);
  }
  ID3D12DeviceCreateRenderTargetViewCommand c;
  c.Key = m_StateService->GetUniqueCommandKey();
  c.m_Object.Key = state->DeviceKey;
  c.m_pResource.Key = state->ResourceKey;
  c.m_pDesc.Value = state->IsDesc ? &state->Desc : nullptr;
  c.m_DestDescriptor.Value = state->DestDescriptor;
  c.m_DestDescriptor.InterfaceKey = state->DestDescriptorKey;
  c.m_DestDescriptor.Index = state->DestDescriptorIndex;
  m_StateService->GetRecorder().Record(ID3D12DeviceCreateRenderTargetViewSerializer(c));
}

void DescriptorService::RestoreD3D12DepthStencilView(D3D12DepthStencilViewState* state) {
  if (m_Resources.find(state->ResourceKey) == m_Resources.end()) {
    return;
  }
  if (m_StateService->GetState(state->ResourceKey)) {
    m_StateService->RestoreState(state->ResourceKey);
  }
  ID3D12DeviceCreateDepthStencilViewCommand c;
  c.Key = m_StateService->GetUniqueCommandKey();
  c.m_Object.Key = state->DeviceKey;
  c.m_pResource.Key = state->ResourceKey;
  c.m_pDesc.Value = state->IsDesc ? &state->Desc : nullptr;
  c.m_DestDescriptor.Value = state->DestDescriptor;
  c.m_DestDescriptor.InterfaceKey = state->DestDescriptorKey;
  c.m_DestDescriptor.Index = state->DestDescriptorIndex;
  m_StateService->GetRecorder().Record(ID3D12DeviceCreateDepthStencilViewSerializer(c));
}

void DescriptorService::RestoreD3D12ShaderResourceView(D3D12ShaderResourceViewState* state) {
  if (m_Resources.find(state->ResourceKey) == m_Resources.end()) {
    return;
  }
  if (m_StateService->GetState(state->ResourceKey)) {
    m_StateService->RestoreState(state->ResourceKey);
  }
  ID3D12DeviceCreateShaderResourceViewCommand c;
  c.Key = m_StateService->GetUniqueCommandKey();
  c.m_Object.Key = state->DeviceKey;
  c.m_pResource.Key = state->ResourceKey;
  if (state->IsDesc) {
    c.m_pDesc.Value = &state->Desc;
    if (c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
      c.m_pDesc.RaytracingLocationKey = state->ResourceKey;
      c.m_pDesc.RaytracingLocationOffset = state->RaytracingLocationOffset;
      c.m_pResource.Key = 0;
    }
  }
  c.m_DestDescriptor.Value = state->DestDescriptor;
  c.m_DestDescriptor.InterfaceKey = state->DestDescriptorKey;
  c.m_DestDescriptor.Index = state->DestDescriptorIndex;
  m_StateService->GetRecorder().Record(ID3D12DeviceCreateShaderResourceViewSerializer(c));
}

void DescriptorService::RestoreD3D12UnorderedAccessView(D3D12UnorderedAccessViewState* state) {
  if (m_Resources.find(state->ResourceKey) == m_Resources.end()) {
    return;
  }
  if (m_StateService->GetState(state->ResourceKey)) {
    m_StateService->RestoreState(state->ResourceKey);
  }
  ID3D12DeviceCreateUnorderedAccessViewCommand c;
  c.Key = m_StateService->GetUniqueCommandKey();
  c.m_Object.Key = state->DeviceKey;
  c.m_pResource.Key = state->ResourceKey;
  c.m_pCounterResource.Key = state->AuxiliaryResourceKey;
  c.m_pDesc.Value = state->IsDesc ? &state->Desc : nullptr;
  c.m_DestDescriptor.Value = state->DestDescriptor;
  c.m_DestDescriptor.InterfaceKey = state->DestDescriptorKey;
  c.m_DestDescriptor.Index = state->DestDescriptorIndex;
  m_StateService->GetRecorder().Record(ID3D12DeviceCreateUnorderedAccessViewSerializer(c));
}

void DescriptorService::RestoreD3D12ConstantBufferView(D3D12ConstantBufferViewState* state) {
  if (m_StateService->GetState(state->ResourceKey)) {
    m_StateService->RestoreState(state->ResourceKey);
  } else if (state->ResourceKey) {
    bool restored = m_ResourceForCBVRestoreService->RestoreResourceObject(state->ResourceKey);
    if (!restored) {
      return;
    }
  }

  ID3D12DeviceCreateConstantBufferViewCommand c;
  c.Key = m_StateService->GetUniqueCommandKey();
  c.m_Object.Key = state->DeviceKey;
  c.m_pDesc.Value = state->IsDesc ? &state->Desc : nullptr;
  c.m_pDesc.BufferLocationKey = state->ResourceKey;
  c.m_pDesc.BufferLocationOffset = state->BufferLocationOffset;
  c.m_DestDescriptor.Value = state->DestDescriptor;
  c.m_DestDescriptor.InterfaceKey = state->DestDescriptorKey;
  c.m_DestDescriptor.Index = state->DestDescriptorIndex;
  m_StateService->GetRecorder().Record(ID3D12DeviceCreateConstantBufferViewSerializer(c));
}

void DescriptorService::RestoreD3D12Sampler(D3D12SamplerState* state) {
  ID3D12DeviceCreateSamplerCommand c;
  c.Key = m_StateService->GetUniqueCommandKey();
  c.m_Object.Key = state->DeviceKey;
  c.m_pDesc.Value = &state->Desc;
  c.m_DestDescriptor.Value = state->DestDescriptor;
  c.m_DestDescriptor.InterfaceKey = state->DestDescriptorKey;
  c.m_DestDescriptor.Index = state->DestDescriptorIndex;
  m_StateService->GetRecorder().Record(ID3D12DeviceCreateSamplerSerializer(c));
}

} // namespace DirectX
} // namespace gits
