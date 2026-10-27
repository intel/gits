// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   playerRunWrapConditions.h
*
* @brief Helper functions for checking various conditions at runtime.
*
*/

#pragma once

#include "gits.h"
#include "openglDrivers.h"

namespace gits {
namespace OpenGL {
inline bool ConditionCurrentContextZero() {
  if (SD().GetCurrentContext() == 0) {
    return false;
  } else {
    return true;
  }
}

inline bool ConditionExtension(std::string ext_name) {
  static std::set<std::string> ext_set;

  if (drv.gl.HasExtension(ext_name)) {
    return true;
  } else {
    if (ext_set.find(ext_name) == ext_set.end()) {
      LOG_WARNING << "" + ext_name + " is not available on current platform.";
      ext_set.insert(std::move(ext_name));
    }
    return false;
  }
}

inline bool ConditionQueries() {
  if (!Configurator::Get().opengl.player.skipQueries) {
    return true;
  } else {
    return false;
  }
}

inline bool ConditionAlwaysFalse(unsigned token_id) {
  static std::set<unsigned> token_ids_set;
  if (token_ids_set.find(token_id) == token_ids_set.end()) {
    CFunction* func = CFunction::Create(token_id);
    LOG_WARNING << func->Name() << " calls skipped";
    token_ids_set.insert(token_id);
  }
  return false;
}
} // namespace OpenGL
} // namespace gits
