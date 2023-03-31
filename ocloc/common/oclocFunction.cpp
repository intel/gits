// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocFunction.h"

#include "oclocFunctions.h"

namespace gits {
namespace ocloc {
CFunction* CFunction::Create(unsigned id) {
  if (id < TId::ID_FUNCTION_BEGIN || id >= TId::ID_FUNCTION_END) {
    return nullptr;
  }
  switch (id) {
  case ID_OCLOC_INVOKE:
    return new CoclocInvoke;
  case ID_OCLOC_FREE_OUTPUT:
    return new CoclocFreeOutput;
  default:;
  }
  Log(ERR) << "Unknown ocloc function with ID: " << id;
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
CArgument& CFunction::Result(unsigned idx) {
  Log(ERR) << "Results not supported in ocloc!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
} // namespace ocloc
} // namespace gits
