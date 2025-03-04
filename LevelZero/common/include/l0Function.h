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
namespace l0 {
/**
    * @brief LevelZero library specific function wrapper
    *
    * gits::l0::CFunction is an LevelZero library specific function
    * call wrapper.
    */
class CFunction : public gits::CFunction {
public:
  enum TId {
    ID_FUNCTION_BEGIN = CToken::ID_LEVELZERO,
    ID_GITS_L0_MEMORY_UPDATE,
    ID_GITS_L0_MEMORY_RESTORE,
    ID_GITS_L0_MAKE_CURRENT_THREAD,
    ID_GITS_ORIGINAL_QUEUE_FAMILY_INFO,
    ID_FUNCTION_AUTOGENERATED_IDs = ID_FUNCTION_BEGIN + 1000,
#include "l0IDs.h"
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

  virtual void Write(CCodeOStream& stream) const;
  virtual CLibrary::TId LibraryId() const {
    return CLibrary::ID_LEVELZERO;
  }
};
} // namespace l0
} // namespace gits
