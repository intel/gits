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
  void Schedule(CreationFunction creationFunction, GITSKey objectKey);
  void AddDependency(GITSKey providerKey, GITSKey consumerKey);
  std::vector<GITSKey> CollectConsumers(GITSKey providerKey);
  std::optional<ObjectCreationOutput> Complete(GITSKey objectKey);
  std::vector<std::pair<GITSKey, ObjectCreationOutput>> CompleteAll();
  bool ScheduleUpdateRefCount(GITSKey objectKey, int count);

private:
  struct ObjectCreationTask {
    ObjectCreationTask(CreationFunction creationFunction, GITSKey objectKey);
    CreationFunction CreationFunctor;
    GITSKey Key{};
    std::future<CreationFunction::result_type> StartedTask;
  };

  ObjectCreationOutput CreateObject(ObjectCreationTask* task);
  void Initialize();
  void WorkerThread();

  bool m_Initialized = false;
  std::unordered_map<GITSKey, std::vector<GITSKey>> m_Dependencies;
  std::vector<std::thread> m_Workers;
  std::unordered_map<GITSKey, std::unique_ptr<ObjectCreationTask>> m_Tasks;
  std::unordered_map<GITSKey, int> m_RefCounts;
  std::deque<ObjectCreationTask*> m_TasksQueue;
  std::mutex m_Mutex;
  std::condition_variable m_Cv;
  bool m_Done{};
};

} // namespace DirectX
} // namespace gits
