// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
void MultithreadedObjectCreationLayer::ScheduleCreate(CommandT& c) {
  if (!m_Manager.ExecuteCommands() || c.m_Result.Value != S_OK || c.Skip) {
    return;
  }

  InterfaceOutputArgument<void>* state{};
  if constexpr (std::is_same_v<CommandT, ID3D12Device5CreateStateObjectCommand>) {
    state = &c.m_ppStateObject;
  } else {
    state = &c.m_ppPipelineState;
  }

  decltype(c.m_pDesc) desc(c.m_pDesc);

  auto device = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;
  unsigned key = state->Key;

  auto& service = m_Manager.GetMultithreadedObjectCreationService();
  service.AddDependency(c.m_Object.Key, key);

  if constexpr (std::is_same_v<CommandT, ID3D12Device5CreateStateObjectCommand>) {
    for (unsigned index = 0; index < c.m_pDesc.Value->NumSubobjects; ++index) {
      D3D12_STATE_SUBOBJECT* subobject =
          const_cast<D3D12_STATE_SUBOBJECT*>(&(c.m_pDesc.Value->pSubobjects[index]));
      if (subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE ||
          subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE ||
          subobject->Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION) {
        service.AddDependency(c.m_pDesc.InterfaceKeysBySubobject[index], key);
      }
    }
  } else {
    service.AddDependency(c.m_pDesc.RootSignatureKey, key);
  }

  service.Schedule(
      [desc, device, riid, key]() -> MultithreadedObjectCreationService::ObjectCreationOutput {
        void* object{};
        HRESULT result;
        if constexpr (std::is_same_v<CommandT, ID3D12DeviceCreateGraphicsPipelineStateCommand>) {
          result = device->CreateGraphicsPipelineState(desc.Value, riid, &object);
        } else if constexpr (std::is_same_v<CommandT,
                                            ID3D12DeviceCreateComputePipelineStateCommand>) {
          result = device->CreateComputePipelineState(desc.Value, riid, &object);
        } else if constexpr (std::is_same_v<CommandT, ID3D12Device2CreatePipelineStateCommand>) {
          result = device->CreatePipelineState(desc.Value, riid, &object);
        } else if constexpr (std::is_same_v<CommandT, ID3D12Device5CreateStateObjectCommand>) {
          result = device->CreateStateObject(desc.Value, riid, &object);
        } else {
          static_assert(false, "Unexpected command type");
        }

        if (result == S_OK) {
          setD3D12ObjectName(object, key);
        }
        return {result, object};
      },
      key);
  c.Skip = true;
  state->Value = nullptr;
}

template <typename CommandT>
void MultithreadedObjectCreationLayer::ScheduleLoad(CommandT& c) {
  if (!m_Manager.ExecuteCommands() || c.m_Result.Value != S_OK || c.Skip) {
    return;
  }

  decltype(c.m_pDesc) desc(c.m_pDesc);

  auto library = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;
  std::wstring name = c.m_pName.Value;
  unsigned key = c.m_ppPipelineState.Key;

  auto& service = m_Manager.GetMultithreadedObjectCreationService();
  service.AddDependency(c.m_Object.Key, key);
  service.AddDependency(c.m_pDesc.RootSignatureKey, key);

  PipelineLibraryService& pipelineLibraryService = m_Manager.GetPipelineLibraryService();

  service.Schedule(
      [desc, library, riid, name = std::move(name), key,
       &pipelineLibraryService]() -> MultithreadedObjectCreationService::ObjectCreationOutput {
        void* object{};
        HRESULT result = pipelineLibraryService.LoadPipelineState(library, name.c_str(), desc.Value,
                                                                  riid, key, &object);
        setD3D12ObjectName(object, key);
        return {result, object};
      },
      key);

  c.Skip = true;
  c.m_ppPipelineState.Value = nullptr;
}

MultithreadedObjectCreationLayer::MultithreadedObjectCreationLayer(PlayerManager& manager)
    : Layer("MultithreadedObjectCreationLayer"), m_Manager(manager) {}

void MultithreadedObjectCreationLayer::Pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  ScheduleCreate(c);
}

void MultithreadedObjectCreationLayer::Pre(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  ScheduleCreate(c);
}

void MultithreadedObjectCreationLayer::Pre(ID3D12Device2CreatePipelineStateCommand& c) {
  ScheduleCreate(c);
}

void MultithreadedObjectCreationLayer::Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  ScheduleLoad(c);
}

void MultithreadedObjectCreationLayer::Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  ScheduleLoad(c);
}

void MultithreadedObjectCreationLayer::Pre(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  ScheduleLoad(c);
}

void MultithreadedObjectCreationLayer::Pre(ID3D12Device5CreateStateObjectCommand& c) {
  ScheduleCreate(c);
}

} // namespace DirectX
} // namespace gits
