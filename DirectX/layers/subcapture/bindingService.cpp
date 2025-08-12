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
                               AnalyzerRaytracingService& raytracingService,
                               AnalyzerExecuteIndirectService& executeIndirectService,
                               bool commandListSubcapture)
    : analyzerService_(analyzerService),
      descriptorService_(descriptorService),
      rootSignatureService_(rootSignatureService),
      raytracingService_(raytracingService),
      executeIndirectService_(executeIndirectService),
      commandListSubcapture_(commandListSubcapture) {
  restoreTlases_ = Configurator::Get().directx.features.subcapture.restoreTLASes;
}

void BindingService::commandListsRestore(const std::set<unsigned>& commandLists) {
  for (unsigned commandListKey : commandLists) {
    commandListRestore(commandListKey);
  }
}

void BindingService::commandListReset(ID3D12GraphicsCommandListResetCommand& c) {
  if (!analyzerService_.inRange()) {
    commandsByCommandList_.erase(c.object_.key);
  }
}

void BindingService::setDescriptorHeaps(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setDescriptorHeapsImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetDescriptorHeapsCommand(c));
  }
}

void BindingService::setDescriptorHeapsImpl(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
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
    commandListRestore(c.object_.key);
    setRootSignatureImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetComputeRootSignatureCommand(c));
  }
}

void BindingService::setRootSignatureImpl(
    ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  computeRootSignatureByCommandList_[c.object_.key] = c.pRootSignature_.key;
}

void BindingService::setRootSignature(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootSignatureImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand(c));
  }
}

void BindingService::setRootSignatureImpl(
    ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  graphicsRootSignatureByCommandList_[c.object_.key] = c.pRootSignature_.key;
}

void BindingService::setRootDescriptorTable(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootDescriptorTableImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand(c));
  }
}

void BindingService::setRootDescriptorTableImpl(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
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
      descriptors_.insert({c.BaseDescriptor_.interfaceKey, index});
    }
  }
  objectsForRestore_.insert(c.BaseDescriptor_.interfaceKey);
}

void BindingService::setRootDescriptorTable(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootDescriptorTableImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand(c));
  }
}

void BindingService::setRootDescriptorTableImpl(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
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
      descriptors_.insert({c.BaseDescriptor_.interfaceKey, index});
    }
  }
  objectsForRestore_.insert(c.BaseDescriptor_.interfaceKey);
}

void BindingService::setRootConstantBufferView(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootConstantBufferViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand(c));
  }
}

void BindingService::setRootConstantBufferViewImpl(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void BindingService::setRootConstantBufferView(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootConstantBufferViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand(c));
  }
}

void BindingService::setRootConstantBufferViewImpl(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void BindingService::setRootShaderResourceView(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootShaderResourceViewImpl(c);
    objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand(c));
  }
}

void BindingService::setRootShaderResourceViewImpl(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void BindingService::setRootShaderResourceView(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootShaderResourceViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand(c));
  }
}

void BindingService::setRootShaderResourceViewImpl(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void BindingService::setRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootUnorderedAccessViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand(c));
  }
}

void BindingService::setRootUnorderedAccessViewImpl(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void BindingService::setRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRootUnorderedAccessViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand(c));
  }
}

void BindingService::setRootUnorderedAccessViewImpl(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void BindingService::setIndexBuffer(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setIndexBufferImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListIASetIndexBufferCommand(c));
  }
}

void BindingService::setIndexBufferImpl(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  objectsForRestore_.insert(c.pView_.bufferLocationKey);
}

void BindingService::setVertexBuffers(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setVertexBuffersImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListIASetVertexBuffersCommand(c));
  }
}

void BindingService::setVertexBuffersImpl(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  for (unsigned i = 0; i < c.pViews_.size; ++i) {
    objectsForRestore_.insert(c.pViews_.bufferLocationKeys[i]);
  }
}

void BindingService::setSOTargets(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setSOTargetsImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSOSetTargetsCommand(c));
  }
}

void BindingService::setSOTargetsImpl(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  for (unsigned i = 0; i < c.pViews_.size; ++i) {
    objectsForRestore_.insert(c.pViews_.bufferLocationKeys[i]);
    objectsForRestore_.insert(c.pViews_.bufferFilledSizeLocationKeys[i]);
  }
}

void BindingService::setRenderTargets(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setRenderTargetsImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListOMSetRenderTargetsCommand(c));
  }
}

void BindingService::setRenderTargetsImpl(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
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
        descriptors_.insert({key, index});
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
        descriptors_.insert({key, index});
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
      descriptors_.insert({key, index});
    }
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    clearViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListClearDepthStencilViewCommand(c));
  }
}

void BindingService::clearViewImpl(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (c.DepthStencilView_.interfaceKey) {
    objectsForRestore_.insert(c.DepthStencilView_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.DepthStencilView_.interfaceKey,
                                                                   c.DepthStencilView_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.DepthStencilView_.interfaceKey, c.DepthStencilView_.index});
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    clearViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListClearRenderTargetViewCommand(c));
  }
}

void BindingService::clearViewImpl(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (c.RenderTargetView_.interfaceKey) {
    objectsForRestore_.insert(c.RenderTargetView_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.RenderTargetView_.interfaceKey,
                                                                   c.RenderTargetView_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.RenderTargetView_.interfaceKey, c.RenderTargetView_.index});
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    clearViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand(c));
  }
}

void BindingService::clearViewImpl(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    objectsForRestore_.insert(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
      descriptors_.insert(
          {c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index});
    }
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    objectsForRestore_.insert(c.ViewCPUHandle_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                   c.ViewCPUHandle_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
      descriptors_.insert({c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index});
    }
  }
}

void BindingService::clearView(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    clearViewImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand(c));
  }
}

void BindingService::clearViewImpl(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    objectsForRestore_.insert(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
      descriptors_.insert(
          {c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index});
    }
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    objectsForRestore_.insert(c.ViewCPUHandle_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                   c.ViewCPUHandle_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
      descriptors_.insert({c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index});
    }
  }
}

void BindingService::setPipelineState(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    setPipelineStateImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4SetPipelineState1Command(c));
  }
}

void BindingService::setPipelineStateImpl(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  std::vector<unsigned>& subobjects =
      raytracingService_.getStateObjectSubobjects(c.pStateObject_.key);
  for (unsigned key : subobjects) {
    objectsForRestore_.insert(key);
  }
}

void BindingService::buildRaytracingAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    buildRaytracingAccelerationStructureImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand(c));
  }
  if (!analyzerService_.inRange() && restoreTlases_) {
    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      raytracingService_.buildTlas(c);
    }
  }

  objectsForRestore_.insert(c.pDesc_.destAccelerationStructureKey);
  if (c.pDesc_.sourceAccelerationStructureKey) {
    objectsForRestore_.insert(c.pDesc_.sourceAccelerationStructureKey);
  }
  if (!(c.pDesc_.scratchAccelerationStructureKey & Command::stateRestoreKeyMask)) {
    objectsForRestore_.insert(c.pDesc_.scratchAccelerationStructureKey);
  }
  for (unsigned key : c.pDesc_.inputKeys) {
    objectsForRestore_.insert(key);
  }
}

void BindingService::buildRaytracingAccelerationStructureImpl(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    raytracingService_.buildTlas(c);
  } else if (c.pDesc_.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (c.pDesc_.sourceAccelerationStructureKey) {
      raytracingService_.addAccelerationStructureSource(c.pDesc_.sourceAccelerationStructureKey,
                                                        c.pDesc_.sourceAccelerationStructureOffset);
    }
  }
}

void BindingService::copyRaytracingAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    copyRaytracingAccelerationStructureImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand(c));
  }

  objectsForRestore_.insert(c.DestAccelerationStructureData_.interfaceKey);
  objectsForRestore_.insert(c.SourceAccelerationStructureData_.interfaceKey);
}

void BindingService::copyRaytracingAccelerationStructureImpl(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  raytracingService_.addAccelerationStructureSource(c.SourceAccelerationStructureData_.interfaceKey,
                                                    c.SourceAccelerationStructureData_.offset);
}

void BindingService::dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    dispatchRaysImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4DispatchRaysCommand(c));
  }
}

void BindingService::dispatchRaysImpl(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  raytracingService_.dispatchRays(c);
  objectsForRestore_.insert(c.pDesc_.rayGenerationShaderRecordKey);
  objectsForRestore_.insert(c.pDesc_.missShaderTableKey);
  objectsForRestore_.insert(c.pDesc_.hitGroupTableKey);
  objectsForRestore_.insert(c.pDesc_.callableShaderTableKey);
}

void BindingService::executeIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    executeIndirectImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListExecuteIndirectCommand(c));
  }
}

void BindingService::executeIndirectImpl(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  executeIndirectService_.executeIndirect(c);
}

void BindingService::writeBufferImmediate(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (analyzerService_.inRange()) {
    commandListRestore(c.object_.key);
    writeBufferImmediateImpl(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList2WriteBufferImmediateCommand(c));
  }
}

void BindingService::writeBufferImmediateImpl(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  for (unsigned key : c.pParams_.destKeys) {
    objectsForRestore_.insert(key);
  }
}

void BindingService::copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.NumDescriptors_.value; ++i) {
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.SrcDescriptorRangeStart_.interfaceKey, c.SrcDescriptorRangeStart_.index + i);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
        descriptors_.insert(
            {c.SrcDescriptorRangeStart_.interfaceKey, c.SrcDescriptorRangeStart_.index + i});
      }
    }
    objectsForRestore_.insert(c.SrcDescriptorRangeStart_.interfaceKey);

    objectsForRestore_.insert(c.DestDescriptorRangeStart_.interfaceKey);
  }
}

void BindingService::copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.NumSrcDescriptorRanges_.value; ++i) {
      unsigned srcRangeSize =
          c.pSrcDescriptorRangeSizes_.value ? c.pSrcDescriptorRangeSizes_.value[i] : 1;
      for (unsigned j = 0; j < srcRangeSize; ++j) {
        DescriptorState* state =
            descriptorService_.getDescriptorState(c.pSrcDescriptorRangeStarts_.interfaceKeys[i],
                                                  c.pSrcDescriptorRangeStarts_.indexes[i] + j);
        if (state) {
          objectsForRestore_.insert(state->resourceKey);
          descriptors_.insert({c.pSrcDescriptorRangeStarts_.interfaceKeys[i],
                               c.pSrcDescriptorRangeStarts_.indexes[i] + j});
        }
      }
      objectsForRestore_.insert(c.pSrcDescriptorRangeStarts_.interfaceKeys[i]);
    }

    for (unsigned key : c.pDestDescriptorRangeStarts_.interfaceKeys) {
      objectsForRestore_.insert(key);
    }
  }
}

void BindingService::commandListRestore(unsigned commandListKey) {
  auto itCommandList = commandsByCommandList_.find(commandListKey);
  if (itCommandList == commandsByCommandList_.end()) {
    return;
  }
  for (auto& command : itCommandList->second) {
    switch (command->getId()) {
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETDESCRIPTORHEAPS:
      setDescriptorHeapsImpl(
          static_cast<ID3D12GraphicsCommandListSetDescriptorHeapsCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSIGNATURE:
      setRootSignatureImpl(
          static_cast<ID3D12GraphicsCommandListSetComputeRootSignatureCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSIGNATURE:
      setRootSignatureImpl(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE:
      setRootDescriptorTableImpl(
          static_cast<ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTDESCRIPTORTABLE:
      setRootDescriptorTableImpl(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW:
      setRootConstantBufferViewImpl(
          static_cast<ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTCONSTANTBUFFERVIEW:
      setRootConstantBufferViewImpl(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW:
      setRootShaderResourceViewImpl(
          static_cast<ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSHADERRESOURCEVIEW:
      setRootShaderResourceViewImpl(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW:
      setRootUnorderedAccessViewImpl(
          static_cast<ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTUNORDEREDACCESSVIEW:
      setRootUnorderedAccessViewImpl(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETINDEXBUFFER:
      setIndexBufferImpl(static_cast<ID3D12GraphicsCommandListIASetIndexBufferCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETVERTEXBUFFERS:
      setVertexBuffersImpl(
          static_cast<ID3D12GraphicsCommandListIASetVertexBuffersCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SOSETTARGETS:
      setSOTargetsImpl(static_cast<ID3D12GraphicsCommandListSOSetTargetsCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS:
      setRenderTargetsImpl(
          static_cast<ID3D12GraphicsCommandListOMSetRenderTargetsCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW:
      clearViewImpl(static_cast<ID3D12GraphicsCommandListClearDepthStencilViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW:
      clearViewImpl(static_cast<ID3D12GraphicsCommandListClearRenderTargetViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT:
      clearViewImpl(
          static_cast<ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT:
      clearViewImpl(
          static_cast<ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_SETPIPELINESTATE1:
      setPipelineStateImpl(
          static_cast<ID3D12GraphicsCommandList4SetPipelineState1Command&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
      buildRaytracingAccelerationStructureImpl(
          static_cast<ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_COPYRAYTRACINGACCELERATIONSTRUCTURE:
      copyRaytracingAccelerationStructureImpl(
          static_cast<ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_DISPATCHRAYS:
      dispatchRaysImpl(static_cast<ID3D12GraphicsCommandList4DispatchRaysCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST2_WRITEBUFFERIMMEDIATE:
      writeBufferImmediateImpl(
          static_cast<ID3D12GraphicsCommandList2WriteBufferImmediateCommand&>(*command));
      break;
    }
  }
  commandsByCommandList_.erase(itCommandList);
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
