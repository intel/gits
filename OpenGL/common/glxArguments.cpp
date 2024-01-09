// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   glxArguments.cpp
*
*/

#include "glxArguments.h"
#include "streams.h"
#include "openglLibrary.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "pragmas.h"
#include "openglDrivers.h"
#include "stateDynamic.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <limits>
#include <cstring>

/* ********************************** CPixmap ******************************** */

const char* gits::OpenGL::CPixmap::NAME = "Pixmap";

gits::OpenGL::CPixmap::CPixmap() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CPixmap::CPixmap(Pixmap value) : CGLtype<GLtype, type, uint64_t>(value) {}

void gits::OpenGL::CPixmap::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(Pixmap)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** CFont ******************************** */

const char* gits::OpenGL::CFont::NAME = "Font";

gits::OpenGL::CFont::CFont() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CFont::CFont(Font value) : CGLtype<GLtype, type, uint64_t>(value) {}

void gits::OpenGL::CFont::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(Font)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** CWindow ******************************** */

const char* gits::OpenGL::CWindow::NAME = "Window";

gits::OpenGL::CWindow::CWindow() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CWindow::CWindow(Window value) : CGLtype<GLtype, type, uint64_t>(value) {}

void gits::OpenGL::CWindow::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(Window)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** CGLXWindow ******************************** */

const char* gits::OpenGL::CGLXWindow::NAME = "GLXWindow";

gits::OpenGL::CGLXWindow::CGLXWindow() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CGLXWindow::CGLXWindow(GLXWindow value) : CGLtype<GLtype, type, uint64_t>(value) {}

void gits::OpenGL::CGLXWindow::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(GLXWindow)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** CGLXPbuffer ******************************** */

const char* gits::OpenGL::CGLXPbuffer::NAME = "GLXPbuffer";

gits::OpenGL::CGLXPbuffer::CGLXPbuffer() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CGLXPbuffer::CGLXPbuffer(GLXPbuffer value) : CGLtype<GLtype, type, uint64_t>(value) {}

void gits::OpenGL::CGLXPbuffer::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(GLXPbuffer)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** CGLXPixmap ******************************** */

const char* gits::OpenGL::CGLXPixmap::NAME = "GLXPixmap";

gits::OpenGL::CGLXPixmap::CGLXPixmap() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CGLXPixmap::CGLXPixmap(GLXPixmap value) : CGLtype<GLtype, type, uint64_t>(value) {}

void gits::OpenGL::CGLXPixmap::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(GLXPixmap)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** Cint64_t ******************************** */

const char* gits::OpenGL::Cint64_t::NAME = "int64_t";

gits::OpenGL::Cint64_t::Cint64_t() : CGLtype<GLtype, type, int64_t>() {}

gits::OpenGL::Cint64_t::Cint64_t(int64_t value) : CGLtype<GLtype, type, int64_t>(value) {}

void gits::OpenGL::Cint64_t::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(int64_t)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** Culong ******************************** */

const char* gits::OpenGL::Culong::NAME = "unsigned long";

gits::OpenGL::Culong::Culong() : CGLtype<GLtype, type, unsigned long>() {}

gits::OpenGL::Culong::Culong(unsigned long value) : CGLtype<GLtype, type, unsigned long>(value) {}

void gits::OpenGL::Culong::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(unsigned long)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** Cuint ******************************** */

const char* gits::OpenGL::Cuint::NAME = "unsigned int";

gits::OpenGL::Cuint::Cuint() : CGLtype<GLtype, type, unsigned int>() {}

gits::OpenGL::Cuint::Cuint(unsigned int value) : CGLtype<GLtype, type, unsigned int>(value) {}

void gits::OpenGL::Cuint::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(unsigned int)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** CBool ******************************** */

const char* gits::OpenGL::CBool::NAME = "Bool";

gits::OpenGL::CBool::CBool() : CGLtype<GLtype, type, Bool>() {}

gits::OpenGL::CBool::CBool(Bool value) : CGLtype<GLtype, type, Bool>(value) {}

void gits::OpenGL::CBool::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(Bool)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** Cfloat ******************************** */

const char* gits::OpenGL::Cfloat::NAME = "float";

gits::OpenGL::Cfloat::Cfloat() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cfloat::Cfloat(float value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::Cfloat::Write(CCodeOStream& stream) const {
  float value = Value();
  std::ios_base::fmtflags streamFlags(stream.flags());

  if (value != value) {
    //NaN - give any valid value
    stream << 0;
  } else {
    //cap value to min/max - discard infinities
    //bizzare parenthesis usage prevents min/max windows macros from kicking in
    value = (std::min)((std::numeric_limits<float>::max)(), value);
    value = (std::max)(-(std::numeric_limits<float>::max)(), value);

    stream << std::showpoint << value << 'f';
  }
  stream.flags(streamFlags);
}
