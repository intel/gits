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
#include <string>
#include <functional>
#include <thread>
#include <unordered_map>
#include <memory>

namespace gits {
namespace DirectX {

class MultithreadedShaderCompilationService {
public:
  MultithreadedShaderCompilationService() = default;
  ~MultithreadedShaderCompilationService();
  MultithreadedShaderCompilationService(const MultithreadedShaderCompilationService&) = delete;
  MultithreadedShaderCompilationService& operator=(const MultithreadedShaderCompilationService&) =
      delete;

  struct CompilationOutput {
    HRESULT result{};
    void* object{};
  };
  using CreationFunction = std::function<CompilationOutput()>;

  void Shutdown();
  void Schedule(CreationFunction creationFunction,
                ObjectKey objectKey,
                std::vector<IUnknown*> keepAlive);
  std::optional<CompilationOutput> Complete(ObjectKey objectKey);
  std::vector<std::pair<ObjectKey, CompilationOutput>> CompleteAll();
  bool ScheduleUpdateRefCount(ObjectKey objectKey, int count);
  bool ScheduleSetName(ObjectKey objectKey, std::wstring name);
  bool CancelPendingTask(ObjectKey objectKey);

private:
  struct CompilationTask {
    CompilationTask(CreationFunction creationFunction,
                    ObjectKey objectKey,
                    std::vector<IUnknown*> keepAlive);
    ~CompilationTask();
    CreationFunction CreationFunctor;
    ObjectKey Key{};
    std::vector<IUnknown*> KeepAlive;
    std::future<CreationFunction::result_type> StartedTask;
    int PendingRefCount{};
    std::optional<std::wstring> PendingName;
    void ReleaseKeepAlive();
  };

  CompilationOutput CreateObject(CompilationTask* task);
  static void ApplyReferenceCount(int refCount, void* object);
  void Initialize();
  void WorkerThread();

  bool m_Initialized = false;
  std::vector<std::thread> m_Workers;
  std::unordered_map<ObjectKey, std::unique_ptr<CompilationTask>> m_Tasks;
  std::deque<CompilationTask*> m_TasksQueue;
  std::mutex m_Mutex;
  std::condition_variable m_Cv;
  bool m_Done{};
};

} // namespace DirectX
} // namespace gits
