// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

#include <fstream>

namespace gits {
namespace DirectX {

void DumpPipelineStateStreamDesc(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                                 std::ofstream& stream);

} // namespace DirectX
} // namespace gits
