// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   scheduler.h
 *
 * @brief Declaration of function calls scheduler.
 *
 */

#pragma once

#include "tools.h"
#include "timer.h"
#include "token.h"
#include "runner.h"

namespace gits {

class CBinOStream;
class CBinIStream;

/**
   * @brief Library function calls scheduler
   *
   * gits::CScheduler class is responsible for loading, storing, running
   * and saving of captured library function calls.
   *
   */
class CScheduler : private gits::noncopyable {
public:
  typedef std::vector<CToken*> CTokenList;
  typedef std::pair<CTokenList::iterator, CTokenList::iterator> CIterPair;

private:
  friend class CStreamLoader;
  friend class CStreamWriter;

  Task<CTokenList> _streamLoader;
  Task<CTokenList> _streamWriter;
  Task<CTokenList> _tokenShredder;

  CTokenList _tokenList; /**< @brief list of registered function calls */
  uint64_t _chunkSize;
  uint64_t _currentChunkSize;

  CTokenList::iterator _nextToPlay;
  unsigned _tokenLimit;
  bool _streamExhausted;

  CBinOStream* _oBinStream;
  CBinIStream* _iBinStream;

  std::mutex _tokenRegisterMutex;

  bool LoadChunk();
  CToken* Token();

public:
  CScheduler(unsigned tokenLimit,
             unsigned tokenBurstNum = 5,
             uint64_t tokenBurstChunkSize = 5242880);
  CScheduler(const CScheduler&) = delete;
  CScheduler& operator=(const CScheduler&) = delete;
  CScheduler(CScheduler&&) = delete;
  CScheduler& operator=(CScheduler&&) = delete;
  ~CScheduler();

  void Register(CToken* token);

  bool Run(CAction& action);

  void Stream(CBinOStream* stream) {
    _oBinStream = stream;
  }
  void Stream(CBinIStream* stream);

  // Last chunk needs to be written by the owner of scheduler.
  void WriteChunk(bool purgeTokens = true);
  void WriteAll();
};

} // namespace gits
