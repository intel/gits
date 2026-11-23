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
  void ReleasePipelineState(ObjectKey pipelineStateKey, unsigned refCount);
  void AddRefPipelineState(ObjectKey pipelineStateKey);
  void CreatePipelineLibrary(ID3D12Device1CreatePipelineLibraryCommand& c);
  void CreatePipelineState(ObjectKey pipelineStateKey);
  HRESULT LoadComputePipeline(ID3D12PipelineLibraryLoadComputePipelineCommand& c);
  HRESULT LoadGraphicsPipeline(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c);
  HRESULT LoadPipeline(ID3D12PipelineLibrary1LoadPipelineCommand& c);

  template <typename Desc, typename PipelineLibrary>
  HRESULT LoadPipelineState(PipelineLibrary* pipelineLibrary,
                            LPCWSTR name,
                            Desc* desc,
                            REFIID iid,
                            ObjectKey pipelineStateKey,
                            void** ppPipelineState);

private:
  std::unordered_map<ObjectKey, unsigned> m_PipelineStateRefCounts;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
