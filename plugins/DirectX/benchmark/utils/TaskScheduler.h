// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <functional>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace gits {
namespace DirectX {

class TaskScheduler {
public:
  using Task = std::function<void()>;
  TaskScheduler(const std::string name = "");
  ~TaskScheduler();
  void schedule(Task task);

private:
  void initialize();
  void threadProc();

  bool initialized_{false};
  std::string name_;
  std::queue<Task> tasks_;
  std::mutex mutex_;
  std::thread thread_;
  std::condition_variable cv_;
  std::atomic<bool> done_{false};
};

} // namespace DirectX
} // namespace gits
