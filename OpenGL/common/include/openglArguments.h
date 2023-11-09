// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglArguments.h
*
* @brief Declarations of OpenGL library function call argument wrappers.
*
*/

#pragma once

#include "argument.h"
#include "exception.h"
#include "key_value.h"
#include "platform.h"
#include "openglTypes.h"
#include "openglEnums.h"
#include "openglDrivers.h"
#include "stateDynamic.h"
#include "config.h"
#include "mapping.h"

#include <set>
DISABLE_WARNINGS
#include <boost/container/flat_set.hpp>
ENABLE_WARNINGS
#include <unordered_map>
#include <vector>

#include "streams.h"
#include "gits.h"
#include "tools.h"

#include <string>
#include <stdexcept>

namespace gits {
/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {

inline GLvoid* _glMapBuffer_wrap(GLint buffer, GLenum target, GLenum access) {
  return drv.gl.glMapBuffer(target, access);
}
inline GLvoid* _glMapBufferARB_wrap(GLint buffer, GLenum target, GLenum access) {
  return drv.gl.glMapBufferARB(target, access);
}
inline GLvoid* _glMapBufferOES_wrap(GLint buffer, GLenum target, GLenum access) {
  return drv.gl.glMapBufferOES(target, access);
}
inline GLvoid* _glMapNamedBufferEXT_wrap(GLint buffer, GLenum target, GLenum access) {
  return drv.gl.glMapNamedBufferEXT(buffer, access);
}
inline void _glUnmapBuffer_wrap(GLint buffer, GLenum target) {
  drv.gl.glUnmapBuffer(target);
}
inline void _glUnmapBufferARB_wrap(GLint buffer, GLenum target) {
  drv.gl.glUnmapBufferARB(target);
}
inline void _glUnmapBufferOES_wrap(GLint buffer, GLenum target) {
  drv.gl.glUnmapBufferOES(target);
}
inline void _glUnmapNamedBufferEXT_wrap(GLint buffer, GLenum target) {
  drv.gl.glUnmapNamedBufferEXT(buffer);
}
inline void _glGetBufferParameteriv_wrap(GLint buffer, GLenum target, GLenum pname, GLint* params) {
  drv.gl.glGetBufferParameteriv(target, pname, params);
}
inline void _glGetBufferParameterivARB_wrap(GLint buffer,
                                            GLenum target,
                                            GLenum pname,
                                            GLint* params) {
  drv.gl.glGetBufferParameterivARB(target, pname, params);
}
inline void _glGetNamedBufferParameterivEXT_wrap(GLint buffer,
                                                 GLenum target,
                                                 GLenum pname,
                                                 GLint* params) {
  drv.gl.glGetNamedBufferParameterivEXT(buffer, pname, params);
}
inline void _glGetBufferPointerv_wrap(GLint buffer, GLenum target, GLenum pname, GLvoid** params) {
  drv.gl.glGetBufferPointerv(target, pname, params);
}
inline void _glGetBufferPointervARB_wrap(GLint buffer,
                                         GLenum target,
                                         GLenum pname,
                                         GLvoid** params) {
  drv.gl.glGetBufferPointervARB(target, pname, params);
}
inline void _glGetBufferPointervOES_wrap(GLint buffer,
                                         GLenum target,
                                         GLenum pname,
                                         GLvoid** params) {
  drv.gl.glGetBufferPointervOES(target, pname, params);
}
inline void _glGetNamedBufferPointervEXT_wrap(GLint buffer,
                                              GLenum target,
                                              GLenum pname,
                                              GLvoid** params) {
  drv.gl.glGetNamedBufferPointervEXT(buffer, pname, params);
}

//*************************** CArgumentParamArray **************************
template <class T, class T_WRAP>
class CArgumentParamArray : public CArgument {
  std::vector<T> _array;
  typedef T_WRAP CGLtype;

public:
  CArgumentParamArray(size_t num = 0) {
    _array.resize(num);
  }
  CArgumentParamArray(GLenum pname, const T* array) {
    switch (pname) {
    case GL_PATCH_VERTICES:
    case GL_TEXTURE_GEN_MODE:
    case GL_SHININESS:
    case GL_FOG_MODE:
    case GL_FOG_DENSITY:
    case GL_FOG_START:
    case GL_FOG_END:
    case GL_FOG_INDEX:
    case GL_FOG_COORD_SRC:
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_LIGHT_MODEL_COLOR_CONTROL:
    case GL_TEXTURE_ENV_MODE:
    case GL_TEXTURE_LOD_BIAS:
    case GL_COMBINE_RGB:
    case GL_COMBINE_ALPHA:
    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
    case GL_RGB_SCALE:
    case GL_ALPHA_SCALE:
    case GL_COORD_REPLACE:
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_PRIORITY:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_COMPARE_FUNC:
    case GL_DEPTH_TEXTURE_MODE:
    case GL_GENERATE_MIPMAP:
    case GL_TEXTURE_SWIZZLE_R:
    case GL_TEXTURE_SWIZZLE_G:
    case GL_TEXTURE_SWIZZLE_B:
    case GL_TEXTURE_SWIZZLE_A:
    case GL_TEXTURE_MEMORY_LAYOUT_INTEL:
    case GL_TEXTURE_SRGB_DECODE_EXT:
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
    case GL_POINT_SIZE_MIN:
    case GL_POINT_SIZE_MAX:
    case GL_POINT_FADE_THRESHOLD_SIZE:
    case GL_POINT_SPRITE_COORD_ORIGIN:
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
    case GL_FRONT:
    case GL_BACK:
    case GL_LEFT:
    case GL_RIGHT:
    case GL_FRONT_AND_BACK:
    case GL_DEPTH:
    case GL_STENCIL:
      _array.assign(array, array + 1);
      break;
    case GL_PATCH_DEFAULT_INNER_LEVEL:
      _array.assign(array, array + 2);
      break;
    case GL_COLOR_INDEXES:
    case GL_SPOT_DIRECTION:
    case GL_POINT_DISTANCE_ATTENUATION:
      _array.assign(array, array + 3);
      break;
    case GL_PATCH_DEFAULT_OUTER_LEVEL:
    case GL_OBJECT_PLANE:
    case GL_EYE_PLANE:
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_AMBIENT_AND_DIFFUSE:
    case GL_POSITION:
    case GL_FOG_COLOR:
    case GL_LIGHT_MODEL_AMBIENT:
    case GL_TEXTURE_ENV_COLOR:
    case GL_TEXTURE_BORDER_COLOR:
    case GL_TEXTURE_SWIZZLE_RGBA:
    case GL_TEXTURE_CROP_RECT_OES:
    case GL_COLOR:
      _array.assign(array, array + 4);
      break;
    default:
      Log(ERR) << "Cannot create array for '" << Name() << "' and type: '0x" << std::hex << pname
               << "'!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }

  CArgumentParamArray(const std::vector<T>& array) : _array(array) {}

  virtual void Write(CBinOStream& stream) const {
    uint32_t size = ensure_unsigned32bit_representible<size_t>(_array.size());
    if (!Config::Get().recorder.extras.utilities.nullIO) {
      stream.write((char*)&size, sizeof(size));
    }

    if (size != 0) {
      for (unsigned idx = 0; idx < size; idx++) {
        CGLtype wrapper_(_array[idx]);
        stream << wrapper_;
      }
    }
  }

  virtual void Read(CBinIStream& stream) {
    uint32_t size;
    stream.read((char*)&size, sizeof(size));
    if (size != 0) {
      _array.resize(size);
      for (unsigned idx = 0; idx < size; idx++) {
        CGLtype wrapper_;
        stream >> wrapper_;
        _array[idx] = wrapper_.Original();
      }
    }
  }

  virtual void Write(CCodeOStream& stream) const {
    stream << stream.VariableName(ScopeKey());
  }

  virtual void Declare(CCodeOStream& stream) const {
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << "[] = ";
    // declare an array
    stream << "{ ";

    // initiate all elements in an array
    if ((int)_array.size() == 0) {
      stream << "0";
    } else {
      for (auto iter = _array.begin(); iter != _array.end(); ++iter) {
        CGLtype wrapper_(*iter);
        stream << wrapper_;
        if (iter < _array.end() - 1) {
          stream << ", ";
        }
      }
    }
    stream << " };\n";
  }

  std::vector<T>& Vector() {
    return _array;
  }
  const std::vector<T>& Vector() const {
    return _array;
  }

  virtual bool Array() const {
    return true;
  }
  T* operator*() {
    if (_array.empty()) {
      return 0;
    }
    return &_array[0];
  }
  const T* operator*() const {
    if (_array.empty()) {
      return 0;
    }
    return &_array[0];
  }

  virtual const char* Name() const {
    return T_WRAP::NAME;
  }
  virtual bool DeclarationNeeded() const {
    return true;
  }
};

//*************************** CArgumentGLMappedArray **************************
template <class T, class T_WRAP>
class CArgumentGLMappedArray : public CArgument {
  std::vector<T> _array;
  typedef T_WRAP CGLtype;

public:
  CArgumentGLMappedArray(size_t num = 0) {
    _array.resize(num);
  }
  CArgumentGLMappedArray(GLenum target, GLint order, const T* array) {
    switch (target) {
    case GL_MAP1_INDEX:
    case GL_MAP1_TEXTURE_COORD_1:
      _array.assign(array, array + order);
      break;
    case GL_MAP1_TEXTURE_COORD_2:
      _array.assign(array, array + order * 2);
      break;
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_3:
      _array.assign(array, array + order * 3);
      break;
    case GL_MAP1_VERTEX_4:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
      _array.assign(array, array + order * 4);
      break;

    default:
      Log(ERR) << "Cannot create array for '" << Name() << "' and target: '0x" << std::hex << target
               << "'!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }

  CArgumentGLMappedArray(GLenum target, GLint uorder, GLint vorder, const T* array) {
    switch (target) {
    case GL_MAP2_INDEX:
    case GL_MAP2_TEXTURE_COORD_1:
      _array.assign(array, array + uorder * vorder);
      break;
    case GL_MAP2_TEXTURE_COORD_2:
      _array.assign(array, array + uorder * vorder * 2);
      break;
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_3:
      _array.assign(array, array + uorder * vorder * 3);
      break;
    case GL_MAP2_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
      _array.assign(array, array + uorder * vorder * 4);
      break;
    default:
      Log(ERR) << "Cannot create array for '" << Name() << "' and target: '0x" << std::hex << target
               << "'!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }

  CArgumentGLMappedArray(const std::vector<T>& array) : _array(array) {}

  virtual void Write(CBinOStream& stream) const {
    uint32_t size = ensure_unsigned32bit_representible<size_t>(_array.size());
    if (!Config::Get().recorder.extras.utilities.nullIO) {
      stream.write((char*)&size, sizeof(size));
    }

    if (size != 0) {
      for (unsigned idx = 0; idx < size; idx++) {
        CGLtype wrapper_(_array[idx]);
        stream << wrapper_;
      }
    }
  }

  virtual void Read(CBinIStream& stream) {
    uint32_t size;
    stream.read((char*)&size, sizeof(size));
    if (size != 0) {
      _array.resize(size);
      for (unsigned idx = 0; idx < size; idx++) {
        CGLtype wrapper_;
        stream >> wrapper_;
        _array[idx] = wrapper_.Original();
      }
    }
  }

  virtual void Write(CCodeOStream& stream) const {
    stream << stream.VariableName(ScopeKey());
  }

  virtual void Declare(CCodeOStream& stream) const {
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << "[] = ";
    // declare an array
    stream << "{ ";

    // initiate all elements in an array
    if ((int)_array.size() == 0) {
      stream << "0";
    } else {
      for (auto iter = _array.begin(); iter != _array.end(); ++iter) {
        CGLtype wrapper_(*iter);
        stream << wrapper_;
        if (iter < _array.end() - 1) {
          stream << ", ";
        }
      }
    }
    stream << " };\n";
  }

  std::vector<T>& Vector() {
    return _array;
  }
  const std::vector<T>& Vector() const {
    return _array;
  }

  virtual bool Array() const {
    return true;
  }
  T* operator*() {
    if (_array.empty()) {
      return 0;
    }
    return &_array[0];
  }
  const T* operator*() const {
    if (_array.empty()) {
      return 0;
    }
    return &_array[0];
  }

  virtual const char* Name() const {
    return T_WRAP::NAME;
  }
  virtual bool DeclarationNeeded() const {
    return true;
  }
};

template <class T, class WrittenType>
struct ReadAsType {
  static void read(CBinIStream& i, T& value) {
    WrittenType to_read;
    memset(&to_read, 0, sizeof(to_read));

    read_from_stream(i, to_read);
    value = (T)to_read;
  }
};

template <class T>
struct ReadAsType<T, T> {
  static void read(CBinIStream& i, T& value) {
    read_from_stream(i, value);
  }
};

/**
    * @brief Base class for wrappers for OpenGL types
    *
    * BaseType OpenGL type to be wrapped
    * DerivedType Wrapper type to be created
    *
    * gits::OpenGL::CGLtype class is a base class for wrappers for
    * OpenGL types values.
    *
    * all wrappers should derive from CGLtype:
    * CGLtypeWrapper : public CGLtype<GLtype,CGLtypeWrapper> { ...
    *
    */
template <typename BaseType, typename DerivedType, typename WrittenType = BaseType>
class CGLtype : public gits::CArgument {
public:
  typedef BaseType GLtype;
  typedef DerivedType type;

  template <int N>
  struct CFArray {
    typedef CArgumentFixedArray<GLtype, N, DerivedType> type;
  };
  typedef CArgumentSizedArray<GLtype, DerivedType> CSArray;
  typedef CArgumentParamArray<GLtype, DerivedType> CSParamArray;
  typedef CArgumentGLMappedArray<GLtype, DerivedType> CGLMapArray;

  CGLtype() : _value(){};
  CGLtype(const GLtype value) : _value(value){};

  virtual const char* Name() const = 0;
  virtual void Write(CCodeOStream& stream) const = 0;

  virtual void Write(CBinOStream& stream) const {
    WrittenType to_write = (WrittenType)_value;
    write_to_stream(stream, to_write);
  }

  bool Array() const {
    return false;
  }

  virtual void Read(CBinIStream& stream) {
    ReadAsType<BaseType, WrittenType>::read(stream, _value);
  }

  void Assign(const GLtype val) {} // Assign method does nothing for non mapped arguments

  GLtype& Value() {
    return _value;
  }
  const GLtype& Value() const {
    return _value;
  }
  GLtype& operator*() {
    return _value;
  }
  const GLtype& operator*() const {
    return _value;
  }
  GLtype& Original() {
    return _value;
  }
  const GLtype& Original() const {
    return _value;
  }

protected:
  GLtype _value;
};

/**
    * @brief A helper class to mark an existing CArgument class as not supported.
    *
    * The target CArgument class needs to derive from this. Note that this is supposed to be used only in cases where a CArgument class
    * is not to be used in its existing form or need modifications for correct behavior.
    */
class CInvalidArgument {
public:
  CInvalidArgument() {
    Log(ERR) << "Unsupported CArgument-derived data type. " << (std::string)EXCEPTION_MESSAGE
             << std::endl;
    UNREACHABLE_CODE_WA
    throw ENotSupported(EXCEPTION_MESSAGE);
  }
};

/**
    * @brief Wrapper for GLenum OpenGL type
    *
    * gits::OpenGL::CGLenum class is a wrapper for GLenum OpenGL
    * type value.
    */
class CGLenum : public CGLtype<GLenum, CGLenum, GLushort> {
  struct TEnumName {
    GLenum value;
    const char* name;
  };

  static TEnumName _enumNames[];

public:
  CGLenum();
  CGLenum(GLenum);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }

  virtual void Write(CCodeOStream& stream) const;
  static std::string EnumString(GLenum value);
};

/**
    * @brief Wrapper for GLboolean OpenGL type
    *
    * gits::OpenGL::CGLboolean class is a wrapper for GLboolean OpenGL
    * type value.
    */
class CGLboolean : public CGLtype<GLboolean, CGLboolean> {
public:
  CGLboolean();
  CGLboolean(GLboolean);
  static const unsigned LENGTH = sizeof(GLboolean);
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
    * @brief Wrapper for GLbitfield OpenGL type
    *
    * gits::OpenGL::CGLbitfield class is a wrapper for GLbitfield OpenGL
    * type value.
    */
class CGLbitfield : public CGLtype<GLbitfield, CGLbitfield> {
public:
  CGLbitfield();
  CGLbitfield(GLbitfield);
  static const unsigned LENGTH = sizeof(GLbitfield);
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
    * @brief Wrapper for GLbyte OpenGL type
    *
    * gits::OpenGL::CGLbyte class is a wrapper for GLbyte OpenGL
    * type value.
    */
class CGLbyte : public CGLtype<GLbyte, CGLbyte> {
public:
  CGLbyte();
  CGLbyte(GLbyte);
  static const unsigned LENGTH = sizeof(GLbyte);
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
    * @brief Wrapper for GLshort OpenGL type
    *
    * gits::OpenGL::CGLshort class is a wrapper for GLshort OpenGL
    * type value.
    */
class CGLshort : public CGLtype<GLshort, CGLshort> {
public:
  CGLshort();
  CGLshort(GLshort);
  static const unsigned LENGTH = sizeof(GLshort);
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
    * @brief Wrapper for GLint OpenGL type
    *
    * gits::OpenGL::CGLint class is a wrapper for GLint OpenGL
    * type value.
    */
class CGLint : public CGLtype<GLint, CGLint> {
public:
  CGLint();
  CGLint(GLint);
  static const unsigned LENGTH = sizeof(GLint);
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
    * @brief Wrapper for GLint64 OpenGL type
    *
    * gits::OpenGL::CGLint64 class is a wrapper for GLint64 OpenGL
    * type value.
    */
class CGLint64 : public CGLtype<GLint64, CGLint64> {
public:
  CGLint64();
  CGLint64(GLint64);
  static const unsigned LENGTH = sizeof(GLint64);
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
    * @brief Wrapper for GLsizei OpenGL type
    *
    * gits::OpenGL::CGLsizei class is a wrapper for GLsizei OpenGL
    * type value.
    */
class CGLsizei : public CGLtype<GLsizei, CGLsizei> {
public:
  CGLsizei();
  CGLsizei(GLsizei);
  static const unsigned LENGTH = sizeof(GLsizei);
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
    * @brief Wrapper for GLubyte OpenGL type
    *
    * gits::OpenGL::CGLubyte class is a wrapper for GLubyte OpenGL
    * type value.
    */
class CGLubyte : public CGLtype<GLubyte, CGLubyte> {
public:
  CGLubyte();
  CGLubyte(GLubyte);
  static const unsigned LENGTH = sizeof(GLubyte);
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
    * @brief Wrapper for GLushort OpenGL type
    *
    * gits::OpenGL::CGLushort class is a wrapper for GLushort OpenGL
    * type value.
    */
class CGLushort : public CGLtype<GLushort, CGLushort> {
public:
  CGLushort();
  CGLushort(GLushort);
  static const unsigned LENGTH = sizeof(GLushort);
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
    * @brief Wrapper for GLuint OpenGL type
    *
    * gits::OpenGL::CGLuint class is a wrapper for GLuint OpenGL
    * type value.
    */
class CGLuint : public CGLtype<GLuint, CGLuint> {
public:
  CGLuint();
  CGLuint(GLuint);
  static const unsigned LENGTH = sizeof(GLuint);
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
    * @brief Wrapper for GLuint64 OpenGL type
    *
    * gits::OpenGL::CGLuint64 class is a wrapper for GLuint64 OpenGL
    * type value.
    */
class CGLuint64 : public CGLtype<GLuint64, CGLuint64> {
public:
  CGLuint64();
  CGLuint64(GLuint64);
  static const unsigned LENGTH = sizeof(GLuint64);
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
    * @brief Wrapper for GLfloat OpenGL type
    *
    * gits::OpenGL::CGLfloat class is a wrapper for GLfloat OpenGL
    * type value.
    */
class CGLfloat : public CGLtype<GLfloat, CGLfloat> {
public:
  CGLfloat();
  CGLfloat(GLfloat);
  static const unsigned LENGTH = sizeof(GLfloat);
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
    * @brief Wrapper for GLclampf OpenGL type
    *
    * gits::OpenGL::CGLclampf class is a wrapper for GLclampf OpenGL
    * type value.
    */
class CGLclampf : public CGLtype<GLclampf, CGLclampf> {
public:
  CGLclampf();
  CGLclampf(GLclampf);
  static const unsigned LENGTH = sizeof(GLclampf);
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
    * @brief Wrapper for GLdouble OpenGL type
    *
    * gits::OpenGL::CGLdouble class is a wrapper for GLdouble OpenGL
    * type value.
    */
class CGLdouble : public CGLtype<GLdouble, CGLdouble> {
public:
  CGLdouble();
  CGLdouble(GLdouble);
  static const unsigned LENGTH = sizeof(GLdouble);
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
    * @brief Wrapper for GLclampd OpenGL type
    *
    * gits::OpenGL::CGLclampd class is a wrapper for GLclampd OpenGL
    * type value.
    */
class CGLclampd : public CGLtype<GLclampd, CGLclampd> {
public:
  CGLclampd();
  CGLclampd(GLclampd);
  static const unsigned LENGTH = sizeof(GLclampd);
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
    * @brief Wrapper for GLchar OpenGL type
    *
    * gits::OpenGL::CGLchar class is a wrapper for GLclampd OpenGL
    * type value.
    */
class CGLchar : public CGLtype<GLchar, CGLchar> {
public:
  CGLchar();
  CGLchar(GLchar);
  static const unsigned LENGTH = sizeof(GLchar);
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
    * @brief Wrapper for GLcharARB OpenGL type
    *
    * gits::OpenGL::CGLcharARB class is a wrapper for GLcharARB OpenGL
    * type value.
    */
class CGLcharARB : public CGLtype<GLcharARB, CGLcharARB> {
public:
  CGLcharARB();
  CGLcharARB(GLcharARB);
  static const unsigned LENGTH = sizeof(GLcharARB);
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
    * @brief Wrapper for GLhalfNV OpenGL type
    *
    * gits::OpenGL::CGLhalfNV  class is a wrapper for GLcharARB OpenGL
    * type value.
    */
class CGLhalfNV : public CGLtype<GLhalfNV, CGLhalfNV> {
public:
  CGLhalfNV();
  CGLhalfNV(GLhalfNV);
  static const unsigned LENGTH = sizeof(GLhalfNV);
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
    * @brief Wrapper for GLhandleARB OpenGL type
    *
    * gits::OpenGL::CGLhandleARB class is a wrapper for GLhandleARB OpenGL
    * type value.
    */
class CGLhandleARB : public CGLtype<GLhandleARB, CGLhandleARB> {
public:
  CGLhandleARB();
  CGLhandleARB(GLhandleARB);
  static const unsigned LENGTH = sizeof(GLhandleARB);
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
    * @brief Wrapper for GLint64EXT OpenGL type
    *
    * gits::OpenGL::CGLint64EXT class is a wrapper for GLint64EXT OpenGL
    * type value.
    */
class CGLint64EXT : public CGLtype<GLint64EXT, CGLint64EXT> {
public:
  CGLint64EXT();
  CGLint64EXT(GLint64EXT);
  static const unsigned LENGTH = sizeof(GLint64EXT);
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
    * @brief Wrapper for GLuint64EXT OpenGL type
    *
    * gits::OpenGL::CGLuint64EXT class is a wrapper for GLuint64EXT OpenGL
    * type value.
    */
class CGLuint64EXT : public CGLtype<GLuint64EXT, CGLuint64EXT> {
public:
  CGLuint64EXT();
  CGLuint64EXT(GLuint64EXT);
  static const unsigned LENGTH = sizeof(GLuint64EXT);
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
    * @brief Wrapper for GLsizeiptrARB OpenGL type
    *
    * gits::OpenGL::CGLsizeiptrARB class is a wrapper for GLsizeiptrARB OpenGL
    * type value.
    */
class CGLsizeiptrARB : public CGLtype<int32_t, CGLsizeiptrARB> {
public:
  CGLsizeiptrARB();
  CGLsizeiptrARB(GLsizeiptrARB);
  static const unsigned LENGTH = sizeof(int32_t);
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
    * @brief Wrapper for GLintptrARB OpenGL type
    *
    * gits::OpenGL::CGLintptrARB class is a wrapper for CGLintptrARB OpenGL
    * type value.
    */
class CGLintptrARB : public CGLtype<int32_t, CGLintptrARB> {
public:
  CGLintptrARB();
  CGLintptrARB(GLintptrARB);
  static const unsigned LENGTH = sizeof(int32_t);
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
    * @brief Wrapper for GLsizeiptr OpenGL type
    *
    * gits::OpenGL::CGLsizeiptr class is a wrapper for GLsizeiptr OpenGL
    * type value.
    */
class CGLsizeiptr : public CGLtype<int32_t, CGLsizeiptr> {
public:
  CGLsizeiptr();
  CGLsizeiptr(GLsizeiptr);
  static const unsigned LENGTH = sizeof(int32_t);
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
    * @brief Wrapper for GLintptr OpenGL type
    *
    * gits::OpenGL::CGLintptr class is a wrapper for GLintptr OpenGL
    * type value.
    */
class CGLintptr : public CGLtype<int32_t, CGLintptr> {
public:
  CGLintptr();
  CGLintptr(GLintptr);
  static const unsigned LENGTH = sizeof(int32_t);
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
    * @brief Wrapper for GLvoid* OpenGL type
    *
    * gits::OpenGL::CGLvoid_ptr class is a wrapper for GLvoid* OpenGL
    * type value.
    */
class CGLvoid_ptr : public CGLtype<uint64_t, CGLvoid_ptr> {
public:
  CGLvoid_ptr();
  CGLvoid_ptr(GLvoid* value);
  CGLvoid_ptr(const GLvoid* value);
  static const unsigned LENGTH = sizeof(uint64_t);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }
  virtual void Write(CCodeOStream& stream) const;
  void Assign(const void* val) {} // Assign method is do nothing for non mapped arguments

  GLvoid* operator*() {
    return reinterpret_cast<GLvoid*>(_value);
  }
  GLvoid* const operator*() const {
    return reinterpret_cast<GLvoid* const>(_value);
  }
  GLvoid* Original() {
    return reinterpret_cast<GLvoid*>(_value);
  }
};

/**
     * @brief Wrapper for GLvoid* to a texture
     *
     * gits::OpenGL::CGLMappedTexturePtr class is a wrapper for a void* that
     * points to a texture. It maps an invalid pointer read from stream to a
     * valid pointer acquired from the driver.
     */
class CGLMappedTexturePtr : public CGLvoid_ptr {
public:
  using CGLvoid_ptr::CGLvoid_ptr;

  void Assign(const void* val);

  GLvoid* Value();
  GLvoid* const Value() const;
  GLvoid* operator*();
  GLvoid* const operator*() const;

private:
  GLvoid* _mappedValue = nullptr;
};

/**
    * @brief Wrapper for GLconstchar_ptr OpenGL type
    *
    * gits::OpenGL::GLconstchar_ptr class is a wrapper for GLConstChar* OpenGL
    * type value.
    */
class CGLconstuchar_ptr : public CGLtype<uint64_t, CGLconstuchar_ptr> {
public:
  CGLconstuchar_ptr();
  CGLconstuchar_ptr(const unsigned char* value);
  static const unsigned LENGTH = sizeof(uint64_t);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }
  virtual void Write(CCodeOStream& stream) const;
  void Assign(const void* val) {} // Assign method is do nothing for non mapped arguments

  const unsigned char* operator*() {
    return reinterpret_cast<const unsigned char*>(_value);
  }
  const unsigned char* const operator*() const {
    return reinterpret_cast<const unsigned char* const>(_value);
  }
  const unsigned char* Original() {
    return reinterpret_cast<const unsigned char*>(_value);
  }
};

/**
    * @brief Wrapper for GLvoid * OpenGL type
    *
    * gits::OpenGL::CGLvoidPtr class is a wrapper for GLvoid * OpenGL
    * type value.
    */
//########IMPORTANT############
// For compatibility between 32/64 bit streams we normalize all pointer types to
// 32 bit unsigned integers. The current usage of this type in various contexts
// is either incorrect (i.e. as a CArray/CSArray) or will be replaced in future
// with buffer management implemented by ResourceMgr.
class CGLvoidPtr : public gits::CArgumentBuffer {
  uint32_t _value;

public:
  typedef void* GLtype;
  static const unsigned LENGTH = sizeof(uint32_t);
  static const char* NAME;
  CGLvoidPtr();
  CGLvoidPtr(GLvoid* value);

  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write(CCodeOStream& stream) const;
  using gits::CArgumentBuffer::Write;
  virtual char* Buffer() {
    return 0;
  }
  virtual const char* Buffer() const {
    return 0;
  }
  virtual size_t Length() const {
    return LENGTH;
  }

  GLvoid* operator*() {
    return reinterpret_cast<GLvoid*>(_value);
  }
  GLvoid* const operator*() const {
    return reinterpret_cast<GLvoid* const>(_value);
  }

  GLvoid* Original() {
    return reinterpret_cast<GLvoid*>(_value);
  }

private:
  void operator=(const CGLvoidPtr&); // no implementation
};

/**
    * @brief Wrapper for GLvoid * OpenGL type
    *
    * gits::OpenGL::CGLvoidPtr class is a wrapper for GLvoid * OpenGL
    * type value.
    */
//########IMPORTANT############
// For compatibility between 32/64 bit streams we normalize all pointer types to
// 32 bit unsigned integers. The current usage of this type in various contexts
// is either incorrect (i.e. as a CArray/CSArray) or will be replaced in future
// with buffer management implemented by ResourceMgr.
class CGLconstVoidPtr : public gits::CArgumentBuffer {
  const uint32_t _value;

public:
  static const unsigned LENGTH = sizeof(uint32_t);
  static const char* NAME;
  CGLconstVoidPtr();
  CGLconstVoidPtr(const GLvoid* value);

  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write(CCodeOStream& stream) const;
  using gits::CArgumentBuffer::Write;

  virtual char* Buffer() {
    return reinterpret_cast<char*>(const_cast<uint32_t*>(&_value));
  }
  virtual const char* Buffer() const {
    return reinterpret_cast<const char*>(&_value);
  }
  virtual size_t Length() const {
    return LENGTH;
  }

  const GLvoid* const operator*() const {
    return reinterpret_cast<GLvoid* const>(_value);
  }

  const GLvoid* const Original() const {
    return reinterpret_cast<GLvoid* const>(_value);
  }
};

// Dummy argument used in place of GL tokens that currently lack correct
// implementation
class CBadArg : public gits::CArgument, private CInvalidArgument {
public:
  CBadArg() {}
  template <class T>
  CBadArg(T) {}

  struct any_t {
    template <class T>
    operator T() {
      return T();
    }
  };

  virtual const char* Name() const {
    return "";
  }
  virtual void Write(CBinOStream& stream) const {}
  virtual void Read(CBinIStream& stream) {}
  virtual void Write(CCodeOStream& stream) const {}

  any_t operator*() {
    return any_t();
  }
};

//*************************** CShaderSource ********************************

class CShaderSource : public CArgumentFileText {
  static unsigned _shaderSourceIdx;
  static unsigned _shaderProgramIdx;
  static unsigned _programStringIdx;
  const char* text_cstr;

public:
  enum ShaderType { SHADER_SOURCE, SHADER_PROGRAM, PROGRAM_STRING };
  CShaderSource() {}
  CShaderSource(GLuint shaderObj,
                GLsizei count,
                const GLchar* const* string,
                const GLint* length,
                ShaderType shaderType);
  CShaderSource(
      const GLchar* string, GLsizei length, ShaderType shaderType, GLenum target, GLenum format);
  CShaderSource(GLsizei count, const GLchar* const* string, ShaderType shaderType);
  std::string GetShaderFileName() const {
    return FileName();
  }

  const char** Value();

  struct PtrConverter {
  private:
    const char** _ptr;

  public:
    explicit PtrConverter(const char** ptr) : _ptr(ptr) {}
    operator const char*() const {
      return *_ptr;
    }
    operator const char**() const {
      return _ptr;
    }
    operator const void*() const {
      return *_ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }

private:
  std::string GetFileName(ShaderType shaderType, GLenum target, GLenum format, GLuint shaderObj);
  std::string GetShaderSource(const GLchar* const* string, GLsizei count, const GLint* length);
  std::string GetShaderSource(const GLchar* const* string, GLsizei count);
};

//*************************** CGLintZero ********************************

class CGLintptrZero : public CArgument {
public:
  CGLintptrZero();
  static const unsigned LENGTH = 0;
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  const GLint* operator*() {
    return (const GLint*)0;
  }

  virtual void Write(CCodeOStream& stream) const;
  virtual void Write(CBinOStream& stream) const {}
  virtual void Read(CBinIStream& stream) {}
};

//*************************** CDataPtr *************************************
// This class is handling buffer offset and pointers to data in client memory.
// In gitsPlayer buffer offsets are being returned with the value from recorder.
// Client memory pointers are being recalculated to player memory area.
// If particular area is not allocated yet it also makes a memory allocation.
#define GITS_MEM_AREA_SIZE 4096;
template <class T>
uint64_t GetAreaPtr(T ptr) {
  return (uint64_t)ptr - (uint64_t)ptr % GITS_MEM_AREA_SIZE;
}
template <class T>
uint64_t GetAreaOffset(T ptr) {
  return (uint64_t)ptr % GITS_MEM_AREA_SIZE;
}

class CDataPtr : public CArgument {
protected:
  CGLuint64 _ptr;
  CGLboolean _isBuff;

  CDataPtr();
  CDataPtr(const void* ptr);
  CDataPtr(GLenum target, const void* ptr);
  CDataPtr(GLint buffer, GLenum target, const void* ptr);

public:
  virtual const char* Name() const {
    return "";
  }
  virtual unsigned Length() const {
    return sizeof(_ptr) + sizeof(_isBuff);
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  void* Value();
  void* operator*() {
    return Value();
  }
  bool IsBuff() {
    return (bool)*_isBuff;
  }
};

class CDataPtrArray : public CArgument {
protected:
  CGLuint64::CSArray _ptrsRecorder;
  std::vector<const void*> _ptrsPlayer;
  CGLboolean _isBuff;

public:
  CDataPtrArray();
  CDataPtrArray(const void** ptrs, size_t size);
  CDataPtrArray(GLenum target, const void* const* ptrs, size_t size);
  CDataPtrArray(GLint buffer, GLenum target, const void** ptrs, size_t size);

  virtual const char* Name() const {
    return "";
  }
  virtual unsigned Length() const {
    return (unsigned)(sizeof(uint64_t) * _ptrsRecorder.Vector().size() + sizeof(_isBuff));
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const;

  const void** Value();
  const void** operator*() {
    return Value();
  }
  bool IsBuff() {
    return (bool)*_isBuff;
  }
};

class CAttribPtr : public CDataPtr {
public:
  CAttribPtr() {}
  CAttribPtr(const void* ptr) : CDataPtr(GL_ARRAY_BUFFER, ptr) {}
  CAttribPtr(GLint buffer, const void* ptr) : CDataPtr(buffer, GL_ARRAY_BUFFER, ptr) {}
};

class CIndexPtr : public CDataPtr {
public:
  CIndexPtr() {}
  CIndexPtr(const void* ptr) : CDataPtr(GL_ELEMENT_ARRAY_BUFFER, ptr) {}
  CIndexPtr(GLint buffer, const void* ptr) : CDataPtr(buffer, GL_ELEMENT_ARRAY_BUFFER, ptr) {}
};

class CIndirectPtr : public CDataPtr {
public:
  CIndirectPtr() {}
  CIndirectPtr(const void* ptr) : CDataPtr(GL_DRAW_INDIRECT_BUFFER, ptr) {}
  CIndirectPtr(GLint buffer, const void* ptr) : CDataPtr(buffer, GL_DRAW_INDIRECT_BUFFER, ptr) {}
};

//************************************** CDataUpdate ********************************************************

// This class is handling client memory update. It consists of multiple diffs to
// multiple areas of client memory pointed by: area (mem page address on
// recorder side) and offset. It stores also hash to dumped data.
class CDataUpdate : public CArgument {
  struct TData {
    uint64_t area;
    uint64_t hash;
    uint64_t offset;
    std::vector<char> playerCache;
    TData(uint64_t area, uint64_t hash, uint64_t offset) : area(area), hash(hash), offset(offset) {}
    TData() : area(0), hash(0), offset(0) {}
  };
  std::vector<TData> _updates;
  // Do not compare any data simply creates diff from entire pointed data
  void DiffAll(uint64_t dataptr, uint64_t updateptr, uint64_t updatesize);
  // Compares data and creates one diff starting at first different byte and
  // ending at last different byte
  void DiffOneRange(uint64_t dataptr, uint64_t updateptr, uint64_t updatesize);
  // Compares data blocks of specified size and dumps as many diffs as different
  // blocks.
  void DiffMultiRange(uint64_t dataptr, uint64_t updateptr, uint64_t updatesize);

public:
  CDataUpdate() {}
  virtual const char* Name() const {
    return "";
  }
  virtual unsigned Length() const {
    return (unsigned)(_updates.size() * sizeof(TData));
  }
  // Creates memory diff using selected method.
  void Diff(uint64_t dataptr, uint64_t updateptr, uint64_t updatesize);
  // Player exclusive method, applies data update to memory.
  void Apply();
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

//************************************** CCoherentBufferUpdate **************************************

class CCoherentBufferUpdate : public CArgument {

public:
  struct TCoherentBufferData {
    uint64_t _hash;
    uint32_t _offset;
    uint32_t _buffer_name;
    uint32_t _length;
    uint32_t _target;
    std::vector<char> playerCache;
    enum UpdateType { TEXTURE_UPDATE, PER_DRAWCALL_UPDATE, PER_FRAME_UPDATE };
    enum UpdateMode { UPDATE_ALL, UPDATE_BOUND };

    TCoherentBufferData(
        uint64_t hash, uint32_t offset, uint32_t buffer_name, uint32_t length, uint32_t target)
        : _hash(hash),
          _offset(offset),
          _buffer_name(buffer_name),
          _length(length),
          _target(target) {}
    TCoherentBufferData() : _hash(0), _offset(0), _buffer_name(0), _length(0), _target(0) {}
  };
  CCoherentBufferUpdate() {}
  virtual const char* Name() const {
    return "";
  }
  virtual unsigned Length() const {
    return (unsigned)(_updates.size() * sizeof(TCoherentBufferData));
  }
  std::unordered_map<GLuint, GLenum> GetCurrentBuffers(TCoherentBufferData::UpdateType updateType,
                                                       bool oncePerFrame);
  void Diff(TCoherentBufferData::UpdateType updateType,
            TCoherentBufferData::UpdateMode updateMode,
            bool oncePerFrame);
  void Apply();
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const {};

private:
  std::vector<TCoherentBufferData> _updates;
};

// template class serves as a wrapper for other arguments which may be present
// or not wrapper takes ownership of argument with which it is initialized
template <class CArgumentType>
class CArgumentOptional : public CArgument {
  CGLboolean _initialized;
  CArgumentType* _heldArg;

public:
  explicit CArgumentOptional(CArgumentType* init)
      : _initialized((init == 0) ? GL_FALSE : GL_TRUE), _heldArg(init) {}

  CArgumentOptional() : _initialized(GL_FALSE), _heldArg(0) {}

  void Reset(CArgumentType* init) {
    delete _heldArg;
    _heldArg = init;
    *_initialized = init ? GL_TRUE : GL_FALSE;
  }

  ~CArgumentOptional() {
    Reset(0);
  }

  bool Initialized() const {
    return *_initialized == GL_TRUE;
  }

  CArgumentType& Argument() {
    if (*_initialized == GL_FALSE) {
      throw ENotInitialized(EXCEPTION_MESSAGE);
    }
    return *_heldArg;
  }

  const CArgumentType& Argument() const {
    if (*_initialized == GL_FALSE) {
      throw ENotInitialized(EXCEPTION_MESSAGE);
    }
    return *_heldArg;
  }

  virtual const char* Name() const {
    if (*_initialized == GL_TRUE) {
      return _heldArg->Name();
    } else {
      return "";
    }
  }

  virtual void Write(CBinOStream& stream) const {
    stream << _initialized;
    if (*_initialized == GL_TRUE) {
      stream << *_heldArg;
    }
  }

  virtual void Read(CBinIStream& stream) {
    stream >> _initialized;
    if (*_initialized == GL_TRUE) {
      Reset(new CArgumentType);
      stream >> *_heldArg;
    }
  }

  virtual void Write(CCodeOStream& stream) const {
    if (*_initialized == GL_TRUE) {
      stream << *_heldArg;
    }
  }
};

void IndicesDataUpdateRecorder(GLsizei count,
                               GLint basevertex,
                               GLenum type,
                               const GLvoid* indices,
                               GLint boundElementsBuffer,
                               CArgument* additionalArgs[]);
void IndicesDataUpdatePlayer(GLenum* type,
                             CBinaryResource::PointerProxy& indices,
                             GLint boundElementsBuffer,
                             CArgument* const additionalArgs[]);

/**
    * @brief Wrapper for samplers
    *
    * gits::OpenGL::CGLSamplerType class is a wrapper for samplers names
    */
class CGLSamplerType : public CGLtype<GLuint, CGLSamplerType> {
public:
  CGLSamplerType() {}
  CGLSamplerType(GLuint value) : CGLtype<GLtype, type>(value) {}
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return sizeof(GLuint);
  }
  virtual void Write(CCodeOStream& stream) const;
};

/* @brief Wrapper for GLclampx OpenGL type
    *
    * gits::OpenGL::CGLclampx class is a wrapper for GLclampx OpenGL
    * type value.
    */
class CGLclampx : public CGLtype<GLclampx, CGLclampx> {
public:
  CGLclampx();
  CGLclampx(GLclampx);
  static const unsigned LENGTH = sizeof(GLclampx);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }

  virtual void Write(CCodeOStream& stream) const;
};

/* @brief Wrapper for GLfixed OpenGL type
    *
    * gits::OpenGL::CGLfixed class is a wrapper for GLfixed OpenGL
    * type value.
    */
class CGLfixed : public CGLtype<GLfixed, CGLfixed> {
public:
  CGLfixed();
  CGLfixed(GLfixed);
  static const unsigned LENGTH = sizeof(GLfixed);
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
    * @brief Wrapper for GLsync OpenGL type
    *
    * gits::OpenGL::CGLsyncType class is a wrapper for GLsync OpenGL
    * type value.
    */
class CGLsyncType : public CGLtype<GLsync, CGLsyncType> {
public:
  CGLsyncType();
  CGLsyncType(GLsync);
  static const unsigned LENGTH = sizeof(GLsync);
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
     * @brief Wrapper for an map of arguments
     *
     */
class CAttribsMap : public CArgument {
public:
  typedef std::unordered_map<std::string, GLint> map_t;

  CAttribsMap() {}
  CAttribsMap(const map_t& attribMap) : _data(attribMap) {}

  map_t& Map() {
    return _data;
  }
  const map_t& Map() const {
    return _data;
  }

  virtual bool Array() const {
    return false;
  }
  virtual const char* Name() const {
    return "attribute_map";
  }
  virtual void Write(CBinOStream& stream) const {
    stream << CGLint(static_cast<GLint>(_data.size()));
    map_t::const_iterator iter = _data.begin();
    for (; iter != _data.end(); ++iter) {
      stream << CGLchar::CSArray(iter->first.c_str(), '\0', 1);
      stream << CGLint(iter->second);
    }
  }
  virtual void Read(CBinIStream& stream) {
    CGLint size;
    stream >> size;
    for (int i = 0; i < *size; ++i) {
      CGLchar::CSArray name;
      stream >> name;
      CGLint location;
      stream >> location;
      _data[&name.Vector().at(0)] = *location;
    }
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << "/*CAttribsMap - not implemented in CCode*/";
  }

private:
  map_t _data;
};

class CStringArray : public CArgument {
  typedef CGLchar::CSArray CGLString;
  std::vector<CGLString*> _array;
  std::vector<GLchar*> _rawarray;

public:
  CStringArray() {}
  CStringArray(const GLchar* const* strarray, unsigned sz) {
    if (sz != 0) {
      _rawarray.resize(sz);
      _array.resize(sz);
      for (unsigned idx = 0; idx < sz; idx++) {
        _array[idx] = new CGLString(strarray[idx], '\0', 1);
        _rawarray[idx] = **_array[idx];
      }
    }
  }

  ~CStringArray() {
    for (unsigned idx = 0; idx < _array.size(); idx++) {
      delete _array[idx];
    }
  }

  virtual const char* Name() const {
    return "const GLchar*";
  }

  virtual bool Array() const {
    return true;
  }

  const GLchar** operator*() {
    if (_rawarray.empty()) {
      return 0;
    }
    return (const GLchar**)&_rawarray[0];
  }

  virtual void Write(CBinOStream& stream) const {
    uint32_t sz = ensure_unsigned32bit_representible<size_t>(_array.size());
    if (!Config::Get().recorder.extras.utilities.nullIO) {
      stream.write((char*)&sz, sizeof(sz));
    }

    for (unsigned idx = 0; idx < _array.size(); idx++) {
      stream << (*_array[idx]);
    }
  }
  virtual void Read(CBinIStream& stream) {
    uint32_t sz;
    stream.read((char*)&sz, sizeof(sz));
    if (sz != 0) {
      _array.resize(sz);
      _rawarray.resize(sz);

      for (unsigned idx = 0; idx < sz; idx++) {
        _array[idx] = new CGLString((size_t)0);
        stream >> (*_array[idx]);
        _rawarray[idx] = **_array[idx];
      }
    }
  }

  virtual bool DeclarationNeeded() const {
    return true;
  }

  virtual void Write(CCodeOStream& stream) const {
    stream << stream.VariableName(ScopeKey());
  }

  void Declare(CCodeOStream& stream) const {
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << "[] = {";
    const size_t alignment =
        strlen(Name()) + strlen(" ") + stream.VariableName(ScopeKey()).length() + strlen("[] = {");

    // Initialize all elements of the array.
    if (_array.size() == 0) {
      stream << "\"\""; // Empty string.
    } else {
      for (size_t idx = 0; idx < _array.size(); ++idx) {
        stream << "\"" << _rawarray[idx] << "\"";
        if (idx < (_array.size() - 1)) {
          stream << ", ";
        }
        if (!((idx + 1) % 16) && (idx != _array.size() - 1)) {
          // If there's too many items in one line, we start a new line,
          // using spaces to align quotation marks in the new and old line.
          stream << '\n';
          stream.Indent();
          for (size_t i = 0; i <= alignment; ++i) {
            stream << ' ';
          }
        }
      }
    }
    stream << " };\n";
  }
};

/**
   * @brief Wrapper for map access argument
   *
   */
class CMapAccess : public CArgument {
  CGLbitfield _access;
  CGLbitfield _mask;

public:
  CMapAccess() : _access(0), _mask(0) {}
  CMapAccess(GLenum access) : _access(access) {
    _mask.Value() = Config::Get().recorder.extras.optimizations.bufferMapAccessMask;
  }
  virtual const char* Name() const {
    return "CMapAccess";
  }
  GLbitfield operator*() {
    if (((~*_mask) & *_access) != 0) {
      CALL_ONCE[&] {
        Log(WARN) << "Map access bit mask " << std::hex << *_mask
                  << " used in recorder has influence on playback.";
      };
    }
    return *_access & *_mask;
  }
  virtual void Write(CBinOStream& stream) const {
    stream << _access;
    stream << _mask;
  }
  virtual void Read(CBinIStream& stream) {
    stream >> _access;
    stream >> _mask;
  }
  virtual void Write(CCodeOStream& stream) const {
    std::ios_base::fmtflags streamFlags(stream.flags());
    stream << std::hex << _access << " & " << _mask;
    stream.flags(streamFlags);
  }
};

/**
   * @brief Wrapper for buffer storage argument
   *
   */
class CBufferStorageFlags : public CArgument {
  CGLbitfield _flags;
  CGLbitfield _mask;

public:
  CBufferStorageFlags() : _flags(0), _mask(0) {}
  CBufferStorageFlags(GLenum flags) : _flags(flags) {
    _mask.Value() = Config::Get().recorder.extras.optimizations.bufferStorageFlagsMask;
  }
  virtual const char* Name() const {
    return "CBufferStorageFlags";
  }
  GLbitfield operator*() {
    return *_flags;
  }
  virtual void Write(CBinOStream& stream) const {
    stream << _flags;
    stream << _mask;
  }
  virtual void Read(CBinIStream& stream) {
    stream >> _flags;
    stream >> _mask;
  }
  virtual void Write(CCodeOStream& stream) const {
    std::ios_base::fmtflags streamFlags(stream.flags());
    stream << std::hex << _flags;
    stream.flags(streamFlags);
  }
};

/**
   * @brief Wrapper for map access argument
   *
   */
class CRecUniformLocation : public CArgument {
  GLint _location;
  GLint _array_size;
  GLint _array_index;

public:
  CRecUniformLocation() : _location(-1), _array_size(1), _array_index(0) {}
  CRecUniformLocation(GLint location, GLuint program, const GLchar* name)
      : _location(location), _array_size(1), _array_index(0) {
    if (location == -1) {
      return;
    }

    GetUniformArraySizeAndOffset(program, name, location, _array_size, _array_index);
  }

  virtual const char* Name() const {
    return "CRecUniformLocation";
  }

  GLint Location() const {
    return _location;
  }
  GLint ArraySize() const {
    return _array_size;
  }
  GLint ArrayIndex() const {
    return _array_index;
  }

  virtual void Write(CBinOStream& stream) const {
    write_to_stream(stream, _location);
    write_to_stream(stream, _array_size);
    write_to_stream(stream, _array_index);
  }
  virtual void Read(CBinIStream& stream) {
    read_from_stream(stream, _location);
    read_from_stream(stream, _array_size);
    read_from_stream(stream, _array_index);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << Name() << "(" << _location << ", " << _array_size << ", " << _array_index << ")";
  }
};

/**
   * @brief Wrapper for map access argument
   *
   */
class CGLResourceIndex : public CArgument {
  // Program name (ID)
  CGLProgram _program;
  // Interface application provided during playback
  GLenum _programInterface;
  // Index application used during playback
  GLuint _index;
  // Name of a variable application referred to through index OR a name of a
  // uniform variable associated with an atomic counter buffer application
  // referred to through index
  CGLchar::CSArray _name;

public:
  CGLResourceIndex() : _program(), _programInterface(), _index(), _name() {}
  CGLResourceIndex(GLuint program, GLenum programInterface, GLuint index)
      : _program(program), _programInterface(programInterface), _index(index), _name() {
    if (GL_ATOMIC_COUNTER_BUFFER == programInterface) {
      GLenum props = GL_NUM_ACTIVE_VARIABLES;
      GLint numActiveVariables = -1;
      drv.gl.glGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, index, 1, &props, 1, nullptr,
                                    &numActiveVariables);

      props = GL_ACTIVE_VARIABLES;
      std::vector<GLint> activeVariablesIndices(numActiveVariables);
      drv.gl.glGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, index, 1, &props,
                                    numActiveVariables, nullptr, activeVariablesIndices.data());

      // Atomic counter magic
      // Active uniform variables are associated with atomic counter buffers;
      // Many variables can be associated with one buffer so we need only one of
      // them (one of variables associated with the same atomic counter buffer)
      index = activeVariablesIndices[0];
      programInterface = GL_UNIFORM;
    }
    GLint nameLength = 0;
    drv.gl.glGetProgramInterfaceiv(program, programInterface, GL_MAX_NAME_LENGTH, &nameLength);
    std::vector<GLchar> name(nameLength);
    drv.gl.glGetProgramResourceName(program, programInterface, index, nameLength, &nameLength,
                                    name.data());
    _name = CGLchar::CSArray(name.data(), '\0', 1);
  }

  virtual const char* Name() const {
    return "CGLResourceIndex";
  }

  const GLchar* ResourceName() const {
    return *_name;
  }

  GLuint operator*() const {
    // In case of atomic counter, stream contains a name of an uniform variable
    // associated with a given atomic counter buffer (not the name of the actual
    // atomic counter buffer)
    if (GL_ATOMIC_COUNTER_BUFFER == _programInterface) {
      GLuint index = drv.gl.glGetProgramResourceIndex(*_program, GL_UNIFORM, *_name);
      GLenum prop = GL_ATOMIC_COUNTER_BUFFER_INDEX;
      GLsizei length = 0;
      GLint params = -1;
      drv.gl.glGetProgramResourceiv(*_program, GL_UNIFORM, index, 1, &prop, 1, &length, &params);
      if (length != 1) {
        Log(WARN) << "CGLResourceIndex: expected 1 param from glGetProgramResourceiv, got "
                  << length;
      }
      return static_cast<GLuint>(params);
    } else {
      return drv.gl.glGetProgramResourceIndex(*_program, _programInterface, *_name);
    }
  }

  GLuint Original() const {
    return _index;
  }

  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override {
    // We hardcode the variable name to "resource_name", so we can use it in
    // the CCode wrap without relying on the name registration system.
    stream.Indent() << "GLchar resource_name[] = \"" << *_name << "\";\n";
    // In case of atomic counter, stream contains the name of a uniform
    // variable associated with a given atomic counter buffer (not the name
    // of the actual atomic counter buffer)
    if (GL_ATOMIC_COUNTER_BUFFER == _programInterface) {
      stream << "\n";
      stream.Indent() << "// Get the index of the atomic counter buffer.\n";
      stream.Indent() << "GLuint temp_program = " << _program << ";\n";
      stream.Indent()
          << "GLuint index = glGetProgramResourceIndex(temp_program, GL_UNIFORM, resource_name);\n";
      stream.Indent() << "GLenum temp_prop = GL_ATOMIC_COUNTER_BUFFER_INDEX;\n";
      stream.Indent() << "GLsizei temp_length = 0;\n";
      stream.Indent() << "GLint temp_params = -1;\n";
      stream.Indent() << "glGetProgramResourceiv(temp_program, GL_UNIFORM, index, 1, &temp_prop, "
                         "1, &temp_length, &temp_params);\n";
      stream.Indent() << "if (temp_length != 1) {\n";
      stream.ScopeBegin();
      stream.Indent() << "Log(WARN) << \"CGLResourceIndex: expected 1 param from "
                         "glGetProgramResourceiv, got \" << temp_length;\n";
      stream.ScopeEnd();
      stream.Indent() << "}\n\n";
    } else {
      stream.Indent() << "GLuint index = glGetProgramResourceIndex(" << _program << ", "
                      << _programInterface << ", resource_name);\n";
    }
  }

  virtual void Write(CBinOStream& stream) const {
    _program.Write(stream);
    write_to_stream(stream, _programInterface);
    write_to_stream(stream, _index);
    _name.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    _program.Read(stream);
    read_from_stream(stream, _programInterface);
    read_from_stream(stream, _index);
    _name.Read(stream);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << "index";
  }
};

/**
   * @brief Wrapper for glGetProgramResourceiv() function (and an array of CRecUniformLocation)
   *
   */
class CGLProgramResourceivHelper : public CArgument {
  CGLint::CSArray _params;
  std::vector<CRecUniformLocation> _locations;
  GLsizei _count; // Not saved to the stream. Only available in recorder!
public:
  CGLProgramResourceivHelper() : _params(), _locations(), _count(0) {}
  CGLProgramResourceivHelper(GLuint program,
                             GLenum programInterface,
                             GLuint index,
                             GLsizei propCount,
                             const GLenum* props,
                             GLsizei bufSize,
                             GLint* params)
      : _params(std::min(propCount, bufSize), params), _locations(), _count(bufSize) {
    GLint nameLength = 0;
    drv.gl.glGetProgramInterfaceiv(program, programInterface, GL_MAX_NAME_LENGTH, &nameLength);
    std::vector<GLchar> name(nameLength);
    drv.gl.glGetProgramResourceName(program, programInterface, index, nameLength, &nameLength,
                                    name.data());

    // TODO: The min(propCount, bufSize) length calculations are wrong. Same
    // for the assumption that params[i] corresponds to props[i]. One prop
    // might result in more than one param being written to params.
    if (GL_UNIFORM == programInterface) {
      for (GLsizei i = 0; i < std::min(propCount, bufSize); ++i) {
        // (location, program, name)
        _locations.emplace_back(GL_LOCATION == props[i] ? params[i] : -1, program, name.data());
      }
    }
  }

  virtual const char* Name() const {
    return "CGLProgramResourceivHelper";
  }

  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override {
    assert(Config::IsRecorder() && "Use _count only in recorder.");
    stream.Indent() << "std::vector<GLint> params(" << _count << ");\n";
    stream.Indent() << "std::vector<CRecUniformLocation> locations = {\n";
    stream.ScopeBegin();
    for (CRecUniformLocation location : _locations) {
      stream.Indent() << location << ",\n";
    }
    stream.ScopeEnd();
    stream.Indent() << "};\n";
  }

  const std::vector<CRecUniformLocation>& Locations() const {
    return _locations;
  }

  virtual void Write(CBinOStream& stream) const {
    _params.Write(stream);
    write_to_stream(stream, _locations.size());
    for (size_t i = 0; i < _locations.size(); ++i) {
      _locations[i].Write(stream);
    }
  }
  virtual void Read(CBinIStream& stream) {
    size_t size = 0;
    _params.Read(stream);
    read_from_stream(stream, size);
    if (size > 0) {
      _locations.resize(size);
      for (size_t i = 0; i < size; ++i) {
        _locations[i].Read(stream);
      }
    }
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << "params.data()";
  }
};

/**
     * @brief Wrapper for an output argument
     *
     */
template <int argumentNumber>
class COutArgument : public CArgument {
public:
  static const size_t space_provided = 64 * 1024;

  class ToAnyPtr {
    void* ptr_;

  public:
    explicit ToAnyPtr(void* ptr) : ptr_(ptr) {}
    template <class T>
    operator T*() {
      return (T*)ptr_;
    }
  };

  ToAnyPtr operator*() {
    static std::vector<char> space(space_provided);
    return ToAnyPtr(&space[0]);
  }

  virtual bool Array() const {
    return false;
  }
  virtual const char* Name() const {
    return "<out_arg>";
  }
  virtual void Write(CBinOStream& stream) const { /* Do nothing */
  }
  virtual void Read(CBinIStream& stream) { /* Do nothing */
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << "outArg()";
  }
};

struct DrawArraysIndirectCommand {
  uint32_t count;
  uint32_t instanceCount;
  uint32_t first;
  uint32_t baseInstance;
};

class CGLIndirectCmds : public CArgument {
private:
  uint64_t ptr_value_;
  uint32_t count_;
  GLint buffer_;
  std::vector<DrawArraysIndirectCommand> cmds_;

public:
  CGLIndirectCmds();
  CGLIndirectCmds(const void* ptr, uint32_t count);

  virtual const char* Name() const {
    return "<indirect_command>";
  }
  virtual bool Array() const {
    return true;
  }

  const void* operator*();

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

  virtual void Write(CCodeOStream& stream) const;
};

class CGLGenericResource : public CArgument {
private:
  CGLboolean _isBuff;
  CGLuint64 _buffOffset;
  CBinaryResource _resource;

public:
  CGLGenericResource() {}
  CGLGenericResource(uint64_t hash) : _isBuff(false), _buffOffset(0), _resource(hash) {}
  CGLGenericResource(const GLvoid* ptr,
                     size_t size,
                     GLenum sourceTarget,
                     TResourceType resourceType);
  virtual const char* Name() const {
    return "CGLObjectData";
  }
  CBinaryResource::PointerProxy Value();
  CBinaryResource::PointerProxy operator*();
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

class CGLTexResource : public CArgument {
private:
  CGLGenericResource _resource;

public:
  CGLTexResource() {}
  CGLTexResource(uint64_t hash) : _resource(hash) {}
  // glTexImage1D
  CGLTexResource(GLenum target, GLenum format, GLenum type, GLsizei width, const GLvoid* ptr);
  // glTexImage2D
  CGLTexResource(
      GLenum target, GLenum format, GLenum type, GLsizei width, GLsizei height, const GLvoid* ptr);
  // glTexImage3D
  CGLTexResource(GLenum target,
                 GLenum format,
                 GLenum type,
                 GLsizei width,
                 GLsizei height,
                 GLsizei depth,
                 const GLvoid* ptr);

  virtual const char* Name() const {
    return "CGLTextureResource";
  }
  CBinaryResource::PointerProxy Value() {
    return _resource.Value();
  }
  CBinaryResource::PointerProxy operator*() {
    return _resource.Value();
  }
  virtual void Write(CBinOStream& stream) const {
    stream << _resource;
  }
  virtual void Read(CBinIStream& stream) {
    stream >> _resource;
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _resource;
  }
};

class CGLCompressedTexResource : public CArgument {
private:
  CGLGenericResource _resource;

public:
  CGLCompressedTexResource() {}
  CGLCompressedTexResource(uint64_t hash) : _resource(hash) {}
  // glCompressedTexImage1D
  CGLCompressedTexResource(GLenum target, GLsizei width, GLsizei size, const GLvoid* ptr);
  // glCompressedTexImage2D
  CGLCompressedTexResource(
      GLenum target, GLsizei width, GLsizei height, GLsizei size, const GLvoid* ptr);
  // glCompressedTexImage3D
  CGLCompressedTexResource(
      GLenum target, GLsizei width, GLsizei height, GLsizei depth, GLsizei size, const GLvoid* ptr);
  // glCompressedTextureSubImage1D
  CGLCompressedTexResource(CGLTexture texture, GLsizei width, GLsizei size, const GLvoid* ptr);
  // glCompressedTextureSubImage2D
  CGLCompressedTexResource(
      CGLTexture texture, GLsizei width, GLsizei height, GLsizei size, const GLvoid* ptr);
  // glCompressedTextureSubImage3D
  CGLCompressedTexResource(CGLTexture texture,
                           GLsizei width,
                           GLsizei height,
                           GLsizei depth,
                           GLsizei size,
                           const GLvoid* ptr);

  virtual const char* Name() const {
    return "CGLCompressedTexResource";
  }
  CBinaryResource::PointerProxy Value() {
    return _resource.Value();
  }
  CBinaryResource::PointerProxy operator*() {
    return _resource.Value();
  }
  virtual void Write(CBinOStream& stream) const {
    stream << _resource;
  }
  virtual void Read(CBinIStream& stream) {
    stream >> _resource;
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _resource;
  }
};

class CGLClearTexResource : public CArgument {
private:
  std::unique_ptr<CGLGenericResource> _resource;
  Cbool _isNullPtr;

public:
  CGLClearTexResource() = default;
  // glClearTexImage
  CGLClearTexResource(const void* data, GLuint texture, GLint level, GLenum format, GLenum type);

  virtual const char* Name() const {
    return "CGLClearTexResource";
  }
  CBinaryResource::PointerProxy Value() {
    if (!*_isNullPtr) {
      return _resource->Value();
    } else {
      return CBinaryResource::PointerProxy(nullptr);
    }
  }
  CBinaryResource::PointerProxy operator*() {
    return Value();
  }
  virtual void Write(CBinOStream& stream) const {
    _isNullPtr.Write(stream);
    if (!*_isNullPtr) {
      _resource->Write(stream);
    }
  }
  virtual void Read(CBinIStream& stream) {
    _isNullPtr.Read(stream);
    if (!*_isNullPtr) {
      _resource = std::make_unique<CGLGenericResource>();
      _resource->Read(stream);
    }
  }
  virtual void Write(CCodeOStream& stream) const {
    if (!*_isNullPtr) {
      _resource->Write(stream);
    } else {
      stream << "nullptr";
    }
  }
};

class CGLBitmapResource : public CArgument {
private:
  CGLGenericResource _resource;

public:
  CGLBitmapResource() {}
  CGLBitmapResource(uint64_t hash) : _resource(hash) {}
  // glBitmap
  CGLBitmapResource(GLsizei width, GLsizei height, const GLubyte* bitmap);

  virtual const char* Name() const {
    return "CGLBitmapResource";
  }
  CBinaryResource::PointerProxy Value() {
    return _resource.Value();
  }
  CBinaryResource::PointerProxy operator*() {
    return _resource.Value();
  }
  virtual void Write(CBinOStream& stream) const {
    stream << _resource;
  }
  virtual void Read(CBinIStream& stream) {
    stream >> _resource;
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _resource;
  }
};

} // namespace OpenGL
} // namespace gits
