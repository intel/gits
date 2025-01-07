// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openglArguments.h"

namespace gits {
/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {
/**
    * @brief Wrapper for EGLint OpenGL type
    *
    * gits::OpenGL::CEGLint class is a wrapper for EGLint OpenGL
    * type value.
    */
class CEGLint : public CGLtype<EGLint, CEGLint> {
public:
  static const char* NAME;

  CEGLint();
  CEGLint(EGLint);

  virtual const char* Name() const {
    return "EGLint";
  }
  virtual unsigned Length() const {
    return sizeof(EGLint);
  }
  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for EGLBoolean OpenGL type
    *
    * gits::OpenGL::ECGLBoolean class is a wrapper for EGLBoolean OpenGL
    * type value.
    */
class CEGLBoolean : public CGLtype<EGLBoolean, CEGLBoolean> {
public:
  CEGLBoolean();
  CEGLBoolean(EGLBoolean);

  virtual const char* Name() const {
    return "EGLBoolean";
  }
  virtual unsigned Length() const {
    return sizeof(EGLBoolean);
  }
  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for EGLenum OpenGL type
    *
    * gits::OpenGL::CEGLenum class is a wrapper for EGLenum OpenGL
    * type value.
    */
class CEGLenum : public CGLtype<EGLenum, CEGLenum> {
public:
  CEGLenum();
  CEGLenum(EGLenum);

  virtual const char* Name() const {
    return "EGLenum";
  }
  virtual unsigned Length() const {
    return sizeof(EGLenum);
  }
  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief Wrapper for EGLConfig OpenGL type
    *
    * gits::OpenGL::CEGLConfig class is a wrapper for EGLConfig OpenGL
    * type value.
    */
class CEGLConfig : public CGLtype<EGLConfig, CEGLConfig, uint64_t> {
public:
  static const char* NAME;

  CEGLConfig();
  CEGLConfig(EGLConfig);

  virtual const char* Name() const {
    return "EGLConfig";
  }
  virtual unsigned Length() const {
    return sizeof(EGLConfig);
  }
  virtual void Write(CCodeOStream& stream) const;
};
} // namespace OpenGL
} // namespace gits
