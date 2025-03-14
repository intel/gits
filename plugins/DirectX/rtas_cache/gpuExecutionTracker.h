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
#include <unordered_set>
#include <deque>
#include <functional>
#include <basetsd.h>

namespace gits {
namespace DirectX {

class GpuExecutionTracker {
private:
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
    struct Hash {
      size_t operator()(const Fence& fence) const {
        size_t h1 = std::hash<unsigned>()(fence.key);
        size_t h2 = std::hash<UINT64>()(fence.value);
        return h1 ^ h2 << 1;
      }
    };
    bool operator==(const Fence& fence) const {
      return fence.key == key && fence.value == value;
    }
  };

  struct WaitEvent : public QueueEvent {
    WaitEvent() : QueueEvent(Wait) {}
    Fence fence{};
  };

  struct SignalEvent : public QueueEvent {
    SignalEvent() : QueueEvent(Signal) {}
    Fence fence{};
  };

public:
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

private:
  std::unordered_map<unsigned, std::deque<QueueEvent*>> queueEvents_;
  std::unordered_set<Fence, Fence::Hash> signaledFences_;
  std::vector<Executable*> readyExecutables_;
};

} // namespace DirectX
} // namespace gits
