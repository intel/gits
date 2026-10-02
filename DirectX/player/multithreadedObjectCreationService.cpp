// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectCreationService.h"
#include "arguments.h"
#include "log.h"

#include <processthreadsapi.h> // Used for SetThreadDescription

namespace gits {
namespace DirectX {

void MultithreadedObjectCreationService::Initialize() {
  if (m_Initialized) {
    return;
  }

  // Create worker threads
  unsigned threadCount = std::thread::hardware_concurrency();
  for (decltype(threadCount) i = 0; i < threadCount; ++i) {
    m_Workers.emplace_back(&MultithreadedObjectCreationService::WorkerThread, this);

    // Set thread description (for ease of debugging)
    auto description = L"object-creation-worker-" + std::to_wstring(i);
    auto handle = m_Workers.back().native_handle();
    auto hr = SetThreadDescription(handle, description.c_str());
    GITS_ASSERT(SUCCEEDED(hr), "MultithreadedObjectCreationService - SetThreadDescription failed!");
  }

  m_Initialized = true;
}

MultithreadedObjectCreationService::~MultithreadedObjectCreationService() {
  Shutdown();
}

void MultithreadedObjectCreationService::Shutdown() {
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Done = true;
  }
  m_Cv.notify_all();
  for (auto& worker : m_Workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void MultithreadedObjectCreationService::Schedule(CreationFunction creationFunction,
                                                  ObjectKey objectKey) {
  Initialize();

  std::lock_guard<std::mutex> guard(m_Mutex);
  auto [it, inserted] = m_Tasks.insert(
      {objectKey, std::make_unique<ObjectCreationTask>(creationFunction, objectKey)});
  if (inserted) {
    m_TasksQueue.push_back(it->second.get());
    m_Cv.notify_one();
  } else {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "MultithreadedObjectCreationService - schedule failed, creation function for "
                   "object key: "
                << objectKey << " already exists!";
      logged = true;
    }
  }
}

void MultithreadedObjectCreationService::AddDependency(ObjectKey providerKey,
                                                       ObjectKey consumerKey) {
  if (!providerKey || !consumerKey) {
    return;
  }

  auto [it, inserted] = m_Dependencies.insert({providerKey, {}});
  it->second.push_back(consumerKey);
}

std::vector<ObjectKey> MultithreadedObjectCreationService::CollectConsumers(ObjectKey providerKey) {
  auto it = m_Dependencies.find(providerKey);
  if (it == m_Dependencies.end()) {
    return {};
  }
  auto consumers = std::move(it->second);
  m_Dependencies.erase(it);

  return consumers;
}

// Ensure the object has been created (either returns the collected result from a worker thread or creates the object)
std::optional<MultithreadedObjectCreationService::ObjectCreationOutput>
MultithreadedObjectCreationService::Complete(ObjectKey objectKey) {
  std::unique_lock<std::mutex> lock(m_Mutex);

  auto it = m_Tasks.find(objectKey);
  if (it == m_Tasks.end()) {
    return {};
  }

  std::unique_ptr<ObjectCreationTask> task = std::move(it->second);
  m_Tasks.erase(it);

  if (task->StartedTask.valid()) {
    lock.unlock();
    ObjectCreationOutput output = task->StartedTask.get();
    ApplyPendingReferenceCount(objectKey, output.object);
    return output;
  } else {
    m_TasksQueue.erase(std::remove(m_TasksQueue.begin(), m_TasksQueue.end(), task.get()),
                       m_TasksQueue.end());
    lock.unlock();
    return CreateObject(task.get());
  }
}

std::vector<std::pair<ObjectKey, MultithreadedObjectCreationService::ObjectCreationOutput>>
MultithreadedObjectCreationService::CompleteAll() {
  std::vector<std::pair<ObjectKey, ObjectCreationOutput>> results;
  while (!m_Tasks.empty()) {
    ObjectKey key = m_Tasks.begin()->first;
    auto creationOutput = Complete(key);
    GITS_ASSERT(creationOutput.has_value());
    results.emplace_back(key, creationOutput.value());
  }
  return results;
}

bool MultithreadedObjectCreationService::ScheduleUpdateRefCount(ObjectKey objectKey, int count) {
  std::lock_guard<std::mutex> guard(m_Mutex);

  auto it = m_Tasks.find(objectKey);
  if (it == m_Tasks.end()) {
    return false;
  }

  m_RefCounts[objectKey] += count;
  return true;
}

bool MultithreadedObjectCreationService::CancelPendingTask(ObjectKey objectKey) {
  std::lock_guard<std::mutex> guard(m_Mutex);

  auto it = m_Tasks.find(objectKey);
  if (it == m_Tasks.end()) {
    return false;
  }

  if (it->second.get()->StartedTask.valid()) {
    return false;
  }

  auto refIt = m_RefCounts.find(objectKey);
  if (refIt != m_RefCounts.end() && refIt->second != 0) {
    return false;
  }

  ObjectCreationTask* task = it->second.get();
  m_TasksQueue.erase(std::remove(m_TasksQueue.begin(), m_TasksQueue.end(), task),
                     m_TasksQueue.end());
  m_Tasks.erase(it);
  m_RefCounts.erase(objectKey);
  return true;
}

void MultithreadedObjectCreationService::WorkerThread() {
  while (true) {
    ObjectCreationTask* task;
    std::promise<CreationFunction::result_type> promise;

    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_Cv.wait(lock, [this] { return !m_TasksQueue.empty() || m_Done; });
      if (m_Done) {
        return;
      }

      task = m_TasksQueue.front();
      m_TasksQueue.pop_front();

      task->StartedTask = promise.get_future();
    }

    promise.set_value(CreateObject(task));
  }
}

void MultithreadedObjectCreationService::ApplyPendingReferenceCount(ObjectKey objectKey,
                                                                    void* object) {
  int refCount = 0;
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    auto it = m_RefCounts.find(objectKey);
    if (it == m_RefCounts.end()) {
      return;
    }
    refCount = it->second;
    m_RefCounts.erase(it);
  }

  if (refCount == 0 || !object) {
    return;
  }

  IUnknown* unknown = static_cast<IUnknown*>(object);
  if (refCount < 0) {
    for (int i = 0; i < -refCount; ++i) {
      unknown->Release();
    }
  } else {
    for (int i = 0; i < refCount; ++i) {
      unknown->AddRef();
    }
  }
}

MultithreadedObjectCreationService::ObjectCreationOutput MultithreadedObjectCreationService::
    CreateObject(ObjectCreationTask* task) {
  ObjectCreationOutput r = task->CreationFunctor();
  if (r.result != S_OK) {
    return r;
  }

  ApplyPendingReferenceCount(task->Key, r.object);
  return r;
}

MultithreadedObjectCreationService::ObjectCreationTask::ObjectCreationTask(
    CreationFunction creationFunction, ObjectKey objectKey)
    : CreationFunctor(std::move(creationFunction)), Key(objectKey) {}

} // namespace DirectX
} // namespace gits
