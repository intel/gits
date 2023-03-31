// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

namespace gits {

// Sequentially executes actions on threads and manage them
class CSequentialExecutor : public CAction {
  typedef std::vector<int> CThreadsIdList;
  class CThreadLoop;

  boost::mutex _mutex;
  boost::condition_variable _condition;
  boost::unique_lock<boost::mutex> _actionLock;
  boost::thread_group _executionThreads;
  CThreadsIdList _activeThreadsIdList;

  // variables shared between threads
  int _syncThreadId;
  CToken* _token;

  // Dispatches actions to threads
  void Dispatch(CToken& token, int thread);

public:
  CSequentialExecutor() : _syncThreadId(0), _token(0) {}
  ~CSequentialExecutor();
  void Run(CToken& token) override;
  const CThreadsIdList& ActiveThreadsIdList() const {
    return _activeThreadsIdList;
  }
};

} // namespace gits
