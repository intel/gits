// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Function.h"
#include "l0HelperFunctions.h"
#include "l0Functions.h"

namespace gits {
namespace l0 {
CFunction * CFunction::Create(unsigned id) {
  if (id < TId::ID_FUNCTION_BEGIN || id >= TId::ID_FUNCTION_END) {
    return nullptr;
  }
  switch (id) {
%for name, func in functions.items():
  %if func['enabled']:
  case ${func.get('id')}:
    return new C${name};
  %endif
%endfor
  case ID_GITS_L0_MEMORY_UPDATE:
    return new CGitsL0MemoryUpdate;
  case ID_GITS_L0_MEMORY_RESTORE:
    return new CGitsL0MemoryRestore;
  case ID_GITS_L0_MAKE_CURRENT_THREAD:
    return new CGitsL0TokenMakeCurrentThread;
  case ID_GITS_ORIGINAL_QUEUE_FAMILY_INFO:
    return new CGitsL0OriginalQueueFamilyInfo;
  default:
    ;
  }
  LOG_ERROR << "Unknown LevelZero function with ID: " << id;
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
CArgument &CFunction::Result(unsigned) {
  LOG_ERROR << "Results not supported in LevelZero!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
} // namespace l0
} // namespace gits
