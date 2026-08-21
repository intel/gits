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
#include <optional>
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
    GITSKey CommandQueueKey{};
    QueueEventKind Kind{};
  };

  struct TrackedFence {
    GITSKey Key{};
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
                        GITSKey commandQueueKey,
                        GITSKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(CommandKey callKey,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(CommandKey callKey, GITSKey fenceKey, UINT64 fenceValue);
  bool IsCommandQueueWaiting(GITSKey commandQueueKey);
  void Execute(CommandKey callKey, GITSKey commandQueueKey, Executable* executable);
  std::optional<UINT64> GetFenceValue(GITSKey fenceKey) const;
  std::vector<Executable*>& GetReadyExecutables() {
    return m_ReadyExecutables;
  }
  std::unordered_map<GITSKey, std::deque<QueueEvent*>>& GetQueueEvents() {
    return m_QueueEvents;
  }

private:
  std::unordered_map<GITSKey, std::deque<QueueEvent*>> m_QueueEvents;
  std::unordered_map<GITSKey, UINT64> m_SignaledFences;
  std::vector<Executable*> m_ReadyExecutables;
};

} // namespace DirectX
} // namespace gits
