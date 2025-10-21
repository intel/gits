// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "fenceService.h"
#include "commandsAuto.h"
#include "captureManager.h"
#include "commandWritersCustom.h"
#include "log.h"

namespace gits {
namespace DirectX {

FenceService::FenceService(GitsRecorder& recorder) : recorder_(recorder) {}

void FenceService::setEventOnCompletion(ID3D12Fence* fence,
                                        unsigned fenceKey,
                                        UINT64 value,
                                        HANDLE event) {
  std::lock_guard<std::mutex> fenceLock(mutex_);

  FenceInfo& fenceInfo = fencesByHandle_[event][fenceKey];
  fenceInfo.fence = fence;
  fenceInfo.fenceKey = fenceKey;
  fenceInfo.value = value;
  fenceInfo.event = event;
  fenceInfo.signaled = false;

  fences_.insert(fenceKey);
}

void FenceService::destroyFence(unsigned fenceKey) {
  std::lock_guard<std::mutex> fenceLock(mutex_);
  auto it = fences_.find(fenceKey);
  if (it == fences_.end()) {
    return;
  }
  for (auto& itHandle = fencesByHandle_.begin(); itHandle != fencesByHandle_.end();) {
    for (auto& it = itHandle->second.begin(); it != itHandle->second.end();) {
      if (it->first == fenceKey) {
        it = itHandle->second.erase(it);
      } else {
        ++it;
      }
    }
    if (itHandle->second.empty()) {
      itHandle = fencesByHandle_.erase(itHandle);
    } else {
      ++itHandle;
    }
  }
}

void FenceService::waitSignaled(HANDLE handle) {

  mutex_.lock();

  auto itHandle = fencesByHandle_.find(handle);
  if (itHandle == fencesByHandle_.end()) {
    mutex_.unlock();
    return;
  }

  std::vector<FenceInfo> fenceInfos;
  bool single = itHandle->second.size() == 1;
  for (auto& it : itHandle->second) {
    FenceInfo& info = it.second;
    if (info.signaled) {
      continue;
    }
    if (single || info.fence->GetCompletedValue() >= info.value) {
      fenceInfos.push_back(info);
      info.signaled = true;
    }
  }

  for (FenceInfo& info : fenceInfos) {
    itHandle->second.erase(info.fenceKey);
  }
  if (itHandle->second.empty()) {
    fencesByHandle_.erase(itHandle);
  }

  mutex_.unlock();

  std::lock_guard<std::mutex> fenceLock(globalMutex_);

  for (unsigned i = 0; i < fenceInfos.size(); ++i) {

    WaitForFenceSignaledCommand command{};
    command.key = CaptureManager::get().createCommandKey();
    command.threadId = GetCurrentThreadId();
    command.event_.value = handle;
    command.fence_.key = fenceInfos[i].fenceKey;
    command.value_.value = fenceInfos[i].fence->GetCompletedValue();

    recorder_.record(command.key, new WaitForFenceSignaledWriter(command));
  }
}

void FenceService::waitSignaled(HANDLE hObjectToWaitOn, HANDLE hObjectToSignal) {

  mutex_.lock();

  auto it = fencesByHandle_.find(hObjectToWaitOn);
  if (it == fencesByHandle_.end()) {
    mutex_.unlock();
    return;
  }

  static bool printed = false;
  if (!printed) {
    LOG_ERROR << "SignalObjectAndWait is not handled!";
    printed = true;
  }
  mutex_.unlock();
}

void FenceService::waitSignaled(DWORD count, const HANDLE* handles) {

  mutex_.lock();

  bool found = false;
  for (unsigned i = 0; i < count; ++i) {
    auto it = fencesByHandle_.find(handles[i]);
    if (it != fencesByHandle_.end()) {
      found = true;
      break;
    }
  }
  if (!found) {
    mutex_.unlock();
    return;
  }

  std::vector<FenceInfo> fenceInfos;

  for (unsigned i = 0; i < count; ++i) {
    auto itHandle = fencesByHandle_.find(handles[i]);
    if (itHandle != fencesByHandle_.end()) {
      bool single = itHandle->second.size() == 1;
      for (auto& it : itHandle->second) {
        FenceInfo& info = it.second;
        if (info.signaled) {
          continue;
        }
        if (single || info.fence->GetCompletedValue() >= info.value) {
          fenceInfos.push_back(info);
          info.signaled = true;
        }
      }
    }
  }

  for (FenceInfo& info : fenceInfos) {
    auto itHandle = fencesByHandle_.find(info.event);
    if (itHandle != fencesByHandle_.end()) {
      itHandle->second.erase(info.fenceKey);
      if (itHandle->second.empty()) {
        fencesByHandle_.erase(itHandle);
      }
    }
  }

  mutex_.unlock();

  std::lock_guard<std::mutex> fenceLock(globalMutex_);

  for (unsigned i = 0; i < fenceInfos.size(); ++i) {

    WaitForFenceSignaledCommand command{};
    command.key = CaptureManager::get().createCommandKey();
    command.threadId = GetCurrentThreadId();
    command.event_.value = fenceInfos[i].event;
    command.fence_.key = fenceInfos[i].fenceKey;
    command.value_.value = fenceInfos[i].fence->GetCompletedValue();

    recorder_.record(command.key, new WaitForFenceSignaledWriter(command));
  }
}

} // namespace DirectX
} // namespace gits
