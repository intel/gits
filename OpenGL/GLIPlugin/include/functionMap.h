// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   functionMap.h
 *
 * @brief
 */

#pragma once

#include "openglFunction.h"

#include <map>
#include <string>
#include <cstring>

namespace gits {
namespace OpenGL {
namespace gli {
struct str_cmp {
  bool operator()(const char* lhs, const char* rhs) const {
    return std::strcmp(lhs, rhs) < 0;
  }
};

class CFunctionMap {
  class CDatabase {
    typedef std::map<const char*, CFunction::TId, str_cmp> CSupportMap;
    CSupportMap _supportedMap;
    CSupportMap _notSupportedMap;

  public:
    CDatabase();
    CFunction::TId SupportedFunctionId(const char* name);
    CFunction::TId NotSupportedFunctionId(const char* name);
  };

  static CDatabase _database;
  //Possible dangerous speed up feature - Changed str_cmp into std::less<const char*>.
  //This will work ony if assumption that gliIntercept will provide always same pointer for identical string.

  typedef std::map<const char*, CFunction::TId, std::less<const char*>> CAppSupportedMap;
  CAppSupportedMap _appSupportedMap;

public:
  CFunction::TId SupportedFunctionId(const char* name);
  CFunction::TId NotSupportedFunctionId(const char* name);
};
} // namespace gli
} // namespace OpenGL
} // namespace gits
