// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file  eventsData.h
 *
 * @brief Declaration of data structs for events fired when processing associated tokens
 *
 */

#pragma once

#include <cstdint>

namespace gits {
struct FRAME_START_DATA {
  uint32_t FrameNumber;
};

struct FRAME_END_DATA {
  uint32_t FrameNumber;
};
} // namespace gits
