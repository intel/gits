// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "command.h"

#include <vector>
#include <unordered_map>
#include <deque>
#include <functional>
#include <basetsd.h>

namespace gits {
namespace DirectX {

class GpuExecutionTracker {
public:
  enum class QueueEventKind {
    Wait,
    Signal,
    Execute
  };

  struct QueueEvent {
    QueueEvent(QueueEventKind kind) : Kind(kind) {}
    virtual ~QueueEvent() = default;
    CommandKey CallKey{};
    ObjectKey CommandQueueKey{};
    QueueEventKind Kind{};
  };

  struct TrackedFence {
    ObjectKey Key{};
    UINT64 Value{};
  };

  struct WaitEvent : public QueueEvent {
    WaitEvent() : QueueEvent(QueueEventKind::Wait) {}
    TrackedFence Fence{};
  };

  struct SignalEvent : public QueueEvent {
    SignalEvent() : QueueEvent(QueueEventKind::Signal) {}
    TrackedFence Fence{};
  };

  struct Executable : public QueueEvent {
    Executable() : QueueEvent(QueueEventKind::Execute) {}
    ~Executable() override = default;
  };

public:
  void CommandQueueWait(CommandKey callKey,
                        ObjectKey commandQueueKey,
                        ObjectKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(CommandKey callKey,
                          ObjectKey commandQueueKey,
                          ObjectKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(CommandKey callKey, ObjectKey fenceKey, UINT64 fenceValue);
  bool IsCommandQueueWaiting(ObjectKey commandQueueKey);
  void Execute(CommandKey callKey, ObjectKey commandQueueKey, Executable* executable);
  std::vector<Executable*>& GetReadyExecutables() {
    return m_ReadyExecutables;
  }
  std::unordered_map<ObjectKey, std::deque<QueueEvent*>>& GetQueueEvents() {
    return m_QueueEvents;
  }

private:
  std::unordered_map<ObjectKey, std::deque<QueueEvent*>> m_QueueEvents;
  std::unordered_map<ObjectKey, UINT64> m_SignaledFences;
  std::vector<Executable*> m_ReadyExecutables;
};

} // namespace DirectX
} // namespace gits
