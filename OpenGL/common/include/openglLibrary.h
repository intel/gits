// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglLibrary.h
 *
 * @brief Declaration of OpenGL library implementation.
 *
 */

#pragma once

#include "library.h"
#include "exception.h"
#include "openglFunction.h"
#include "openglTools.h"
#include "stateDynamic.h"
#include "version.h"
#include "windowing.h"
#include "platform.h"
#include "openglEnums.h"
#include "tools.h"
#include <unordered_map>
#include <list>
#include <set>
#include <vector>

namespace gits {
/**
   * @brief OpenGL library specific GITS namespace
   */

class CResourceManager2;

namespace OpenGL {
/**
     * @brief OpenGL library class
     *
     * gits::OpenGL::CLibrary class provides OpenGL library tools
     * for GITS project. It is responsible for creating OpenGL
     * function call wrappers based on unique ID, OpenGL library state
     * getter class and for creating OpenGL display window for gitsPlayer.
     */
class CLibrary : public gits::CLibrary {
  uint32_t _linkProgramNo;
  std::unique_ptr<CResourceManager2> _progBinManager;

public:
  static CLibrary& Get();
  CLibrary(gits::CLibrary::state_creator_t stc = gits::CLibrary::state_creator_t());
  ~CLibrary();

  CFunction* FunctionCreate(unsigned id) const override;

  const char* Name() const override {
    return "OpenGL";
  }

  CResourceManager2& ProgramBinaryManager();

  uint32_t GetLinkProgramNumber() const {
    return _linkProgramNo;
  }
  void IncrementLinkProgramNumber() {
    _linkProgramNo++;
  }

  std::function<void()> CreateRestorePoint();
};

void PreSwap();
} // namespace OpenGL
} // namespace gits
