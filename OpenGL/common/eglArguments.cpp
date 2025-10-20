// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "eglArguments.h"
#include "pragmas.h"
#include "streams.h"

namespace gits {
/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {
/* ******************************** EGLint ****************************** */
const char* CEGLint::NAME = "int";

CEGLint::CEGLint() {}

CEGLint::CEGLint(EGLint value) : CGLtype<GLtype, type>(value) {}

/* ******************************** EGLboolean ****************************** */
CEGLBoolean::CEGLBoolean() : CGLtype<GLtype, type>() {}

CEGLBoolean::CEGLBoolean(EGLBoolean value) : CGLtype<GLtype, type>(value) {}

/* ******************************** EGLenum ****************************** */
CEGLenum::CEGLenum() {}

CEGLenum::CEGLenum(EGLenum value) : CGLtype<GLtype, type>(value) {}

/* ******************************** EGLConfig ****************************** */
const char* CEGLConfig::NAME = "CEGLConfig";

CEGLConfig::CEGLConfig() {}

CEGLConfig::CEGLConfig(EGLConfig value) : CGLtype(value) {}

} // namespace OpenGL
} // namespace gits
