// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
namespace DirectX {

class CommonEventsHandler {
public:
  CommonEventsHandler();
  void RegisterEvents();

private:
  static void frameBegin(int frameNumber) {}
  static void frameEnd(int frameNumber);
  static void loopBegin(int num) {}
  static void loopEnd(int num) {}
  static void stateRestoreBegin();
  static void stateRestoreEnd();
  static void programExit() {}
  static void programStart() {}
  static void logging(const char* msg) {}
};

} // namespace DirectX
} // namespace gits
