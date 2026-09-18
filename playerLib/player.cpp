// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "player.h"
#include "stateDynamic.h"
#include "openglTools.h"
#include "statistics.h"
#include "function.h"
#include "gits.h"
#include "streams.h"
#include "exception.h"
#include "log.h"
#include "configurationLib.h"
#include "buffer.h"

#include <iostream>

#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#if defined GITS_PLATFORM_LINUX
#include <unistd.h>
#include <sys/wait.h>
#endif

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
gits::CPlayer::CPlayer() : m_State(STATE_RUNNING) {
  CGits::Instance().SetSC(&this->m_Sc);
  const gits::Configuration& cfg = Configurator::Get();
  m_Interactive = cfg.common.player.interactive;

  m_Sc.scheduler.reset(new CScheduler(cfg.common.player.tokenBurst, cfg.common.player.tokenBurstNum,
                                      cfg.common.player.tokenBurstChunkSize));
}

gits::CPlayer::~CPlayer() {}

gits::CPlayer::TState gits::CPlayer::State() const {
  return m_State;
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
  m_Sc.iBinStream.reset(new CBinIStream(fileName));

  // load headers
  gits::CGits& inst = gits::CGits::Instance();
  *m_Sc.iBinStream >> inst;

  // load function call wrappers to the scheduler
  m_Sc.scheduler->Stream(m_Sc.iBinStream.get());
}

/**
 * @brief Plays loaded function calls wrappers
 *
 * Method plays loaded function calls wrappers. If there are some calls
 * loaded than library window is initialized. After that function calls
 * are run.
 */
void gits::CPlayer::Play() {
  if (!m_Sc.action) {
    throw ENotInitialized(EXCEPTION_MESSAGE);
  }

  const bool finished = m_Sc.scheduler->Run(*m_Sc.action);

  const auto& cfgPlayer = Configurator::Get().common.player;
  // Finish when scheduler doesn't have anything more for us,
  // or we are on a token that makes us process events and
  // frame number is matching our last frame + 1
  // (so we have played back frame 'exitFrame'.
  if (finished || CGits::Instance().CurrentFrame() == cfgPlayer.exitFrame + 1 ||
      CGits::Instance().Finished() == true) {
    m_State = STATE_FINISHED;
  }

  // Only pause when
  if (m_Interactive ||
      cfgPlayer.stopAfterFrames[static_cast<size_t>(CGits::Instance().CurrentFrame()) - 1]) {
    m_State = STATE_PAUSED;
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
    m_State = STATE_FINISHED;
    break;

  case ' ':
    if (m_State == STATE_RUNNING) {
      m_State = STATE_PAUSED;
    } else {
      m_State = STATE_RUNNING;
    }
    break;

  case 'i':
  case 'I':
    m_Interactive = !m_Interactive;
    break;

  default:;
  }
}

void gits::CPlayer::StatisticsPrint() const {
  CStatistics stats;
  CStatsComputer comp(stats);

  stats.Get(*m_Sc.scheduler, comp);
  stats.Print();
}

void gits::CPlayer::NotSupportedFunctionsPrint() const {
  const CFile::CSkippedCalls& skippedCalls = gits::CGits::Instance().FilePlayer().SkippedCalls();

  if (skippedCalls.size()) {
    LOG_INFO << "Following not supported functions were skipped during recording:";

    auto& inst = CGits::Instance();
    for (const auto& skipped : skippedCalls) {
      std::unique_ptr<CFunction> function(
          dynamic_cast<CFunction*>(inst.TokenCreate(CId(static_cast<uint16_t>(skipped.first)))));
      if (function == nullptr) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      LOG_INFO << " - " << function->Name();
    }
  }
}
