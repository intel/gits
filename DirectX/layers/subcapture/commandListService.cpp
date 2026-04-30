// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListService.h"
#include "commandsAuto.h"
#include "stateTrackingService.h"
#include "commandSerializersAuto.h"
#include "analyzerResults.h"
#include "commandSerializersFactory.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace DirectX {

CommandListService::CommandListService(StateTrackingService& stateService)
    : m_StateService(stateService) {

  m_RestoreCommandLists = m_StateService.GetAnalyzerResults().RestoreCommandLists();
}

void CommandListService::AddCommandList(CommandListState* state) {
  m_CommandListsByKey[state->Key] = state;
}
void CommandListService::RemoveCommandList(unsigned key) {
  m_CommandListsByKey.erase(key);
}

void CommandListService::RestoreCommandLists() {
  if (!m_RestoreCommandLists) {
    return;
  }

  std::map<unsigned, CommandListCommand*> commandsByKey;
  std::map<unsigned, unsigned> commandListAllocatorsForReset;

  for (auto& it : m_CommandListsByKey) {
    if (!m_StateService.GetAnalyzerResults().RestoreCommandList(it.first)) {
      continue;
    }
    CommandListState* state = it.second;
    if (state->AllocatorKey && !state->Commands.empty()) {
      if (state->Commands.front()->Id != CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESET) {
        commandListAllocatorsForReset[state->Key] = state->AllocatorKey;
      }
    }
    for (CommandListCommand* command : state->Commands) {
      commandsByKey[command->CommandKey] = command;
    }
  }

  for (auto& it : commandsByKey) {
    CommandListCommand* command = it.second;

    auto itReset = commandListAllocatorsForReset.find(command->CommandListKey);
    if (itReset != commandListAllocatorsForReset.end()) {
      ID3D12GraphicsCommandListResetCommand reset;
      reset.Key = m_StateService.GetUniqueCommandKey();
      reset.m_Object.Key = itReset->first;
      reset.m_pAllocator.Key = itReset->second;
      m_StateService.GetRecorder().Record(*createCommandSerializer(&reset));
      commandListAllocatorsForReset.erase(itReset);
    }

    if (command->Id == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS) {
      RestoreCommandState(static_cast<CommandListOMSetRenderTargets*>(command));
    } else if (command->Id == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW) {
      RestoreCommandState(static_cast<CommandListClearRenderTargetView*>(command));
    } else if (command->Id == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW) {
      RestoreCommandState(static_cast<CommandListClearDepthStencilView*>(command));
    } else if (command->Id ==
               CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT) {
      RestoreCommandState(static_cast<CommandListClearUnorderedAccessViewUint*>(command));
    } else if (command->Id ==
               CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT) {
      RestoreCommandState(static_cast<CommandListClearUnorderedAccessViewFloat*>(command));
    } else {
      m_StateService.GetRecorder().Record(*command->CommandSerializer);
    }
  }
}

void CommandListService::RestoreCommandState(CommandListOMSetRenderTargets* command) {
  bool changed = false;
  for (unsigned i = 0; i < command->RenderTargetViews.size(); ++i) {
    D3D12RenderTargetViewState* view = command->RenderTargetViews[i].get();
    if (view) {
      DescriptorState* descriptor = m_StateService.GetDescriptorService().GetDescriptorState(
          view->DestDescriptorKey, view->DestDescriptorIndex);
      if (!EqualRtv(view, descriptor)) {
        CreateAuxiliaryRtv(view);
        changed = true;
      }
    }
  }
  D3D12DepthStencilViewState* view = command->DepthStencilView.get();
  if (view) {
    DescriptorState* descriptor = m_StateService.GetDescriptorService().GetDescriptorState(
        view->DestDescriptorKey, view->DestDescriptorIndex);
    if (!EqualDsv(view, descriptor)) {
      CreateAuxiliaryDsv(view);
      changed = true;
    }
  }

  if (changed) {
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = command->CommandListKey;
    c.m_NumRenderTargetDescriptors.Value = command->RenderTargetViews.size();
    c.m_RTsSingleHandleToDescriptorRange.Value = command->RtsSingleHandleToDescriptorRange;
    c.m_pRenderTargetDescriptors.InterfaceKeys.resize(command->RenderTargetViews.size());
    c.m_pRenderTargetDescriptors.Indexes.resize(command->RenderTargetViews.size());
    c.m_pRenderTargetDescriptors.Size = command->RenderTargetViews.size();
    c.m_pRenderTargetDescriptors.Value =
        static_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(m_StateService.GetUniqueFakePointer());
    for (unsigned i = 0; i < command->RenderTargetViews.size(); ++i) {
      D3D12RenderTargetViewState* view = command->RenderTargetViews[i].get();
      if (view) {
        c.m_pRenderTargetDescriptors.InterfaceKeys[i] = view->DestDescriptorKey;
        c.m_pRenderTargetDescriptors.Indexes[i] = view->DestDescriptorIndex;
      }
    }
    D3D12DepthStencilViewState* view = command->DepthStencilView.get();
    if (view) {
      c.m_pDepthStencilDescriptor.InterfaceKeys.resize(1);
      c.m_pDepthStencilDescriptor.Indexes.resize(1);
      c.m_pDepthStencilDescriptor.InterfaceKeys[0] = view->DestDescriptorKey;
      c.m_pDepthStencilDescriptor.Indexes[0] = view->DestDescriptorIndex;
      c.m_pDepthStencilDescriptor.Size = 1;
      c.m_pDepthStencilDescriptor.Value =
          static_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(m_StateService.GetUniqueFakePointer());
    }
    m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListOMSetRenderTargetsSerializer(c));
  } else {
    m_StateService.GetRecorder().Record(*command->CommandSerializer);
  }
}

void CommandListService::RestoreCommandState(CommandListClearRenderTargetView* command) {
  bool changed = false;
  D3D12RenderTargetViewState* view = command->RenderTargetView.get();
  if (view) {
    DescriptorState* descriptor = m_StateService.GetDescriptorService().GetDescriptorState(
        view->DestDescriptorKey, view->DestDescriptorIndex);
    if (!EqualRtv(view, descriptor)) {
      CreateAuxiliaryRtv(view);
      changed = true;
    }
  }
  if (changed) {
    ID3D12GraphicsCommandListClearRenderTargetViewCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = command->CommandListKey;
    c.m_RenderTargetView.Value = view->DestDescriptor;
    c.m_RenderTargetView.InterfaceKey = view->DestDescriptorKey;
    c.m_RenderTargetView.Index = view->DestDescriptorIndex;
    for (unsigned i = 0; i < 4; ++i) {
      c.m_ColorRGBA.Value[i] = command->ColorRGBA[i];
    }
    if (!command->Rects.empty()) {
      c.m_NumRects.Value = command->Rects.size();
      c.m_pRects.Size = command->Rects.size();
      c.m_pRects.Value = command->Rects.data();
    }
    m_StateService.GetRecorder().Record(
        ID3D12GraphicsCommandListClearRenderTargetViewSerializer(c));
  } else {
    m_StateService.GetRecorder().Record(*command->CommandSerializer);
  }
}

void CommandListService::RestoreCommandState(CommandListClearDepthStencilView* command) {
  bool changed = false;
  D3D12DepthStencilViewState* view = command->m_DepthStencilView.get();
  if (view) {
    DescriptorState* descriptor = m_StateService.GetDescriptorService().GetDescriptorState(
        view->DestDescriptorKey, view->DestDescriptorIndex);
    if (!EqualDsv(view, descriptor)) {
      CreateAuxiliaryDsv(view);
      changed = true;
    }
  }
  if (changed) {
    ID3D12GraphicsCommandListClearDepthStencilViewCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = command->CommandListKey;
    c.m_DepthStencilView.Value = view->DestDescriptor;
    c.m_DepthStencilView.InterfaceKey = view->DestDescriptorKey;
    c.m_DepthStencilView.Index = view->DestDescriptorIndex;
    c.m_Depth.Value = command->Depth;
    c.m_Stencil.Value = command->Stencil;
    if (!command->Rects.empty()) {
      c.m_NumRects.Value = command->Rects.size();
      c.m_pRects.Size = command->Rects.size();
      c.m_pRects.Value = command->Rects.data();
    }
    m_StateService.GetRecorder().Record(
        ID3D12GraphicsCommandListClearDepthStencilViewSerializer(c));
  } else {
    m_StateService.GetRecorder().Record(*command->CommandSerializer);
  }
}

template <typename CommandListClearUnorderedAccessView>
void CommandListService::RestoreCommandState(CommandListClearUnorderedAccessView* command) {
  bool changedGpu = false;
  D3D12UnorderedAccessViewState* viewGPUHandleInCurrentHeap =
      command->ViewGPUHandleInCurrentHeap.get();
  if (viewGPUHandleInCurrentHeap) {
    DescriptorState* descriptor = m_StateService.GetDescriptorService().GetDescriptorState(
        viewGPUHandleInCurrentHeap->DestDescriptorKey,
        viewGPUHandleInCurrentHeap->DestDescriptorIndex);
    if (!EqualUav(viewGPUHandleInCurrentHeap, descriptor)) {
      CreateAuxiliaryUavGpu(viewGPUHandleInCurrentHeap);
      changedGpu = true;
    }
  }
  bool changedCpu = false;
  D3D12UnorderedAccessViewState* viewCPUHandle = command->ViewCPUHandle.get();
  if (viewCPUHandle) {
    DescriptorState* descriptor = m_StateService.GetDescriptorService().GetDescriptorState(
        viewCPUHandle->DestDescriptorKey, viewCPUHandle->DestDescriptorIndex);
    if (!EqualUav(viewCPUHandle, descriptor)) {
      CreateAuxiliaryUavCpu(viewCPUHandle);
      changedCpu = true;
    }
  }
  if (changedGpu || changedCpu) {
    if (changedGpu) {
      ID3D12GraphicsCommandListSetDescriptorHeapsCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_Object.Key = command->CommandListKey;
      c.m_NumDescriptorHeaps.Value = 1;
      c.m_ppDescriptorHeaps.Keys.resize(1);
      c.m_ppDescriptorHeaps.Size = 1;
      c.m_ppDescriptorHeaps.Keys[0] = m_AuxiliaryUavGpuDescriptorHeapKey;
      ID3D12DescriptorHeap* fakePointer =
          static_cast<ID3D12DescriptorHeap*>(m_StateService.GetUniqueFakePointer());
      c.m_ppDescriptorHeaps.Value = &fakePointer;
      m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListSetDescriptorHeapsSerializer(c));
    }
    {
      ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_Object.Key = command->CommandListKey;
      if (viewGPUHandleInCurrentHeap) {
        c.m_ViewGPUHandleInCurrentHeap.InterfaceKey = viewGPUHandleInCurrentHeap->DestDescriptorKey;
        c.m_ViewGPUHandleInCurrentHeap.Index = viewGPUHandleInCurrentHeap->DestDescriptorIndex;
      }
      if (viewCPUHandle) {
        c.m_ViewCPUHandle.Value = viewCPUHandle->DestDescriptor;
        c.m_ViewCPUHandle.InterfaceKey = viewCPUHandle->DestDescriptorKey;
        c.m_ViewCPUHandle.Index = viewCPUHandle->DestDescriptorIndex;
      }
      c.m_pResource.Key = command->ResourceKey;
      for (unsigned i = 0; i < 4; ++i) {
        c.m_Values.Value[i] = command->Values[i];
      }
      if (!command->Rects.empty()) {
        c.m_NumRects.Value = command->Rects.size();
        c.m_pRects.Size = command->Rects.size();
        c.m_pRects.Value = command->Rects.data();
      }
      m_StateService.GetRecorder().Record(
          ID3D12GraphicsCommandListClearUnorderedAccessViewUintSerializer(c));
    }
    if (changedGpu) {
      CommandListState* commandListState = m_CommandListsByKey[command->CommandListKey];
      ID3D12GraphicsCommandListSetDescriptorHeapsCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_Object.Key = command->CommandListKey;
      c.m_NumDescriptorHeaps.Value = commandListState->DescriptorHeapKeys.size();
      c.m_ppDescriptorHeaps.Size = c.m_NumDescriptorHeaps.Value;
      c.m_ppDescriptorHeaps.Keys = commandListState->DescriptorHeapKeys;
      ID3D12DescriptorHeap* fakePointer =
          static_cast<ID3D12DescriptorHeap*>(m_StateService.GetUniqueFakePointer());
      c.m_ppDescriptorHeaps.Value = &fakePointer;
      m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListSetDescriptorHeapsSerializer(c));
    }
  } else {
    m_StateService.GetRecorder().Record(*command->CommandSerializer);
  }
}

void CommandListService::InitAuxiliaryRtvHeap(unsigned deviceKey) {
  if (m_AuxiliaryRtvDescriptorHeapKey) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  desc.NumDescriptors = m_AuxiliaryHeapSize;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  c.m_pDescriptorHeapDesc.Value = &desc;
  c.m_riid.Value = IID_ID3D12DescriptorHeap;
  m_AuxiliaryRtvDescriptorHeapKey = m_StateService.GetUniqueObjectKey();
  c.m_ppvHeap.Key = m_AuxiliaryRtvDescriptorHeapKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateDescriptorHeapSerializer(c));
}

void CommandListService::InitAuxiliaryDsvHeap(unsigned deviceKey) {
  if (m_AuxiliaryDsvDescriptorHeapKey) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  desc.NumDescriptors = m_AuxiliaryHeapSize;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  c.m_pDescriptorHeapDesc.Value = &desc;
  c.m_riid.Value = IID_ID3D12DescriptorHeap;
  m_AuxiliaryDsvDescriptorHeapKey = m_StateService.GetUniqueObjectKey();
  c.m_ppvHeap.Key = m_AuxiliaryDsvDescriptorHeapKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateDescriptorHeapSerializer(c));
}

void CommandListService::InitAuxiliaryUavGpuHeap(unsigned deviceKey) {
  if (m_AuxiliaryUavGpuDescriptorHeapKey) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = m_AuxiliaryHeapSize;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  desc.NodeMask = 1;
  c.m_pDescriptorHeapDesc.Value = &desc;
  c.m_riid.Value = IID_ID3D12DescriptorHeap;
  m_AuxiliaryUavGpuDescriptorHeapKey = m_StateService.GetUniqueObjectKey();
  c.m_ppvHeap.Key = m_AuxiliaryUavGpuDescriptorHeapKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateDescriptorHeapSerializer(c));
}

void CommandListService::InitAuxiliaryUavCpuHeap(unsigned deviceKey) {
  if (m_AuxiliaryUavCpuDescriptorHeapKey) {
    return;
  }
  ID3D12DeviceCreateDescriptorHeapCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = deviceKey;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = m_AuxiliaryHeapSize;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  c.m_pDescriptorHeapDesc.Value = &desc;
  c.m_riid.Value = IID_ID3D12DescriptorHeap;
  m_AuxiliaryUavCpuDescriptorHeapKey = m_StateService.GetUniqueObjectKey();
  c.m_ppvHeap.Key = m_AuxiliaryUavCpuDescriptorHeapKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateDescriptorHeapSerializer(c));
}

void CommandListService::CreateAuxiliaryRtv(D3D12RenderTargetViewState* view) {
  InitAuxiliaryRtvHeap(view->DeviceKey);

  if (m_AuxiliaryRtvDescriptorHeapIndex >= m_AuxiliaryHeapSize) {
    LOG_ERROR << "Auxiliary RTV descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateRenderTargetViewCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = view->DeviceKey;
  c.m_pResource.Key = view->ResourceKey;
  c.m_pDesc.Value = &view->Desc;
  c.m_DestDescriptor.InterfaceKey = m_AuxiliaryRtvDescriptorHeapKey;
  c.m_DestDescriptor.Index = m_AuxiliaryRtvDescriptorHeapIndex;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateRenderTargetViewSerializer(c));
  view->DestDescriptorKey = m_AuxiliaryRtvDescriptorHeapKey;
  view->DestDescriptorIndex = m_AuxiliaryRtvDescriptorHeapIndex;
  ++m_AuxiliaryRtvDescriptorHeapIndex;
}

void CommandListService::CreateAuxiliaryDsv(D3D12DepthStencilViewState* view) {
  InitAuxiliaryDsvHeap(view->DeviceKey);

  if (m_AuxiliaryDsvDescriptorHeapIndex >= m_AuxiliaryHeapSize) {
    LOG_ERROR << "Auxiliary DSV descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateDepthStencilViewCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = view->DeviceKey;
  c.m_pResource.Key = view->ResourceKey;
  c.m_pDesc.Value = &view->Desc;
  c.m_DestDescriptor.InterfaceKey = m_AuxiliaryDsvDescriptorHeapKey;
  c.m_DestDescriptor.Index = m_AuxiliaryDsvDescriptorHeapIndex;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateDepthStencilViewSerializer(c));
  view->DestDescriptorKey = m_AuxiliaryDsvDescriptorHeapKey;
  view->DestDescriptorIndex = m_AuxiliaryDsvDescriptorHeapIndex;
  ++m_AuxiliaryDsvDescriptorHeapIndex;
}

void CommandListService::CreateAuxiliaryUavGpu(D3D12UnorderedAccessViewState* view) {
  InitAuxiliaryUavGpuHeap(view->DeviceKey);

  if (m_AuxiliaryUavGpuDescriptorHeapIndex >= m_AuxiliaryHeapSize) {
    LOG_ERROR << "Auxiliary UAV GPU descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateUnorderedAccessViewCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = view->DeviceKey;
  c.m_pResource.Key = view->ResourceKey;
  c.m_pDesc.Value = &view->Desc;
  c.m_DestDescriptor.InterfaceKey = m_AuxiliaryUavGpuDescriptorHeapKey;
  c.m_DestDescriptor.Index = m_AuxiliaryUavGpuDescriptorHeapIndex;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateUnorderedAccessViewSerializer(c));
  view->DestDescriptorKey = m_AuxiliaryUavGpuDescriptorHeapKey;
  view->DestDescriptorIndex = m_AuxiliaryUavGpuDescriptorHeapIndex;
  ++m_AuxiliaryUavGpuDescriptorHeapIndex;
}

void CommandListService::CreateAuxiliaryUavCpu(D3D12UnorderedAccessViewState* view) {
  InitAuxiliaryUavCpuHeap(view->DeviceKey);

  if (m_AuxiliaryUavCpuDescriptorHeapIndex >= m_AuxiliaryHeapSize) {
    LOG_ERROR << "Auxiliary UAV CPU descriptor heap too small.";
    exit(EXIT_FAILURE);
  }
  ID3D12DeviceCreateUnorderedAccessViewCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_Object.Key = view->DeviceKey;
  c.m_pResource.Key = view->ResourceKey;
  c.m_pDesc.Value = &view->Desc;
  c.m_DestDescriptor.InterfaceKey = m_AuxiliaryUavCpuDescriptorHeapKey;
  c.m_DestDescriptor.Index = m_AuxiliaryUavCpuDescriptorHeapIndex;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateUnorderedAccessViewSerializer(c));
  view->DestDescriptorKey = m_AuxiliaryUavCpuDescriptorHeapKey;
  view->DestDescriptorIndex = m_AuxiliaryUavCpuDescriptorHeapIndex;
  ++m_AuxiliaryUavCpuDescriptorHeapIndex;
}

bool CommandListService::EqualRtv(D3D12RenderTargetViewState* view, DescriptorState* descriptor) {
  if (descriptor->Id != DescriptorState::D3D12_RENDERTARGETVIEW ||
      descriptor->ResourceKey != view->ResourceKey) {
    return false;
  }
  D3D12RenderTargetViewState* descriptorView = static_cast<D3D12RenderTargetViewState*>(descriptor);
  if (view->IsDesc != descriptorView->IsDesc ||
      memcmp(&view->Desc, &descriptorView->Desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC))) {
    return false;
  }
  return true;
}

bool CommandListService::EqualDsv(D3D12DepthStencilViewState* view, DescriptorState* descriptor) {
  if (descriptor->Id != DescriptorState::D3D12_DEPTHSTENCILVIEW ||
      descriptor->ResourceKey != view->ResourceKey) {
    return false;
  }
  D3D12DepthStencilViewState* descriptorView = static_cast<D3D12DepthStencilViewState*>(descriptor);
  if (view->IsDesc != descriptorView->IsDesc ||
      memcmp(&view->Desc, &descriptorView->Desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC))) {
    return false;
  }
  return true;
}

bool CommandListService::EqualUav(D3D12UnorderedAccessViewState* view,
                                  DescriptorState* descriptor) {
  if (descriptor->Id != DescriptorState::D3D12_UNORDEREDACCESSVIEW ||
      descriptor->ResourceKey != view->ResourceKey) {
    return false;
  }
  D3D12UnorderedAccessViewState* descriptorView =
      static_cast<D3D12UnorderedAccessViewState*>(descriptor);
  if (view->IsDesc != descriptorView->IsDesc ||
      memcmp(&view->Desc, &descriptorView->Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC))) {
    return false;
  }
  return true;
}

} // namespace DirectX
} // namespace gits
