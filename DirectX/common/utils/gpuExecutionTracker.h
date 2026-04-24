// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
    unsigned CallKey{};
    unsigned CommandQueueKey{};
    QueueEventKind Kind{};
  };

  struct TrackedFence {
    unsigned Key{};
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
  void CommandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  bool IsCommandQueueWaiting(unsigned commandQueueKey);
  void Execute(unsigned callKey, unsigned commandQueueKey, Executable* executable);
  std::vector<Executable*>& GetReadyExecutables() {
    return m_ReadyExecutables;
  }
  std::unordered_map<unsigned, std::deque<QueueEvent*>>& GetQueueEvents() {
    return m_QueueEvents;
  }

private:
  std::unordered_map<unsigned, std::deque<QueueEvent*>> m_QueueEvents;
  std::unordered_map<unsigned, UINT64> m_SignaledFences;
  std::vector<Executable*> m_ReadyExecutables;
};

} // namespace DirectX
} // namespace gits
