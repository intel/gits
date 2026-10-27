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
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gits::opengl2 {

class ThreadContextService {
public:
  static ThreadContextService& Get();

  // Called first by every runner, before anything that can touch GL state. When `threadId`
  // differs from the previous command's, makes current the context that recorded thread had after
  // its last replayed make-current. A thread that has not replayed one yet keeps the current
  // context.
  void SwitchTo(uint32_t threadId);

  // The entry point that made a context current, and so the one that restores it: the
  // make-context-current ones keep a separate read DC, such as a pbuffer's. Every GLX one is
  // restored with glXMakeContextCurrent, which keeps the read drawable.
  enum class MakeCurrentEntryPoint {
    Core,
    ARB,
    EXT,
    Glx
  };

  // Records the context current now, and its draw and read DCs or drawables, as `threadId`'s. A
  // WGL make-current through `entryPoint` that failed leaves the entry point of the current
  // context.
  void NoteCurrent(uint32_t threadId, MakeCurrentEntryPoint entryPoint, bool succeeded);

  // The entry point that made the current context current.
  MakeCurrentEntryPoint CurrentEntryPoint() const {
    return m_EntryPoint;
  }

  // The current read DC, through the getter matching `entryPoint`.
  static HDC CurrentReadDc(MakeCurrentEntryPoint entryPoint, HDC drawDc);

  static bool MakeCurrent(MakeCurrentEntryPoint entryPoint, HDC drawDc, HDC readDc, HGLRC context);

  // Detaches deleted `context` from every thread, so a later context reusing its handle is never
  // made current in its place.
  void Forget(HGLRC context);
  // A destroyed GLX context stays usable until the thread it is current on releases it, so
  // glXDestroyContext destroys a context a recorded thread has current only once no recorded
  // thread does. Switching to another recorded thread's context would release and so destroy it.
  void Destroy(GLXContext context);
  // The same for a GLX drawable: `destroy` destroys `drawable` and forgets its substitute once no
  // recorded thread has it current, so switching back to a thread that has can restore it.
  void DestroyDrawable(GLXDrawable drawable, std::function<void()> destroy);

private:
  ThreadContextService() = default;

  struct Current {
    HDC Dc{};
    HDC ReadDc{};
    HGLRC Context{};
    GLXDrawable Drawable{};
    GLXDrawable ReadDrawable{};
    GLXContext GlxContext{};
    MakeCurrentEntryPoint EntryPoint{MakeCurrentEntryPoint::Core};
  };
  static bool IsCurrent(const Current& current);
  static bool MakeCurrent(const Current& current);
  bool IsBound(GLXContext context) const;
  bool IsDrawableBound(GLXDrawable drawable) const;
  void DestroyReleased();
  std::unordered_map<uint32_t, Current> m_Current;
  std::vector<GLXContext> m_DestroyPending;
  std::vector<std::pair<GLXDrawable, std::function<void()>>> m_DrawableDestroyPending;
  std::optional<uint32_t> m_ThreadId;
  MakeCurrentEntryPoint m_EntryPoint{MakeCurrentEntryPoint::Core};
};

// Updates ThreadContextService after every replayed call that can change the current context.
class ThreadContextLayer : public Layer {
public:
  ThreadContextLayer() : Layer("ThreadContextLayer") {}

  void Post(wglMakeCurrentCommand& command) override;
  void Post(wglMakeContextCurrentARBCommand& command) override;
  void Post(wglMakeContextCurrentEXTCommand& command) override;
  void Post(wglDeleteContextCommand& command) override;
  void Post(glXMakeCurrentCommand& command) override;
  void Post(glXMakeContextCurrentCommand& command) override;
  void Post(glXMakeCurrentReadSGICommand& command) override;
};

} // namespace gits::opengl2
