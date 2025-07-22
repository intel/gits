// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "stateTrackingService.h"

namespace gits {
namespace DirectX {

void DescriptorService::storeState(DescriptorState* state) {
  statesByHeapIndex_[state->destDescriptorKey][state->destDescriptorIndex].reset(state);
  resources_.insert(state->resourceKey);
}

void DescriptorService::removeState(unsigned key) {
  auto itHeap = statesByHeapIndex_.find(key);
  if (itHeap != statesByHeapIndex_.end()) {
    statesByHeapIndex_.erase(itHeap);
    return;
  }
  resources_.erase(key);
}

void DescriptorService::restoreState() {
  for (auto& itHeap : statesByHeapIndex_) {
    for (auto& it : itHeap.second) {
      DescriptorState* state = it.second.get();
      if (resources_.find(state->resourceKey) != resources_.end()) {
        if (stateService_->getAnalyzerResults().restoreDescriptor(state->destDescriptorKey,
                                                                  state->destDescriptorIndex)) {
          restoreState(state);
        }
      }
    }
  }
}

void DescriptorService::restoreState(DescriptorState* state) {

  switch (state->id) {
  case DescriptorState::D3D12_RENDERTARGETVIEW:
    restoreD3D12RenderTargetView(static_cast<D3D12RenderTargetViewState*>(state));
    break;
  case DescriptorState::D3D12_DEPTHSTENCILVIEW:
    restoreD3D12DepthStencilView(static_cast<D3D12DepthStencilViewState*>(state));
    break;
  case DescriptorState::D3D12_SHADERRESOURCEVIEW:
    restoreD3D12ShaderResourceView(static_cast<D3D12ShaderResourceViewState*>(state));
    break;
  case DescriptorState::D3D12_UNORDEREDACCESSVIEW:
    restoreD3D12UnorderedAccessView(static_cast<D3D12UnorderedAccessViewState*>(state));
    break;
  case DescriptorState::D3D12_CONSTANTBUFFERVIEW:
    restoreD3D12ConstantBufferView(static_cast<D3D12ConstantBufferViewState*>(state));
    break;
  case DescriptorState::D3D12_SAMPLER:
    restoreD3D12Sampler(static_cast<D3D12SamplerState*>(state));
    break;
  }
}

DescriptorState* DescriptorService::copyDescriptor(DescriptorState* state,
                                                   unsigned destHeapKey,
                                                   unsigned destHeapIndex) {
  DescriptorState* destState = nullptr;
  switch (state->id) {
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
  destState->destDescriptor.ptr = 0;
  destState->destDescriptorKey = destHeapKey;
  destState->destDescriptorIndex = destHeapIndex;
  return destState;
}

void DescriptorService::copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (!c.NumDescriptors_.value) {
    return;
  }
  auto& srcHeapIt = statesByHeapIndex_.find(c.SrcDescriptorRangeStart_.interfaceKey);
  if (srcHeapIt == statesByHeapIndex_.end()) {
    static bool logged = false;
    if (!logged) {
      Log(ERR) << "DescriptorService::copyDescriptors - descriptor states for heap not found";
      logged = true;
    }
    return;
  }
  auto& destHeap = statesByHeapIndex_[c.DestDescriptorRangeStart_.interfaceKey];
  for (unsigned i = 0; i < c.NumDescriptors_.value; ++i) {
    auto& srcIt = srcHeapIt->second.find(c.SrcDescriptorRangeStart_.index + i);
    if (srcIt == srcHeapIt->second.end()) {
      continue;
    }
    unsigned destHeapIndex = c.DestDescriptorRangeStart_.index + i;
    destHeap[destHeapIndex].reset(copyDescriptor(
        srcIt->second.get(), c.DestDescriptorRangeStart_.interfaceKey, destHeapIndex));
  }
}

void DescriptorService::copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (!c.NumDestDescriptorRanges_.value || !c.NumSrcDescriptorRanges_.value) {
    return;
  }

  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize =
      c.pDestDescriptorRangeSizes_.value ? c.pDestDescriptorRangeSizes_.value[destRangeIndex] : 1;

  unsigned destHeapKey = c.pDestDescriptorRangeStarts_.interfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < c.NumSrcDescriptorRanges_.value;
       ++srcRangeIndex) {
    unsigned srcRangeSize =
        c.pSrcDescriptorRangeSizes_.value ? c.pSrcDescriptorRangeSizes_.value[srcRangeIndex] : 1;
    auto& srcHeapIt =
        statesByHeapIndex_.find(c.pSrcDescriptorRangeStarts_.interfaceKeys[srcRangeIndex]);
    GITS_ASSERT(srcHeapIt != statesByHeapIndex_.end());
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      auto& srcIt =
          srcHeapIt->second.find(c.pSrcDescriptorRangeStarts_.indexes[srcRangeIndex] + srcIndex);
      if (srcIt == srcHeapIt->second.end()) {
        continue;
      }
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = c.pDestDescriptorRangeSizes_.value
                            ? c.pDestDescriptorRangeSizes_.value[destRangeIndex]
                            : 1;
        destHeapKey = c.pDestDescriptorRangeStarts_.interfaceKeys[destRangeIndex];
      }
      unsigned destHeapIndex = c.pDestDescriptorRangeStarts_.indexes[destRangeIndex] + destIndex;
      statesByHeapIndex_[destHeapKey][destHeapIndex].reset(
          copyDescriptor(srcIt->second.get(), destHeapKey, destHeapIndex));
    }
  }
}

void DescriptorService::restoreD3D12RenderTargetView(D3D12RenderTargetViewState* state) {
  ID3D12DeviceCreateRenderTargetViewCommand c;
  c.key = stateService_->getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pResource_.key = state->resourceKey;
  c.pDesc_.value = state->isDesc ? &state->desc : nullptr;
  c.DestDescriptor_.value = state->destDescriptor;
  c.DestDescriptor_.interfaceKey = state->destDescriptorKey;
  c.DestDescriptor_.index = state->destDescriptorIndex;
  stateService_->getRecorder().record(new ID3D12DeviceCreateRenderTargetViewWriter(c));
}

void DescriptorService::restoreD3D12DepthStencilView(D3D12DepthStencilViewState* state) {
  ID3D12DeviceCreateDepthStencilViewCommand c;
  c.key = stateService_->getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pResource_.key = state->resourceKey;
  c.pDesc_.value = state->isDesc ? &state->desc : nullptr;
  c.DestDescriptor_.value = state->destDescriptor;
  c.DestDescriptor_.interfaceKey = state->destDescriptorKey;
  c.DestDescriptor_.index = state->destDescriptorIndex;
  stateService_->getRecorder().record(new ID3D12DeviceCreateDepthStencilViewWriter(c));
}

void DescriptorService::restoreD3D12ShaderResourceView(D3D12ShaderResourceViewState* state) {
  ID3D12DeviceCreateShaderResourceViewCommand c;
  c.key = stateService_->getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pResource_.key = state->resourceKey;
  if (state->isDesc) {
    c.pDesc_.value = &state->desc;
    if (c.pDesc_.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
      c.pDesc_.raytracingLocationKey = state->resourceKey;
      c.pDesc_.raytracingLocationOffset = state->raytracingLocationOffset;
      c.pResource_.key = 0;
    }
  }
  c.DestDescriptor_.value = state->destDescriptor;
  c.DestDescriptor_.interfaceKey = state->destDescriptorKey;
  c.DestDescriptor_.index = state->destDescriptorIndex;
  stateService_->getRecorder().record(new ID3D12DeviceCreateShaderResourceViewWriter(c));
}

void DescriptorService::restoreD3D12UnorderedAccessView(D3D12UnorderedAccessViewState* state) {
  ID3D12DeviceCreateUnorderedAccessViewCommand c;
  c.key = stateService_->getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pResource_.key = state->resourceKey;
  c.pCounterResource_.key = state->counterResourceKey;
  c.pDesc_.value = state->isDesc ? &state->desc : nullptr;
  c.DestDescriptor_.value = state->destDescriptor;
  c.DestDescriptor_.interfaceKey = state->destDescriptorKey;
  c.DestDescriptor_.index = state->destDescriptorIndex;
  stateService_->getRecorder().record(new ID3D12DeviceCreateUnorderedAccessViewWriter(c));
}

void DescriptorService::restoreD3D12ConstantBufferView(D3D12ConstantBufferViewState* state) {
  ID3D12DeviceCreateConstantBufferViewCommand c;
  c.key = stateService_->getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = state->isDesc ? &state->desc : nullptr;
  c.pDesc_.bufferLocationKey = state->bufferLocationKey;
  c.pDesc_.bufferLocationOffset = state->bufferLocationOffset;
  c.DestDescriptor_.value = state->destDescriptor;
  c.DestDescriptor_.interfaceKey = state->destDescriptorKey;
  c.DestDescriptor_.index = state->destDescriptorIndex;
  stateService_->getRecorder().record(new ID3D12DeviceCreateConstantBufferViewWriter(c));
}

void DescriptorService::restoreD3D12Sampler(D3D12SamplerState* state) {
  ID3D12DeviceCreateSamplerCommand c;
  c.key = stateService_->getUniqueCommandKey();
  c.object_.key = state->deviceKey;
  c.pDesc_.value = &state->desc;
  c.DestDescriptor_.value = state->destDescriptor;
  c.DestDescriptor_.interfaceKey = state->destDescriptorKey;
  c.DestDescriptor_.index = state->destDescriptorIndex;
  stateService_->getRecorder().record(new ID3D12DeviceCreateSamplerWriter(c));
}

} // namespace DirectX
} // namespace gits
