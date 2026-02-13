// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx/directx.h"

#include <filesystem>

namespace directx {
void LoadIntelExtensions();
HMODULE LoadAgilitySdk(const std::filesystem::path& path);
void WaitForFence(ID3D12Fence* fence, uint64_t fenceValue);
void PatchPipelineState(D3D12_PIPELINE_STATE_STREAM_DESC& desc,
                        ID3D12RootSignature* pRootSignature,
                        void* subobjectsData,
                        size_t subobjectsDataSize);
void PreloadComputePipeline(ID3D12PipelineLibrary* pPipelineLibrary,
                            LPCWSTR pName,
                            const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc);
void PreloadGraphicsPipeline(ID3D12PipelineLibrary* pPipelineLibrary,
                             LPCWSTR pName,
                             const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc);
void PreloadPipeline(ID3D12PipelineLibrary1* pPipelineLibrary,
                     LPCWSTR pName,
                     const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc);
} // namespace directx
