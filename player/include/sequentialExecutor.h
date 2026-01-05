// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   sequentialExecutor.h
* 
* @brief GITS tokens sequential executor.
* 
*/

#pragma once

#include "runner.h"

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace gits {

// Sequentially executes actions on threads and manage them
class CSequentialExecutor : public CAction {
  typedef std::vector<int> CThreadsIdList;
  class CThreadLoop;

  std::mutex _mutex;
  std::condition_variable _condition;
  std::unique_lock<std::mutex> _actionLock;
  std::vector<std::thread> _executionThreads;
  CThreadsIdList _activeThreadsIdList;

  // variables shared between threads
  int _syncThreadId;
  CToken* _token;

  // Dispatches actions to threads
  void Dispatch(CToken& token, int thread);

public:
  CSequentialExecutor() : _syncThreadId(0), _token(0) {}
  ~CSequentialExecutor();
  CSequentialExecutor(const CSequentialExecutor& other) = delete;
  CSequentialExecutor& operator=(const CSequentialExecutor& other) = delete;
  void Run(CToken& token) override;
  const CThreadsIdList& ActiveThreadsIdList() const {
    return _activeThreadsIdList;
  }
};

} // namespace gits
