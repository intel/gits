// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Replays each recorded thread's commands on that thread's current context. The player runs every
// command on one thread, while an application can keep a different context current on each of its
// threads - typically a loader thread whose context shares objects with the render context.
// Without switching, a command would run on whichever context the last replayed make-current
// attached, and would read and change that context's bindings instead of its own.

#include "glTypes.h"
#include "layerAuto.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace gits::opengl2 {

class ThreadContextService {
public:
  static ThreadContextService& Get();

  // Called first by every runner, before anything that can touch GL state. When `threadId`
  // differs from the previous command's, makes current the context that recorded thread had after
  // its last replayed make-current. A thread that has not replayed one yet keeps the current
  // context.
  void SwitchTo(uint32_t threadId);

  // Records the context current now as `threadId`'s.
  void NoteCurrent(uint32_t threadId);

  // Detaches deleted `context` from every thread, so a later context reusing its handle is never
  // made current in its place.
  void Forget(HGLRC context);

private:
  ThreadContextService() = default;

  struct Current {
    HDC Dc{};
    HGLRC Context{};
  };
  std::unordered_map<uint32_t, Current> m_Current;
  std::optional<uint32_t> m_ThreadId;
};

// Updates ThreadContextService after every replayed call that can change the current context.
class ThreadContextLayer : public Layer {
public:
  ThreadContextLayer() : Layer("ThreadContextLayer") {}

  void Post(wglMakeCurrentCommand& command) override;
  void Post(wglMakeContextCurrentARBCommand& command) override;
  void Post(wglMakeContextCurrentEXTCommand& command) override;
  void Post(wglDeleteContextCommand& command) override;
};

} // namespace gits::opengl2
