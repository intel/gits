// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectCreationTask.h"
#include <thread>
#include <mutex>
#include <memory>
#include <deque>
#include <optional>
#include <condition_variable>
#include <unordered_map>

namespace gits {
namespace vulkan {

class MultithreadedObjectCreationService {
public:
  MultithreadedObjectCreationService();
  ~MultithreadedObjectCreationService();

  void Schedule(std::span<const GITSKey> keys, std::shared_ptr<ObjectCreationTask> task);
  bool Cancel(GITSKey key);
  std::optional<VkResult> Complete(GITSKey key);
  void WaitForConsumers(GITSKey key);
  void Shutdown();

private:
  bool m_Finish{};
  std::mutex m_Mutex;
  std::condition_variable m_ConditionVariable;

  std::unordered_map<GITSKey, std::shared_ptr<ObjectCreationTask>> m_Tasks;
  std::unordered_map<GITSKey, std::vector<std::shared_ptr<ObjectCreationTask>>> m_Consumers;
  std::vector<std::thread> m_Workers;
  std::deque<std::shared_ptr<ObjectCreationTask>> m_TasksQueue;

  void Initialize();
  void RemoveTask(const std::shared_ptr<ObjectCreationTask>& task);
  void RemoveDependencies(const std::shared_ptr<ObjectCreationTask>& task);
  void WorkerThread();
}; // class MultithreadedObjectCreationService

} // namespace vulkan
} // namespace gits
