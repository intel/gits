// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListService.h"
#include "commandsAuto.h"
#include "stateTrackingService.h"
#include "commandWritersAuto.h"
#include "analyzerResults.h"

#include <fstream>

namespace gits {
namespace DirectX {

CommandListService::CommandListService(StateTrackingService& stateService)
    : stateService_(stateService) {

  restoreCommandLists_ = stateService_.analyzerResults_.restoreCommandLists();
}

void CommandListService::addCommandList(CommandListState* state) {
  commandListsByKey_[state->key] = state;
}
void CommandListService::removeCommandList(unsigned key) {
  commandListsByKey_.erase(key);
}

void CommandListService::restoreCommandLists() {
  if (!restoreCommandLists_) {
    return;
  }

  std::map<unsigned, CommandListCommand*> commandsByKey;

  for (auto& it : commandListsByKey_) {
    if (!stateService_.analyzerResults_.restoreCommandList(it.first)) {
      continue;
    }
    CommandListState* state = it.second;
    for (CommandListCommand* command : it.second->commands) {
      commandsByKey[command->commandKey] = command;
    }
  }

  for (auto& it : commandsByKey) {
    CommandListCommand* command = it.second;
    if (command->id == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS) {
      restoreCommandState(static_cast<CommandListOMSetRenderTargets*>(command));
    } else if (command->id == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW) {
      restoreCommandState(static_cast<CommandListClearRenderTargetView*>(command));
    } else if (command->id == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW) {
      restoreCommandState(static_cast<CommandListClearDepthStencilView*>(command));
    } else if (command->id ==
               CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT) {
      restoreCommandState(static_cast<CommandListClearUnorderedAccessViewUint*>(command));
    } else if (command->id ==
               CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT) {
      restoreCommandState(static_cast<CommandListClearUnorderedAccessViewFloat*>(command));
    } else {
      stateService_.recorder_.record(command->commandWriter.release());
    }
  }
}

void CommandListService::restoreCommandState(CommandListOMSetRenderTargets* command) {
  bool changed = false;
  for (unsigned i = 0; i < command->renderTargetViews.size(); ++i) {
    D3D12RenderTargetViewState* view = command->renderTargetViews[i].get();
    if (view) {
      DescriptorState* descriptor = stateService_.descriptorService_.getDescriptorState(
          view->destDescriptorKey, view->destDescriptorIndex);
      if (!equalRtv(view, descriptor)) {
        createAuxiliaryRtv(view);
        changed = true;
      }
    }
  }
  D3D12DepthStencilViewState* view = command->depthStencilView.get();
  if (view) {
    DescriptorState* descriptor = stateService_.descriptorService_.getDescriptorState(
        view->destDescriptorKey, view->destDescriptorIndex);
    if (!equalDsv(view, descriptor)) {
      createAuxiliaryDsv(view);
      changed = true;
    }
  }

  if (changed) {
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.object_.key = command->commandListKey;
    c.NumRenderTargetDescriptors_.value = command->renderTargetViews.size();
    c.RTsSingleHandleToDescriptorRange_.value = command->rtsSingleHandleToDescriptorRange;
    c.pRenderTargetDescriptors_.interfaceKeys.resize(command->renderTargetViews.size());
    c.pRenderTargetDescriptors_.indexes.resize(command->renderTargetViews.size());
    c.pRenderTargetDescriptors_.size = command->renderTargetViews.size();
    c.pRenderTargetDescriptors_.value =
        static_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(stateService_.getUniqueFakePointer());
    for (unsigned i = 0; i < command->renderTargetViews.size(); ++i) {
      D3D12RenderTargetViewState* view = command->renderTargetViews[i].get();
      if (view) {
        c.pRenderTargetDescriptors_.interfaceKeys[i] = view->destDescriptorKey;
        c.pRenderTargetDescriptors_.indexes[i] = view->destDescriptorIndex;
      }
    }
    D3D12DepthStencilViewState* view = command->depthStencilView.get();
    if (view) {
      c.pDepthStencilDescriptor_.interfaceKeys[0] = view->destDescriptorKey;
      c.pDepthStencilDescriptor_.indexes[0] = view->destDescriptorIndex;
      c.pDepthStencilDescriptor_.size = 1;
      c.pDepthStencilDescriptor_.value =
          static_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(stateService_.getUniqueFakePointer());
    }
    stateService_.recorder_.record(new ID3D12GraphicsCommandListOMSetRenderTargetsWriter(c));
  } else {
    stateService_.recorder_.record(command->commandWriter.release());
  }
}

void CommandListService::restoreCommandState(CommandListClearRenderTargetView* command) {
  bool changed = false;
  D3D12RenderTargetViewState* view = command->renderTargetView.get();
  if (view) {
    DescriptorState* descriptor = stateService_.descriptorService_.getDescriptorState(
        view->destDescriptorKey, view->destDescriptorIndex);
    if (!equalRtv(view, descriptor)) {
      createAuxiliaryRtv(view);
      changed = true;
    }
  }
  if (changed) {
    ID3D12GraphicsCommandListClearRenderTargetViewCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.object_.key = command->commandListKey;
    c.RenderTargetView_.value = view->destDescriptor;
    c.RenderTargetView_.interfaceKey = view->destDescriptorKey;
    c.RenderTargetView_.index = view->destDescriptorIndex;
    for (unsigned i = 0; i < 4; ++i) {
      c.ColorRGBA_.value[i] = command->colorRGBA[i];
    }
    if (!command->rects.empty()) {
      c.NumRects_.value = command->rects.size();
      c.pRects_.size = command->rects.size();
      c.pRects_.value = command->rects.data();
    }
    stateService_.recorder_.record(new ID3D12GraphicsCommandListClearRenderTargetViewWriter(c));
  } else {
    stateService_.recorder_.record(command->commandWriter.release());
  }
}

void CommandListService::restoreCommandState(CommandListClearDepthStencilView* command) {
  bool changed = false;
  D3D12DepthStencilViewState* view = command->depthStencilView.get();
  if (view) {
    DescriptorState* descriptor = stateService_.descriptorService_.getDescriptorState(
        view->destDescriptorKey, view->destDescriptorIndex);
    if (!equalDsv(view, descriptor)) {
      createAuxiliaryDsv(view);
      changed = true;
    }
  }
  if (changed) {
    ID3D12GraphicsCommandListClearDepthStencilViewCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.object_.key = command->commandListKey;
    c.DepthStencilView_.value = view->destDescriptor;
    c.DepthStencilView_.interfaceKey = view->destDescriptorKey;
    c.DepthStencilView_.index = view->destDescriptorIndex;
    c.Depth_.value = command->depth;
    c.Stencil_.value = command->stencil;
    if (!command->rects.empty()) {
      c.NumRects_.value = command->rects.size();
      c.pRects_.size = command->rects.size();
      c.pRects_.value = command->rects.data();
    }
    stateService_.recorder_.record(new ID3D12GraphicsCommandListClearDepthStencilViewWriter(c));
  } else {
    stateService_.recorder_.record(command->commandWriter.release());
  }
}

template <typename CommandListClearUnorderedAccessView>
void CommandListService::restoreCommandState(CommandListClearUnorderedAccessView* command) {
  bool changedGpu = false;
  D3D12UnorderedAccessViewState* viewGPUHandleInCurrentHeap =
      command->viewGPUHandleInCurrentHeap.get();
  if (viewGPUHandleInCurrentHeap) {
    DescriptorState* descriptor = stateService_.descriptorService_.getDescriptorState(
        viewGPUHandleInCurrentHeap->destDescriptorKey,
        viewGPUHandleInCurrentHeap->destDescriptorIndex);
    if (!equalUav(viewGPUHandleInCurrentHeap, descriptor)) {
      createAuxiliaryUavGpu(viewGPUHandleInCurrentHeap);
      changedGpu = true;
    }
  }
  bool changedCpu = false;
  D3D12UnorderedAccessViewState* viewCPUHandle = command->viewCPUHandle.get();
  if (viewCPUHandle) {
    DescriptorState* descriptor = stateService_.descriptorService_.getDescriptorState(
        viewCPUHandle->destDescriptorKey, viewCPUHandle->destDescriptorIndex);
    if (!equalUav(viewCPUHandle, descriptor)) {
      createAuxiliaryUavCpu(viewCPUHandle);
      changedCpu = true;
    }
  }
  if (changedGpu || changedCpu) {
    if (changedGpu) {
      ID3D12GraphicsCommandListSetDescriptorHeapsCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.object_.key = command->commandListKey;
      c.NumDescriptorHeaps_.value = 1;
      c.ppDescriptorHeaps_.keys.resize(1);
      c.ppDescriptorHeaps_.size = 1;
      c.ppDescriptorHeaps_.keys[0] = auxiliaryUavGpuDescriptorHeapKey_;
      ID3D12DescriptorHeap* fakePointer =
          static_cast<ID3D12DescriptorHeap*>(stateService_.getUniqueFakePointer());
      c.ppDescriptorHeaps_.value = &fakePointer;
      stateService_.recorder_.record(new ID3D12GraphicsCommandListSetDescriptorHeapsWriter(c));
    }
    {
      ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.object_.key = command->commandListKey;
      c.ViewGPUHandleInCurrentHeap_.interfaceKey = viewGPUHandleInCurrentHeap->destDescriptorKey;
      c.ViewGPUHandleInCurrentHeap_.index = viewGPUHandleInCurrentHeap->destDescriptorIndex;
      c.ViewCPUHandle_.value = viewCPUHandle->destDescriptor;
      c.ViewCPUHandle_.interfaceKey = viewCPUHandle->destDescriptorKey;
      c.ViewCPUHandle_.index = viewCPUHandle->destDescriptorIndex;
      c.pResource_.key = command->resourceKey;
      for (unsigned i = 0; i < 4; ++i) {
        c.Values_.value[i] = command->values[i];
      }
      if (!command->rects.empty()) {
        c.NumRects_.value = command->rects.size();
        c.pRects_.size = command->rects.size();
        c.pRects_.value = command->rects.data();
      }
      stateService_.recorder_.record(
          new ID3D12GraphicsCommandListClearUnorderedAccessViewUintWriter(c));
    }
    if (changedGpu) {
      CommandListState* commandListState = commandListsByKey_[command->commandListKey];
      ID3D12GraphicsCommandListSetDescriptorHeapsCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.object_.key = command->commandListKey;
      c.NumDescriptorHeaps_.value = commandListState->descriptorHeapKeys.size();
      c.ppDescriptorHeaps_.size = c.NumDescriptorHeaps_.value;
      c.ppDescriptorHeaps_.keys = commandListState->descriptorHeapKeys;
      ID3D12DescriptorHeap* fakePointer =
          static_cast<ID3D12DescriptorHeap*>(stateService_.getUniqueFakePointer());
      c.ppDescriptorHeaps_.value = &fakePointer;
      stateService_.recorder_.record(new ID3D12GraphicsCommandListSetDescriptorHeapsWriter(c));
    }
  } else {
    stateService_.recorder_.record(command->commandWriter.release());
  }
}

void CommandListService::initAuxiliaryRtvHeap(unsigned deviceKey) {
  if (auxiliaryRtvDescriptorHeapKey_) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  desc.NumDescriptors = auxiliaryHeapSize_;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  c.pDescriptorHeapDesc_.value = &desc;
  c.riid_.value = IID_ID3D12DescriptorHeap;
  auxiliaryRtvDescriptorHeapKey_ = stateService_.getUniqueObjectKey();
  c.ppvHeap_.key = auxiliaryRtvDescriptorHeapKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateDescriptorHeapWriter(c));
}

void CommandListService::initAuxiliaryDsvHeap(unsigned deviceKey) {
  if (auxiliaryDsvDescriptorHeapKey_) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  desc.NumDescriptors = auxiliaryHeapSize_;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  c.pDescriptorHeapDesc_.value = &desc;
  c.riid_.value = IID_ID3D12DescriptorHeap;
  auxiliaryDsvDescriptorHeapKey_ = stateService_.getUniqueObjectKey();
  c.ppvHeap_.key = auxiliaryDsvDescriptorHeapKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateDescriptorHeapWriter(c));
}

void CommandListService::initAuxiliaryUavGpuHeap(unsigned deviceKey) {
  if (auxiliaryUavGpuDescriptorHeapKey_) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = auxiliaryHeapSize_;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  desc.NodeMask = 1;
  c.pDescriptorHeapDesc_.value = &desc;
  c.riid_.value = IID_ID3D12DescriptorHeap;
  auxiliaryUavGpuDescriptorHeapKey_ = stateService_.getUniqueObjectKey();
  c.ppvHeap_.key = auxiliaryUavGpuDescriptorHeapKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateDescriptorHeapWriter(c));
}

void CommandListService::initAuxiliaryUavCpuHeap(unsigned deviceKey) {
  if (auxiliaryUavCpuDescriptorHeapKey_) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = auxiliaryHeapSize_;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  c.pDescriptorHeapDesc_.value = &desc;
  c.riid_.value = IID_ID3D12DescriptorHeap;
  auxiliaryUavCpuDescriptorHeapKey_ = stateService_.getUniqueObjectKey();
  c.ppvHeap_.key = auxiliaryUavCpuDescriptorHeapKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateDescriptorHeapWriter(c));
}

void CommandListService::createAuxiliaryRtv(D3D12RenderTargetViewState* view) {
  initAuxiliaryRtvHeap(view->deviceKey);

  if (auxiliaryRtvDescriptorHeapIndex_ >= auxiliaryHeapSize_) {
    Log(ERR) << "Auxiliary RTV descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateRenderTargetViewCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = view->deviceKey;
  c.pResource_.key = view->resourceKey;
  c.pDesc_.value = &view->desc;
  c.DestDescriptor_.interfaceKey = auxiliaryRtvDescriptorHeapKey_;
  c.DestDescriptor_.index = auxiliaryRtvDescriptorHeapIndex_;
  stateService_.recorder_.record(new ID3D12DeviceCreateRenderTargetViewWriter(c));
  view->destDescriptorKey = auxiliaryRtvDescriptorHeapKey_;
  view->destDescriptorIndex = auxiliaryRtvDescriptorHeapIndex_;
  ++auxiliaryRtvDescriptorHeapIndex_;
}

void CommandListService::createAuxiliaryDsv(D3D12DepthStencilViewState* view) {
  initAuxiliaryDsvHeap(view->deviceKey);

  if (auxiliaryDsvDescriptorHeapIndex_ >= auxiliaryHeapSize_) {
    Log(ERR) << "Auxiliary DSV descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateDepthStencilViewCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = view->deviceKey;
  c.pResource_.key = view->resourceKey;
  c.pDesc_.value = &view->desc;
  c.DestDescriptor_.interfaceKey = auxiliaryDsvDescriptorHeapKey_;
  c.DestDescriptor_.index = auxiliaryDsvDescriptorHeapIndex_;
  stateService_.recorder_.record(new ID3D12DeviceCreateDepthStencilViewWriter(c));
  view->destDescriptorKey = auxiliaryDsvDescriptorHeapKey_;
  view->destDescriptorIndex = auxiliaryDsvDescriptorHeapIndex_;
  ++auxiliaryDsvDescriptorHeapIndex_;
}

void CommandListService::createAuxiliaryUavGpu(D3D12UnorderedAccessViewState* view) {
  initAuxiliaryUavGpuHeap(view->deviceKey);

  if (auxiliaryUavGpuDescriptorHeapIndex_ >= auxiliaryHeapSize_) {
    Log(ERR) << "Auxiliary UAV GPU descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateUnorderedAccessViewCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = view->deviceKey;
  c.pResource_.key = view->resourceKey;
  c.pDesc_.value = &view->desc;
  c.DestDescriptor_.interfaceKey = auxiliaryUavGpuDescriptorHeapKey_;
  c.DestDescriptor_.index = auxiliaryUavGpuDescriptorHeapIndex_;
  stateService_.recorder_.record(new ID3D12DeviceCreateUnorderedAccessViewWriter(c));
  view->destDescriptorKey = auxiliaryUavGpuDescriptorHeapKey_;
  view->destDescriptorIndex = auxiliaryUavGpuDescriptorHeapIndex_;
  ++auxiliaryUavGpuDescriptorHeapIndex_;
}

void CommandListService::createAuxiliaryUavCpu(D3D12UnorderedAccessViewState* view) {
  initAuxiliaryUavCpuHeap(view->deviceKey);

  if (auxiliaryUavCpuDescriptorHeapIndex_ >= auxiliaryHeapSize_) {
    Log(ERR) << "Auxiliary UAV CPU descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateUnorderedAccessViewCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.object_.key = view->deviceKey;
  c.pResource_.key = view->resourceKey;
  c.pDesc_.value = &view->desc;
  c.DestDescriptor_.interfaceKey = auxiliaryUavCpuDescriptorHeapKey_;
  c.DestDescriptor_.index = auxiliaryUavCpuDescriptorHeapIndex_;
  stateService_.recorder_.record(new ID3D12DeviceCreateUnorderedAccessViewWriter(c));
  view->destDescriptorKey = auxiliaryUavCpuDescriptorHeapKey_;
  view->destDescriptorIndex = auxiliaryUavCpuDescriptorHeapIndex_;
  ++auxiliaryUavCpuDescriptorHeapIndex_;
}

bool CommandListService::equalRtv(D3D12RenderTargetViewState* view, DescriptorState* descriptor) {
  if (descriptor->id != DescriptorState::D3D12_RENDERTARGETVIEW ||
      descriptor->resourceKey != view->resourceKey) {
    return false;
  }
  D3D12RenderTargetViewState* descriptorView = static_cast<D3D12RenderTargetViewState*>(descriptor);
  if (view->isDesc != descriptorView->isDesc ||
      memcmp(&view->desc, &descriptorView->desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC))) {
    return false;
  }
  return true;
}

bool CommandListService::equalDsv(D3D12DepthStencilViewState* view, DescriptorState* descriptor) {
  if (descriptor->id != DescriptorState::D3D12_DEPTHSTENCILVIEW ||
      descriptor->resourceKey != view->resourceKey) {
    return false;
  }
  D3D12DepthStencilViewState* descriptorView = static_cast<D3D12DepthStencilViewState*>(descriptor);
  if (view->isDesc != descriptorView->isDesc ||
      memcmp(&view->desc, &descriptorView->desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC))) {
    return false;
  }
  return true;
}

bool CommandListService::equalUav(D3D12UnorderedAccessViewState* view,
                                  DescriptorState* descriptor) {
  if (descriptor->id != DescriptorState::D3D12_UNORDEREDACCESSVIEW ||
      descriptor->resourceKey != view->resourceKey) {
    return false;
  }
  D3D12UnorderedAccessViewState* descriptorView =
      static_cast<D3D12UnorderedAccessViewState*>(descriptor);
  if (view->isDesc != descriptorView->isDesc ||
      memcmp(&view->desc, &descriptorView->desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC))) {
    return false;
  }
  return true;
}

} // namespace DirectX
} // namespace gits
