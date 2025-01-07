// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "function.h"

namespace gits {
namespace ocloc {
/**
    * @brief Ocloc library specific function wrapper
    *
    * gits::ocloc::CFunction is an LevelZero library specific function
    * call wrapper.
    */
class CFunction : public gits::CFunction {
public:
  enum TId {
    ID_FUNCTION_BEGIN = CToken::ID_OCLOC,
    ID_OCLOC_INVOKE,
    ID_OCLOC_INVOKE_V1,
    ID_OCLOC_FREE_OUTPUT,
    ID_FUNCTION_AUTOGENERATED_IDs = ID_FUNCTION_BEGIN + 1000,
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
    return CLibrary::ID_OCLOC;
  }
};
} // namespace ocloc
} // namespace gits
