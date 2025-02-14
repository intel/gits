// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectAwaitLayer.h"
#include "interfaceArgumentUpdaters.h"

namespace gits {
namespace DirectX {

std::optional<MultithreadedObjectCreationService::ObjectCreationOutput>
MultithreadedObjectAwaitLayer::collectResult(unsigned objectKey) {
  auto it = preCollectedOutputs_.find(objectKey);
  if (it != preCollectedOutputs_.end()) {
    auto result = it->second;
    preCollectedOutputs_.erase(it);
    return result;
  }

  return manager_.getMultithreadedObjectCreationService().complete(objectKey);
}

bool MultithreadedObjectAwaitLayer::completeObject(unsigned key) {
  if (!key) {
    return false;
  }

  auto creationOutput = collectResult(key);
  if (!creationOutput.has_value()) {
    return false;
  }

  if (creationOutput.value().result != S_OK) {
    return false;
  }

  ObjectInfoPlayer* info = new ObjectInfoPlayer();
  info->object = static_cast<IUnknown*>(creationOutput.value().object);
  manager_.addObject(key, info);
  return true;
}

template <typename T>
void MultithreadedObjectAwaitLayer::completeArgument(InterfaceArgument<T>& argument) {
  if (completeObject(argument.key)) {
    updateInterface(manager_, argument);
  }
}

template <typename T>
void MultithreadedObjectAwaitLayer::completeArrayArgument(
    InterfaceArrayArgument<T>& arrayArgument) {
  bool requiresUpdate = false;
  for (size_t i = 0; i < arrayArgument.size; ++i) {
    if (completeObject(arrayArgument.keys[i])) {
      requiresUpdate = true;
    }
  }

  if (requiresUpdate) {
    updateInterface(manager_, arrayArgument);
  }
}

MultithreadedObjectAwaitLayer::MultithreadedObjectAwaitLayer(PlayerManager& manager)
    : Layer("MultithreadedObjectAwaitLayer"), manager_(manager) {}

void MultithreadedObjectAwaitLayer::pre(IUnknownAddRefCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(IUnknownReleaseCommand& c) {
  completeArgument(c.object_);

  auto& service = manager_.getMultithreadedObjectCreationService();

  if (c.result_.value == 0) {
    auto objectsToComplete = service.collectConsumers(c.object_.key);
    for (const auto objectKey : objectsToComplete) {
      auto creationOutput = service.complete(objectKey);
      if (!creationOutput.has_value()) {
        continue;
      }

      auto [it, inserted] = preCollectedOutputs_.insert({objectKey, creationOutput.value()});
      GITS_ASSERT(inserted);
    }
  }
}

void MultithreadedObjectAwaitLayer::pre(IUnknownQueryInterfaceCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12ObjectGetPrivateDataCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12ObjectSetNameCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12ObjectSetPrivateDataCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12ObjectSetPrivateDataInterfaceCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12DeviceChildGetDeviceCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12PipelineStateGetCachedBlobCommand& c) {
  completeArgument(c.object_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12DeviceCreateCommandListCommand& c) {
  completeArgument(c.pInitialState_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12GraphicsCommandListResetCommand& c) {
  completeArgument(c.pInitialState_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12GraphicsCommandListClearStateCommand& c) {
  completeArgument(c.pPipelineState_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  completeArgument(c.pPipelineState_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  completeArgument(c.pStateObject_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12PipelineLibraryStorePipelineCommand& c) {
  completeArgument(c.pPipeline_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12DeviceMakeResidentCommand& c) {
  completeArrayArgument(c.ppObjects_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12DeviceEvictCommand& c) {
  completeArrayArgument(c.ppObjects_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12Device1SetResidencyPriorityCommand& c) {
  completeArrayArgument(c.ppObjects_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12Device3EnqueueMakeResidentCommand& c) {
  completeArrayArgument(c.ppObjects_);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12Device5CreateStateObjectCommand& c) {
  bool requiresUpdate = false;
  for (unsigned index = 0; index < c.pDesc_.value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT* subobject =
        const_cast<D3D12_STATE_SUBOBJECT*>(&(c.pDesc_.value->pSubobjects[index]));
    if (subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
      unsigned objectKey = c.pDesc_.interfaceKeysBySubobject[index];
      if (!objectKey || manager_.findObject(objectKey)) {
        continue;
      }

      if (completeObject(objectKey)) {
        requiresUpdate = true;
      }
    }
  }
  if (requiresUpdate) {
    updateInterface(manager_, c.pDesc_);
  }
}

void MultithreadedObjectAwaitLayer::pre(ID3D12Device7AddToStateObjectCommand& c) {
  completeArgument(c.pStateObjectToGrowFrom_);
  bool requiresUpdate = false;
  for (unsigned index = 0; index < c.pAddition_.value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT* subobject =
        const_cast<D3D12_STATE_SUBOBJECT*>(&(c.pAddition_.value->pSubobjects[index]));
    if (subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
      unsigned objectKey = c.pAddition_.interfaceKeysBySubobject[index];
      if (!objectKey || manager_.findObject(objectKey)) {
        continue;
      }

      if (completeObject(objectKey)) {
        requiresUpdate = true;
      }
    }
  }
  if (requiresUpdate) {
    updateInterface(manager_, c.pAddition_);
  }
}

void MultithreadedObjectAwaitLayer::pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  completeObject(c.ppPipelineState_.key);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  completeObject(c.ppPipelineState_.key);
}

void MultithreadedObjectAwaitLayer::pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  completeObject(c.ppPipelineState_.key);
}

} // namespace DirectX
} // namespace gits
