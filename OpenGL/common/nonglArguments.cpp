// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   nonglArguments.cpp
*
* @brief  Definitions of OpenGL library function call argument wrappers.
*
*/

#include "nonglArguments.h"

/* ********************************** UINT ******************************** */

const char* gits::OpenGL::CUINT::NAME = "UINT";

gits::OpenGL::CUINT::CUINT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CUINT::CUINT(UINT value) : CGLtype<GLtype, type>(value) {}

/* ********************************** USHORT ******************************** */

const char* gits::OpenGL::CUSHORT::NAME = "USHORT";

gits::OpenGL::CUSHORT::CUSHORT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CUSHORT::CUSHORT(USHORT value) : CGLtype<GLtype, type>(value) {}

/* ********************************** BOOL ******************************** */

const char* gits::OpenGL::CBOOL::NAME = "BOOL";

gits::OpenGL::CBOOL::CBOOL() : CGLtype<GLtype, type>() {}

gits::OpenGL::CBOOL::CBOOL(BOOL value) : CGLtype<GLtype, type>(value) {}

/* ********************************** FLOAT ******************************** */

const char* gits::OpenGL::CFLOAT::NAME = "FLOAT";

gits::OpenGL::CFLOAT::CFLOAT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CFLOAT::CFLOAT(float value) : CGLtype<GLtype, type>(value) {}

/* ********************************** DWORD ******************************** */

const char* gits::OpenGL::CDWORD::NAME = "DWORD";

gits::OpenGL::CDWORD::CDWORD() : CGLtype<GLtype, type>() {}

gits::OpenGL::CDWORD::CDWORD(DWORD value) : CGLtype<GLtype, type>(value) {}

/* ********************************** unsigned ******************************** */

const char* gits::OpenGL::Cunsigned::NAME = "unsigned";

gits::OpenGL::Cunsigned::Cunsigned() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cunsigned::Cunsigned(unsigned value) : CGLtype<GLtype, type>(value) {}

/* ********************************** unsigned long ******************************** */

const char* gits::OpenGL::Cunsigned_long::NAME = "unsigned long";

gits::OpenGL::Cunsigned_long::Cunsigned_long() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cunsigned_long::Cunsigned_long(unsigned long value) : CGLtype<GLtype, type>(value) {}

/* ********************************* C H A R ******************************* */

const char* gits::OpenGL::Cchar::NAME = "char";

gits::OpenGL::Cchar::Cchar() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cchar::Cchar(char value) : CGLtype<GLtype, type>(value) {}

/* ********************************** INT32 ******************************** */

const char* gits::OpenGL::CINT32::NAME = "INT32";

gits::OpenGL::CINT32::CINT32() : CGLtype<GLtype, type>() {}

gits::OpenGL::CINT32::CINT32(INT32 value) : CGLtype<GLtype, type>(value) {}

/* ********************************** INT64 ******************************** */

const char* gits::OpenGL::CINT64::NAME = "INT64";

gits::OpenGL::CINT64::CINT64() : CGLtype<GLtype, type>() {}

gits::OpenGL::CINT64::CINT64(INT64 value) : CGLtype<GLtype, type>(value) {}

/* ********************************* V O I D P T R ******************************* */

const char* gits::OpenGL::CvoidPtr::NAME = "VoidPointer";

gits::OpenGL::CvoidPtr::CvoidPtr() : _value(nullptr), _valuePtr(&_value) {}

gits::OpenGL::CvoidPtr::CvoidPtr(void* value) : _value(value), _valuePtr(&_value) {}

/*gits::OpenGL::CvoidPtr::CvoidPtr(void **valuePtr):
_valuePtr(valuePtr)
{
}*/
