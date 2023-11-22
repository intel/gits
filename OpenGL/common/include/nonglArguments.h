// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   nonglArguments.h
*
* @brief Declarations of OpenGL library function call argument wrappers.
*
*/

#pragma once

#include "argument.h"
#include "exception.h"
#include "openglArguments.h"

#include <string>
#include <stdexcept>

namespace gits {
/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {
/**
    * @brief Wrapper for UINT type
    *
    * gits::OpenGL::CUINT class is a wrapper for UINT
    * type value.
    */
class CUINT : public CGLtype<UINT, CUINT> {
public:
  CUINT();
  CUINT(UINT);
  static const unsigned LENGTH = sizeof(UINT);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for USHORT type
    *
    * gits::OpenGL::CUSHORT class is a wrapper for USHORT
    * type value.
    */
class CUSHORT : public CGLtype<USHORT, CUSHORT> {
public:
  CUSHORT();
  CUSHORT(USHORT);
  static const unsigned LENGTH = sizeof(USHORT);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for BOOL type
    *
    * gits::OpenGL::CBOOL class is a wrapper for BOOL
    * type value.
    */
class CBOOL : public CGLtype<BOOL, CBOOL> {
public:
  CBOOL();
  CBOOL(BOOL);
  static const unsigned LENGTH = sizeof(BOOL);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for FLOAT type
    *
    * gits::OpenGL::CFLOAT class is a wrapper for FLOAT
    * type value.
    */
class CFLOAT : public CGLtype<float, CFLOAT> {
public:
  CFLOAT();
  CFLOAT(float);
  static const unsigned LENGTH = sizeof(float);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for DWORD type
    *
    * gits::OpenGL::CDWORD class is a wrapper for DWORD
    * type value.
    */
class CDWORD : public CGLtype<DWORD, CDWORD> {
public:
  CDWORD();
  CDWORD(DWORD);
  static const unsigned LENGTH = sizeof(DWORD);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for unsigned type
    *
    * gits::OpenGL::Cunsigned class is a wrapper for unsigned
    * type value.
    */
class Cunsigned : public CGLtype<unsigned, Cunsigned> {
public:
  Cunsigned();
  Cunsigned(unsigned);
  static const unsigned LENGTH = sizeof(unsigned);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for unsigned long type
    *
    * gits::OpenGL::Cunsigned_long class is a wrapper for unsigned_long
    * type value.
    */
class Cunsigned_long : public CGLtype<unsigned long, Cunsigned_long> {
public:
  Cunsigned_long();
  Cunsigned_long(unsigned long);
  static const unsigned LENGTH = sizeof(unsigned long);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for INT32 type
    *
    * gits::OpenGL::CINT32 class is a wrapper for INT32
    * type value.
    */
class CINT32 : public CGLtype<INT32, CINT32> {
public:
  CINT32();
  CINT32(INT32);
  static const unsigned LENGTH = sizeof(INT32);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for INT64 type
    *
    * gits::OpenGL::CINT64 class is a wrapper for INT64
    * type value.
    */
class CINT64 : public CGLtype<INT64, CINT64> {
public:
  CINT64();
  CINT64(INT64);
  static const unsigned LENGTH = sizeof(INT64);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for void * OpenGL type
    *
    * gits::OpenGL::CvoidPtr class is a wrapper for void * OpenGL
    * type value.
    */
class CvoidPtr : public CGLtype<void*, CvoidPtr> {
  void* _value;
  void** const _valuePtr;

public:
  static const unsigned LENGTH = sizeof(void*);
  static const char* NAME;
  CvoidPtr();
  CvoidPtr(void* value);
  //    CvoidPtr(void **valuePtr);

  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write(CCodeOStream& stream) const;

  virtual char* Buffer() {
    return 0;
  }
  virtual const char* Buffer() const {
    return 0;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  void*& operator*() {
    return _value;
  }
  void* const& operator*() const {
    return _value;
  }
};

/**
    * @brief Wrapper for char OpenGL type
    *
    * gits::OpenGL::Cchar class is a wrapper for GLclampd OpenGL
    * type value.
    */
class Cchar : public CGLtype<char, Cchar> {
public:
  Cchar();
  Cchar(char);
  static const unsigned LENGTH = sizeof(char);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};
} // namespace OpenGL
} // namespace gits
