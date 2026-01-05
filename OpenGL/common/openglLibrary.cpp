// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglLibrary.cpp
 *
 * @brief Definition of OpenGL common part library implementation.
 *
 */

#include "openglLibrary.h"
#include "openglFunction.h"
#include "openglTools.h"
#include "windowContextState.h"
#include "token.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "platform.h"
#include "pragmas.h"

#include <algorithm>
#include <sstream>
#include <iterator>
#include <memory>
#include <iomanip>
#include <filesystem>

namespace gits {
namespace OpenGL {

/**
 * @brief Constructor
 *
 * CLibrary class constructor.
 */
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc)
    : gits::CLibrary(ID_OPENGL, std::move(stc)), _linkProgramNo(0), _eventsRegistered(false) {}

CLibrary::~CLibrary() {}

gits::CResourceManager2& gits::OpenGL::CLibrary::ProgramBinaryManager() {
  if (_progBinManager) {
    return *_progBinManager;
  }

  std::unordered_map<uint32_t, std::filesystem::path> the_map;
  the_map[RESOURCE_INDEX] = "gitsPlayerDataIndex.dat";
  the_map[RESOURCE_BUFFER] = "gitsPlayerBuffers.dat";

  _progBinManager.reset(new CResourceManager2(the_map));
  return *_progBinManager;
}

/**
 * @brief Creates OpenGL function call wrapper
 *
 * Method creates OpenGL function call wrappers based on unique
 * identifier.
 *
 * @param id Unique OpenGL function identifier.
 *
 * @exception EOperationFailed Unknown OpenGL function identifier
 *
 * @return OpenGL function call wrapper.
 */
CFunction* CLibrary::FunctionCreate(unsigned id) const {
  if (!_eventsRegistered) {
    const_cast<CLibrary*>(this)->RegisterEvents();
  }
  return OpenGL::CFunction::Create(id);
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_OPENGL));
}

void PreSwap() {
  using gits::Config;

  if (Configurator::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()]) {
    FrameBufferSave(CGits::Instance().CurrentFrame());
  }

  LOG_TRACE << "Frame end. Total drawcalls: " << CGits::Instance().CurrentDrawCount();
}

void CLibrary::RegisterEvents() {
  auto eventHandler = [this](Topic t, const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<GitsEventMessage>(m);
    if (!msg) {
      return;
    }

    auto& cfg = Configurator::Get();
    auto& data = msg->getData();
    switch (data.Id) {
    case CToken::TId::ID_INIT_END:
      break;
    case CToken::TId::ID_FRAME_START:
      break;
    default:
      break;
    }
  };

  gits::CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_GITS_EVENT},
                                                    eventHandler);
  _eventsRegistered = true;
}

} //namespace OpenGL
} //namespace gits
