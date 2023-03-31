// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ccodeWriteWrap.h
*
* @brief Manual overrides of CArgument::Write(CCodeOStream &stream) methods.
*
* These overrides are used when the regular ccodeWrap mechanism is not enough.
* For example, when the *_wrap function has different parameters from the API
* call it wraps. The manual overrides allow us to write anything to the CCode.
*/

#pragma once

#include "argument.h"
#include "streams.h"

namespace gits {

// Helpers
void RegisterAndDeclareIfNeeded(CCodeOStream& stream, const CArgument& arg);
void StartScope(CCodeOStream& stream);
void EndScope(CCodeOStream& stream);

} // namespace gits
