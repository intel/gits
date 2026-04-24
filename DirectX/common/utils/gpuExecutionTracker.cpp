// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuExecutionTracker.h"
#include "log.h"

#include <queue>

namespace gits {
namespace DirectX {

void GpuExecutionTracker::CommandQueueWait(unsigned callKey,
                                           unsigned commandQueueKey,
                                           unsigned fenceKey,
                                           UINT64 fenceValue) {
  auto it = m_SignaledFences.find(fenceKey);
  if (it != m_SignaledFences.end() && it->second >= fenceValue) {
    return;
  }

  WaitEvent* waitEvent = new WaitEvent();
  waitEvent->CallKey = callKey;
  waitEvent->CommandQueueKey = commandQueueKey;
  waitEvent->Fence.Key = fenceKey;
  waitEvent->Fence.Value = fenceValue;
  m_QueueEvents[commandQueueKey].push_back(waitEvent);
}

void GpuExecutionTracker::CommandQueueSignal(unsigned callKey,
                                             unsigned commandQueueKey,
                                             unsigned fenceKey,
                                             UINT64 fenceValue) {
  auto it = m_QueueEvents.find(commandQueueKey);
  if (it == m_QueueEvents.end() || it->second.empty()) {
    FenceSignal(callKey, fenceKey, fenceValue);
  } else {
    SignalEvent* signalEvent = new SignalEvent();
    signalEvent->CallKey = callKey;
    signalEvent->CommandQueueKey = commandQueueKey;
    signalEvent->Fence.Key = fenceKey;
    signalEvent->Fence.Value = fenceValue;
    it->second.push_back(signalEvent);
  }
}

void GpuExecutionTracker::FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue) {
  std::queue<SignalEvent*> signaled;
  auto* initialSignal = new SignalEvent{};
  initialSignal->CallKey = callKey;
  initialSignal->Fence = {fenceKey, fenceValue};
  signaled.push(initialSignal);

  while (!signaled.empty()) {
    SignalEvent* signalEvent = signaled.front();
    m_SignaledFences[signalEvent->Fence.Key] = signalEvent->Fence.Value;

    for (auto& it : m_QueueEvents) {
      while (!it.second.empty()) {
        QueueEvent* queueEvent = it.second.front();
        if (queueEvent->Kind == QueueEventKind::Wait) {
          WaitEvent* waitEvent = static_cast<WaitEvent*>(queueEvent);
          auto itFence = m_SignaledFences.find(waitEvent->Fence.Key);
          if (itFence != m_SignaledFences.end() && itFence->second >= waitEvent->Fence.Value) {
            delete queueEvent;
          } else {
            break;
          }
        } else if (queueEvent->Kind == QueueEventKind::Execute) {
          m_ReadyExecutables.push_back(static_cast<Executable*>(queueEvent));
        } else if (queueEvent->Kind == QueueEventKind::Signal) {
          signaled.push(static_cast<SignalEvent*>(queueEvent));
        }
        it.second.pop_front();
      }
    }

    delete signalEvent;
    signaled.pop();
  }
}

bool GpuExecutionTracker::IsCommandQueueWaiting(unsigned commandQueueKey) {
  auto it = m_QueueEvents.find(commandQueueKey);
  if (it == m_QueueEvents.end() || it->second.empty()) {
    return false;
  }
  return true;
}

void GpuExecutionTracker::Execute(unsigned callKey,
                                  unsigned commandQueueKey,
                                  Executable* executable) {
  executable->CallKey = callKey;
  executable->CommandQueueKey = commandQueueKey;
  auto it = m_QueueEvents.find(commandQueueKey);
  if (it == m_QueueEvents.end() || it->second.empty()) {
    m_ReadyExecutables.push_back(executable);
  } else {
    it->second.push_back(executable);
  }
}

} // namespace DirectX
} // namespace gits
