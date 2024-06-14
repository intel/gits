// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
#include "config.h"
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
    : gits::CLibrary(ID_OPENGL, std::move(stc)), _linkProgramNo(0) {}

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
  return OpenGL::CFunction::Create(id);
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_OPENGL));
}

std::function<void()> CLibrary::CreateRestorePoint() {
  return SD().CreateCArraysRestorePoint();
}

void PreSwap() {
  using gits::Config;

  if (Config::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()]) {
    FrameBufferSave(CGits::Instance().CurrentFrame());
  }

  Log(TRACE) << "Frame end. Total drawcalls: " << CGits::Instance().CurrentDrawCount();
}

} //namespace OpenGL
} //namespace gits
