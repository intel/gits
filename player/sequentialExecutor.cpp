// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
#include "config.h"
#include "token.h"
#include "gits.h"

namespace gits {

// thread loop for sequential actions execution implementation
class CSequentialExecutor::CThreadLoop {
  int _threadId;
  CSequentialExecutor& _seqExec;

public:
  CThreadLoop& operator=(const CThreadLoop& other) = delete;
  CThreadLoop(CSequentialExecutor& seqexec, int threadId)
      : _threadId(threadId), _seqExec(seqexec) {}

  void operator()() {
    try {
      for (;;) {
        std::unique_lock<std::mutex> localLock(_seqExec._mutex);

        // wait for action for this thread
        while (!(_seqExec._syncThreadId == _threadId && _seqExec._token != nullptr)) {
          _seqExec._condition.wait(localLock);
        }

        if (_seqExec._token != nullptr) {
          if (!Config::Get().common.player.nullRun) {
            _seqExec._token->Run();
          }
        }

        _seqExec._token = nullptr;
        _seqExec._condition.notify_all();
      }
    } catch (gits::Exception& ex) {
      Log(ERR) << "Unhandled exception: " << ex.what() << " on thread: " << _threadId;
      fast_exit(1);
    } catch (std::exception& ex) {
      Log(ERR) << "Unhandled system exception: " << ex.what() << " on thread: " << _threadId;
      fast_exit(1);
    } catch (...) {
      Log(ERR) << "Unhandled exception caught on thread: " << _threadId;
      fast_exit(1);
    }
  }
};

} // namespace gits

gits::CSequentialExecutor::~CSequentialExecutor() {
  try {
    for (auto& t : _executionThreads) {
      if (t.joinable()) {
        t.join();
      }
    }
  } catch (...) {
    topmost_exception_handler("CSequentialExecutor::~CSequentialExecutor");
  }
}

void gits::CSequentialExecutor::Dispatch(CToken& token, int thread) {
  std::unique_lock<std::mutex> localLock(_mutex);

  // Set target thread and action
  _syncThreadId = thread;
  _token = &token;
  _condition.notify_all();

  // Wait for thread to finish execution
  while (_token != nullptr) {
    _condition.wait(localLock);
  }
}

void gits::CSequentialExecutor::Run(CToken& token) {
  int threadId = CGits::Instance().CurrentThreadId();

  // main thread execution
  if (threadId == 0) {
    if (!Config::Get().common.player.nullRun) {
      token.Run();
    }
  } else {
    // create additional thread if needed
    if (find(begin(_activeThreadsIdList), end(_activeThreadsIdList), threadId) ==
        end(_activeThreadsIdList)) {
      _executionThreads.emplace_back(CThreadLoop(*this, threadId));
      _activeThreadsIdList.push_back(threadId);
    }

    // dispatch action to thread
    Dispatch(token, threadId);
  }
}
