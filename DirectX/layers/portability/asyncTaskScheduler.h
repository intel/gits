// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace gits {
namespace DirectX {

class AsyncTaskScheduler : public gits::noncopyable {
public:
  AsyncTaskScheduler(size_t maxTasks = std::thread::hardware_concurrency() / 2)
      : maxTasks_(maxTasks), runningTasks_(0), stop_(false) {}

  ~AsyncTaskScheduler() {
    stopScheduler();
  }

public:
  template <class Ret, class F, class... Args>
  std::future<Ret> enqueue(F&& f, Args&&... args) {
    using returnType = Ret;

    if (stop_) {
      return std::future<returnType>();
    }

    auto task = std::make_shared<std::packaged_task<returnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<returnType> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      tasks_.emplace([task = std::move(task)]() { (*task)(); });
    }
    condition_.notify_one();

    return res;
  }

  void run() {
    auto waitingCall = [this]() -> bool {
      return stop_ || (!tasks_.empty() && runningTasks_ < maxTasks_);
    };

    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queueMutex_);
        condition_.wait(lock, waitingCall);

        if (stop_ && tasks_.empty()) {
          return;
        }
        if (runningTasks_ >= maxTasks_) {
          continue;
        }

        task = std::move(tasks_.front());
        tasks_.pop();
        ++runningTasks_;
      }

      auto res = std::async(std::launch::async, [this, task = std::move(task)]() {
        task();
        --runningTasks_;
        condition_.notify_one();
      });
    }
  }

  void stopScheduler() {
    stop_ = true;
    condition_.notify_all();
  }

private:
  size_t maxTasks_;
  std::queue<std::function<void()>> tasks_;

  std::mutex queueMutex_;
  std::condition_variable condition_;
  std::atomic<size_t> runningTasks_;
  std::atomic<bool> stop_;
};

} // namespace DirectX
} // namespace gits
