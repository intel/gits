// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
  struct QueueEvent {
    enum Type {
      Wait,
      Signal,
      Execute
    };
    QueueEvent(Type t) : type(t) {}
    virtual ~QueueEvent() {}
    unsigned callKey{};
    unsigned commandQueueKey{};
    Type type{};
  };

  struct Fence {
    unsigned key{};
    UINT64 value{};
  };

  struct WaitEvent : public QueueEvent {
    WaitEvent() : QueueEvent(Wait) {}
    Fence fence{};
  };

  struct SignalEvent : public QueueEvent {
    SignalEvent() : QueueEvent(Signal) {}
    Fence fence{};
  };

  struct Executable : public QueueEvent {
    Executable() : QueueEvent(Execute) {}
    virtual ~Executable() {}
  };

public:
  void commandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  bool isCommandQueueWaiting(unsigned commandQueueKey);
  void execute(unsigned callKey, unsigned commandQueueKey, Executable* executable);
  std::vector<Executable*>& getReadyExecutables() {
    return readyExecutables_;
  }
  std::unordered_map<unsigned, std::deque<QueueEvent*>>& getQueueEvents() {
    return queueEvents_;
  }

private:
  std::unordered_map<unsigned, std::deque<QueueEvent*>> queueEvents_;
  std::unordered_map<unsigned, UINT64> signaledFences_;
  std::vector<Executable*> readyExecutables_;
};

} // namespace DirectX
} // namespace gits
