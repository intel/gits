// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   sequentialExecutor.cpp
*
* @brief  GITS tokens sequential executor.
*
*/

#include "sequentialExecutor.h"
#include "token.h"
#include "gits.h"

namespace gits {

// thread loop for sequential actions execution implementation
class CSequentialExecutor::CThreadLoop {
  int _threadId;
  CSequentialExecutor& _seqExec;

public:
  // Delete the copy constructor
  CThreadLoop(const CThreadLoop& other) = delete;

  // Delete the copy assignment operator
  CThreadLoop& operator=(const CThreadLoop& other) = delete;

  // Default destructor
  ~CThreadLoop() = default;

  // Constructor
  CThreadLoop(CSequentialExecutor& seqexec, int threadId)
      : _threadId(threadId), _seqExec(seqexec) {}

  void operator()() {
    try {
      for (;;) {
        std::unique_lock<std::mutex> localLock(_seqExec.m_Mutex);

        // wait for action for this thread
        while (!(_seqExec.m_SyncThreadId == _threadId && _seqExec.m_Token != nullptr)) {
          _seqExec.m_Condition.wait(localLock);
        }

        if (_seqExec.m_Token != nullptr) {
          if (!Configurator::Get().common.player.nullRun) {
            _seqExec.m_Token->Run();
          }
        }

        _seqExec.m_Token = nullptr;
        _seqExec.m_Condition.notify_all();
      }
    } catch (gits::Exception& ex) {
      LOG_ERROR << "Unhandled exception: " << ex.what() << " on thread: " << _threadId;
      std::quick_exit(EXIT_FAILURE);
    } catch (std::exception& ex) {
      LOG_ERROR << "Unhandled system exception: " << ex.what() << " on thread: " << _threadId;
      std::quick_exit(EXIT_FAILURE);
    } catch (...) {
      LOG_ERROR << "Unhandled exception caught on thread: " << _threadId;
      std::quick_exit(EXIT_FAILURE);
    }
  }
};

} // namespace gits

gits::CSequentialExecutor::~CSequentialExecutor() {
  try {
    for (auto& t : m_ExecutionThreads) {
      if (t.joinable()) {
        t.join();
      }
    }
  } catch (...) {
    topmost_exception_handler("CSequentialExecutor::~CSequentialExecutor");
  }
}

void gits::CSequentialExecutor::Dispatch(CToken& token, int thread) {
  std::unique_lock<std::mutex> localLock(m_Mutex);

  // Set target thread and action
  m_SyncThreadId = thread;
  m_Token = &token;
  m_Condition.notify_all();

  // Wait for thread to finish execution
  while (m_Token != nullptr) {
    m_Condition.wait(localLock);
  }
}

void gits::CSequentialExecutor::Run(CToken& token) {
  int threadId = CGits::Instance().CurrentThreadId();

  // main thread execution
  if (threadId == 0) {
    if (!Configurator::Get().common.player.nullRun) {
      token.Run();
    }
  } else {
    // create additional thread if needed
    if (find(begin(m_ActiveThreadsIdList), end(m_ActiveThreadsIdList), threadId) ==
        end(m_ActiveThreadsIdList)) {
      m_ExecutionThreads.emplace_back([this, threadId]() { CThreadLoop(*this, threadId)(); });
      m_ActiveThreadsIdList.push_back(threadId);
    }

    // dispatch action to thread
    Dispatch(token, threadId);
  }
}
