// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "threadContextService.h"

#include "log.h"
#include "openglDriversAuto.h"

namespace gits::opengl2 {

namespace {
// wglMakeCurrent makes the draw DC the read DC too.
HDC CurrentReadDc(ThreadContextService::MakeCurrentEntryPoint entryPoint, HDC drawDc) {
  auto& table = player::g_DispatchTable;
  switch (entryPoint) {
  case ThreadContextService::MakeCurrentEntryPoint::ARB:
    return table.wglGetCurrentReadDCARB.load(std::memory_order_acquire)();
  case ThreadContextService::MakeCurrentEntryPoint::EXT:
    return table.wglGetCurrentReadDCEXT.load(std::memory_order_acquire)();
  default:
    return drawDc;
  }
}
} // namespace

ThreadContextService& ThreadContextService::Get() {
  static ThreadContextService instance;
  return instance;
}

void ThreadContextService::SwitchTo(uint32_t threadId) {
  if (m_ThreadId == threadId) {
    return;
  }
  m_ThreadId = threadId;
  const auto it = m_Current.find(threadId);
  if (it == m_Current.end()) {
    return;
  }
  const Current& current = it->second;
  auto& table = player::g_DispatchTable;
  if (table.wglGetCurrentContext.load(std::memory_order_acquire)() == current.Context &&
      (current.Context == nullptr ||
       (table.wglGetCurrentDC.load(std::memory_order_acquire)() == current.Dc &&
        CurrentReadDc(current.EntryPoint, current.Dc) == current.ReadDc))) {
    return;
  }
  bool switched = false;
  switch (current.EntryPoint) {
  case MakeCurrentEntryPoint::Core:
    switched = table.wglMakeCurrent.load(std::memory_order_acquire)(current.Dc, current.Context);
    break;
  case MakeCurrentEntryPoint::ARB:
    switched = table.wglMakeContextCurrentARB.load(std::memory_order_acquire)(
        current.Dc, current.ReadDc, current.Context);
    break;
  case MakeCurrentEntryPoint::EXT:
    switched = table.wglMakeContextCurrentEXT.load(std::memory_order_acquire)(
        current.Dc, current.ReadDc, current.Context);
    break;
  }
  if (!switched) {
    m_ThreadId.reset();
    static bool warned = false;
    if (!warned) {
      warned = true;
      LOG_WARNING << "ThreadContextService: making current the context of recorded thread "
                  << threadId << " failed - its commands run without that context";
    }
  }
}

void ThreadContextService::NoteCurrent(uint32_t threadId, MakeCurrentEntryPoint entryPoint) {
  auto& table = player::g_DispatchTable;
  Current& current = m_Current[threadId];
  current.Context = table.wglGetCurrentContext.load(std::memory_order_acquire)();
  current.Dc = current.Context != nullptr ? table.wglGetCurrentDC.load(std::memory_order_acquire)()
                                          : nullptr;
  current.ReadDc = current.Context != nullptr ? CurrentReadDc(entryPoint, current.Dc) : nullptr;
  current.EntryPoint = entryPoint;
}

void ThreadContextService::Forget(HGLRC context) {
  for (auto& [threadId, current] : m_Current) {
    if (current.Context == context) {
      current = {};
    }
  }
}

void ThreadContextLayer::Post(wglMakeCurrentCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::Core);
}

void ThreadContextLayer::Post(wglMakeContextCurrentARBCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::ARB);
}

void ThreadContextLayer::Post(wglMakeContextCurrentEXTCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::EXT);
}

void ThreadContextLayer::Post(wglDeleteContextCommand& command) {
  // Deleting a context leaves the current one unchanged, unless it was the one deleted, which
  // Forget() detaches.
  if (command.m_Return.Value) {
    ThreadContextService::Get().Forget(command.m_oldContext.Value);
  }
}

} // namespace gits::opengl2
