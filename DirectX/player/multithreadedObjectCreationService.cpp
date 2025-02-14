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

namespace gits {
namespace DirectX {

namespace {

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName) {
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#pragma warning(pop)
}

} // namespace

void MultithreadedObjectCreationService::initialize() {
  if (initialized_) {
    return;
  }

  unsigned threadCount = std::thread::hardware_concurrency();
  for (decltype(threadCount) i = 0; i < threadCount; ++i) {
    workers_.emplace_back(&MultithreadedObjectCreationService::workerThread, this);
    DWORD threadId = GetThreadId(static_cast<HANDLE>(workers_.back().native_handle()));
    SetThreadName(threadId, ("object-creation-worker-" + std::to_string(i)).c_str());
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
    return task->creationFunction_();
  }
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

    promise.set_value(task->creationFunction_());
  }
}

MultithreadedObjectCreationService::ObjectCreationTask::ObjectCreationTask(
    CreationFunction creationFunction, unsigned objectKey)
    : creationFunction_{std::move(creationFunction)}, objectKey_{objectKey} {}

} // namespace DirectX
} // namespace gits
