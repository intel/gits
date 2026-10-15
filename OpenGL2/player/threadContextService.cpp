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

// wglMakeCurrent makes the draw DC the read DC too.
HDC ThreadContextService::CurrentReadDc(MakeCurrentEntryPoint entryPoint, HDC drawDc) {
  auto& table = player::g_DispatchTable;
  switch (entryPoint) {
  case MakeCurrentEntryPoint::ARB:
    return table.wglGetCurrentReadDCARB.load(std::memory_order_acquire)();
  case MakeCurrentEntryPoint::EXT:
    return table.wglGetCurrentReadDCEXT.load(std::memory_order_acquire)();
  default:
    return drawDc;
  }
}

bool ThreadContextService::MakeCurrent(MakeCurrentEntryPoint entryPoint,
                                       HDC drawDc,
                                       HDC readDc,
                                       HGLRC context) {
  auto& table = player::g_DispatchTable;
  switch (entryPoint) {
  case MakeCurrentEntryPoint::ARB:
    return table.wglMakeContextCurrentARB.load(std::memory_order_acquire)(drawDc, readDc, context);
  case MakeCurrentEntryPoint::EXT:
    return table.wglMakeContextCurrentEXT.load(std::memory_order_acquire)(drawDc, readDc, context);
  default:
    return table.wglMakeCurrent.load(std::memory_order_acquire)(drawDc, context);
  }
}

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
    m_EntryPoint = current.EntryPoint;
    return;
  }
  if (MakeCurrent(current.EntryPoint, current.Dc, current.ReadDc, current.Context)) {
    m_EntryPoint = current.EntryPoint;
  } else {
    m_ThreadId.reset();
    static bool warned = false;
    if (!warned) {
      warned = true;
      LOG_WARNING << "ThreadContextService: making current the context of recorded thread "
                  << threadId << " failed - its commands run without that context";
    }
  }
}

void ThreadContextService::NoteCurrent(uint32_t threadId,
                                       MakeCurrentEntryPoint entryPoint,
                                       bool succeeded) {
  if (!succeeded) {
    entryPoint = m_EntryPoint;
  }
  auto& table = player::g_DispatchTable;
  Current& current = m_Current[threadId];
  current.Context = table.wglGetCurrentContext.load(std::memory_order_acquire)();
  current.Dc = current.Context != nullptr ? table.wglGetCurrentDC.load(std::memory_order_acquire)()
                                          : nullptr;
  current.ReadDc = current.Context != nullptr ? CurrentReadDc(entryPoint, current.Dc) : nullptr;
  current.EntryPoint = entryPoint;
  m_EntryPoint = entryPoint;
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
                                          ThreadContextService::MakeCurrentEntryPoint::Core,
                                          command.m_Return.Value != 0);
}

void ThreadContextLayer::Post(wglMakeContextCurrentARBCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::ARB,
                                          command.m_Return.Value != 0);
}

void ThreadContextLayer::Post(wglMakeContextCurrentEXTCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::EXT,
                                          command.m_Return.Value != 0);
}

void ThreadContextLayer::Post(wglDeleteContextCommand& command) {
  // Deleting a context leaves the current one unchanged, unless it was the one deleted, which
  // Forget() detaches.
  if (command.m_Return.Value) {
    ThreadContextService::Get().Forget(command.m_oldContext.Value);
  }
}

} // namespace gits::opengl2
