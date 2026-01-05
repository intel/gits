// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <unordered_map>
#include <mutex>

namespace gits {
namespace DirectX {

class PipelineLibraryService {
public:
  void releasePipelineState(unsigned pipelineStateKey, unsigned refCount);
  void addRefPipelineState(unsigned pipelineStateKey);
  void createPipelineLibrary(ID3D12Device1CreatePipelineLibraryCommand& c);
  void createPipelineState(unsigned pipelineStateKey);
  void loadComputePipeline(ID3D12PipelineLibraryLoadComputePipelineCommand& c);
  void loadGraphicsPipeline(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c);
  void loadPipeline(ID3D12PipelineLibrary1LoadPipelineCommand& c);

  template <typename Desc, typename PipelineLibrary>
  HRESULT loadPipelineState(PipelineLibrary* pipelineLibrary,
                            LPCWSTR name,
                            Desc* desc,
                            REFIID iid,
                            unsigned pipelineStateKey,
                            void** ppPipelineState);

private:
  std::unordered_map<unsigned, unsigned> pipelineStateRefCounts_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
