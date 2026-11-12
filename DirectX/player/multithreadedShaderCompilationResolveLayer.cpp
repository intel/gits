// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedShaderCompilationResolveLayer.h"
#include "arguments.h"
#include "interfaceArgumentUpdaters.h"
#include "pipelineLibraryService.h"

namespace gits {
namespace DirectX {

bool MultithreadedShaderCompilationResolveLayer::ResolveObject(ObjectKey key) {
  if (!key || m_Manager.FindObject(key)) {
    return false;
  }
  return ResolvePending(key);
}

bool MultithreadedShaderCompilationResolveLayer::ResolvePending(ObjectKey key) {
  auto creationOutput = m_Manager.GetMultithreadedShaderCompilationService().Complete(key);
  if (!creationOutput.has_value() || creationOutput.value().result != S_OK) {
    return false;
  }

  m_Manager.AddObject(key, static_cast<IUnknown*>(creationOutput.value().object));
  return true;
}

template <typename T>
void MultithreadedShaderCompilationResolveLayer::ResolveArgument(InterfaceArgument<T>& argument) {
  if (ResolveObject(argument.Key)) {
    UpdateInterface(m_Manager, argument);
  }
}

template <typename T>
void MultithreadedShaderCompilationResolveLayer::ResolveArrayArgument(
    InterfaceArrayArgument<T>& arrayArgument) {
  bool requiresUpdate = false;
  for (size_t i = 0; i < arrayArgument.Size; ++i) {
    if (ResolveObject(arrayArgument.Keys[i])) {
      requiresUpdate = true;
    }
  }

  if (requiresUpdate) {
    UpdateInterface(m_Manager, arrayArgument);
  }
}

void MultithreadedShaderCompilationResolveLayer::ResolveExistingCollections(
    const std::map<unsigned, ObjectKey>& interfaceKeysBySubobject,
    const D3D12_STATE_OBJECT_DESC* desc,
    bool& requiresUpdate) {
  for (unsigned index = 0; index < desc->NumSubobjects; ++index) {
    const D3D12_STATE_SUBOBJECT& subobject = desc->pSubobjects[index];
    if (subobject.Type != D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
      continue;
    }
    auto it = interfaceKeysBySubobject.find(index);
    if (it == interfaceKeysBySubobject.end()) {
      continue;
    }
    if (ResolveObject(it->second)) {
      requiresUpdate = true;
    }
  }
}

MultithreadedShaderCompilationResolveLayer::MultithreadedShaderCompilationResolveLayer(
    PlayerManager& manager)
    : Layer("MultithreadedShaderCompilationResolve"), m_Manager(manager) {}

void MultithreadedShaderCompilationResolveLayer::Pre(StateRestoreEndCommand& c) {
  m_Manager.CompleteAllShaderCompilations();
}

void MultithreadedShaderCompilationResolveLayer::Pre(IUnknownAddRefCommand& c) {
  if (!m_Manager.FindObject(c.m_Object.Key)) {
    auto& service = m_Manager.GetMultithreadedShaderCompilationService();
    bool scheduled = service.ScheduleUpdateRefCount(c.m_Object.Key, 1);
    if (scheduled) {
      c.Skip = true;
    } else {
      ResolveArgument(c.m_Object);
    }
  }
}

void MultithreadedShaderCompilationResolveLayer::Pre(IUnknownReleaseCommand& c) {
  auto& service = m_Manager.GetMultithreadedShaderCompilationService();
  if (c.m_Result.Value == 0) {
    if (!c.Skip && service.CancelPendingTask(c.m_Object.Key)) {
      m_Manager.GetPipelineLibraryService().ReleasePipelineState(c.m_Object.Key, 0);
      c.Skip = true;
      return;
    }

    ResolveArgument(c.m_Object);
    return;
  }

  if (!m_Manager.FindObject(c.m_Object.Key)) {
    bool scheduled = service.ScheduleUpdateRefCount(c.m_Object.Key, -1);
    if (scheduled) {
      c.Skip = true;
    } else {
      ResolveArgument(c.m_Object);
    }
  }
}

void MultithreadedShaderCompilationResolveLayer::Pre(IUnknownQueryInterfaceCommand& c) {
  ResolveArgument(c.m_Object);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12ObjectGetPrivateDataCommand& c) {
  c.Skip = true;
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12ObjectSetNameCommand& c) {
  // Name of an object still being compiled is applied once it completes
  if (!c.Skip && !m_Manager.FindObject(c.m_Object.Key) &&
      m_Manager.GetMultithreadedShaderCompilationService().ScheduleSetName(
          c.m_Object.Key, c.m_Name.Value ? c.m_Name.Value : L"")) {
    c.Skip = true;
  }
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12ObjectSetPrivateDataCommand& c) {
  c.Skip = true;
}

void MultithreadedShaderCompilationResolveLayer::Pre(
    ID3D12ObjectSetPrivateDataInterfaceCommand& c) {
  c.Skip = true;
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12DeviceChildGetDeviceCommand& c) {
  ResolveArgument(c.m_Object);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12PipelineStateGetCachedBlobCommand& c) {
  c.Skip = true;
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  ResolveArgument(c.m_pInitialState);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12GraphicsCommandListResetCommand& c) {
  ResolveArgument(c.m_pInitialState);
}

void MultithreadedShaderCompilationResolveLayer::Pre(
    ID3D12GraphicsCommandListClearStateCommand& c) {
  ResolveArgument(c.m_pPipelineState);
}

void MultithreadedShaderCompilationResolveLayer::Pre(
    ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  ResolveArgument(c.m_pPipelineState);
}

void MultithreadedShaderCompilationResolveLayer::Pre(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  ResolveArgument(c.m_pStateObject);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12DeviceMakeResidentCommand& c) {
  ResolveArrayArgument(c.m_ppObjects);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12DeviceEvictCommand& c) {
  ResolveArrayArgument(c.m_ppObjects);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12Device1SetResidencyPriorityCommand& c) {
  ResolveArrayArgument(c.m_ppObjects);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12Device3EnqueueMakeResidentCommand& c) {
  ResolveArrayArgument(c.m_ppObjects);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12Device5CreateStateObjectCommand& c) {
  ResolvePending(c.m_ppStateObject.Key);

  bool requiresUpdate = false;
  ResolveExistingCollections(c.m_pDesc.InterfaceKeysBySubobject, c.m_pDesc.Value, requiresUpdate);
  if (requiresUpdate) {
    UpdateInterface(m_Manager, c.m_pDesc);
  }
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12Device7AddToStateObjectCommand& c) {
  ResolveArgument(c.m_pStateObjectToGrowFrom);
  bool requiresUpdate = false;
  ResolveExistingCollections(c.m_pAddition.InterfaceKeysBySubobject, c.m_pAddition.Value,
                             requiresUpdate);
  if (requiresUpdate) {
    UpdateInterface(m_Manager, c.m_pAddition);
  }
}

void MultithreadedShaderCompilationResolveLayer::Pre(
    ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  ResolvePending(c.m_ppPipelineState.Key);
}

void MultithreadedShaderCompilationResolveLayer::Pre(
    ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  ResolvePending(c.m_ppPipelineState.Key);
}

void MultithreadedShaderCompilationResolveLayer::Pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  ResolvePending(c.m_ppPipelineState.Key);
}

} // namespace DirectX
} // namespace gits
