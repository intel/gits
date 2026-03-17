// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
namespace stream {

enum class ApiId {
  ID_COMMON = 0,
  ID_DIRECTX = 13,
};

enum class CommonCommandId {
  ID_INIT_START = 1,
  ID_INIT_END = 2,
  ID_FRAME_END = 4,
  ID_MARKER_UINT64 = 12,
};

} // namespace stream
} // namespace gits
