// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectCreationService.h"
#include "exception.h"
#include "log.h"

#include <algorithm>
#include <unordered_set>

namespace gits {
namespace vulkan {

MultithreadedObjectCreationService::MultithreadedObjectCreationService() {
  Initialize();
}

void MultithreadedObjectCreationService::Initialize() {
  unsigned threadCount =
      std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;

  try {
    for (unsigned index = 0; index < threadCount; ++index) {
      m_Workers.emplace_back(&MultithreadedObjectCreationService::WorkerThread, this);
    }
  } catch (...) {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_Finish = true;
    }
    m_ConditionVariable.notify_all();
    for (auto& worker : m_Workers) {
      worker.join();
    }
    throw;
  }
}

MultithreadedObjectCreationService::~MultithreadedObjectCreationService() {
  try {
    Shutdown();
  } catch (...) {
    topmost_exception_handler(
        "MultithreadedObjectCreationService::~MultithreadedObjectCreationService");
  }
}

void MultithreadedObjectCreationService::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Finish = true;
  }
  m_ConditionVariable.notify_all();
  for (auto& worker : m_Workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  std::exception_ptr failure;
  while (true) {
    GITSKey key;
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      if (m_Tasks.empty()) {
        break;
      }
      key = m_Tasks.begin()->first;
    }
    try {
      auto result = Complete(key);
      if (result && *result < 0) {
        LOG_ERROR << "Background pipeline creation failed during shutdown for key " << key
                  << ": VkResult " << static_cast<int>(*result);
      }
    } catch (...) {
      if (!failure) {
        failure = std::current_exception();
      }
    }
  }
  if (failure) {
    std::rethrow_exception(failure);
  }
}

void MultithreadedObjectCreationService::Schedule(std::span<const GITSKey> keys,
                                                  std::shared_ptr<ObjectCreationTask> task) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  if (m_Finish || !task || (task->m_State != ObjectCreationTask::State::Unscheduled)) {
    throw std::logic_error("Invalid pipeline creation task scheduling.");
  }

  std::unordered_set<GITSKey> uniqueKeys;
  for (auto key : keys) {
    if (key && (!uniqueKeys.insert(key).second || m_Tasks.contains(key))) {
      throw std::logic_error("Duplicate pipeline creation output key.");
    }
  }
  if (uniqueKeys.empty()) {
    throw std::logic_error("Pipeline creation task has no output keys.");
  }

  task->m_OutputKeys.assign(keys.begin(), keys.end());
  try {
    for (auto key : task->m_InputKeys) {
      m_Consumers[key].push_back(task);
    }
    for (auto key : keys) {
      if (key) {
        m_Tasks.emplace(key, task);
      }
    }
    m_TasksQueue.push_back(task);
  } catch (...) {
    RemoveTask(task);
    RemoveDependencies(task);
    throw;
  }
  task->m_State = ObjectCreationTask::State::Queued;
  m_ConditionVariable.notify_one();
}

bool MultithreadedObjectCreationService::Cancel(GITSKey key) {
  std::lock_guard<std::mutex> guard(m_Mutex);

  auto it = m_Tasks.find(key);
  if (it == m_Tasks.end()) {
    return false;
  }

  auto task = it->second;
  if (task->m_State != ObjectCreationTask::State::Queued || task->m_OutputKeys.size() != 1) {
    return false;
  }

  task->m_State = ObjectCreationTask::State::Cancelled;
  RemoveTask(task);
  RemoveDependencies(task);
  return true;
}

std::optional<VkResult> MultithreadedObjectCreationService::Complete(GITSKey key) {
  std::unique_lock<std::mutex> lock(m_Mutex);

  auto it = m_Tasks.find(key);
  if (it == m_Tasks.end()) {
    return {};
  }

  auto task = it->second;
  const bool execute = task->m_State == ObjectCreationTask::State::Queued;
  if (execute) {
    task->m_State = ObjectCreationTask::State::Running;
  }
  RemoveTask(task);
  lock.unlock();

  VkResult result;
  try {
    result = execute ? task->CreateObject() : task->m_Result.get();
  } catch (...) {
    lock.lock();
    RemoveDependencies(task);
    throw;
  }
  lock.lock();
  RemoveDependencies(task);
  return result;
}

void MultithreadedObjectCreationService::WaitForConsumers(GITSKey key) {
  while (key) {
    GITSKey outputKey;
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      auto it = m_Consumers.find(key);
      if (it == m_Consumers.end()) {
        return;
      }
      const auto& outputs = it->second.front()->m_OutputKeys;
      outputKey =
          *std::find_if(outputs.begin(), outputs.end(), [](GITSKey output) { return output != 0; });
      GITS_ASSERT(m_Tasks.contains(outputKey));
    }
    Complete(outputKey);
  }
}

void MultithreadedObjectCreationService::RemoveDependencies(
    const std::shared_ptr<ObjectCreationTask>& task) {
  for (auto key : task->m_InputKeys) {
    auto it = m_Consumers.find(key);
    if (it != m_Consumers.end()) {
      std::erase(it->second, task);
      if (it->second.empty()) {
        m_Consumers.erase(it);
      }
    }
  }
}

void MultithreadedObjectCreationService::RemoveTask(
    const std::shared_ptr<ObjectCreationTask>& task) {
  std::erase(m_TasksQueue, task);
  for (auto key : task->m_OutputKeys) {
    auto it = m_Tasks.find(key);
    if (it != m_Tasks.end() && it->second == task) {
      m_Tasks.erase(it);
    }
  }
}

void MultithreadedObjectCreationService::WorkerThread() {
  while (true) {
    std::shared_ptr<ObjectCreationTask> task;
    std::promise<VkResult> promise;

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_ConditionVariable.wait(lock, [this] { return !m_TasksQueue.empty() || m_Finish; });
      if (m_Finish && m_TasksQueue.empty()) {
        return;
      }

      auto result = promise.get_future();
      task = m_TasksQueue.front();
      m_TasksQueue.pop_front();

      task->m_Result = std::move(result);
      GITS_ASSERT(task->m_State == ObjectCreationTask::State::Queued);
      task->m_State = ObjectCreationTask::State::Running;
    }

    try {
      promise.set_value(task->CreateObject());
    } catch (...) {
      promise.set_exception(std::current_exception());
    }
  }
}

} // namespace vulkan
} // namespace gits
