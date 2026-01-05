// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>

namespace gits {
namespace DirectX {

class CommonEventsHandler {
public:
  CommonEventsHandler();
  void RegisterEvents();

private:
  static void stateRestoreBegin();
  static void stateRestoreEnd();
  static void frameEnd(int frameNumber);
  static void markerUInt64(uint64_t value);
};

} // namespace DirectX
} // namespace gits
