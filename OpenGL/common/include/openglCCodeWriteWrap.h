// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglCCodeWriteWrap.h
*
* @brief Manual overrides of CArgument::Write(CCodeOStream &stream) methods. Applies for OpenGL API.
*
* These overrides are used when the regular ccodeWrap mechanism is not enough.
* For example, when the *_wrap function has different parameters from the API
* call it wraps. The manual overrides allow us to write anything to the CCode.
*/

#pragma once

#include "ccodeWriteWrap.h"
#include "glFunctions.h"

namespace gits {
namespace OpenGL {

void CglGetProgramResourceiv_CCODEWRITEWRAP(CCodeOStream& stream,
                                            const CglGetProgramResourceiv& function);

} // namespace OpenGL
} // namespace gits
