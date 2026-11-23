// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedShaderCompilationScheduleLayer.h"
#include "arguments.h"
#include "pipelineLibraryService.h"

#include <string>

namespace gits {
namespace DirectX {

template <typename CommandT>
bool MultithreadedShaderCompilationScheduleLayer::CanSchedule(const CommandT& c) const {
  return m_Manager.ExecuteCommands() && c.m_Result.Value == S_OK && !c.Skip;
}

void MultithreadedShaderCompilationScheduleLayer::Enqueue(ObjectKey key,
                                                          bool& skip,
                                                          void**& outputValue,
                                                          std::vector<IUnknown*> keepAlive,
                                                          std::function<HRESULT(void**)> create) {
  m_Manager.GetMultithreadedShaderCompilationService().Schedule(
      [key,
       create = std::move(create)]() -> MultithreadedShaderCompilationService::CompilationOutput {
        void* object{};
        HRESULT result = create(&object);
        if (result == S_OK) {
          std::wstring name = L"O" + std::to_wstring(key);
          static_cast<ID3D12Object*>(object)->SetName(name.c_str());
        }
        return {result, object};
      },
      key, std::move(keepAlive));
  skip = true;
  outputValue = nullptr;
}

template <typename CommandT>
void MultithreadedShaderCompilationScheduleLayer::ScheduleLibraryLoad(CommandT& c,
                                                                      IUnknown* rootSignature) {
  decltype(c.m_pDesc) desc(c.m_pDesc);
  auto* library = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;
  std::wstring name = c.m_pName.Value;
  ObjectKey key = c.m_ppPipelineState.Key;
  PipelineLibraryService& pipelineLibraryService = m_Manager.GetPipelineLibraryService();

  Enqueue(key, c.Skip, c.m_ppPipelineState.Value, {library, rootSignature},
          [desc, library, riid, name = std::move(name), key, &pipelineLibraryService](void** out) {
            return pipelineLibraryService.LoadPipelineState(library, name.c_str(), desc.Value, riid,
                                                            key, out);
          });
}

MultithreadedShaderCompilationScheduleLayer::MultithreadedShaderCompilationScheduleLayer(
    PlayerManager& manager)
    : Layer("MultithreadedShaderCompilationSchedule"), m_Manager(manager) {}

void MultithreadedShaderCompilationScheduleLayer::Pre(
    ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (!CanSchedule(c)) {
    return;
  }

  decltype(c.m_pDesc) desc(c.m_pDesc);
  auto* device = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;

  Enqueue(c.m_ppPipelineState.Key, c.Skip, c.m_ppPipelineState.Value,
          {device, desc.Value->pRootSignature}, [desc, device, riid](void** out) {
            return device->CreateGraphicsPipelineState(desc.Value, riid, out);
          });
}

void MultithreadedShaderCompilationScheduleLayer::Pre(
    ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (!CanSchedule(c)) {
    return;
  }

  decltype(c.m_pDesc) desc(c.m_pDesc);
  auto* device = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;

  Enqueue(c.m_ppPipelineState.Key, c.Skip, c.m_ppPipelineState.Value,
          {device, desc.Value->pRootSignature}, [desc, device, riid](void** out) {
            return device->CreateComputePipelineState(desc.Value, riid, out);
          });
}

void MultithreadedShaderCompilationScheduleLayer::Pre(ID3D12Device2CreatePipelineStateCommand& c) {
  if (!CanSchedule(c)) {
    return;
  }

  decltype(c.m_pDesc) desc(c.m_pDesc);
  auto* device = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;

  Enqueue(c.m_ppPipelineState.Key, c.Skip, c.m_ppPipelineState.Value,
          {device, m_Manager.FindObject(desc.RootSignatureKey)}, [desc, device, riid](void** out) {
            return device->CreatePipelineState(desc.Value, riid, out);
          });
}

void MultithreadedShaderCompilationScheduleLayer::Pre(ID3D12Device5CreateStateObjectCommand& c) {
  if (!CanSchedule(c)) {
    return;
  }

  decltype(c.m_pDesc) desc(c.m_pDesc);
  auto* device = c.m_Object.Value;
  REFIID riid = c.m_riid.Value;

  std::vector<IUnknown*> keepAlive{device};
  for (const auto& [index, key] : c.m_pDesc.InterfaceKeysBySubobject) {
    if (IUnknown* dependency = m_Manager.FindObject(key)) {
      keepAlive.push_back(dependency);
    }
  }

  Enqueue(c.m_ppStateObject.Key, c.Skip, c.m_ppStateObject.Value, std::move(keepAlive),
          [desc, device, riid](void** out) {
            return device->CreateStateObject(desc.Value, riid, out);
          });
}

void MultithreadedShaderCompilationScheduleLayer::Pre(
    ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (CanSchedule(c)) {
    ScheduleLibraryLoad(c, c.m_pDesc.Value->pRootSignature);
  }
}

void MultithreadedShaderCompilationScheduleLayer::Pre(
    ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (CanSchedule(c)) {
    ScheduleLibraryLoad(c, c.m_pDesc.Value->pRootSignature);
  }
}

void MultithreadedShaderCompilationScheduleLayer::Pre(
    ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (CanSchedule(c)) {
    ScheduleLibraryLoad(c, m_Manager.FindObject(c.m_pDesc.RootSignatureKey));
  }
}

} // namespace DirectX
} // namespace gits
