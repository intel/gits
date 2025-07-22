// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuExecutionTracker.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void GpuExecutionTracker::commandQueueWait(unsigned callKey,
                                           unsigned commandQueueKey,
                                           unsigned fenceKey,
                                           UINT64 fenceValue) {
  auto it = signaledFences_.find(fenceKey);
  if (it != signaledFences_.end() && it->second >= fenceValue) {
    return;
  }

  WaitEvent* waitEvent = new WaitEvent();
  waitEvent->callKey = callKey;
  waitEvent->commandQueueKey = commandQueueKey;
  waitEvent->fence.key = fenceKey;
  waitEvent->fence.value = fenceValue;
  queueEvents_[commandQueueKey].push_back(waitEvent);
}

void GpuExecutionTracker::commandQueueSignal(unsigned callKey,
                                             unsigned commandQueueKey,
                                             unsigned fenceKey,
                                             UINT64 fenceValue) {
  auto it = queueEvents_.find(commandQueueKey);
  if (it == queueEvents_.end() || it->second.empty()) {
    fenceSignal(callKey, fenceKey, fenceValue);
  } else {
    SignalEvent* signalEvent = new SignalEvent();
    signalEvent->callKey = callKey;
    signalEvent->commandQueueKey = commandQueueKey;
    signalEvent->fence.key = fenceKey;
    signalEvent->fence.value = fenceValue;
    it->second.push_back(signalEvent);
  }
}

void GpuExecutionTracker::fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue) {
  signaledFences_[fenceKey] = fenceValue;

  std::vector<SignalEvent*> signaled;
  for (auto& it : queueEvents_) {
    while (!it.second.empty()) {
      QueueEvent* queueEvent = it.second.front();
      if (queueEvent->type == QueueEvent::Wait) {
        WaitEvent* waitEvent = static_cast<WaitEvent*>(queueEvent);
        auto itFence = signaledFences_.find(waitEvent->fence.key);
        if (itFence != signaledFences_.end() && itFence->second >= waitEvent->fence.value) {
          delete queueEvent;
        } else {
          break;
        }
      } else if (queueEvent->type == QueueEvent::Execute) {
        readyExecutables_.push_back(static_cast<Executable*>(queueEvent));
      } else if (queueEvent->type == QueueEvent::Signal) {
        signaled.push_back(static_cast<SignalEvent*>(queueEvent));
      }
      it.second.pop_front();
    }
  }

  for (SignalEvent* signal : signaled) {
    fenceSignal(signal->callKey, signal->fence.key, signal->fence.value);
    delete signal;
  }
}

bool GpuExecutionTracker::isCommandQueueWaiting(unsigned commandQueueKey) {
  auto it = queueEvents_.find(commandQueueKey);
  if (it == queueEvents_.end() || it->second.empty()) {
    return false;
  }
  return true;
}

void GpuExecutionTracker::execute(unsigned callKey,
                                  unsigned commandQueueKey,
                                  Executable* executable) {
  executable->callKey = callKey;
  executable->commandQueueKey = commandQueueKey;
  auto it = queueEvents_.find(commandQueueKey);
  if (it == queueEvents_.end() || it->second.empty()) {
    readyExecutables_.push_back(executable);
  } else {
    it->second.push_back(executable);
  }
}

} // namespace DirectX
} // namespace gits
