// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>

namespace gits::opengl {

// `key` must already be reserved by the caller via CaptureManager::CreateCommandKey(), at
// wrapper entry before the real driver dispatch - see CaptureManager::RecordUnsupportedCommand's
// own header comment for why this can no longer be minted inside this function (as it used to
// be, implicitly, via RecordUnsupportedCommand's old internal CreateCommandKey() call) or inside
// CaptureManager::RecordUnsupportedCommand itself.
void ReportUnsupportedCommand(const char* name, uint64_t key);

} // namespace gits::opengl
