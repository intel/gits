// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "directx.h"

#include <future>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace gits {
namespace DirectX {

class MultithreadedObjectCreationService {
public:
  MultithreadedObjectCreationService() = default;
  ~MultithreadedObjectCreationService();
  MultithreadedObjectCreationService(const MultithreadedObjectCreationService&) = delete;
  MultithreadedObjectCreationService& operator=(const MultithreadedObjectCreationService&) = delete;

  struct ObjectCreationOutput {
    HRESULT result{};
    void* object{};
  };
  using CreationFunction = std::function<ObjectCreationOutput()>;

  void Shutdown();
  void Schedule(CreationFunction creationFunction, ObjectKey objectKey);
  void AddDependency(ObjectKey providerKey, ObjectKey consumerKey);
  std::vector<ObjectKey> CollectConsumers(ObjectKey providerKey);
  std::optional<ObjectCreationOutput> Complete(ObjectKey objectKey);
  std::vector<std::pair<ObjectKey, ObjectCreationOutput>> CompleteAll();
  bool ScheduleUpdateRefCount(ObjectKey objectKey, int count);

private:
  struct ObjectCreationTask {
    ObjectCreationTask(CreationFunction creationFunction, ObjectKey objectKey);
    CreationFunction CreationFunctor;
    ObjectKey Key{};
    std::future<CreationFunction::result_type> StartedTask;
  };

  ObjectCreationOutput CreateObject(ObjectCreationTask* task);
  void Initialize();
  void WorkerThread();

  bool m_Initialized = false;
  std::unordered_map<ObjectKey, std::vector<ObjectKey>> m_Dependencies;
  std::vector<std::thread> m_Workers;
  std::unordered_map<ObjectKey, std::unique_ptr<ObjectCreationTask>> m_Tasks;
  std::unordered_map<ObjectKey, int> m_RefCounts;
  std::deque<ObjectCreationTask*> m_TasksQueue;
  std::mutex m_Mutex;
  std::condition_variable m_Cv;
  bool m_Done{};
};

} // namespace DirectX
} // namespace gits
