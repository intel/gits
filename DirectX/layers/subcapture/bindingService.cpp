// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bindingService.h"
#include "analyzerService.h"
#include "rootSignatureService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

BindingService::BindingService(AnalyzerService& analyzerService,
                               DescriptorService& descriptorService,
                               RootSignatureService& rootSignatureService,
                               AnalyzerRaytracingService& raytracingService)
    : analyzerService_(analyzerService),
      descriptorService_(descriptorService),
      rootSignatureService_(rootSignatureService),
      raytracingService_(raytracingService) {
  restoreTlases_ = Configurator::Get().directx.features.subcapture.restoreTLASes;
}

void BindingService::setDescriptorHeaps(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  std::vector<DescriptorHeap> descriptorHeaps(c.NumDescriptorHeaps_.value);
  for (unsigned i = 0; i < c.NumDescriptorHeaps_.value; ++i) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = c.ppDescriptorHeaps_.value[i]->GetDesc();
    descriptorHeaps[i].key = c.ppDescriptorHeaps_.keys[i];
    descriptorHeaps[i].numDescriptors = desc.NumDescriptors;
  }
  descriptorHeapsByCommandList_[c.object_.key] = std::move(descriptorHeaps);
}

void BindingService::setRootSignature(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (analyzerService_.inRange()) {
    computeRootSignatureByCommandList_[c.object_.key] = c.pRootSignature_.key;
  }
}

void BindingService::setRootSignature(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  if (analyzerService_.inRange()) {
    graphicsRootSignatureByCommandList_[c.object_.key] = c.pRootSignature_.key;
  }
}

void BindingService::setRootDescriptorTable(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (analyzerService_.inRange()) {
    unsigned rootSignatureKey = computeRootSignatureByCommandList_[c.object_.key];
    GITS_ASSERT(rootSignatureKey);
    unsigned numDescriptors = getNumDescriptors(c.object_.key, c.BaseDescriptor_.interfaceKey);
    GITS_ASSERT(numDescriptors);
    std::vector<unsigned> indexes = rootSignatureService_.getDescriptorTableIndexes(
        rootSignatureKey, c.BaseDescriptor_.interfaceKey, c.RootParameterIndex_.value,
        c.BaseDescriptor_.index, numDescriptors);
    for (unsigned index : indexes) {
      DescriptorState* state =
          descriptorService_.getDescriptorState(c.BaseDescriptor_.interfaceKey, index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(std::make_pair(c.BaseDescriptor_.interfaceKey, index));
      }
    }
    objectsForRestore_.insert(c.BaseDescriptor_.interfaceKey);
  }
}

void BindingService::setRootDescriptorTable(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (analyzerService_.inRange()) {
    unsigned rootSignatureKey = graphicsRootSignatureByCommandList_[c.object_.key];
    GITS_ASSERT(rootSignatureKey);
    unsigned numDescriptors = getNumDescriptors(c.object_.key, c.BaseDescriptor_.interfaceKey);
    GITS_ASSERT(numDescriptors);
    std::vector<unsigned> indexes = rootSignatureService_.getDescriptorTableIndexes(
        rootSignatureKey, c.BaseDescriptor_.interfaceKey, c.RootParameterIndex_.value,
        c.BaseDescriptor_.index, numDescriptors);
    for (unsigned index : indexes) {
      DescriptorState* state =
          descriptorService_.getDescriptorState(c.BaseDescriptor_.interfaceKey, index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(std::make_pair(c.BaseDescriptor_.interfaceKey, index));
      }
    }
    objectsForRestore_.insert(c.BaseDescriptor_.interfaceKey);
  }
}

void BindingService::setRootConstantBufferView(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  }
}

void BindingService::setRootConstantBufferView(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  }
}

void BindingService::setRootShaderResourceView(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  }
}

void BindingService::setRootShaderResourceView(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  }
}

void BindingService::setRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  }
}

void BindingService::setRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  }
}

void BindingService::setIndexBuffer(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.pView_.bufferLocationKey);
  }
}

void BindingService::setVertexBuffers(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.pViews_.size; ++i) {
      objectsForRestore_.insert(c.pViews_.bufferLocationKeys[i]);
    }
  }
}

void BindingService::setSOTargets(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.pViews_.size; ++i) {
      objectsForRestore_.insert(c.pViews_.bufferLocationKeys[i]);
      objectsForRestore_.insert(c.pViews_.bufferFilledSizeLocationKeys[i]);
    }
  }
}

void BindingService::setRenderTargets(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (analyzerService_.inRange()) {
    if (!c.RTsSingleHandleToDescriptorRange_.value) {
      for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
        unsigned key = c.pRenderTargetDescriptors_.interfaceKeys[i];
        unsigned index = c.pRenderTargetDescriptors_.indexes[i];
        if (key) {
          objectsForRestore_.insert(key);
          DescriptorState* state = descriptorService_.getDescriptorState(key, index);
          if (state) {
            objectsForRestore_.insert(state->resourceKey);
          }
          descriptors_.insert(std::make_pair(key, index));
        }
      }
    } else if (c.NumRenderTargetDescriptors_.value) {
      unsigned key = c.pRenderTargetDescriptors_.interfaceKeys[0];
      unsigned index = c.pRenderTargetDescriptors_.indexes[0];
      if (key) {
        objectsForRestore_.insert(key);
        for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
          DescriptorState* state = descriptorService_.getDescriptorState(key, index);
          if (state) {
            objectsForRestore_.insert(state->resourceKey);
          }
          descriptors_.insert(std::make_pair(key, index));
          ++index;
        }
      }
    }
    if (c.pDepthStencilDescriptor_.value) {
      unsigned key = c.pDepthStencilDescriptor_.interfaceKeys[0];
      unsigned index = c.pDepthStencilDescriptor_.indexes[0];
      if (key) {
        objectsForRestore_.insert(key);
        DescriptorState* state = descriptorService_.getDescriptorState(key, index);
        if (state) {
          objectsForRestore_.insert(state->resourceKey);
        }
        descriptors_.insert(std::make_pair(key, index));
      }
    }
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (analyzerService_.inRange()) {
    if (c.DepthStencilView_.interfaceKey) {
      objectsForRestore_.insert(c.DepthStencilView_.interfaceKey);
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.DepthStencilView_.interfaceKey, c.DepthStencilView_.index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
      }
      descriptors_.insert(
          std::make_pair(c.DepthStencilView_.interfaceKey, c.DepthStencilView_.index));
    }
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (analyzerService_.inRange()) {
    if (c.RenderTargetView_.interfaceKey) {
      objectsForRestore_.insert(c.RenderTargetView_.interfaceKey);
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.RenderTargetView_.interfaceKey, c.RenderTargetView_.index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
      }
      descriptors_.insert(
          std::make_pair(c.RenderTargetView_.interfaceKey, c.RenderTargetView_.index));
    }
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (analyzerService_.inRange()) {
    if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
      objectsForRestore_.insert(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(std::make_pair(c.ViewGPUHandleInCurrentHeap_.interfaceKey,
                                           c.ViewGPUHandleInCurrentHeap_.index));
      }
    }
    if (c.ViewCPUHandle_.interfaceKey) {
      objectsForRestore_.insert(c.ViewCPUHandle_.interfaceKey);
      DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                     c.ViewCPUHandle_.index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(std::make_pair(c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index));
      }
    }
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (analyzerService_.inRange()) {
    if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
      objectsForRestore_.insert(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(std::make_pair(c.ViewGPUHandleInCurrentHeap_.interfaceKey,
                                           c.ViewGPUHandleInCurrentHeap_.index));
      }
    }
    if (c.ViewCPUHandle_.interfaceKey) {
      objectsForRestore_.insert(c.ViewCPUHandle_.interfaceKey);
      DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                     c.ViewCPUHandle_.index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(std::make_pair(c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index));
      }
    }
  }
}

void BindingService::copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (analyzerService_.inRange()) {
    objectsForRestore_.insert(c.SrcDescriptorRangeStart_.interfaceKey);
    objectsForRestore_.insert(c.DestDescriptorRangeStart_.interfaceKey);
  }
}

void BindingService::copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned key : c.pDestDescriptorRangeStarts_.interfaceKeys) {
      objectsForRestore_.insert(key);
    }
    for (unsigned key : c.pSrcDescriptorRangeStarts_.interfaceKeys) {
      objectsForRestore_.insert(key);
    }
  }
}

void BindingService::setPipelineState(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (analyzerService_.inRange()) {
    std::vector<unsigned>& subobjects =
        raytracingService_.getStateObjectSubobjects(c.pStateObject_.key);
    for (unsigned key : subobjects) {
      objectsForRestore_.insert(key);
    }
  }
}

void BindingService::buildRaytracingAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.inRange() || restoreTlases_) {
    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      raytracingService_.buildTlas(c);
    }
  }
}

unsigned BindingService::getNumDescriptors(unsigned commandListKey, unsigned descriptorHeapKey) {
  auto it = descriptorHeapsByCommandList_.find(commandListKey);
  if (it != descriptorHeapsByCommandList_.end()) {
    for (unsigned i = 0; i < it->second.size(); ++i) {
      if (it->second[i].key == descriptorHeapKey) {
        return it->second[i].numDescriptors;
      }
    }
  }
  return 0;
}

} // namespace DirectX
} // namespace gits
