// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

void CEGLint::Write(CCodeOStream& stream) const {
  stream << std::dec << Value();
}

/* ******************************** EGLboolean ****************************** */
CEGLBoolean::CEGLBoolean() : CGLtype<GLtype, type>() {}

CEGLBoolean::CEGLBoolean(EGLBoolean value) : CGLtype<GLtype, type>(value) {}

void CEGLBoolean::Write(CCodeOStream& stream) const {
  if (Value() == EGL_FALSE) {
    stream << "EGL_FALSE";
  } else {
    stream << "EGL_TRUE";
  }
}

/* ******************************** EGLenum ****************************** */
CEGLenum::CEGLenum() {}

CEGLenum::CEGLenum(EGLenum value) : CGLtype<GLtype, type>(value) {}

void CEGLenum::Write(CCodeOStream& stream) const {
  stream << std::hex << Value();
}

/* ******************************** EGLConfig ****************************** */
const char* CEGLConfig::NAME = "CEGLConfig";

CEGLConfig::CEGLConfig() {}

CEGLConfig::CEGLConfig(EGLConfig value) : CGLtype(value) {}

void CEGLConfig::Write(CCodeOStream& stream) const {
  TODO("EGLConfig in sourcecode")
}
} // namespace OpenGL
} // namespace gits
