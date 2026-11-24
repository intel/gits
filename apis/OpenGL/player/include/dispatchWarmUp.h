// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

namespace gits::opengl::player {

// Best-effort resolution after a successful make-current, while a bootstrap
// context can still resolve functions needed after it is released. Run once per
// stream-stable context key, not per native handle (which can be reused).
void WarmUpResolvableFunctions(GITSKey contextKey);

} // namespace gits::opengl::player
