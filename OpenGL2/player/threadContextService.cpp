// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "threadContextService.h"

#include "glxDrawableService.h"
#include "log.h"
#include "openglDriversAuto.h"

#include <algorithm>

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
  if (IsCurrent(current) || MakeCurrent(current)) {
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

bool ThreadContextService::IsCurrent(const Current& current) {
  auto& table = player::g_DispatchTable;
  if (current.EntryPoint == MakeCurrentEntryPoint::Glx) {
    return table.glXGetCurrentContext.load(std::memory_order_acquire)() == current.GlxContext &&
           (current.GlxContext == nullptr ||
            (table.glXGetCurrentDrawable.load(std::memory_order_acquire)() == current.Drawable &&
             table.glXGetCurrentReadDrawable.load(std::memory_order_acquire)() ==
                 current.ReadDrawable));
  }
  return table.wglGetCurrentContext.load(std::memory_order_acquire)() == current.Context &&
         (current.Context == nullptr ||
          (table.wglGetCurrentDC.load(std::memory_order_acquire)() == current.Dc &&
           CurrentReadDc(current.EntryPoint, current.Dc) == current.ReadDc));
}

bool ThreadContextService::MakeCurrent(const Current& current) {
  if (current.EntryPoint == MakeCurrentEntryPoint::Glx) {
    return player::g_DispatchTable.glXMakeContextCurrent.load(std::memory_order_acquire)(
               GlxDrawableService::Get().GetDisplay(), current.Drawable, current.ReadDrawable,
               current.GlxContext) != 0;
  }
  return MakeCurrent(current.EntryPoint, current.Dc, current.ReadDc, current.Context);
}

void ThreadContextService::NoteCurrent(uint32_t threadId,
                                       MakeCurrentEntryPoint entryPoint,
                                       bool succeeded) {
  auto& table = player::g_DispatchTable;
  Current& current = m_Current[threadId];
  if (entryPoint == MakeCurrentEntryPoint::Glx) {
    current = {};
    current.GlxContext = table.glXGetCurrentContext.load(std::memory_order_acquire)();
    if (current.GlxContext != nullptr) {
      current.Drawable = table.glXGetCurrentDrawable.load(std::memory_order_acquire)();
      current.ReadDrawable = table.glXGetCurrentReadDrawable.load(std::memory_order_acquire)();
    }
    current.EntryPoint = entryPoint;
    m_EntryPoint = entryPoint;
    return;
  }
  if (!succeeded) {
    entryPoint = m_EntryPoint;
  }
  current.Context = table.wglGetCurrentContext.load(std::memory_order_acquire)();
  current.Dc = current.Context != nullptr ? table.wglGetCurrentDC.load(std::memory_order_acquire)()
                                          : nullptr;
  current.ReadDc = current.Context != nullptr ? CurrentReadDc(entryPoint, current.Dc) : nullptr;
  current.EntryPoint = entryPoint;
  m_EntryPoint = entryPoint;
  DestroyReleased();
}

void ThreadContextService::Forget(HGLRC context) {
  for (auto& [threadId, current] : m_Current) {
    if (current.Context == context) {
      current = {};
    }
  }
}

void ThreadContextService::Destroy(GLXContext context) {
  if (context != nullptr && IsBound(context)) {
    m_DestroyPending.push_back(context);
    return;
  }
  player::g_DispatchTable.glXDestroyContext.load(std::memory_order_acquire)(
      GlxDrawableService::Get().GetDisplay(), context);
}

void ThreadContextService::DestroyDrawable(GLXDrawable drawable, std::function<void()> destroy) {
  if (drawable != 0 && IsDrawableBound(drawable)) {
    m_DrawableDestroyPending.emplace_back(drawable, std::move(destroy));
    return;
  }
  destroy();
}

bool ThreadContextService::IsBound(GLXContext context) const {
  return std::any_of(m_Current.begin(), m_Current.end(), [context](const auto& entry) {
    return entry.second.EntryPoint == MakeCurrentEntryPoint::Glx &&
           entry.second.GlxContext == context;
  });
}

bool ThreadContextService::IsDrawableBound(GLXDrawable drawable) const {
  return std::any_of(m_Current.begin(), m_Current.end(), [drawable](const auto& entry) {
    return entry.second.EntryPoint == MakeCurrentEntryPoint::Glx &&
           (entry.second.Drawable == drawable || entry.second.ReadDrawable == drawable);
  });
}

void ThreadContextService::DestroyReleased() {
  std::erase_if(m_DestroyPending, [this](GLXContext context) {
    if (IsBound(context)) {
      return false;
    }
    player::g_DispatchTable.glXDestroyContext.load(std::memory_order_acquire)(
        GlxDrawableService::Get().GetDisplay(), context);
    return true;
  });
  std::erase_if(m_DrawableDestroyPending, [this](const auto& pending) {
    if (IsDrawableBound(pending.first)) {
      return false;
    }
    pending.second();
    return true;
  });
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

void ThreadContextLayer::Post(glXMakeCurrentCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::Glx,
                                          command.m_Return.Value != 0);
}

void ThreadContextLayer::Post(glXMakeContextCurrentCommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::Glx,
                                          command.m_Return.Value != 0);
}

void ThreadContextLayer::Post(glXMakeCurrentReadSGICommand& command) {
  ThreadContextService::Get().NoteCurrent(command.m_ThreadId,
                                          ThreadContextService::MakeCurrentEntryPoint::Glx,
                                          command.m_Return.Value != 0);
}
} // namespace gits::opengl2
