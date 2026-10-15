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
       table.wglGetCurrentDC.load(std::memory_order_acquire)() == current.Dc)) {
    return;
  }
  if (!table.wglMakeCurrent.load(std::memory_order_acquire)(current.Dc, current.Context)) {
    m_ThreadId.reset();
    static bool warned = false;
    if (!warned) {
      warned = true;
      LOG_WARNING << "ThreadContextService: wglMakeCurrent failed switching to the context of "
                     "recorded thread "
                  << threadId << " - its commands run without that context";
    }
  }
}

void ThreadContextService::NoteCurrent(uint32_t threadId) {
  auto& table = player::g_DispatchTable;
  Current& current = m_Current[threadId];
  current.Context = table.wglGetCurrentContext.load(std::memory_order_acquire)();
  current.Dc = current.Context != nullptr ? table.wglGetCurrentDC.load(std::memory_order_acquire)()
                                          : nullptr;
}

void ThreadContextService::Forget(HGLRC context) {
  for (auto& [threadId, current] : m_Current) {
    if (current.Context == context) {
      current = {};
    }
  }
}

void ThreadContextLayer::Post(wglMakeCurrentCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId);
}

void ThreadContextLayer::Post(wglMakeContextCurrentARBCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId);
}

void ThreadContextLayer::Post(wglMakeContextCurrentEXTCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId);
}

void ThreadContextLayer::Post(wglDeleteContextCommand& command) {
  if (command.m_Return.Value) {
    ThreadContextService::Get().Forget(command.m_oldContext.Value);
  }
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId);
}

} // namespace gits::opengl2
