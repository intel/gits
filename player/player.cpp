// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   player.cpp
 *
 * @brief Definition of function calls player.
 *
 */

#include "player.h"
#include "stateDynamic.h"
#include "openglTools.h"
#include "statistics.h"
#include "function.h"
#include "gits.h"
#include "streams.h"
#include "exception.h"
#include "log.h"
#include "config.h"
#include "configurationLib.h"
#include "buffer.h"

#include <iostream>

/* ********************************* P L A Y E R ******************************* */

/**
 * @brief Constructor
 *
 * Constructor of gits::CPlayer class.
 *
 * @param interactive Defines if player should be run in interactive mode.
 * @param frameNumberMode Defines if frame number should be drawn and what the value should be
 * @param dumpScreenshots Defines if player should dump screenshot for every replayed frame
 */
gits::CPlayer::CPlayer() : _state(STATE_RUNNING) {
  const auto& config = Configurator::Get();
  CGits::Instance().SetSC(&this->_sc);
  const auto& cfg = Configurator::Get();
  _interactive = cfg.common.player.interactive;

  _sc.scheduler.reset(
#if defined GITS_PLATFORM_WINDOWS
      new CScheduler(cfg.common.player.tokenBurst, cfg.common.player.tokenBurstNum,
                     cfg.directx.player.tokenBurstChunkSize));
#else
      new CScheduler(cfg.common.player.tokenBurst, cfg.common.player.tokenBurstNum));
#endif
}

gits::CPlayer::~CPlayer() {}

gits::CPlayer::TState gits::CPlayer::State() const {
  return _state;
}

/**
 * @brief Loads function calls wrappers from the binary file
 *
 * Method loads function calls wrappers from the specified binary
 * file.
 *
 * @param fileName Name of a file to use
 */
void gits::CPlayer::Load(const std::filesystem::path& fileName) {
  // open file
  _sc.iBinStream.reset(new CBinIStream(fileName));

  // load headers
  gits::CGits& inst = gits::CGits::Instance();
  (*_sc.iBinStream) >> inst;

  // load function call wrappers to the scheduler
  _sc.scheduler->Stream(_sc.iBinStream.get());
}

/**
 * @brief Plays loaded function calls wrappers
 *
 * Method plays loaded function calls wrappers. If there are some calls
 * loaded than library window is initialized. After that function calls
 * are run.
 */
void gits::CPlayer::Play() {
  if (!_sc.action) {
    throw ENotInitialized(EXCEPTION_MESSAGE);
  }

  const bool finished = _sc.scheduler->Run(*_sc.action);

  const auto& cfgPlayer = Configurator::Get().common.player;
  // Finish when scheduler doesn't have anything more for us,
  // or we are on a token that makes us process events and
  // frame number is matching our last frame + 1
  // (so we have played back frame 'exitFrame'.
  if (finished || CGits::Instance().CurrentFrame() == cfgPlayer.exitFrame + 1 ||
      CGits::Instance().Finished() == true) {
    _state = STATE_FINISHED;
    CGits::Instance().ProcessEndPlaybackEvents();
  }

  // Only pause when
  if (_interactive ||
      cfgPlayer.stopAfterFrames[static_cast<size_t>(CGits::Instance().CurrentFrame()) - 1]) {
    _state = STATE_PAUSED;
  }
}

void gits::CPlayer::GLResourceCleanup() {
  if (Configurator::Get().common.player.cleanResourcesOnExit) {
    gits::OpenGL::CleanResources();
  }
}

void gits::CPlayer::GLContextsCleanup() {
  if (Configurator::Get().opengl.player.destroyContextsOnExit) {
    gits::OpenGL::DestroyAllContexts();
  }
}

void gits::CPlayer::Key(unsigned code) {
  switch (code) {
  case 27:
    // ESC
    _state = STATE_FINISHED;
    break;

  case ' ':
    if (_state == STATE_RUNNING) {
      _state = STATE_PAUSED;
    } else {
      _state = STATE_RUNNING;
    }
    break;

  case 'i':
  case 'I':
    _interactive = !_interactive;
    break;

  default:;
  }
}

void gits::CPlayer::StatisticsPrint() const {
  CStatistics stats;
  CStatsComputer comp(stats);

  stats.Get(*_sc.scheduler, comp);

  Log(INFO, RAW) << std::endl;
  stats.Print();
  Log(INFO, RAW) << std::endl;
}

void gits::CPlayer::NotSupportedFunctionsPrint() const {
  const CFile::CSkippedCalls& skippedCalls = gits::CGits::Instance().FilePlayer().SkippedCalls();

  if (skippedCalls.size()) {
    Log(INFO, RAW) << std::endl;
    Log(INFO) << "Following not supported functions were skipped during recording:";

    auto& inst = CGits::Instance();
    for (const auto& skipped : skippedCalls) {
      std::unique_ptr<CFunction> function(
          dynamic_cast<CFunction*>(inst.TokenCreate(CId(static_cast<uint16_t>(skipped.first)))));
      if (function == nullptr) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      Log(INFO) << " - " << function->Name();
    }
  }
}
