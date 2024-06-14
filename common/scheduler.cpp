// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   scheduler.cpp
 *
 * @brief Definition of function calls scheduler.
 *
 */
#include "scheduler.h"
#include "gits.h"
#include "library.h"
#include "function.h"
#include "streams.h"
#include "buffer.h"
#include "exception.h"
#include "log.h"
#include "config.h"
#include "pragmas.h"

#include <iostream>
#include <filesystem>

//#define GITS_DEBUG_TOKEN_SIZE
#ifdef GITS_DEBUG_TOKEN_SIZE
#include <map>
std::map<const char*, uint64_t> token_size;
#endif

namespace gits {

void zone_allocator_next_zone();
void zone_allocator_reinitialize(size_t zones, size_t size);

class CStreamLoader {
  CScheduler& _sched;
  unsigned _ignore_mask;
  CStreamLoader operator=(const CStreamLoader&) = delete;

public:
  CStreamLoader(CScheduler& sched, unsigned ignore_mask = 0)
      : _sched(sched), _ignore_mask(ignore_mask) {}

  void operator()(ProducerConsumer<CScheduler::CTokenList>& queue) {
    CScheduler::CTokenList tokenList;
    // These values are guessed, and should probably be configurable.
    // The aim is to always have token burst fitting inside one zone.
    // Bounded number of token burst alive at any one time is needed too.
    // The value is also guessed as it can vary depending on how various
    // stages retire them.
    zone_allocator_reinitialize(Config::Get().common.player.tokenBurstNum + 2,
                                Config::Get().common.player.tokenBurst * 64);

    try {
      unsigned totalLoaded = 0;
      unsigned sinceLastChk = 0;

      // Token creating function.
      auto tokenCtor = [](CId id) -> CToken* {
        CToken* token = CGits::Instance().TokenCreate(id);
        if (Config::Get().common.player.logLoadedTokens) {
          Log(INFO) << "Loading " << TryToDemangle(typeid(*token).name()).name << "...";
        }
        return token;
      };

      const auto stream = _sched._iBinStream;
      const auto tokenBurstLimit = _sched._tokenLimit;
      const auto checkpointSize = _sched._checkpointSize;
      const auto tokenLoadLimit = Config::Get().common.player.tokenLoadLimit;

      for (;;) {
        unsigned loaded = 0;
        bool everything_loaded = false;
        uint64_t maxLoaded = std::min((uint64_t)std::numeric_limits<decltype(loaded)>::max(),
                                      (uint64_t)tokenList.max_size());
        while ((loaded < tokenBurstLimit ||
                Config::Get().common.player.loadWholeStreamBeforePlayback) &&
               !everything_loaded) {
#ifdef GITS_DEBUG_TOKEN_SIZE
          uint64_t tokBegin = stream->tellg();
#endif
          CToken* token = CToken::Deserialize(*stream, tokenCtor);
          if (token == nullptr) {
            everything_loaded = true;
            break;
          }

#ifdef GITS_DEBUG_TOKEN_SIZE
          uint64_t tokEnd = stream->tellg();
          token_size[typeid(*token).name()] += tokEnd - tokBegin + 2;
#endif
          // this token is to be ignored, remove it and carry on with rest
          auto type = token->Type();
          if ((type & _ignore_mask) != 0) {
            delete token;
            continue;
          }
          if (loaded >= maxLoaded) {
            Log(ERR) << "Max token loaded limit in one burst exceeded.";
            throw std::runtime_error(EXCEPTION_MESSAGE);
          }

          tokenList.push_back(token);

          loaded++;
          totalLoaded++;
          sinceLastChk++;

          if (sinceLastChk == checkpointSize) {
            Log(INFO, RAW) << "#" << std::flush;
            sinceLastChk = 0;
          }

          if (tokenLoadLimit == totalLoaded) {
            everything_loaded = true;
          }
        }
        zone_allocator_next_zone();

        // Finish if the queue won't accept more products.
        if (!queue.produce(tokenList)) {
          break;
        }

        if (everything_loaded) {
          queue.break_pipe();
          break;
        }
      }
    } catch (std::exception& e) {
      Log(ERR) << "Loader reading thread failed: Exiting!!!:" << e.what();
      fast_exit(1);
    }

    // If we were forced to finish we may still own few tokens
    Purge(tokenList);
#ifdef GITS_DEBUG_TOKEN_SIZE
    for (auto& one_size : token_size) {
      Log(INFO, NO_PREFIX) << one_size.first << ": " << one_size.second;
    }
#endif
  }
};

class CStreamWriter {
  CScheduler& _sched;
  CStreamWriter& operator=(const CStreamWriter&) = delete;

public:
  CStreamWriter(CScheduler& sched) : _sched(sched) {}

  static void consume_tokens(CScheduler::CTokenList& tokenList,
                             CScheduler& sched,
                             bool purgeTokens = true) {
    // Write to code stream.
    if (sched._oCodeStream) {
      for (auto token : tokenList) {
        if (!token->IsSerialized()) {
          token->Serialize(*sched._oCodeStream);
        }
      }
    }

    // Write to binary stream.
    if (sched._oBinStream) {
      for (auto token : tokenList) {
        if (!token->IsSerialized()) {
          token->Serialize(*sched._oBinStream);
        }
      }
    }

    // Remove tokens from scheduler.
    if (purgeTokens) {
      Purge(tokenList);
    }
  }

  void operator()(ProducerConsumer<CScheduler::CTokenList>& sync) {
    try {
      CScheduler::CTokenList tokenList;

      while (sync.consume(tokenList)) {
        consume_tokens(tokenList, _sched);
      }
    } catch (std::exception& e) {
      Log(ERR) << "Error in writer thread: " << e.what();
      fast_exit(1);
    } catch (...) {
      Log(ERR) << "Unknown error in writer thread";
      fast_exit(1);
    }
    Log(INFO) << "Stream writer thread finished.";
  }
};

//CScheduler class definition
CScheduler::CScheduler(unsigned tokenLimit, unsigned tokenBurstNum)
    : _streamLoader(tokenBurstNum),
      _streamWriter(tokenBurstNum),
      _tokenShredder(tokenBurstNum),
      _nextToPlay(_tokenList.begin()),
      _tokenLimit(tokenLimit),
      _checkpointSize(10000),
      _streamExhausted(false),
      _oBinStream(nullptr),
      _oCodeStream(nullptr),
      _iBinStream(nullptr) {
  CGits::Instance().Timers().loading.Pause();
}

CScheduler::~CScheduler() {
  try {
    // Delete any tokens owned by scheduler.
    Purge(_tokenList);

    // Wait for the shredder to dispose of any outstanding tokens.
    _tokenShredder.finish();

    // Scheduler is responsible for playing back all the tokens.
    // Thus it owns any abandoned tokens that are stuck in the queue
    // of stream loader task due to early exit of player.
    // Can be done more elegantly if payload is pushed in unique_ptr's.
    _streamLoader.finish();
    for (;;) {
      CTokenList leftovers;
      bool done = !_streamLoader.queue().consume(leftovers);
      Purge(leftovers);
      if (done) {
        break;
      }
    }

    _streamWriter.finish();
  } catch (...) {
    topmost_exception_handler("CScheduler::~CScheduler");
  }
}

/**
 * @brief Registers new function call wrapper
 *
 * Method registers new function call wrapper.
 *
 * @note Ownership is passed to recorder class.
 *
 * @note Scheduler takes ownership of the token
 *
 * @param function Function call wrapper to register.
 */
void CScheduler::Register(CToken* token) {
  if (Config::Get().common.mode == Config::MODE_RECORDER) {
    if (_tokenList.size() > _tokenLimit) {
      WriteChunk();
    }
  }

  // This mutex is necessary, because each recorded api (GL/CL/rs)
  // has its own api-mutex and thus don't serialize access to scheduler
  // for multi api applications. Proper solution would require sharing
  // single api-mutex among all recorded apis interceptors.
  std::unique_lock<std::mutex> lock(_tokenRegisterMutex);
  _tokenList.push_back(token);
  if (Config::Get().common.recorder.highIntegrity) {
    WriteChunk(false);
  }
}

bool CScheduler::LoadChunk() {
  if (!_streamLoader.running()) {
    _streamLoader.start(CStreamLoader(*this));
  }

  // We are timing all the stalls here.
  CGits::Instance().Timers().loading.Resume();

  //create token shredder thread here
  if (!_tokenShredder.running()) {
    _tokenShredder.start([](ProducerConsumer<CScheduler::CTokenList>& queue) {
      CScheduler::CTokenList list;
      while (queue.consume(list)) {
        Purge(list);
      }
    });
  }
  _tokenShredder.queue().produce(_tokenList);
  bool produced = _streamLoader.queue().consume(_tokenList);
  _nextToPlay = _tokenList.begin();

  // Skip first interval measured as the stream is not yet
  // playing (issuing API calls), so this time does not affect
  // playback performance of the API.
  CALL_ONCE[&] {
    CGits::Instance().Timers().loading.Restart();
  };
  CGits::Instance().Timers().loading.Pause();

  return !produced;
}

void CScheduler::WriteChunk(bool purgeTokens) {
  if (Config::Get().common.recorder.highIntegrity) {
    CStreamWriter::consume_tokens(_tokenList, *this, purgeTokens);
  } else {
    if (!_streamWriter.running()) {
      _streamWriter.start(CStreamWriter(*this));
    }

    _streamWriter.queue().produce(_tokenList);
  }
}

void CScheduler::WriteAll() {
  WriteChunk();
}

void CScheduler::Stream(CBinIStream* stream) {
  // Empirically calculated from 'random' stream constant value.
  const uint32_t averageTokenSize = 13;
  const uint64_t estimatedTokenNum = std::filesystem::file_size(stream->Path()) / averageTokenSize;

  _checkpointSize = estimatedTokenNum / 30;
  _checkpointSize = (_checkpointSize == 0) ? 1 : _checkpointSize;

  std::string name = stream->Path().filename().string();
  if (name.size() > 3 && name.substr(name.size() - 3) == ".gz") {
    _checkpointSize /= 8;
  }

  Log(INFO) << "Will output checkpoint character '#' every " << _checkpointSize << " loaded tokens";

  _iBinStream = stream;
}

CToken* CScheduler::Token() {
  if (_nextToPlay != _tokenList.end()) {
    CToken* result = *_nextToPlay;
    ++_nextToPlay;
    return result;
  }

  if (!_streamExhausted) {
    _streamExhausted = LoadChunk();
    return Token();
  }

  return nullptr;
}
bool CScheduler::Run(CAction& action) {
  auto& runner = CGits::Instance().Runner();

  while (auto result = Token()) {
    // run token
    runner(action, *result);
    // Give control to GITS framework on frame begin.
    const unsigned id = result->Id();
    if (id == CToken::ID_FRAME_END || id == CToken::ID_INIT_END) {
      return false;
    }

    if (CGits::Instance().Finished()) {
      return true;
    }
  }
  return true;
}
} // namespace gits
