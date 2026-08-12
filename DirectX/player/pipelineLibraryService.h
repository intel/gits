// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "commandsAuto.h"

#include <unordered_map>
#include <mutex>

namespace gits {
namespace DirectX {

class PipelineLibraryService {
public:
  void ReleasePipelineState(GITSKey pipelineStateKey, unsigned refCount);
  void AddRefPipelineState(GITSKey pipelineStateKey);
  void CreatePipelineLibrary(ID3D12Device1CreatePipelineLibraryCommand& c);
  void CreatePipelineState(GITSKey pipelineStateKey);
  HRESULT LoadComputePipeline(ID3D12PipelineLibraryLoadComputePipelineCommand& c);
  HRESULT LoadGraphicsPipeline(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c);
  HRESULT LoadPipeline(ID3D12PipelineLibrary1LoadPipelineCommand& c);

  template <typename Desc, typename PipelineLibrary>
  HRESULT LoadPipelineState(PipelineLibrary* pipelineLibrary,
                            LPCWSTR name,
                            Desc* desc,
                            REFIID iid,
                            GITSKey pipelineStateKey,
                            void** ppPipelineState);

private:
  std::unordered_map<GITSKey, unsigned> m_PipelineStateRefCounts;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
