// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedShaderCompilationService.h"
#include "arguments.h"
#include "log.h"

#include <processthreadsapi.h> // Used for SetThreadDescription
#include <algorithm>

namespace gits {
namespace DirectX {

void MultithreadedShaderCompilationService::Initialize() {
  if (m_Initialized) {
    return;
  }

  unsigned threadCount = std::max(1u, std::thread::hardware_concurrency());
  for (decltype(threadCount) i = 0; i < threadCount; ++i) {
    m_Workers.emplace_back(&MultithreadedShaderCompilationService::WorkerThread, this);

    auto description = L"shader-compilation-worker-" + std::to_wstring(i);
    auto handle = m_Workers.back().native_handle();
    auto hr = SetThreadDescription(handle, description.c_str());
    GITS_ASSERT(SUCCEEDED(hr),
                "MultithreadedShaderCompilationService - SetThreadDescription failed!");
  }

  m_Initialized = true;
}

MultithreadedShaderCompilationService::~MultithreadedShaderCompilationService() {
  Shutdown();
}

void MultithreadedShaderCompilationService::Shutdown() {
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

  // Destroy unresolved tasks now so their keep-alive references are released before the
  // objects they refer to are torn down.
  std::lock_guard<std::mutex> guard(m_Mutex);
  m_TasksQueue.clear();
  m_Tasks.clear();
}

void MultithreadedShaderCompilationService::Schedule(CreationFunction creationFunction,
                                                     ObjectKey objectKey,
                                                     std::vector<IUnknown*> keepAlive) {
  Initialize();

  std::lock_guard<std::mutex> guard(m_Mutex);
  GITS_ASSERT(!m_Tasks.contains(objectKey),
              "MultithreadedShaderCompilationService - object already scheduled");
  auto& task = m_Tasks[objectKey] = std::make_unique<CompilationTask>(
      std::move(creationFunction), objectKey, std::move(keepAlive));
  m_TasksQueue.push_back(task.get());
  m_Cv.notify_one();
}

std::optional<MultithreadedShaderCompilationService::CompilationOutput>
MultithreadedShaderCompilationService::Complete(ObjectKey objectKey) {
  std::unique_ptr<CompilationTask> task;
  bool started = false;
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    auto it = m_Tasks.find(objectKey);
    if (it == m_Tasks.end()) {
      return {};
    }
    task = std::move(it->second);
    m_Tasks.erase(it);

    started = task->StartedTask.valid();
    if (!started) {
      m_TasksQueue.erase(std::remove(m_TasksQueue.begin(), m_TasksQueue.end(), task.get()),
                         m_TasksQueue.end());
    }
  }

  CompilationOutput output = started ? task->StartedTask.get() : CreateObject(task.get());
  if (output.result == S_OK) {
    if (task->PendingName) {
      static_cast<ID3D12Object*>(output.object)->SetName(task->PendingName->c_str());
    }
    ApplyReferenceCount(task->PendingRefCount, output.object);
  }
  return output;
}

std::vector<std::pair<ObjectKey, MultithreadedShaderCompilationService::CompilationOutput>>
MultithreadedShaderCompilationService::CompleteAll() {
  std::vector<std::pair<ObjectKey, CompilationOutput>> results;
  while (true) {
    ObjectKey key;
    {
      std::lock_guard<std::mutex> guard(m_Mutex);
      if (m_Tasks.empty()) {
        break;
      }
      key = m_Tasks.begin()->first;
    }
    auto creationOutput = Complete(key);
    GITS_ASSERT(creationOutput.has_value());
    results.emplace_back(key, creationOutput.value());
  }
  return results;
}

bool MultithreadedShaderCompilationService::ScheduleUpdateRefCount(ObjectKey objectKey, int count) {
  std::lock_guard<std::mutex> guard(m_Mutex);

  auto it = m_Tasks.find(objectKey);
  if (it == m_Tasks.end()) {
    return false;
  }

  it->second->PendingRefCount += count;
  return true;
}

bool MultithreadedShaderCompilationService::ScheduleSetName(ObjectKey objectKey,
                                                            std::wstring name) {
  std::lock_guard<std::mutex> guard(m_Mutex);

  auto it = m_Tasks.find(objectKey);
  if (it == m_Tasks.end()) {
    return false;
  }

  it->second->PendingName = std::move(name);
  return true;
}

bool MultithreadedShaderCompilationService::CancelPendingTask(ObjectKey objectKey) {
  std::lock_guard<std::mutex> guard(m_Mutex);

  auto it = m_Tasks.find(objectKey);
  if (it == m_Tasks.end()) {
    return false;
  }

  CompilationTask* task = it->second.get();
  if (task->StartedTask.valid() || task->PendingRefCount != 0) {
    return false;
  }

  m_TasksQueue.erase(std::remove(m_TasksQueue.begin(), m_TasksQueue.end(), task),
                     m_TasksQueue.end());
  m_Tasks.erase(it);
  return true;
}

void MultithreadedShaderCompilationService::WorkerThread() {
  while (true) {
    CompilationTask* task;
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

void MultithreadedShaderCompilationService::ApplyReferenceCount(int refCount, void* object) {
  if (!object) {
    return;
  }

  IUnknown* unknown = static_cast<IUnknown*>(object);
  for (; refCount > 0; --refCount) {
    unknown->AddRef();
  }
  for (; refCount < 0; ++refCount) {
    unknown->Release();
  }
}

MultithreadedShaderCompilationService::CompilationOutput MultithreadedShaderCompilationService::
    CreateObject(CompilationTask* task) {
  CompilationOutput output = task->CreationFunctor();
  task->ReleaseKeepAlive();
  return output;
}

MultithreadedShaderCompilationService::CompilationTask::CompilationTask(
    CreationFunction creationFunction, ObjectKey objectKey, std::vector<IUnknown*> keepAlive)
    : CreationFunctor(std::move(creationFunction)),
      Key(objectKey),
      KeepAlive(std::move(keepAlive)) {
  for (IUnknown* dep : KeepAlive) {
    if (dep) {
      dep->AddRef();
    }
  }
}

MultithreadedShaderCompilationService::CompilationTask::~CompilationTask() {
  ReleaseKeepAlive();
}

void MultithreadedShaderCompilationService::CompilationTask::ReleaseKeepAlive() {
  for (IUnknown* dep : KeepAlive) {
    if (dep) {
      dep->Release();
    }
  }
  KeepAlive.clear();
}

} // namespace DirectX
} // namespace gits
