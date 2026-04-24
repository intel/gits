// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectAwaitLayer.h"
#include "interfaceArgumentUpdaters.h"

namespace gits {
namespace DirectX {

std::optional<MultithreadedObjectCreationService::ObjectCreationOutput>
MultithreadedObjectAwaitLayer::CollectResult(unsigned objectKey) {
  auto it = m_PreCollectedOutputs.find(objectKey);
  if (it != m_PreCollectedOutputs.end()) {
    auto result = it->second;
    m_PreCollectedOutputs.erase(it);
    return result;
  }
  return m_Manager.GetMultithreadedObjectCreationService().Complete(objectKey);
}

bool MultithreadedObjectAwaitLayer::CompleteObject(unsigned key) {
  if (!key || m_Manager.FindObject(key)) {
    return false;
  }

  auto creationOutput = CollectResult(key);
  if (!creationOutput.has_value()) {
    return false;
  }

  if (creationOutput.value().result != S_OK) {
    return false;
  }

  m_Manager.AddObject(key, static_cast<IUnknown*>(creationOutput.value().object));
  return true;
}

template <typename T>
void MultithreadedObjectAwaitLayer::CompleteArgument(InterfaceArgument<T>& argument) {
  if (CompleteObject(argument.Key)) {
    UpdateInterface(m_Manager, argument);
  }
}

template <typename T>
void MultithreadedObjectAwaitLayer::CompleteArrayArgument(
    InterfaceArrayArgument<T>& arrayArgument) {
  bool requiresUpdate = false;
  for (size_t i = 0; i < arrayArgument.Size; ++i) {
    if (CompleteObject(arrayArgument.Keys[i])) {
      requiresUpdate = true;
    }
  }

  if (requiresUpdate) {
    UpdateInterface(m_Manager, arrayArgument);
  }
}

MultithreadedObjectAwaitLayer::MultithreadedObjectAwaitLayer(PlayerManager& manager)
    : Layer("MultithreadedObjectAwaitLayer"), m_Manager(manager) {}

void MultithreadedObjectAwaitLayer::Pre(IUnknownAddRefCommand& c) {
  // If the object has not been created yet, defer the reference count update
  if (!m_Manager.FindObject(c.m_Object.Key)) {
    auto& service = m_Manager.GetMultithreadedObjectCreationService();
    bool scheduled = service.ScheduleUpdateRefCount(c.m_Object.Key, 1);
    if (scheduled) {
      c.Skip = true;
    } else {
      CompleteArgument(c.m_Object);
    }
  }
}

void MultithreadedObjectAwaitLayer::Pre(IUnknownReleaseCommand& c) {
  auto& service = m_Manager.GetMultithreadedObjectCreationService();
  if (c.m_Result.Value == 0) {
    CompleteArgument(c.m_Object);

    // If the object is being released we need to create all objects that depend on it
    auto objectsToComplete = service.CollectConsumers(c.m_Object.Key);
    for (const auto objectKey : objectsToComplete) {
      auto creationOutput = service.Complete(objectKey);
      if (!creationOutput.has_value()) {
        continue;
      }

      auto [it, inserted] = m_PreCollectedOutputs.insert({objectKey, creationOutput.value()});
      GITS_ASSERT(inserted);
    }

    return;
  }

  // If the object has not been created yet, defer the reference count update
  if (!m_Manager.FindObject(c.m_Object.Key)) {
    bool scheduled = service.ScheduleUpdateRefCount(c.m_Object.Key, -1);
    if (scheduled) {
      c.Skip = true;
    } else {
      CompleteArgument(c.m_Object);
    }
  }
}

void MultithreadedObjectAwaitLayer::Pre(IUnknownQueryInterfaceCommand& c) {
  CompleteArgument(c.m_Object);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12ObjectGetPrivateDataCommand& c) {
  // This command is not necessary (skipping it prevents any potential stalling)
  c.Skip = true;
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12ObjectSetNameCommand& c) {
  // This command is not necessary (skipping it prevents any potential stalling)
  c.Skip = true;
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12ObjectSetPrivateDataCommand& c) {
  // This command is not necessary (skipping it prevents any potential stalling)
  c.Skip = true;
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12ObjectSetPrivateDataInterfaceCommand& c) {
  // This command is not necessary (skipping it prevents any potential stalling)
  c.Skip = true;
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12DeviceChildGetDeviceCommand& c) {
  CompleteArgument(c.m_Object);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12PipelineStateGetCachedBlobCommand& c) {
  // This command is not necessary (skipping it prevents any potential stalling)
  c.Skip = true;
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  CompleteArgument(c.m_pInitialState);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12GraphicsCommandListResetCommand& c) {
  CompleteArgument(c.m_pInitialState);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12GraphicsCommandListClearStateCommand& c) {
  CompleteArgument(c.m_pPipelineState);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  CompleteArgument(c.m_pPipelineState);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  CompleteArgument(c.m_pStateObject);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12DeviceMakeResidentCommand& c) {
  CompleteArrayArgument(c.m_ppObjects);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12DeviceEvictCommand& c) {
  CompleteArrayArgument(c.m_ppObjects);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12Device1SetResidencyPriorityCommand& c) {
  CompleteArrayArgument(c.m_ppObjects);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12Device3EnqueueMakeResidentCommand& c) {
  CompleteArrayArgument(c.m_ppObjects);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12Device5CreateStateObjectCommand& c) {
  bool requiresUpdate = false;
  for (unsigned index = 0; index < c.m_pDesc.Value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT* subobject =
        const_cast<D3D12_STATE_SUBOBJECT*>(&(c.m_pDesc.Value->pSubobjects[index]));
    if (subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
      unsigned objectKey = c.m_pDesc.InterfaceKeysBySubobject[index];
      if (!objectKey || m_Manager.FindObject(objectKey)) {
        continue;
      }

      if (CompleteObject(objectKey)) {
        requiresUpdate = true;
      }
    }
  }
  if (requiresUpdate) {
    UpdateInterface(m_Manager, c.m_pDesc);
  }
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12Device7AddToStateObjectCommand& c) {
  CompleteArgument(c.m_pStateObjectToGrowFrom);
  bool requiresUpdate = false;
  for (unsigned index = 0; index < c.m_pAddition.Value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT* subobject =
        const_cast<D3D12_STATE_SUBOBJECT*>(&(c.m_pAddition.Value->pSubobjects[index]));
    if (subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
      unsigned objectKey = c.m_pAddition.InterfaceKeysBySubobject[index];
      if (!objectKey || m_Manager.FindObject(objectKey)) {
        continue;
      }

      if (CompleteObject(objectKey)) {
        requiresUpdate = true;
      }
    }
  }
  if (requiresUpdate) {
    UpdateInterface(m_Manager, c.m_pAddition);
  }
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  CompleteObject(c.m_ppPipelineState.Key);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  CompleteObject(c.m_ppPipelineState.Key);
}

void MultithreadedObjectAwaitLayer::Pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  CompleteObject(c.m_ppPipelineState.Key);
}

} // namespace DirectX
} // namespace gits
