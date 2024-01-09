// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclLibrary.h
*
* @brief Declaration of OpenCL library implementation.
*
*/

#pragma once

#include "oclFunction.h"

#include "library.h"

namespace gits {
/**
  * @brief OpenCL library specific GITS namespace
  */

namespace OpenCL {
/**
    * @brief OpenCL library class
    *
    * gits::OpenCL::CLibrary class provides OpenCL library tools
    * for GITS project. It is responsible for creating OpenCL
    * function call wrappers based on unique ID.
    */
class CLibrary : public gits::CLibrary {
public:
  static CLibrary& Get();
  CLibrary(gits::CLibrary::state_creator_t stc = gits::CLibrary::state_creator_t());
  ~CLibrary();

  OpenCL::CFunction* FunctionCreate(unsigned id) const override;

  const char* Name() const override {
    return "OpenCL";
  }
};
} // namespace OpenCL
} // namespace gits
