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
void CFunction::Write(CCodeOStream& stream) const {
  // implementation - captured function calls go to this file:
  stream.select(CCodeOStream::GITS_FRAMES_CPP);

  auto returnVal = Return();

  unsigned varNum = 0;
  for (unsigned i = 0; i < ArgumentCount(); i++) {
    const CArgument& arg = Argument(i);
    if (arg.DeclarationNeeded()) {
      if (arg.GlobalScopeVariable()) {
        arg.VariableNameRegister(stream, false);
      } else {
        ++varNum;
      }
    }
  }
  if (returnVal && returnVal->GlobalScopeVariable()) {
    returnVal->VariableNameRegister(stream, true);
  }

  // print arguments declarations
  if (varNum || returnVal) {
    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
  }
  for (unsigned i = 0; i < ArgumentCount(); i++) {
    const CArgument& arg = Argument(i);
    if (arg.DeclarationNeeded()) {
      if (!arg.GlobalScopeVariable()) {
        arg.VariableNameRegister(stream, false);
      }
      arg.Declare(stream);
    }
  }
  if (returnVal) {
    if (!returnVal->GlobalScopeVariable()) {
      returnVal->VariableNameRegister(stream, true);
    }
    returnVal->Declare(stream);
  }

  // print function
  stream.Indent();
  if (returnVal) {
    stream << stream.VariableName(returnVal->ScopeKey()) << " = ";
  }
  stream << Name() << Suffix() << "(";
  for (unsigned idx = 0; idx < ArgumentCount(); idx++) {
    if (Argument(idx).AmpersandNeeded()) {
      stream << '&';
    }
    stream << Argument(idx);
    if (idx < ArgumentCount() - 1) {
      stream << ", ";
    }
  }
  if (strcmp(Suffix(), "") != 0 && Return()) {
    stream << ", " << *Return();
  }
  stream << ");" << std::endl;

  for (unsigned i = 0; i < ArgumentCount(); i++) {
    const CArgument& arg = Argument(i);
    if (arg.PostActionNeeded()) {
      arg.PostAction(stream);
    }
  }

  WritePostCall(stream);

  if (varNum || returnVal) {
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
  }
}
} // namespace l0
} // namespace gits
