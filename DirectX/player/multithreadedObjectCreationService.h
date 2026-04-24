// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  void Schedule(CreationFunction creationFunction, unsigned objectKey);
  void AddDependency(unsigned providerKey, unsigned consumerKey);
  std::vector<unsigned> CollectConsumers(unsigned providerKey);
  std::optional<ObjectCreationOutput> Complete(unsigned objectKey);
  std::vector<std::pair<unsigned, ObjectCreationOutput>> CompleteAll();
  bool ScheduleUpdateRefCount(unsigned objectKey, int count);

private:
  struct ObjectCreationTask {
    ObjectCreationTask(CreationFunction creationFunction, unsigned objectKey);
    CreationFunction CreationFunctor;
    unsigned ObjectKey{};
    std::future<CreationFunction::result_type> StartedTask;
  };

  ObjectCreationOutput CreateObject(ObjectCreationTask* task);
  void Initialize();
  void WorkerThread();

  bool m_Initialized = false;
  std::unordered_map<unsigned, std::vector<unsigned>> m_Dependencies;
  std::vector<std::thread> m_Workers;
  std::unordered_map<unsigned, std::unique_ptr<ObjectCreationTask>> m_Tasks;
  std::unordered_map<unsigned, int> m_RefCounts;
  std::deque<ObjectCreationTask*> m_TasksQueue;
  std::mutex m_Mutex;
  std::condition_variable m_Cv;
  bool m_Done{};
};

} // namespace DirectX
} // namespace gits
