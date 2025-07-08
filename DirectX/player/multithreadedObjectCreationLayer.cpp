// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectCreationLayer.h"
#include "interfaceArgumentUpdaters.h"
#include "pipelineLibraryService.h"

namespace gits {
namespace DirectX {

namespace {

void setD3D12ObjectName(void* obj, unsigned key) {
  ID3D12Object* object = static_cast<ID3D12Object*>(obj);
  std::wstringstream s;
  s << "O" << key;
  object->SetName(s.str().c_str());
}

} // namespace

template <typename CommandT>
void MultithreadedObjectCreationLayer::scheduleCreate(CommandT& c) {
  if (!manager_.executeCommands() || c.result_.value != S_OK || c.skip) {
    return;
  }

  InterfaceOutputArgument<void>* state{};
  if constexpr (std::is_same_v<CommandT, ID3D12Device5CreateStateObjectCommand>) {
    state = &c.ppStateObject_;
  } else {
    state = &c.ppPipelineState_;
  }

  decltype(c.pDesc_) desc(c.pDesc_);

  auto device = c.object_.value;
  REFIID riid = c.riid_.value;
  unsigned key = state->key;

  auto& service = manager_.getMultithreadedObjectCreationService();
  service.addDependency(c.object_.key, key);

  if constexpr (std::is_same_v<CommandT, ID3D12Device5CreateStateObjectCommand>) {
    for (unsigned index = 0; index < c.pDesc_.value->NumSubobjects; ++index) {
      D3D12_STATE_SUBOBJECT* subobject =
          const_cast<D3D12_STATE_SUBOBJECT*>(&(c.pDesc_.value->pSubobjects[index]));
      if (subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE ||
          subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE ||
          subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
        service.addDependency(c.pDesc_.interfaceKeysBySubobject[index], key);
      }
    }
  } else {
    service.addDependency(c.pDesc_.rootSignatureKey, key);
  }

  service.schedule(
      [desc, device, riid, key]() -> MultithreadedObjectCreationService::ObjectCreationOutput {
        void* object{};
        HRESULT result;
        if constexpr (std::is_same_v<CommandT, ID3D12DeviceCreateGraphicsPipelineStateCommand>) {
          result = device->CreateGraphicsPipelineState(desc.value, riid, &object);
        } else if constexpr (std::is_same_v<CommandT,
                                            ID3D12DeviceCreateComputePipelineStateCommand>) {
          result = device->CreateComputePipelineState(desc.value, riid, &object);
        } else if constexpr (std::is_same_v<CommandT, ID3D12Device2CreatePipelineStateCommand>) {
          result = device->CreatePipelineState(desc.value, riid, &object);
        } else if constexpr (std::is_same_v<CommandT, ID3D12Device5CreateStateObjectCommand>) {
          result = device->CreateStateObject(desc.value, riid, &object);
        } else {
          static_assert(false, "Unexpected command type");
        }

        if (result == S_OK) {
          setD3D12ObjectName(object, key);
        }
        return {result, object};
      },
      key);
  c.skip = true;
  state->value = nullptr;
}

template <typename CommandT>
void MultithreadedObjectCreationLayer::scheduleLoad(CommandT& c) {
  if (!manager_.executeCommands() || c.result_.value != S_OK || c.skip) {
    return;
  }

  decltype(c.pDesc_) desc(c.pDesc_);

  auto library = c.object_.value;
  REFIID riid = c.riid_.value;
  std::wstring name = c.pName_.value;
  unsigned key = c.ppPipelineState_.key;

  auto& service = manager_.getMultithreadedObjectCreationService();
  service.addDependency(c.object_.key, key);
  service.addDependency(c.pDesc_.rootSignatureKey, key);

  PipelineLibraryService& pipelineLibraryService = manager_.getPipelineLibraryService();

  service.schedule(
      [desc, library, riid, name = std::move(name), key,
       &pipelineLibraryService]() -> MultithreadedObjectCreationService::ObjectCreationOutput {
        void* object{};
        HRESULT result = pipelineLibraryService.loadPipelineState(library, name.c_str(), desc.value,
                                                                  riid, key, &object);
        setD3D12ObjectName(object, key);
        return {result, object};
      },
      key);

  c.skip = true;
  c.ppPipelineState_.value = nullptr;
}

MultithreadedObjectCreationLayer::MultithreadedObjectCreationLayer(PlayerManager& manager)
    : Layer("MultithreadedObjectCreationLayer"), manager_(manager) {}

void MultithreadedObjectCreationLayer::pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  scheduleCreate(c);
}

void MultithreadedObjectCreationLayer::pre(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  scheduleCreate(c);
}

void MultithreadedObjectCreationLayer::pre(ID3D12Device2CreatePipelineStateCommand& c) {
  scheduleCreate(c);
}

void MultithreadedObjectCreationLayer::pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  scheduleLoad(c);
}

void MultithreadedObjectCreationLayer::pre(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  scheduleLoad(c);
}

void MultithreadedObjectCreationLayer::pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  scheduleLoad(c);
}

void MultithreadedObjectCreationLayer::pre(ID3D12Device5CreateStateObjectCommand& c) {
  scheduleCreate(c);
}

} // namespace DirectX
} // namespace gits
