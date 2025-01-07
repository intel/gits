// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   wglArguments.h
*
* @brief Declarations of OpenGL library function call argument wrappers.
*
*/

#pragma once

#include "argument.h"
#include "exception.h"
#include "platform.h"
#include "openglTypes.h"
#include "openglArguments.h"

#include <string>
#include <stdexcept>

namespace gits {
/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {

/**
    * @brief Wrapper for LPVOID type
    *
    * gits::OpenGL::CLPVOID class is a wrapper for LPVOID
    * type value.
    */
class CLPVOID : public CGLtype<void*, CLPVOID>, private CInvalidArgument {
public:
  CLPVOID();
  CLPVOID(void*);
  static const unsigned LENGTH = sizeof(void*);
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
    * @brief Wrapper for HVIDEOOUTPUTDEVICENV type
    *
    * gits::OpenGL::CHVIDEOOUTPUTDEVICENV class is a wrapper for HVIDEOOUTPUTDEVICENV
    * type value.
    */
class CHVIDEOOUTPUTDEVICENV : public CGLtype<void*, CHVIDEOOUTPUTDEVICENV>,
                              private CInvalidArgument {
public:
  CHVIDEOOUTPUTDEVICENV();
  CHVIDEOOUTPUTDEVICENV(void*);
  static const unsigned LENGTH = sizeof(void*);
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
    * @brief Wrapper for HPVIDEODEV type
    *
    * gits::OpenGL::CHPVIDEODEV class is a wrapper for HPVIDEODEV
    * type value.
    */
class CHPVIDEODEV : public CGLtype<void*, CHPVIDEODEV>, private CInvalidArgument {
public:
  CHPVIDEODEV();
  CHPVIDEODEV(void*);
  static const unsigned LENGTH = sizeof(void*);
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
    * @brief Wrapper for HPGPUNV type
    *
    * gits::OpenGL::CHPGPUNV class is a wrapper for HPGPUNV
    * type value.
    */
class CHPGPUNV : public CGLtype<void*, CHPGPUNV>, private CInvalidArgument {
public:
  CHPGPUNV();
  CHPGPUNV(void*);
  static const unsigned LENGTH = sizeof(void*);
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
    * @brief Wrapper for HGPUNV type
    *
    * gits::OpenGL::CHGPUNV class is a wrapper for HGPUNV
    * type value.
    */
class CHGPUNV : public CGLtype<void*, CHGPUNV>, private CInvalidArgument {
public:
  CHGPUNV();
  CHGPUNV(void*);
  static const unsigned LENGTH = sizeof(void*);
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
    * @brief Wrapper for HVIDEOINPUTDEVICENV type
    *
    * gits::OpenGL::CHVIDEOINPUTDEVICENV class is a wrapper for HVIDEOINPUTDEVICENV
    * type value.
    */
class CHVIDEOINPUTDEVICENV : public CGLtype<void*, CHVIDEOINPUTDEVICENV>, private CInvalidArgument {
public:
  CHVIDEOINPUTDEVICENV();
  CHVIDEOINPUTDEVICENV(void*);
  static const unsigned LENGTH = sizeof(void*);
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
    * @brief Wrapper for PIXELFORMATDESCRIPTOR type
    *
    * gits::OpenGL::CPIXELFORMATDESCRIPTOR class is a wrapper for PIXELFORMATDESCRIPTOR
    * type value.
    */
class CPIXELFORMATDESCRIPTOR : public CGLtype<PIXELFORMATDESCRIPTOR_, CPIXELFORMATDESCRIPTOR> {
public:
  CPIXELFORMATDESCRIPTOR();
  CPIXELFORMATDESCRIPTOR(PIXELFORMATDESCRIPTOR_);
  static const unsigned LENGTH = sizeof(PIXELFORMATDESCRIPTOR_);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
};
} // namespace OpenGL
} // namespace gits
