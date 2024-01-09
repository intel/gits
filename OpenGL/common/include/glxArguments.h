// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   glxArguments.h
* 
* @brief Declarations of OpenGL library function call argument wrappers.
* 
*/

#pragma once

#include "argument.h"
#include "openglArguments.h"
#include "exception.h"
#include "openglTypes.h"

namespace gits {

/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {
/**
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CPixmap class is a wrapper for Pixmap OpenGL
    * type value.
    */
class CPixmap : public CGLtype<Pixmap, CPixmap, uint64_t> {
public:
  CPixmap();
  CPixmap(Pixmap);
  static const unsigned LENGTH = sizeof(Pixmap);
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
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CFont class is a wrapper for Font OpenGL
    * type value.
    */
class CFont : public CGLtype<Font, CFont, uint64_t> {
public:
  CFont();
  CFont(Font);
  static const unsigned LENGTH = sizeof(Font);
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
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CWindow class is a wrapper for Window OpenGL
    * type value.
    */
class CWindow : public CGLtype<Window, CWindow, uint64_t> {
public:
  CWindow();
  CWindow(Window);
  static const unsigned LENGTH = sizeof(Window);
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
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CGLXWindow class is a wrapper for GLXWindow OpenGL
    * type value.
    */
class CGLXWindow : public CGLtype<GLXWindow, CGLXWindow, uint64_t> {
public:
  CGLXWindow();
  CGLXWindow(GLXWindow);
  static const unsigned LENGTH = sizeof(GLXWindow);
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
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CGLXPbuffer class is a wrapper for GLXPbuffer OpenGL
    * type value.
    */
class CGLXPbuffer : public CGLtype<GLXPbuffer, CGLXPbuffer, uint64_t> {
public:
  CGLXPbuffer();
  CGLXPbuffer(GLXPbuffer);
  static const unsigned LENGTH = sizeof(GLXPbuffer);
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
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CGLXPixmap class is a wrapper for GLXPixmap OpenGL
    * type value.
    */
class CGLXPixmap : public CGLtype<GLXPixmap, CGLXPixmap, uint64_t> {
public:
  CGLXPixmap();
  CGLXPixmap(GLXPixmap);
  static const unsigned LENGTH = sizeof(GLXPixmap);
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
    * @brief Wrapper for char OpenGL type
    * 
    * gits::OpenGL::CGLXFBConfig class is a wrapper for GLXFBConfig OpenGL
    * type value.
    */
// typedef CMappedArgument<GLX_FBCONFIG_MAP, GLXFBConfig, NativeObjectTraits> CGLXFBConfig;

/**
    * @brief Wrapper for int64_t OpenGL type
    * 
    * gits::OpenGL::Cint64_t class is a wrapper for int64_t OpenGL
    * type value.
    */
class Cint64_t : public CGLtype<int64_t, Cint64_t> {
public:
  Cint64_t();
  Cint64_t(int64_t);
  static const unsigned LENGTH = sizeof(int64_t);
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
    * @brief Wrapper for unsigned long OpenGL type
    * 
    * gits::OpenGL::Culong class is a wrapper for unsigned long OpenGL
    * type value.
    */
class Culong : public CGLtype<unsigned long, Culong> {
public:
  Culong();
  Culong(unsigned long);
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
    * @brief Wrapper for unsigned long OpenGL type
    * 
    * gits::OpenGL::Cuint class is a wrapper for unsigned long OpenGL
    * type value.
    */
class Cuint : public CGLtype<unsigned int, Cuint> {
public:
  Cuint();
  Cuint(unsigned int);
  static const unsigned LENGTH = sizeof(unsigned int);
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
    * @brief Wrapper for unsigned long OpenGL type
    * 
    * gits::OpenGL::CBool class is a wrapper for unsigned long OpenGL
    * type value.
    */
class CBool : public CGLtype<Bool, CBool> {
public:
  CBool();
  CBool(Bool);
  static const unsigned LENGTH = sizeof(Bool);
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
    * @brief Wrapper for float type
    * 
    * gits::OpenGL::CCfloat class is a wrapper for float
    * type value.
    */
class Cfloat : public CGLtype<float, Cfloat> {
public:
  Cfloat();
  Cfloat(float);
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

} // namespace OpenGL

} // namespace gits
