// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "function.h"

namespace gits {
namespace OpenCL {
/**
    * @brief OpenCL library specific function wrapper
    *
    * gits::OpenCL::CFunction is an OpenCL library specific function
    * call wrapper.
    */
class CFunction : public gits::CFunction {
public:
  enum TVersion {
    VERSION_UNKNOWN,
    VERSION_1_0,
    VERSION_1_0_DEPRECATED,
    VERSION_1_1,
    VERSION_1_1_DEPRECATED,
    VERSION_1_2,
    VERSION_1_2_DEPRECATED,
    VERSION_2_0,
    VERSION_2_1,
    VERSION_2_2,
    VERSION_3_0,
  };
  enum TId {
    ID_FUNCTION_BEGIN = CToken::ID_OPENCL,
#include "openclIDs.h"
    ID_GITS_CL_MEMORY_UPDATE = ID_FUNCTION_BEGIN + 380, //for backward compatibility
    ID_GITS_CL_MEMORY_RESTORE,
    ID_GITS_CL_MEMORY_REGION_RESTORE,
    ID_GITS_CL_MAKE_CURRENT_THREAD,
    ID_FUNCTION_END,
    ID_FUNCTION_NUM = ID_FUNCTION_END - ID_FUNCTION_BEGIN
  };
  static CFunction* Create(unsigned id);
  virtual const CArgument* Return() const {
    return nullptr;
  }
  virtual unsigned ResultCount() const {
    return 0;
  }
  virtual CArgument& Result(unsigned idx);

  virtual CLibrary::TId LibraryId() const {
    return CLibrary::ID_OPENCL;
  }
};
} // namespace OpenCL
} // namespace gits
