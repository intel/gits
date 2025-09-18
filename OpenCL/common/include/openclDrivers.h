// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclHeader.h"

#include "log2.h"
#include "tools.h"
#include "dynamic_linker.h"

#ifndef BUILD_FOR_CCODE
#include "openclTools.h"
#endif

#ifndef VOID_T_DEFINED
#define VOID_T_DEFINED
typedef struct void_type_tag {
}* void_t;
#endif

namespace gits {
namespace OpenCL {

class COclDriver {
public:
  COclDriver();
  COclDriver(const COclDriver& other) = delete;
  COclDriver& operator=(const COclDriver& other) = delete;
  ~COclDriver();
  void Initialize();
  dl::SharedObject Library() const {
    return _lib->getHandle();
  }
#include "openclDriversAuto.inl"

private:
  bool _initialized = false;
  std::unique_ptr<SharedLibrary> _lib;
};

extern COclDriver drvOcl WEAK;
} // namespace OpenCL
} // namespace gits
