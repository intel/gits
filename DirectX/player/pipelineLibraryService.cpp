// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineLibraryService.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void PipelineLibraryService::releasePipelineState(unsigned pipelineStateKey, unsigned refCount) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (refCount == 0) {
    pipelineStateRefCounts_.erase(pipelineStateKey);
  } else {
    auto it = pipelineStateRefCounts_.find(pipelineStateKey);
    if (it != pipelineStateRefCounts_.end()) {
      if (it->second) {
        --it->second;
      }
    }
  }
}

void PipelineLibraryService::addRefPipelineState(unsigned pipelineStateKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = pipelineStateRefCounts_.find(pipelineStateKey);
  if (it != pipelineStateRefCounts_.end()) {
    ++it->second;
  }
}

void PipelineLibraryService::createPipelineLibrary(ID3D12Device1CreatePipelineLibraryCommand& c) {
  c.pLibraryBlob_.value = nullptr;
  c.pLibraryBlob_.size = 0;
  c.BlobLength_.value = 0;
}

void PipelineLibraryService::createPipelineState(unsigned pipelineStateKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  pipelineStateRefCounts_[pipelineStateKey] = 1;
}

void PipelineLibraryService::loadComputePipeline(
    ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  loadPipelineState(c.object_.value, c.pName_.value, c.pDesc_.value, c.riid_.value,
                    c.ppPipelineState_.key, c.ppPipelineState_.value);
}

void PipelineLibraryService::loadGraphicsPipeline(
    ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  loadPipelineState(c.object_.value, c.pName_.value, c.pDesc_.value, c.riid_.value,
                    c.ppPipelineState_.key, c.ppPipelineState_.value);
}

void PipelineLibraryService::loadPipeline(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  loadPipelineState(c.object_.value, c.pName_.value, c.pDesc_.value, c.riid_.value,
                    c.ppPipelineState_.key, c.ppPipelineState_.value);
}

template <typename Desc, typename PipelineLibrary>
HRESULT PipelineLibraryService::loadPipelineState(PipelineLibrary* pipelineLibrary,
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
    std::lock_guard<std::mutex> lock(mutex_);
    refCount = pipelineStateRefCounts_[pipelineStateKey] += 1;
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
