// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
// Returns true if GITS can finish after the function execution.
// This happens when only help was requested from GITS.
bool configure_player(int arg, char** argv);

template <>
CEnumParser<TraceData>::CEnumParser();
} // namespace gits
