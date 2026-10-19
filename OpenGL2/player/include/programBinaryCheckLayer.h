// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

namespace gits::opengl2 {

// Warns once when a program whose recorded binary the driver rejected is used. Only the driver
// that produced a binary is guaranteed to load it, so a stream recorded without
// OpenGL.Recorder.SuppressProgramBinary may replay such programs unlinked. A rejected binary the
// application replaced by linking from source, as after a stale cache, is not reported.
class ProgramBinaryCheckLayer : public Layer {
public:
  ProgramBinaryCheckLayer() : Layer("ProgramBinaryCheckLayer") {}

  void Post(glProgramBinaryCommand& command) override;
  void Post(glProgramBinaryOESCommand& command) override;
  void Post(glLinkProgramCommand& command) override;
  void Post(glLinkProgramARBCommand& command) override;
  void Post(glDeleteProgramCommand& command) override;
  void Post(glDeleteObjectARBCommand& command) override;
  void Post(glUseProgramCommand& command) override;
  void Post(glUseProgramObjectARBCommand& command) override;
  void Post(glUseProgramStagesCommand& command) override;

private:
  void OnBinaryLoad(GITSKey key);
  void Forget(GITSKey key);
  void OnUse(GITSKey key, GLuint program);

  std::mutex m_Mutex;
  // Programs loaded from a binary and not linked since, checked at their first use: a load may
  // complete asynchronously, and only the use has to wait for it. Replay names are unique only in
  // a share group.
  std::unordered_set<GITSKey> m_Loaded;
  std::atomic<bool> m_Tracking{false};
  std::atomic<bool> m_Warned{false};
};

} // namespace gits::opengl2
