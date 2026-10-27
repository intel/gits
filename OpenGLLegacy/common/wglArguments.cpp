// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

/* ********************************** HVIDEOOUTPUTDEVICENV ******************************** */

const char* gits::OpenGL::CHVIDEOOUTPUTDEVICENV::NAME = "HVIDEOOUTPUTDEVICENV";

gits::OpenGL::CHVIDEOOUTPUTDEVICENV::CHVIDEOOUTPUTDEVICENV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHVIDEOOUTPUTDEVICENV::CHVIDEOOUTPUTDEVICENV(void* value)
    : CGLtype<GLtype, type>(value) {}

/* ********************************** HPVIDEODEV ******************************** */

const char* gits::OpenGL::CHPVIDEODEV::NAME = "HPVIDEODEV";

gits::OpenGL::CHPVIDEODEV::CHPVIDEODEV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHPVIDEODEV::CHPVIDEODEV(void* value) : CGLtype<GLtype, type>(value) {}

/* ********************************** HPGPUNV ******************************** */

const char* gits::OpenGL::CHPGPUNV::NAME = "HPGPUNV";

gits::OpenGL::CHPGPUNV::CHPGPUNV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHPGPUNV::CHPGPUNV(void* value) : CGLtype<GLtype, type>(value) {}

/* ********************************** HGPUNV ******************************** */

const char* gits::OpenGL::CHGPUNV::NAME = "HGPUNV";

gits::OpenGL::CHGPUNV::CHGPUNV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHGPUNV::CHGPUNV(void* value) : CGLtype<GLtype, type>(value) {}

/* ********************************** HVIDEOINPUTDEVICENV ******************************** */

const char* gits::OpenGL::CHVIDEOINPUTDEVICENV::NAME = "HVIDEOINPUTDEVICENV";

gits::OpenGL::CHVIDEOINPUTDEVICENV::CHVIDEOINPUTDEVICENV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CHVIDEOINPUTDEVICENV::CHVIDEOINPUTDEVICENV(void* value)
    : CGLtype<GLtype, type>(value) {}

/* ********************************** PIXELFORMATDESCRIPTOR ******************************** */

const char* gits::OpenGL::CPIXELFORMATDESCRIPTOR::NAME = "PIXELFORMATDESCRIPTOR";

gits::OpenGL::CPIXELFORMATDESCRIPTOR::CPIXELFORMATDESCRIPTOR() : CGLtype<GLtype, type>() {}

gits::OpenGL::CPIXELFORMATDESCRIPTOR::CPIXELFORMATDESCRIPTOR(PIXELFORMATDESCRIPTOR_ value)
    : CGLtype<GLtype, type>(value) {}
