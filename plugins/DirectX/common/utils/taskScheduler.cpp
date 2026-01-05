// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "taskScheduler.h"

#include "Windows.h"
#include <processthreadsapi.h>

namespace gits {
namespace DirectX {

TaskScheduler::TaskScheduler(const std::string name) : name_{name} {}

TaskScheduler::~TaskScheduler() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    done_ = true;
  }
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
  std::wstring wName(name_.begin(), name_.end());
  SetThreadDescription(thread_.native_handle(), (L"task-scheduler-thread-" + wName).c_str());
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
