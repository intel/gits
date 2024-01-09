// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   wglArguments.cpp
*
* @brief  Definitions of OpenGL library function call argument wrappers.
*
*/

#include "openglDrivers.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "wglArguments.h"
#include "platform.h"
#include "streams.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "pragmas.h"
#include "nonglArguments.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <limits>
#include <cstring>

/* ********************************** LPVOID ******************************** */

const char* gits::OpenGL::CLPVOID::NAME = "LPVOID";

gits::OpenGL::CLPVOID::CLPVOID() : CGLtype<GLtype, type>() {}

gits::OpenGL::CLPVOID::CLPVOID(void* value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CLPVOID::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** HVIDEOOUTPUTDEVICENV ******************************** */

const char* gits::OpenGL::CHVIDEOOUTPUTDEVICENV::NAME = "HVIDEOOUTPUTDEVICENV";

gits::OpenGL::CHVIDEOOUTPUTDEVICENV::CHVIDEOOUTPUTDEVICENV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHVIDEOOUTPUTDEVICENV::CHVIDEOOUTPUTDEVICENV(void* value)
    : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CHVIDEOOUTPUTDEVICENV::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(HVIDEOOUTPUTDEVICENV)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** HPVIDEODEV ******************************** */

const char* gits::OpenGL::CHPVIDEODEV::NAME = "HPVIDEODEV";

gits::OpenGL::CHPVIDEODEV::CHPVIDEODEV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHPVIDEODEV::CHPVIDEODEV(void* value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CHPVIDEODEV::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(HPVIDEODEV)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** HPGPUNV ******************************** */

const char* gits::OpenGL::CHPGPUNV::NAME = "HPGPUNV";

gits::OpenGL::CHPGPUNV::CHPGPUNV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHPGPUNV::CHPGPUNV(void* value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CHPGPUNV::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(HPGPUNV)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** HGPUNV ******************************** */

const char* gits::OpenGL::CHGPUNV::NAME = "HGPUNV";

gits::OpenGL::CHGPUNV::CHGPUNV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHGPUNV::CHGPUNV(void* value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CHGPUNV::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(HGPUNV)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** HVIDEOINPUTDEVICENV ******************************** */

const char* gits::OpenGL::CHVIDEOINPUTDEVICENV::NAME = "HVIDEOINPUTDEVICENV";

gits::OpenGL::CHVIDEOINPUTDEVICENV::CHVIDEOINPUTDEVICENV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHVIDEOINPUTDEVICENV::CHVIDEOINPUTDEVICENV(void* value)
    : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CHVIDEOINPUTDEVICENV::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << "(HVIDEOINPUTDEVICENV)0x" << std::hex << Value();
  stream.flags(streamFlags);
}

/* ********************************** PIXELFORMATDESCRIPTOR ******************************** */

const char* gits::OpenGL::CPIXELFORMATDESCRIPTOR::NAME = "PIXELFORMATDESCRIPTOR";

gits::OpenGL::CPIXELFORMATDESCRIPTOR::CPIXELFORMATDESCRIPTOR() : CGLtype<GLtype, type>() {}

gits::OpenGL::CPIXELFORMATDESCRIPTOR::CPIXELFORMATDESCRIPTOR(PIXELFORMATDESCRIPTOR_ value)
    : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CPIXELFORMATDESCRIPTOR::Write(CCodeOStream& stream) const {
#if defined GITS_PLATFORM_WINDOWS
  static_assert(sizeof(PIXELFORMATDESCRIPTOR) == sizeof(PIXELFORMATDESCRIPTOR_),
                "PIXELFORMATDESCRIPTOR_ is not in sync with PIXELFORMATDESCRIPTOR");
#endif
  stream << "pxfmt";
}

void gits::OpenGL::CPIXELFORMATDESCRIPTOR::Declare(CCodeOStream& stream) const {
  stream << std::showbase;
  PIXELFORMATDESCRIPTOR_ to_write = Value();
  stream.Indent() << "int pxfmt[] = "
                  << "{ " << to_write.nSize << ", " << to_write.nVersion << ", " << to_write.dwFlags
                  << ", " << Cchar(to_write.iPixelType) << ", " << Cchar(to_write.cColorBits)
                  << ", " << Cchar(to_write.cRedBits) << ", " << Cchar(to_write.cRedShift) << ", "
                  << Cchar(to_write.cGreenBits) << ", " << Cchar(to_write.cGreenShift) << ", "
                  << Cchar(to_write.cBlueBits) << ", " << Cchar(to_write.cBlueShift) << ", "
                  << Cchar(to_write.cAlphaBits) << ", " << Cchar(to_write.cAlphaShift) << ", "
                  << Cchar(to_write.cAccumBits) << ", " << Cchar(to_write.cAccumRedBits) << ", "
                  << Cchar(to_write.cAccumGreenBits) << ", " << Cchar(to_write.cAccumBlueBits)
                  << ", " << Cchar(to_write.cAccumAlphaBits) << ", " << Cchar(to_write.cDepthBits)
                  << ", " << Cchar(to_write.cStencilBits) << ", " << Cchar(to_write.cAuxBuffers)
                  << ", " << Cchar(to_write.iLayerType) << ", " << Cchar(to_write.bReserved) << ", "
                  << to_write.dwLayerMask << ", " << to_write.dwVisibleMask << ", "
                  << to_write.dwDamageMask << " };\n";
}
