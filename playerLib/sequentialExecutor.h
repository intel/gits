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

  std::mutex m_Mutex;
  std::condition_variable m_Condition;
  std::vector<std::thread> m_ExecutionThreads;
  CThreadsIdList m_ActiveThreadsIdList;

  // variables shared between threads
  int m_SyncThreadId;
  CToken* m_Token;

  // Dispatches actions to threads
  void Dispatch(CToken& token, int thread);

public:
  CSequentialExecutor() : m_SyncThreadId(0), m_Token(0) {}
  ~CSequentialExecutor();
  CSequentialExecutor(const CSequentialExecutor& other) = delete;
  CSequentialExecutor& operator=(const CSequentialExecutor& other) = delete;
  void Run(CToken& token) override;
  const CThreadsIdList& ActiveThreadsIdList() const {
    return m_ActiveThreadsIdList;
  }
};

} // namespace gits
