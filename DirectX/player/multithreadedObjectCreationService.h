// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "tools_lite.h"

#include <future>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace gits {
namespace DirectX {

class MultithreadedObjectCreationService : public gits::noncopyable {
public:
  struct ObjectCreationOutput {
    HRESULT result{};
    void* object{};
  };
  using CreationFunction = std::function<ObjectCreationOutput()>;

  ~MultithreadedObjectCreationService();

  void shutdown();
  void schedule(CreationFunction creationFunction, unsigned objectKey);
  void addDependency(unsigned providerKey, unsigned consumerKey);
  std::vector<unsigned> collectConsumers(unsigned providerKey);
  std::optional<ObjectCreationOutput> complete(unsigned objectKey);
  bool scheduleUpdateRefCount(unsigned objectKey, int count);

private:
  struct ObjectCreationTask {
    ObjectCreationTask(CreationFunction creationFunction, unsigned objectKey);
    CreationFunction creationFunction_;
    unsigned objectKey_;
    std::future<CreationFunction::result_type> startedTask_;
  };

  ObjectCreationOutput createObject(ObjectCreationTask* task);
  void initialize();
  void workerThread();

  bool initialized_ = false;
  std::unordered_map<unsigned, std::vector<unsigned>> dependencies_;
  std::vector<std::thread> workers_;
  std::unordered_map<unsigned, std::unique_ptr<ObjectCreationTask>> tasks_;
  std::unordered_map<unsigned, int> refCounts_;
  std::deque<ObjectCreationTask*> tasksQueue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool done_{};
};

} // namespace DirectX
} // namespace gits
