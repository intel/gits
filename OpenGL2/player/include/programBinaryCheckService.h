// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "glTypes.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

namespace gits::opengl2 {

// Warns once when a program whose recorded binary the driver rejected is used. Only the driver
// that produced a binary is guaranteed to load it, so a stream recorded without
// OpenGL.Recorder.SuppressProgramBinary may replay such programs unlinked. A rejected binary the
// application replaced by linking from source, as after a stale cache, is not reported.
class ProgramBinaryCheckService {
public:
  static ProgramBinaryCheckService& Get();

  void OnBinaryLoad(GITSKey program);
  // The program was linked from source or deleted.
  void Forget(GITSKey program);
  void OnUse(GITSKey program, GLuint name);

private:
  ProgramBinaryCheckService() = default;

  std::mutex m_Mutex;
  // Programs loaded from a binary and not linked since, checked at their first use: a load may
  // complete asynchronously, and only the use has to wait for it. Replay names are unique only in
  // a share group.
  std::unordered_set<GITSKey> m_Loaded;
  std::atomic<bool> m_Tracking{false};
  std::atomic<bool> m_Warned{false};
};

} // namespace gits::opengl2
