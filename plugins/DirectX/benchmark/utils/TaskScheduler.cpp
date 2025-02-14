// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "TaskScheduler.h"

#include "Windows.h"

namespace {

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName) {
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#pragma warning(pop)
}

} // namespace

namespace gits {
namespace DirectX {

TaskScheduler::TaskScheduler(const std::string name) : name_{name} {}

TaskScheduler::~TaskScheduler() {
  done_ = true;
  cv_.notify_all();
  if (thread_.joinable()) {
    thread_.join();
  }
}

void TaskScheduler::schedule(Task task) {
  initialize();

  {
    std::lock_guard<std::mutex> guard(mutex_);
    tasks_.push(task);
  }
  cv_.notify_one();
}

void TaskScheduler::initialize() {
  if (initialized_) {
    return;
  }

  initialized_ = true;
  thread_ = std::thread{&TaskScheduler::threadProc, this};
  DWORD threadId = GetThreadId(static_cast<HANDLE>(thread_.native_handle()));
  SetThreadName(threadId, ("task-scheduler-thread-" + name_).c_str());
}

void TaskScheduler::threadProc() {
  const auto& emptyQueue = [this]() {
    while (!tasks_.empty()) {
      Task task = std::move(tasks_.front());
      tasks_.pop();
      task();
    }
  };

  while (true) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !tasks_.empty() || done_; });
      if (done_) {
        emptyQueue();
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }

    task();
  }
}

} // namespace DirectX
} // namespace gits
