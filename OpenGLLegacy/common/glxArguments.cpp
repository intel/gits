// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

/* ********************************** CFont ******************************** */

const char* gits::OpenGL::CFont::NAME = "Font";

gits::OpenGL::CFont::CFont() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CFont::CFont(Font value) : CGLtype<GLtype, type, uint64_t>(value) {}

/* ********************************** CWindow ******************************** */

const char* gits::OpenGL::CWindow::NAME = "Window";

gits::OpenGL::CWindow::CWindow() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CWindow::CWindow(Window value) : CGLtype<GLtype, type, uint64_t>(value) {}

/* ********************************** CGLXWindow ******************************** */

const char* gits::OpenGL::CGLXWindow::NAME = "GLXWindow";

gits::OpenGL::CGLXWindow::CGLXWindow() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CGLXWindow::CGLXWindow(GLXWindow value) : CGLtype<GLtype, type, uint64_t>(value) {}

/* ********************************** CGLXPbuffer ******************************** */

const char* gits::OpenGL::CGLXPbuffer::NAME = "GLXPbuffer";

gits::OpenGL::CGLXPbuffer::CGLXPbuffer() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CGLXPbuffer::CGLXPbuffer(GLXPbuffer value) : CGLtype<GLtype, type, uint64_t>(value) {}

/* ********************************** CGLXPixmap ******************************** */

const char* gits::OpenGL::CGLXPixmap::NAME = "GLXPixmap";

gits::OpenGL::CGLXPixmap::CGLXPixmap() : CGLtype<GLtype, type, uint64_t>() {}

gits::OpenGL::CGLXPixmap::CGLXPixmap(GLXPixmap value) : CGLtype<GLtype, type, uint64_t>(value) {}

/* ********************************** Cint64_t ******************************** */

const char* gits::OpenGL::Cint64_t::NAME = "int64_t";

gits::OpenGL::Cint64_t::Cint64_t() : CGLtype<GLtype, type, int64_t>() {}

gits::OpenGL::Cint64_t::Cint64_t(int64_t value) : CGLtype<GLtype, type, int64_t>(value) {}

/* ********************************** Culong ******************************** */

const char* gits::OpenGL::Culong::NAME = "unsigned long";

gits::OpenGL::Culong::Culong() : CGLtype<GLtype, type, unsigned long>() {}

gits::OpenGL::Culong::Culong(unsigned long value) : CGLtype<GLtype, type, unsigned long>(value) {}

/* ********************************** Cuint ******************************** */

const char* gits::OpenGL::Cuint::NAME = "unsigned int";

gits::OpenGL::Cuint::Cuint() : CGLtype<GLtype, type, unsigned int>() {}

gits::OpenGL::Cuint::Cuint(unsigned int value) : CGLtype<GLtype, type, unsigned int>(value) {}

/* ********************************** CBool ******************************** */

const char* gits::OpenGL::CBool::NAME = "Bool";

gits::OpenGL::CBool::CBool() : CGLtype<GLtype, type, Bool>() {}

gits::OpenGL::CBool::CBool(Bool value) : CGLtype<GLtype, type, Bool>(value) {}

/* ********************************** Cfloat ******************************** */

const char* gits::OpenGL::Cfloat::NAME = "float";

gits::OpenGL::Cfloat::Cfloat() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cfloat::Cfloat(float value) : CGLtype<GLtype, type>(value) {}
