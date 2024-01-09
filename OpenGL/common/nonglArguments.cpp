// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

void gits::OpenGL::CUINT::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::dec << Value();
  stream.flags(streamFlags);
}

/* ********************************** USHORT ******************************** */

const char* gits::OpenGL::CUSHORT::NAME = "USHORT";

gits::OpenGL::CUSHORT::CUSHORT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CUSHORT::CUSHORT(USHORT value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CUSHORT::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::dec << Value();
  stream.flags(streamFlags);
}

/* ********************************** BOOL ******************************** */

const char* gits::OpenGL::CBOOL::NAME = "BOOL";

gits::OpenGL::CBOOL::CBOOL() : CGLtype<GLtype, type>() {}

gits::OpenGL::CBOOL::CBOOL(BOOL value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CBOOL::Write(CCodeOStream& stream) const {
  if (Value()) {
    stream << "true";
  } else {
    stream << "false";
  }
}

/* ********************************** FLOAT ******************************** */

const char* gits::OpenGL::CFLOAT::NAME = "FLOAT";

gits::OpenGL::CFLOAT::CFLOAT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CFLOAT::CFLOAT(float value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CFLOAT::Write(CCodeOStream& stream) const {
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

/* ********************************** DWORD ******************************** */

const char* gits::OpenGL::CDWORD::NAME = "DWORD";

gits::OpenGL::CDWORD::CDWORD() : CGLtype<GLtype, type>() {}

gits::OpenGL::CDWORD::CDWORD(DWORD value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CDWORD::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::showpoint << Value();
  stream.flags(streamFlags);
}

/* ********************************** unsigned ******************************** */

const char* gits::OpenGL::Cunsigned::NAME = "unsigned";

gits::OpenGL::Cunsigned::Cunsigned() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cunsigned::Cunsigned(unsigned value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::Cunsigned::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::showpoint << Value();
  stream.flags(streamFlags);
}

/* ********************************** unsigned long ******************************** */

const char* gits::OpenGL::Cunsigned_long::NAME = "unsigned long";

gits::OpenGL::Cunsigned_long::Cunsigned_long() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cunsigned_long::Cunsigned_long(unsigned long value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::Cunsigned_long::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::dec << Value();
  stream.flags(streamFlags);
}

/* ********************************* C H A R ******************************* */

const char* gits::OpenGL::Cchar::NAME = "char";

gits::OpenGL::Cchar::Cchar() : CGLtype<GLtype, type>() {}

gits::OpenGL::Cchar::Cchar(char value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::Cchar::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::dec << static_cast<int>(Value());
  stream.flags(streamFlags);
}

/* ********************************** INT32 ******************************** */

const char* gits::OpenGL::CINT32::NAME = "INT32";

gits::OpenGL::CINT32::CINT32() : CGLtype<GLtype, type>() {}

gits::OpenGL::CINT32::CINT32(INT32 value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CINT32::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::dec << Value();
  stream.flags(streamFlags);
}

/* ********************************** INT64 ******************************** */

const char* gits::OpenGL::CINT64::NAME = "INT64";

gits::OpenGL::CINT64::CINT64() : CGLtype<GLtype, type>() {}

gits::OpenGL::CINT64::CINT64(INT64 value) : CGLtype<GLtype, type>(value) {}

void gits::OpenGL::CINT64::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::dec << Value();
  stream.flags(streamFlags);
}

/* ********************************* V O I D P T R ******************************* */

const char* gits::OpenGL::CvoidPtr::NAME = "VoidPointer";

gits::OpenGL::CvoidPtr::CvoidPtr() : _value(nullptr), _valuePtr(&_value) {}

gits::OpenGL::CvoidPtr::CvoidPtr(void* value) : _value(value), _valuePtr(&_value) {}

/*gits::OpenGL::CvoidPtr::CvoidPtr(void **valuePtr):
_valuePtr(valuePtr)
{
}*/

void gits::OpenGL::CvoidPtr::Write(CCodeOStream& stream) const {
  std::ios_base::fmtflags streamFlags(stream.flags());
  stream << std::hex << _value;
  stream.flags(streamFlags);
}
