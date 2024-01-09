// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclHeader.h"

#include "log.h"
#include "tools.h"
#include "dynamic_linker.h"

#include "openclDriversAuto.h"
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

#define OCL_FUNCTION(a, b, c, d, e, f) a##OCL_FUNCTION(b, c, d, e, f)
#define DECLARE_PTR_OCL_FUNCTION(a, b, c, e, f)                                                    \
  a(STDCALL* b) c = nullptr;                                                                       \
  a(STDCALL* orig_##b) c = nullptr;

class COclDriver {
public:
  COclDriver();
  COclDriver(const COclDriver& other) = delete;
  COclDriver& operator=(const COclDriver& other) = delete;
  ~COclDriver();
  void Initialize();
  dl::SharedObject Library() const {
    return _lib;
  }
  OCL_FUNCTIONS(DECLARE_PTR_)

private:
  bool _initialized;
  dl::SharedObject _lib;
};
#undef DECLARE_PTR_OCL_FUNCTION

extern COclDriver drvOcl WEAK;
} // namespace OpenCL
} // namespace gits
