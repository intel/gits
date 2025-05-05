// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
class CCodeOStream;

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
#ifdef GITS_PLATFORM_WINDOWS
  uint64_t _chunkSize;
  uint64_t _currentChunkSize;
#endif

  CTokenList::iterator _nextToPlay;
  unsigned _tokenLimit;
  uint64_t _checkpointSize;
  bool _streamExhausted;

  CBinOStream* _oBinStream;
  CCodeOStream* _oCodeStream;
  CBinIStream* _iBinStream;

  std::mutex _tokenRegisterMutex;

  bool LoadChunk();
  CToken* Token();

public:
#if defined GITS_PLATFORM_WINDOWS
  CScheduler(unsigned tokenLimit,
             unsigned tokenBurstNum = 5,
             uint64_t tokenBurstChunkSize = 5242880);
#else
  CScheduler(unsigned tokenLimit, unsigned tokenBurstNum = 5);
#endif
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
  void Stream(CCodeOStream* stream) {
    _oCodeStream = stream;
  }

  // Last chunk needs to be written by the owner of scheduler.
  void WriteChunk(bool purgeTokens = true);
  void WriteAll();
};

} // namespace gits
