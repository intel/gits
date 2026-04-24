// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineLibraryService.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void PipelineLibraryService::ReleasePipelineState(unsigned pipelineStateKey, unsigned refCount) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (refCount == 0) {
    m_PipelineStateRefCounts.erase(pipelineStateKey);
  } else {
    auto it = m_PipelineStateRefCounts.find(pipelineStateKey);
    if (it != m_PipelineStateRefCounts.end()) {
      if (it->second) {
        --it->second;
      }
    }
  }
}

void PipelineLibraryService::AddRefPipelineState(unsigned pipelineStateKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_PipelineStateRefCounts.find(pipelineStateKey);
  if (it != m_PipelineStateRefCounts.end()) {
    ++it->second;
  }
}

void PipelineLibraryService::CreatePipelineLibrary(ID3D12Device1CreatePipelineLibraryCommand& c) {
  c.m_pLibraryBlob.Value = nullptr;
  c.m_pLibraryBlob.Size = 0;
  c.m_BlobLength.Value = 0;
}

void PipelineLibraryService::CreatePipelineState(unsigned pipelineStateKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PipelineStateRefCounts[pipelineStateKey] = 1;
}

HRESULT PipelineLibraryService::LoadComputePipeline(
    ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  return LoadPipelineState(c.m_Object.Value, c.m_pName.Value, c.m_pDesc.Value, c.m_riid.Value,
                           c.m_ppPipelineState.Key, c.m_ppPipelineState.Value);
}

HRESULT PipelineLibraryService::LoadGraphicsPipeline(
    ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  return LoadPipelineState(c.m_Object.Value, c.m_pName.Value, c.m_pDesc.Value, c.m_riid.Value,
                           c.m_ppPipelineState.Key, c.m_ppPipelineState.Value);
}

HRESULT PipelineLibraryService::LoadPipeline(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  return LoadPipelineState(c.m_Object.Value, c.m_pName.Value, c.m_pDesc.Value, c.m_riid.Value,
                           c.m_ppPipelineState.Key, c.m_ppPipelineState.Value);
}

template <typename Desc, typename PipelineLibrary>
HRESULT PipelineLibraryService::LoadPipelineState(PipelineLibrary* pipelineLibrary,
                                                  LPCWSTR name,
                                                  Desc* desc,
                                                  REFIID iid,
                                                  unsigned pipelineStateKey,
                                                  void** ppPipelineState) {
  ID3D12PipelineState* pipelineState{};
  GITS_ASSERT(iid == IID_ID3D12PipelineState);

  Microsoft::WRL::ComPtr<ID3D12Device2> device;
  pipelineLibrary->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(device);

  HRESULT hr{};
  if constexpr (std::is_same_v<Desc, D3D12_GRAPHICS_PIPELINE_STATE_DESC>) {
    hr = device->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&pipelineState));
  } else if constexpr (std::is_same_v<Desc, D3D12_COMPUTE_PIPELINE_STATE_DESC>) {
    hr = device->CreateComputePipelineState(desc, IID_PPV_ARGS(&pipelineState));
  } else if constexpr (std::is_same_v<Desc, D3D12_PIPELINE_STATE_STREAM_DESC>) {
    hr = device->CreatePipelineState(desc, IID_PPV_ARGS(&pipelineState));
  }
  GITS_ASSERT(hr == S_OK);

  hr = pipelineLibrary->StorePipeline(name, pipelineState);
  if constexpr (std::is_same_v<Desc, D3D12_GRAPHICS_PIPELINE_STATE_DESC>) {
    hr = pipelineLibrary->LoadGraphicsPipeline(name, desc, iid, ppPipelineState);
  } else if constexpr (std::is_same_v<Desc, D3D12_COMPUTE_PIPELINE_STATE_DESC>) {
    hr = pipelineLibrary->LoadComputePipeline(name, desc, iid, ppPipelineState);
  } else if constexpr (std::is_same_v<Desc, D3D12_PIPELINE_STATE_STREAM_DESC>) {
    hr = pipelineLibrary->LoadPipeline(name, desc, iid, ppPipelineState);
  }
  unsigned refCount{};
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    refCount = m_PipelineStateRefCounts[pipelineStateKey] += 1;
  }
  if (hr != S_OK) {
    pipelineState->AddRef();
    unsigned currentRefCount = pipelineState->Release();
    while (currentRefCount < refCount) {
      currentRefCount = pipelineState->AddRef();
    }
    *ppPipelineState = pipelineState;
    hr = S_OK;
  }

  return hr;
}

} // namespace DirectX
} // namespace gits
