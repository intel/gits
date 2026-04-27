// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "fenceService.h"
#include "commandsAuto.h"
#include "captureManager.h"
#include "commandSerializersCustom.h"
#include "log.h"

namespace gits {
namespace DirectX {

FenceService::FenceService(stream::OrderingRecorder& recorder) : m_Recorder(recorder) {}

void FenceService::SetEventOnCompletion(ID3D12Fence* fence,
                                        unsigned fenceKey,
                                        UINT64 value,
                                        HANDLE event) {
  std::lock_guard<std::mutex> fenceLock(m_Mutex);

  FenceInfo& fenceInfo = m_FencesByHandle[event][fenceKey];
  fenceInfo.Fence = fence;
  fenceInfo.FenceKey = fenceKey;
  fenceInfo.Value = value;
  fenceInfo.Event = event;
  fenceInfo.Signaled = false;

  m_Fences.insert(fenceKey);
}

void FenceService::DestroyFence(unsigned fenceKey) {
  std::lock_guard<std::mutex> fenceLock(m_Mutex);
  auto it = m_Fences.find(fenceKey);
  if (it == m_Fences.end()) {
    return;
  }
  for (auto itHandle = m_FencesByHandle.begin(); itHandle != m_FencesByHandle.end();) {
    for (auto it = itHandle->second.begin(); it != itHandle->second.end();) {
      if (it->first == fenceKey) {
        it = itHandle->second.erase(it);
      } else {
        ++it;
      }
    }
    if (itHandle->second.empty()) {
      itHandle = m_FencesByHandle.erase(itHandle);
    } else {
      ++itHandle;
    }
  }
}

void FenceService::WaitSignaled(HANDLE handle) {

  m_Mutex.lock();

  auto itHandle = m_FencesByHandle.find(handle);
  if (itHandle == m_FencesByHandle.end()) {
    m_Mutex.unlock();
    return;
  }

  std::vector<FenceInfo> fenceInfos;
  bool single = itHandle->second.size() == 1;
  for (auto& it : itHandle->second) {
    FenceInfo& info = it.second;
    if (info.Signaled) {
      continue;
    }
    if (single || info.Fence->GetCompletedValue() >= info.Value) {
      fenceInfos.push_back(info);
      info.Signaled = true;
    }
  }

  for (FenceInfo& info : fenceInfos) {
    itHandle->second.erase(info.FenceKey);
  }
  if (itHandle->second.empty()) {
    m_FencesByHandle.erase(itHandle);
  }

  m_Mutex.unlock();

  std::lock_guard<std::mutex> fenceLock(m_GlobalMutex);

  for (unsigned i = 0; i < fenceInfos.size(); ++i) {

    WaitForFenceSignaledCommand command{};
    command.Key = CaptureManager::get().createCommandKey();
    command.ThreadId = GetCurrentThreadId();
    command.m_event.Value = handle;
    command.m_fence.Key = fenceInfos[i].FenceKey;
    command.m_Value.Value = fenceInfos[i].Fence->GetCompletedValue();

    m_Recorder.Record(command.Key, new WaitForFenceSignaledSerializer(command));
  }
}

void FenceService::WaitSignaled(HANDLE hObjectToWaitOn, HANDLE hObjectToSignal) {

  m_Mutex.lock();

  auto it = m_FencesByHandle.find(hObjectToWaitOn);
  if (it == m_FencesByHandle.end()) {
    m_Mutex.unlock();
    return;
  }

  static bool printed = false;
  if (!printed) {
    LOG_ERROR << "SignalObjectAndWait is not handled!";
    printed = true;
  }
  m_Mutex.unlock();
}

void FenceService::WaitSignaled(DWORD count, const HANDLE* handles) {

  m_Mutex.lock();

  bool found = false;
  for (unsigned i = 0; i < count; ++i) {
    auto it = m_FencesByHandle.find(handles[i]);
    if (it != m_FencesByHandle.end()) {
      found = true;
      break;
    }
  }
  if (!found) {
    m_Mutex.unlock();
    return;
  }

  std::vector<FenceInfo> fenceInfos;

  for (unsigned i = 0; i < count; ++i) {
    auto itHandle = m_FencesByHandle.find(handles[i]);
    if (itHandle != m_FencesByHandle.end()) {
      bool single = itHandle->second.size() == 1;
      for (auto& it : itHandle->second) {
        FenceInfo& info = it.second;
        if (info.Signaled) {
          continue;
        }
        if (single || info.Fence->GetCompletedValue() >= info.Value) {
          fenceInfos.push_back(info);
          info.Signaled = true;
        }
      }
    }
  }

  for (FenceInfo& info : fenceInfos) {
    auto itHandle = m_FencesByHandle.find(info.Event);
    if (itHandle != m_FencesByHandle.end()) {
      itHandle->second.erase(info.FenceKey);
      if (itHandle->second.empty()) {
        m_FencesByHandle.erase(itHandle);
      }
    }
  }

  m_Mutex.unlock();

  std::lock_guard<std::mutex> fenceLock(m_GlobalMutex);

  for (unsigned i = 0; i < fenceInfos.size(); ++i) {

    WaitForFenceSignaledCommand command{};
    command.Key = CaptureManager::get().createCommandKey();
    command.ThreadId = GetCurrentThreadId();
    command.m_event.Value = fenceInfos[i].Event;
    command.m_fence.Key = fenceInfos[i].FenceKey;
    command.m_Value.Value = fenceInfos[i].Fence->GetCompletedValue();

    m_Recorder.Record(command.Key, new WaitForFenceSignaledSerializer(command));
  }
}

} // namespace DirectX
} // namespace gits
