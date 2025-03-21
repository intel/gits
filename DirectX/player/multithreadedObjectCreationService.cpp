// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "multithreadedObjectCreationService.h"
#include "log.h"
#include "gits.h"

#include <processthreadsapi.h> // Used for SetThreadDescription

namespace gits {
namespace DirectX {

void MultithreadedObjectCreationService::initialize() {
  if (initialized_) {
    return;
  }

  // Create worker threads
  unsigned threadCount = std::thread::hardware_concurrency();
  for (decltype(threadCount) i = 0; i < threadCount; ++i) {
    workers_.emplace_back(&MultithreadedObjectCreationService::workerThread, this);

    // Set thread description (for ease of debugging)
    auto description = L"object-creation-worker-" + std::to_wstring(i);
    auto handle = workers_.back().native_handle();
    auto hr = SetThreadDescription(handle, description.c_str());
    GITS_ASSERT(SUCCEEDED(hr), "MultithreadedObjectCreationService - SetThreadDescription failed!");
  }

  initialized_ = true;
}

MultithreadedObjectCreationService::~MultithreadedObjectCreationService() {
  shutdown();
}

void MultithreadedObjectCreationService::shutdown() {
  done_ = true;
  cv_.notify_all();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void MultithreadedObjectCreationService::schedule(CreationFunction creationFunction,
                                                  unsigned objectKey) {
  initialize();

  std::lock_guard<std::mutex> guard(mutex_);
  auto [it, inserted] =
      tasks_.insert({objectKey, std::make_unique<ObjectCreationTask>(creationFunction, objectKey)});
  if (inserted) {
    tasksQueue_.push_back(it->second.get());
    cv_.notify_one();
  } else {
    static bool logged = false;
    if (!logged) {
      Log(ERR) << "MultithreadedObjectCreationService - schedule failed, creation function for "
                  "object key: "
               << objectKey << " already exists!";
      logged = true;
    }
  }
}

void MultithreadedObjectCreationService::addDependency(unsigned providerKey, unsigned consumerKey) {
  if (!providerKey || !consumerKey) {
    return;
  }

  auto [it, inserted] = dependencies_.insert({providerKey, {}});
  it->second.push_back(consumerKey);
}

std::vector<unsigned> MultithreadedObjectCreationService::collectConsumers(unsigned providerKey) {
  auto it = dependencies_.find(providerKey);
  if (it == dependencies_.end()) {
    return {};
  }
  auto consumers = std::move(it->second);
  dependencies_.erase(it);

  return consumers;
}

// Ensure the object has been created (either returns the collected result from a worker thread or creates the object)
std::optional<MultithreadedObjectCreationService::ObjectCreationOutput>
MultithreadedObjectCreationService::complete(unsigned objectKey) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto it = tasks_.find(objectKey);
  if (it == tasks_.end()) {
    return {};
  }

  std::unique_ptr<ObjectCreationTask> task = std::move(it->second);
  tasks_.erase(it);

  if (task->startedTask_.valid()) {
    lock.unlock();
    return task->startedTask_.get();
  } else {
    tasksQueue_.erase(std::remove(tasksQueue_.begin(), tasksQueue_.end(), task.get()),
                      tasksQueue_.end());
    lock.unlock();
    return createObject(task.get());
  }
}

bool MultithreadedObjectCreationService::scheduleUpdateRefCount(unsigned objectKey, int count) {
  std::lock_guard<std::mutex> guard(mutex_);

  auto it = tasks_.find(objectKey);
  if (it == tasks_.end()) {
    return false;
  }

  // Task is not pending
  if (it->second.get()->startedTask_.valid()) {
    return false;
  }

  // Update the reference count for pending objects
  refCounts_[objectKey] += count;
  return true;
}

void MultithreadedObjectCreationService::workerThread() {
  while (true) {
    ObjectCreationTask* task;
    std::promise<CreationFunction::result_type> promise;

    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !tasksQueue_.empty() || done_; });
      if (done_) {
        return;
      }

      task = tasksQueue_.front();
      tasksQueue_.pop_front();

      task->startedTask_ = promise.get_future();
    }

    promise.set_value(createObject(task));
  }
}

MultithreadedObjectCreationService::ObjectCreationOutput MultithreadedObjectCreationService::
    createObject(ObjectCreationTask* task) {
  // Create object
  ObjectCreationOutput r = task->creationFunction_();
  if (r.result != S_OK) {
    return r;
  }

  // Get the reference count and remove it from the map
  int refCount = 0;
  {
    std::lock_guard<std::mutex> guard(mutex_);
    refCount = refCounts_[task->objectKey_];
    refCounts_.erase(task->objectKey_);
  }

  // Set the reference count on newly created object
  if (refCount != 0) {
    IUnknown* object = static_cast<IUnknown*>(r.object);
    if (refCount < 0) {
      GITS_ASSERT(refCount == -1);
      object->Release();
    }
    for (int i = 0; i < refCount; ++i) {
      object->AddRef();
    }
  }

  return r;
}

MultithreadedObjectCreationService::ObjectCreationTask::ObjectCreationTask(
    CreationFunction creationFunction, unsigned objectKey)
    : creationFunction_{std::move(creationFunction)}, objectKey_{objectKey} {}

} // namespace DirectX
} // namespace gits
