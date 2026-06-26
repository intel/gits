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

TaskScheduler::TaskScheduler(const std::string name) : m_Name(name) {}

TaskScheduler::~TaskScheduler() {
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Done = true;
  }
  m_Cv.notify_all();
  if (m_Thread.joinable()) {
    m_Thread.join();
  }
}

void TaskScheduler::Schedule(Task task) {
  Initialize();

  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    m_Tasks.push(task);
  }
  m_Cv.notify_one();
}

void TaskScheduler::Initialize() {
  if (m_Initialized) {
    return;
  }

  m_Initialized = true;
  m_Thread = std::thread{&TaskScheduler::ThreadProc, this};
  std::wstring wName(m_Name.begin(), m_Name.end());
  SetThreadDescription(m_Thread.native_handle(), (L"task-scheduler-thread-" + wName).c_str());
}

void TaskScheduler::ThreadProc() {
  const auto& emptyQueue = [this]() {
    while (!m_Tasks.empty()) {
      Task task = std::move(m_Tasks.front());
      m_Tasks.pop();
      task();
    }
  };

  while (true) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_Cv.wait(lock, [this] { return !m_Tasks.empty() || m_Done; });
      if (m_Done) {
        emptyQueue();
        return;
      }

      task = std::move(m_Tasks.front());
      m_Tasks.pop();
    }

    task();
  }
}

} // namespace DirectX
} // namespace gits
