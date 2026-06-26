// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace gits {
namespace DirectX {

class TaskScheduler : public gits::noncopyable {
public:
  using Task = std::function<void()>;
  TaskScheduler(const std::string name = "");
  ~TaskScheduler();
  void Schedule(Task task);

private:
  void Initialize();
  void ThreadProc();

  bool m_Initialized{false};
  std::string m_Name;
  std::queue<Task> m_Tasks;
  std::mutex m_Mutex;
  std::thread m_Thread;
  std::condition_variable m_Cv;
  bool m_Done{false};
};

} // namespace DirectX
} // namespace gits
