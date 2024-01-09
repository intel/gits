// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ccodeWriteWrap.cpp
*
* @brief Manual overrides of CArgument::Write(CCodeOStream &stream) methods.
*
* These overrides are used when the regular ccodeWrap mechanism is not enough.
* For example, when the *_wrap function has different parameters from the API
* call it wraps. The manual overrides allow us to write anything to the CCode.
*/

#include "ccodeWriteWrap.h"

#include <cstddef>
#include <iostream>

namespace gits {
// Helpers
void RegisterAndDeclareIfNeeded(CCodeOStream& stream, const CArgument& arg) {
  if (arg.DeclarationNeeded()) {
    arg.VariableNameRegister(stream, false);
    arg.Declare(stream);
  }
}

void StartScope(CCodeOStream& stream) {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
}

void EndScope(CCodeOStream& stream) {
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}
} // namespace gits
