// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "log2.h"
#include "pragmas.h"

#include <iostream>
#include <filesystem>

//#define GITS_DEBUG_TOKEN_SIZE
#ifdef GITS_DEBUG_TOKEN_SIZE
#include <map>
std::map<const char*, uint64_t> token_size;
#endif

namespace gits {

class CStreamLoader {
  CScheduler& _sched;

public:
  // User-defined copy assignment operator.
  CStreamLoader& operator=(const CStreamLoader&) = delete;

  // User-defined copy constructor.
  CStreamLoader(const CStreamLoader& other) : _sched(other._sched) {}

  // User-defined destructor.
  ~CStreamLoader() = default;
  CStreamLoader(CScheduler& sched) : _sched(sched) {}

  void operator()(ProducerConsumer<CScheduler::CTokenList>& queue) {
    CScheduler::CTokenList tokenList;
#if defined GITS_PLATFORM_WINDOWS
    static bool isDirectX =
        (CGits::Instance().GetApi3D() == ApisIface::TApi::DirectX); // Static variable for the check
#endif

    try {
      unsigned sinceLastChk = 0;

      // Token creating function.
      auto tokenCtor = [](CId id) -> CToken* {
        CToken* token = CGits::Instance().TokenCreate(id);
        if (Configurator::Get().common.player.logLoadedTokens) {
          LOG_INFO << "Loading " << TryToDemangle(typeid(*token).name()).name << "...";
        }
        return token;
      };

      const auto stream = _sched._iBinStream;
      const auto tokenBurstLimit = _sched._tokenLimit;
      const auto checkpointSize = _sched._checkpointSize;
#if defined GITS_PLATFORM_WINDOWS
      const uint64_t chunkSize = _sched._chunkSize;
#endif

      unsigned currentLoadedFrame = 0;
      bool stopLoading = false;
      for (;;) {
        unsigned loaded = 0;
        uint64_t maxLoaded = std::min((uint64_t)std::numeric_limits<decltype(loaded)>::max(),
                                      (uint64_t)tokenList.max_size());
#if defined GITS_PLATFORM_WINDOWS
        uint64_t currentChunkSize = 0;
        while (((isDirectX && currentChunkSize < chunkSize) ||
                (!isDirectX && loaded < tokenBurstLimit) ||
#else
        while ((loaded < tokenBurstLimit ||
#endif
                Configurator::Get().common.player.loadWholeStreamBeforePlayback) &&
               !stopLoading) {
#ifdef GITS_DEBUG_TOKEN_SIZE
          uint64_t tokBegin = stream->tellg();
#endif
          CToken* token = CToken::Deserialize(*stream, tokenCtor);
          if (token == nullptr) {
            stopLoading = true;
            break;
          }
#if defined GITS_PLATFORM_WINDOWS
          currentChunkSize += token->Size();
#endif

#ifdef GITS_DEBUG_TOKEN_SIZE
          uint64_t tokEnd = stream->tellg();
          token_size[typeid(*token).name()] += tokEnd - tokBegin + 2;
#endif
          if (loaded >= maxLoaded) {
            LOG_ERROR << "Max token loaded limit in one burst exceeded.";
            throw std::runtime_error(EXCEPTION_MESSAGE);
          }

          tokenList.push_back(token);

          loaded++;
          sinceLastChk++;

          if (sinceLastChk == checkpointSize) {
            LOG_FORMAT_RAW
            LOG_INFO << "#" << std::flush;
            sinceLastChk = 0;
          }

          // Stop loading tokens past 'exitFrame'
          if (token->Id() == CToken::ID_FRAME_END) {
            currentLoadedFrame++;
            if (currentLoadedFrame > Configurator::Get().common.player.exitFrame) {
              stopLoading = true;
            }
          }
        }

        // Finish if the queue won't accept more products.
        if (!queue.produce(tokenList)) {
          break;
        }

        if (stopLoading) {
          queue.break_pipe();
          break;
        }
      }
    } catch (std::exception& e) {
      LOG_ERROR << "Loader reading thread failed: Exiting!!!:" << e.what();
      fast_exit(1);
    }

    // If we were forced to finish we may still own few tokens
    Purge(tokenList);
#ifdef GITS_DEBUG_TOKEN_SIZE
    for (auto& one_size : token_size) {
      LOG_INFO << one_size.first << ": " << one_size.second;
    }
#endif
  }
};

class CStreamWriter {
  CScheduler& _sched;

public:
  // User-defined copy assignment operator.
  CStreamWriter& operator=(const CStreamWriter&) = delete;
  // User-defined destructor.
  ~CStreamWriter() = default;
  // User-defined copy constructor.
  CStreamWriter(const CStreamWriter& other) : _sched(other._sched) {}
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
      LOG_ERROR << "Error in writer thread: " << e.what();
      fast_exit(1);
    } catch (...) {
      LOG_ERROR << "Unknown error in writer thread";
      fast_exit(1);
    }
    LOG_INFO << "Stream writer thread finished.";
  }
};

//CScheduler class definition
#if defined GITS_PLATFORM_WINDOWS
CScheduler::CScheduler(unsigned tokenLimit, unsigned tokenBurstNum, uint64_t tokenBurstChunkSize)
#else
CScheduler::CScheduler(unsigned tokenLimit, unsigned tokenBurstNum)
#endif
    : _streamLoader(tokenBurstNum),
      _streamWriter(tokenBurstNum),
      _tokenShredder(tokenBurstNum),
#ifdef GITS_PLATFORM_WINDOWS
      _chunkSize(tokenBurstChunkSize),
      _currentChunkSize(0),
#endif
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
#if defined GITS_PLATFORM_WINDOWS
  static bool isDirectX =
      (CGits::Instance().GetApi3D() == ApisIface::TApi::DirectX); // Static variable for the check
  if ((isDirectX && _currentChunkSize > _chunkSize) ||
      (!isDirectX && _tokenList.size() > _tokenLimit)) {
    _currentChunkSize = 0;
#else
  if (_tokenList.size() > _tokenLimit) {
#endif
    WriteChunk();
  }

  // This mutex is necessary, because each recorded api (GL/CL/rs)
  // has its own api-mutex and thus don't serialize access to scheduler
  // for multi api applications. Proper solution would require sharing
  // single api-mutex among all recorded apis interceptors.
  std::unique_lock<std::mutex> lock(_tokenRegisterMutex);
  _tokenList.push_back(token);
#if defined GITS_PLATFORM_WINDOWS
  _currentChunkSize += token->Size();
#endif
  if (Configurator::Get().common.recorder.highIntegrity) {
    WriteChunk(false);
#if defined GITS_PLATFORM_WINDOWS
    _currentChunkSize = 0;
#endif
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
  if (Configurator::Get().common.recorder.highIntegrity) {
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
#if defined GITS_PLATFORM_WINDOWS
  _currentChunkSize = 0;
#endif
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

  LOG_INFO << "Will output checkpoint character '#' every " << _checkpointSize << " loaded tokens";

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
