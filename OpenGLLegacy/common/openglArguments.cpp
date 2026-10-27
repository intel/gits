// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglArguments.cpp
*
* @brief  Definitions of OpenGL library function call argument wrappers.
*
*/

#include "openglArguments.h"

#include "buffer.h"
#include "exception.h"
#include "gits.h"
#include "glxArguments.h"
#include "log.h"
#include "mapping.h"
#include "openglDrivers.h"
#include "openglLibrary.h"
#include "platform.h"
#include "stateDynamic.h"
#include "streams.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>

/* *********************************** E N U M ********************************* */

const char* gits::OpenGL::CGLenum::NAME = "GLenum";

gits::OpenGL::CGLenum::CGLenum() {}

gits::OpenGL::CGLenum::CGLenum(GLenum value) : CGLtype<GLtype, type, GLushort>(value) {}

/* ******************************** B O O L E A N ****************************** */

const char* gits::OpenGL::CGLboolean::NAME = "GLboolean";

gits::OpenGL::CGLboolean::CGLboolean() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLboolean::CGLboolean(GLboolean value) : CGLtype<GLtype, type>(value) {}

/* ******************************* B I T F I E L D ***************************** */

const char* gits::OpenGL::CGLbitfield::NAME = "GLbitfield";

gits::OpenGL::CGLbitfield::CGLbitfield() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLbitfield::CGLbitfield(GLbitfield value) : CGLtype<GLtype, type>(value) {}

/* *********************************** B Y T E ********************************* */

const char* gits::OpenGL::CGLbyte::NAME = "GLbyte";

gits::OpenGL::CGLbyte::CGLbyte() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLbyte::CGLbyte(GLbyte value) : CGLtype<GLtype, type>(value) {}

/* ********************************** S H O R T ******************************** */

const char* gits::OpenGL::CGLshort::NAME = "GLshort";

gits::OpenGL::CGLshort::CGLshort() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLshort::CGLshort(GLshort value) : CGLtype<GLtype, type>(value) {}

/* ************************************ I N T ********************************** */

const char* gits::OpenGL::CGLint::NAME = "GLint";

gits::OpenGL::CGLint::CGLint() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLint::CGLint(GLint value) : CGLtype<GLtype, type>(value) {}

/* ************************************ I N T 6 4 ********************************** */

const char* gits::OpenGL::CGLint64::NAME = "GLint64";

gits::OpenGL::CGLint64::CGLint64() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLint64::CGLint64(GLint64 value) : CGLtype<GLtype, type>(value) {}

/* ********************************** S I Z E I ******************************** */

const char* gits::OpenGL::CGLsizei::NAME = "GLsizei";

gits::OpenGL::CGLsizei::CGLsizei() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLsizei::CGLsizei(GLsizei value) : CGLtype<GLtype, type>(value) {}

/* ********************************** U B Y T E ******************************** */

const char* gits::OpenGL::CGLubyte::NAME = "GLubyte";

gits::OpenGL::CGLubyte::CGLubyte() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLubyte::CGLubyte(GLubyte value) : CGLtype<GLtype, type>(value) {}

/* ********************************* U S H O R T ******************************* */

const char* gits::OpenGL::CGLushort::NAME = "GLushort";

gits::OpenGL::CGLushort::CGLushort() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLushort::CGLushort(GLushort value) : CGLtype<GLtype, type>(value) {}

/* *********************************** U I N T ********************************* */

const char* gits::OpenGL::CGLuint::NAME = "GLuint";

gits::OpenGL::CGLuint::CGLuint() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLuint::CGLuint(GLuint value) : CGLtype<GLtype, type>(value) {}

/* *********************************** U I N T 6 4 ********************************* */

const char* gits::OpenGL::CGLuint64::NAME = "GLuint64";

gits::OpenGL::CGLuint64::CGLuint64() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLuint64::CGLuint64(GLuint64 value) : CGLtype<GLtype, type>(value) {}

/* ********************************** F L O A T ******************************** */

const char* gits::OpenGL::CGLfloat::NAME = "GLfloat";

gits::OpenGL::CGLfloat::CGLfloat() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLfloat::CGLfloat(GLfloat value) : CGLtype<GLtype, type>(value) {}

/* ********************************* C L A M P F ******************************* */

const char* gits::OpenGL::CGLclampf::NAME = "GLclampf";

gits::OpenGL::CGLclampf::CGLclampf() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLclampf::CGLclampf(GLclampf value) : CGLtype<GLtype, type>(value) {}

/* ********************************* D O U B L E ******************************* */

const char* gits::OpenGL::CGLdouble::NAME = "GLdouble";

gits::OpenGL::CGLdouble::CGLdouble() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLdouble::CGLdouble(GLdouble value) : CGLtype<GLtype, type>(value) {}

/* ********************************* C L A M P D ******************************* */

const char* gits::OpenGL::CGLclampd::NAME = "GLclampd";

gits::OpenGL::CGLclampd::CGLclampd() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLclampd::CGLclampd(GLclampd value) : CGLtype<GLtype, type>(value) {}

/* ******************************** C H A R A R B ****************************** */

const char* gits::OpenGL::CGLcharARB::NAME = "GLcharARB";

gits::OpenGL::CGLcharARB::CGLcharARB() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLcharARB::CGLcharARB(GLcharARB value) : CGLtype<GLtype, type>(value) {}

/* ********************************* C H A R ******************************* */

const char* gits::OpenGL::CGLchar::NAME = "GLchar";

gits::OpenGL::CGLchar::CGLchar() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLchar::CGLchar(GLchar value) : CGLtype<GLtype, type>(value) {}

/* ********************************* I N T P T R ******************************* */

const char* gits::OpenGL::CGLsizeiptr::NAME = "GLsizeiptr";

gits::OpenGL::CGLsizeiptr::CGLsizeiptr() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLsizeiptr::CGLsizeiptr(GLsizeiptr value)
    : CGLtype<GLtype, type>(ensure_signed32bit_representible<GLsizeiptr>(value)) {}

/* ********************************* H A L F N V ******************************* */

const char* gits::OpenGL::CGLhalfNV::NAME = "GLhalfNV";

gits::OpenGL::CGLhalfNV::CGLhalfNV() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLhalfNV::CGLhalfNV(GLhalfNV value) : CGLtype<GLtype, type>(value) {}

/* ********************************* H A N D L E A R B ******************************* */

const char* gits::OpenGL::CGLhandleARB::NAME = "GLhandleARB";

gits::OpenGL::CGLhandleARB::CGLhandleARB() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLhandleARB::CGLhandleARB(GLhandleARB value) : CGLtype<GLtype, type>(value) {}

/* ********************************* I N T P T R ******************************* */

const char* gits::OpenGL::CGLintptr::NAME = "GLintptr";

gits::OpenGL::CGLintptr::CGLintptr() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLintptr::CGLintptr(GLintptr value)
    : CGLtype<GLtype, type>(ensure_signed32bit_representible<GLintptr>(value)) {}

/* ********************************* GLint64EXT ******************************* */

const char* gits::OpenGL::CGLint64EXT::NAME = "GLint64EXT";

gits::OpenGL::CGLint64EXT::CGLint64EXT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLint64EXT::CGLint64EXT(GLint64EXT value) : CGLtype<GLtype, type>(value) {}

/* ********************************* GLuint64EXT ******************************* */

const char* gits::OpenGL::CGLuint64EXT::NAME = "GLuint64EXT";

gits::OpenGL::CGLuint64EXT::CGLuint64EXT() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLuint64EXT::CGLuint64EXT(GLuint64EXT value) : CGLtype<GLtype, type>(value) {}

/* ********************************* S I Z E I P T R A R B ******************************* */

const char* gits::OpenGL::CGLsizeiptrARB::NAME = "GLsizeiptrARB";

gits::OpenGL::CGLsizeiptrARB::CGLsizeiptrARB() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLsizeiptrARB::CGLsizeiptrARB(GLsizeiptrARB value)
    : CGLtype<GLtype, type>(ensure_signed32bit_representible<GLsizeiptrARB>(value)) {}

/* ********************************* I N T P T R A R B ******************************* */

const char* gits::OpenGL::CGLintptrARB::NAME = "GLintptrARB";

gits::OpenGL::CGLintptrARB::CGLintptrARB() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLintptrARB::CGLintptrARB(GLintptrARB value)
    : CGLtype<GLtype, type>(ensure_signed32bit_representible<GLintptrARB>(value)) {}

/* ********************************* V O I D _ P T R ******************************* */

const char* gits::OpenGL::CGLvoid_ptr::NAME = "void *";

gits::OpenGL::CGLvoid_ptr::CGLvoid_ptr() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLvoid_ptr::CGLvoid_ptr(GLvoid* value)
    : CGLtype<GLtype, type>(reinterpret_cast<uint64_t>(value)) {}

gits::OpenGL::CGLvoid_ptr::CGLvoid_ptr(const GLvoid* value)
    : CGLtype<GLtype, type>(reinterpret_cast<uint64_t>(value)) {}

/* ********************************* CGLMAPPEDTEXTUREPTR ******************************* */
void gits::OpenGL::CGLMappedTexturePtr::Assign(const void* val) {
  _mappedValue = const_cast<GLvoid*>(val);
}

GLvoid* gits::OpenGL::CGLMappedTexturePtr::Value() {
  return _mappedValue;
}

GLvoid* const gits::OpenGL::CGLMappedTexturePtr::Value() const {
  return _mappedValue;
}

GLvoid* gits::OpenGL::CGLMappedTexturePtr::operator*() {
  return Value();
}

GLvoid* const gits::OpenGL::CGLMappedTexturePtr::operator*() const {
  return Value();
}

/* ********************************* CONST CHAR _ P T R ******************************* */
const char* gits::OpenGL::CGLconstuchar_ptr::NAME = "const unsigned char*";

gits::OpenGL::CGLconstuchar_ptr::CGLconstuchar_ptr() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLconstuchar_ptr::CGLconstuchar_ptr(const unsigned char* value)
    : CGLtype<GLtype, type>(reinterpret_cast<uint64_t>(value)) {}

/* ********************************* V O I D P T R ******************************* */

const char* gits::OpenGL::CGLvoidPtr::NAME = "void *";

gits::OpenGL::CGLvoidPtr::CGLvoidPtr() : _value(0) {}

gits::OpenGL::CGLvoidPtr::CGLvoidPtr(GLvoid* value)
    : _value(ensure_unsigned32bit_representible<uintptr_t>(reinterpret_cast<uintptr_t>(value))) {}

/* ********************************* C O N S T V O I D P T R ******************************* */

const char* gits::OpenGL::CGLconstVoidPtr::NAME = "const VoidPointer";

gits::OpenGL::CGLconstVoidPtr::CGLconstVoidPtr() : _value(0) {}

gits::OpenGL::CGLconstVoidPtr::CGLconstVoidPtr(const GLvoid* value)
    : _value(ensure_unsigned32bit_representible<uintptr_t>(reinterpret_cast<uintptr_t>(value))) {}

/* ********************************** CShaderSource **************************** */

unsigned gits::OpenGL::CShaderSource::_shaderSourceIdx = 0;
unsigned gits::OpenGL::CShaderSource::_shaderProgramIdx = 0;
unsigned gits::OpenGL::CShaderSource::_programStringIdx = 0;
std::string gits::OpenGL::CShaderSource::GetFileName(ShaderType shaderType,
                                                     GLenum target,
                                                     GLenum format,
                                                     GLuint shaderObj) {
  std::stringstream stream;
  if (shaderType == SHADER_SOURCE) {
    // obtain shader type
    GLint type = 999;
    drv.gl.glGetShaderiv(shaderObj, GL_SHADER_TYPE, &type);
    const char* typeAbrv = shaderTypeAbrvName(type);

    stream << "gitsPrograms/" << typeAbrv << "_shader_" << std::setfill('0') << std::setw(2)
           << _shaderSourceIdx++ << ".txt";
  } else if (shaderType == SHADER_PROGRAM) {
    stream << "gitsPrograms/shaderProgram_" << std::setfill('0') << std::setw(2)
           << _shaderProgramIdx++ << ".txt";
  } else if (shaderType == PROGRAM_STRING) {
    if (format == GL_PROGRAM_FORMAT_ASCII_ARB) {
      stream << "gitsPrograms/program_";

      switch (target) {
      case GL_VERTEX_PROGRAM_ARB:
        stream << "vertex";
        break;
      case GL_FRAGMENT_PROGRAM_ARB:
        stream << "fragment";
        break;
      case GL_GEOMETRY_PROGRAM_NV:
        stream << "geometryNV";
        break;
      default:
        LOG_ERROR << "Unknown target: " << target << " for '" << Name() << "'!!!";
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }

      stream << "_ARB_" << std::setfill('0') << std::setw(2) << _programStringIdx++ << ".txt";
    } else {
      LOG_ERROR << "Unknown target: 0x" << std::hex << target << " for " << Name() << "()!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  return stream.str();
}

std::string gits::OpenGL::CShaderSource::GetShaderSource(const GLchar* const* string,
                                                         GLsizei count,
                                                         const GLint* length) {
  return ConcatenateShaderFromParts(string, count, length);
}

std::string gits::OpenGL::CShaderSource::GetShaderSource(const GLchar* const* string,
                                                         GLsizei count) {
  return ConcatenateShaderFromParts(string, count, nullptr);
}

gits::OpenGL::CShaderSource::CShaderSource(GLuint shaderObj,
                                           GLsizei count,
                                           const GLchar* const* string,
                                           const GLint* length,
                                           ShaderType shaderType)
    : CArgumentFileText(GetFileName(shaderType, 0, 0, shaderObj),
                        GetShaderSource(string, count, length)) {}

gits::OpenGL::CShaderSource::CShaderSource(
    const GLchar* string, GLsizei length, ShaderType shaderType, GLenum target, GLenum format)
    : CArgumentFileText(GetFileName(shaderType, target, format, 0),
                        GetShaderSource(&string, 1, &length)) {}

gits::OpenGL::CShaderSource::CShaderSource(GLsizei count,
                                           const GLchar* const* string,
                                           ShaderType shaderType)
    : CArgumentFileText(GetFileName(shaderType, 0, 0, 0), GetShaderSource(string, count)) {}

void gits::OpenGL::CShaderSource::Read(CBinIStream& stream) {
  CArgumentFileText::Read(stream);
  if (_text.find('\0') == std::string::npos) {
    // Null-terminate shader source if it has no null characters at all. (Older streams don't.)
    _text += '\0';
  } else if (_text.back() != '\0') {
    LOG_ERROR << "Shader source is not null-terminated. If you manually edited shaders, "
                 "ensure each source part ends with a null character. If not, this is a bug.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

std::vector<const GLchar*> gits::OpenGL::CShaderSource::Value() const {
  // Parent class CArgumentFileText owns the full source string. Here we simply get pointers to
  // every source part. Separating parts with \0 naturally causes them to be null-terminated.
  return SplitShaderIntoParts(Text());
}

/* ************************************ I N T  Z E R O ********************************** */

const char* gits::OpenGL::CGLintptrZero::NAME = "GLintptrZero";

gits::OpenGL::CGLintptrZero::CGLintptrZero() {}

uint64_t GetCArraySizeFromId(uint64_t id) {
  typedef std::map<uint64_t, uint64_t> map_t;
  INIT_NEW_STATIC_OBJ(idsMap, map_t);
  CALL_ONCE[&] {
    idsMap = gits::read_map<map_t>(gits::Configurator::Get().common.player.streamDir /
                                   "gitsClientSizes.dat");
  };
  if (idsMap.find(id) == idsMap.end()) {
    return 0;
  }
  return idsMap[id];
}

/* ********************************** CDataPtr ********************************* */
gits::OpenGL::CDataPtr::CDataPtr() : _ptr(0), _isBuff(false) {}

gits::OpenGL::CDataPtr::CDataPtr(const void* ptr) : _ptr((uint64_t)ptr), _isBuff(false) {}

gits::OpenGL::CDataPtr::CDataPtr(GLenum target, const void* ptr)
    : _ptr((uint64_t)ptr), _isBuff(false) {
  if (CGits::Instance().IsStateRestoration()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  GLint buffer;
  drv.gl.glGetIntegerv(GetBindingEnum(target), &buffer);
  if (buffer > 0) {
    *_isBuff = true;
  }
}

gits::OpenGL::CDataPtr::CDataPtr(GLint buffer, GLenum target, const void* ptr)
    : _ptr((uint64_t)ptr), _isBuff(false) {
  if (buffer < 0) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (buffer > 0) {
    *_isBuff = true;
  }

  if (target == GL_DRAW_INDIRECT_BUFFER && !*_isBuff) {
    LOG_ERROR << "Indirect client data not supported";
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

void gits::OpenGL::CDataPtr::Write(CBinOStream& stream) const {
  stream << _isBuff << _ptr;
}

void gits::OpenGL::CDataPtr::Read(CBinIStream& stream) {
  stream >> _isBuff;
  stream >> _ptr;
}

void* gits::OpenGL::CDataPtr::Value() {
  if (!Configurator::IsPlayer()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  // Buffer offset
  if (*_isBuff != 0) {
    return (void*)*_ptr;
  }

  if (*_ptr == 0) {
    CALL_ONCE[&] {
      LOG_WARNING << "passing invalid NULL pointer: " << std::hex << *_ptr << " to GL API.";
    };
    return (char*)*_ptr;
  }

  uint64_t areaOffset = GetAreaOffset(*_ptr);
  uint64_t areaRecPtr = GetAreaPtr(*_ptr);
  auto& memTracker = SD()._memTracker;

  // Alloc client data if needed
  if (memTracker.find(areaRecPtr) == memTracker.end()) {
    memTracker[areaRecPtr].resize((int)GetCArraySizeFromId(areaRecPtr), 0);
  }

  // Return client data ptr
  if (memTracker[areaRecPtr].size() <= areaOffset) {
    CALL_ONCE[&] {
      LOG_WARNING << "passing invalid pointer: " << std::hex << *_ptr
                  << " to GL API because it seems to not being used.";
    };
    return (char*)*_ptr;
  } else {
    char* ptr = &memTracker.at(areaRecPtr)[0] + areaOffset;
    return ptr;
  }
}

uint64_t gits::OpenGL::CDataPtr::Size() const {
  return _ptr.Size() + _isBuff.Size();
}

/* ********************************** CDataPtrArray ********************************* */
gits::OpenGL::CDataPtrArray::CDataPtrArray() : _isBuff(false) {}

gits::OpenGL::CDataPtrArray::CDataPtrArray(const void** ptrs, size_t size) : _isBuff(false) {
  for (unsigned i = 0; i < size; i++) {
    _ptrsRecorder.Vector().push_back((GLuint64)ptrs[i]);
  }
}

gits::OpenGL::CDataPtrArray::CDataPtrArray(GLenum target, const void* const* ptrs, size_t size)
    : _isBuff(false) {
  for (unsigned i = 0; i < size; i++) {
    _ptrsRecorder.Vector().push_back((GLuint64)ptrs[i]);
  }

  GLint buffer;
  drv.gl.glGetIntegerv(GetBindingEnum(target), &buffer);
  if (buffer > 0) {
    *_isBuff = true;
  }
}

gits::OpenGL::CDataPtrArray::CDataPtrArray(GLint buffer,
                                           GLenum target,
                                           const void** ptrs,
                                           size_t size)
    : _isBuff(false) {
  for (unsigned i = 0; i < size; i++) {
    _ptrsRecorder.Vector().push_back((GLuint64)ptrs[i]);
  }

  if (buffer == -1) {
    drv.gl.glGetIntegerv(GetBindingEnum(target), &buffer);
  }
  if (buffer > 0) {
    *_isBuff = true;
  }
}

void gits::OpenGL::CDataPtrArray::Write(CBinOStream& stream) const {
  stream << _isBuff << _ptrsRecorder;
}
void gits::OpenGL::CDataPtrArray::Read(CBinIStream& stream) {
  stream >> _isBuff;
  stream >> _ptrsRecorder;
}

const void** gits::OpenGL::CDataPtrArray::Value() {
  if (!Configurator::IsPlayer()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (_ptrsPlayer.size() == 0) {
    for (auto ptr : _ptrsRecorder.Vector()) {
      // Buffer offset
      if (*_isBuff != 0) {
        if (ptr != (uintptr_t)ptr) {
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
        _ptrsPlayer.push_back((void*)ptr);
        continue;
      }

      uint64_t areaOffset = GetAreaOffset(ptr);
      uint64_t areaRecPtr = GetAreaPtr(ptr);
      auto& memTracker = SD()._memTracker;

      // Alloc client data if needed
      if (memTracker.find(areaRecPtr) == memTracker.end()) {
        memTracker[areaRecPtr].resize((int)GetCArraySizeFromId(areaRecPtr), 0);
      }

      // Return client data ptr
      if (memTracker[areaRecPtr].size() <= areaOffset) {
        CALL_ONCE[&] {
          LOG_WARNING << "passing invalid pointer: " << std::hex << areaRecPtr
                      << " to GL API because it seems to not being used.";
        };
        _ptrsPlayer.push_back((void*)areaRecPtr);
      } else {
        _ptrsPlayer.push_back((void*)(&memTracker.at(areaRecPtr)[0] + areaOffset));
      }
    }
  }

  return &_ptrsPlayer[0];
}

uint64_t gits::OpenGL::CDataPtrArray::Size() const {
  return _ptrsRecorder.Size() + _isBuff.Size();
}

/* ******************************** CDataUpdate **************************** */

// Select diff method.
void gits::OpenGL::CDataUpdate::Diff(uint64_t dataptr, uint64_t updateptr, uint64_t updatesize) {
  if (Configurator::Get().opengl.recorder.carrayMemCmpType == 0) {
    DiffAll(dataptr, updateptr, updatesize);
  } else if (Configurator::Get().opengl.recorder.carrayMemCmpType == 1) {
    DiffOneRange(dataptr, updateptr, updatesize);
  } else if (Configurator::Get().opengl.recorder.carrayMemCmpType == 2) {
    DiffMultiRange(dataptr, updateptr, updatesize);
  } else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

// Do not compare any data, simply create diff from entire pointed data.
void gits::OpenGL::CDataUpdate::DiffAll(uint64_t dataptr, uint64_t updateptr, uint64_t updatesize) {
  uint64_t areaPtr = GetAreaPtr(dataptr);
  uint64_t areaOffset = GetAreaOffset(dataptr);
  uint64_t updateOffset = areaOffset + (updateptr - dataptr);
  uint64_t totalSize = (updateptr - areaPtr) + updatesize;
  auto& memTracker = SD()._memTracker;

  // Extend tracked memory area.
  if (memTracker[areaPtr].size() < totalSize) {
    memTracker[areaPtr].resize((size_t)(totalSize), 0);
  }

  // Dump entire update data.
  uint64_t hash = CGits::Instance().ResourceManager2().put(RESOURCE_BUFFER, (void*)updateptr,
                                                           (size_t)updatesize);
  _updates.push_back(TData(areaPtr, hash, updateOffset));

  if (Configurator::Get().common.recorder.highIntegrity) {
    SD().WriteClientSizes();
  }
}

// Compares data and creates one diff starting at first different byte and
// ending at last different byte.
void gits::OpenGL::CDataUpdate::DiffOneRange(uint64_t dataptr,
                                             uint64_t updateptr,
                                             uint64_t updatesize) {
  uint64_t areaPtr = GetAreaPtr(dataptr);
  uint64_t areaOffset = GetAreaOffset(dataptr);
  uint64_t updateOffset = areaOffset + (updateptr - dataptr);
  uint64_t totalSize = (updateptr - areaPtr) + updatesize;

  // Extend tracked memory area.
  auto& memTracker = SD()._memTracker;
  if (memTracker[areaPtr].size() < totalSize) {
    memTracker[areaPtr].resize((size_t)(totalSize), 0);
  }

  char* updateBegin = (char*)updateptr;
  char* updateEnd = updateBegin + updatesize;
  char* storeUpdateBegin = &memTracker[areaPtr][0] + updateOffset;
  char* storeUpdateEnd = &memTracker[areaPtr][0] + updateOffset + updatesize;
  char* diffBegin = updateBegin;
  char* diffEnd = updateEnd;
  char* storeDiffBegin = storeUpdateBegin;

  char* dataPtr;
  char* storeDataPtr;

  // Find diff begin.
  for (dataPtr = updateBegin, storeDataPtr = storeUpdateBegin; dataPtr < updateEnd;
       dataPtr++, storeDataPtr++) {
    if (*dataPtr != *storeDataPtr) {
      break;
    }
  }
  diffBegin = dataPtr;
  storeDiffBegin = storeDataPtr;

  // Find diff end.
  if (diffBegin != updateEnd) {
    char* updateBack = --updateEnd;
    char* updateDataBack = --storeUpdateEnd;
    for (dataPtr = updateBack, storeDataPtr = updateDataBack; dataPtr >= diffBegin;
         dataPtr--, storeDataPtr--) {
      if (*dataPtr != *storeDataPtr) {
        break;
      }
    }
    diffEnd = ++dataPtr;
  }

  if (diffBegin != diffEnd) {
    unsigned diffSize = (unsigned)(diffEnd - diffBegin);
    // Dump diff
    uint64_t hash = CGits::Instance().ResourceManager2().put(RESOURCE_BUFFER, diffBegin, diffSize);
    uint64_t diffOffset = (uint64_t)diffBegin - (uint64_t)areaPtr;
    _updates.push_back(TData(areaPtr, hash, diffOffset));

    // Update stored memory area
    memcpy(storeDiffBegin, diffBegin, diffSize);
  }

  if (Configurator::Get().common.recorder.highIntegrity) {
    SD().WriteClientSizes();
  }
}

// Compares data blocks of specified size and dumps as many diffs as different blocks.
void gits::OpenGL::CDataUpdate::DiffMultiRange(uint64_t dataptr,
                                               uint64_t updateptr,
                                               uint64_t updatesize) {
  uint64_t areaPtr = GetAreaPtr(dataptr);
  uint64_t updateOffset = updateptr - areaPtr;
  uint64_t totalSize = updateOffset + updatesize;

  // Extend tracked memory area.
  auto& memTracker = SD()._memTracker;
  if (memTracker[areaPtr].size() < totalSize) {
    memTracker[areaPtr].resize((size_t)(totalSize), 0);
  }

  // Get diff area.
  char* updateBegin = (char*)updateptr;
  char* updateEnd = updateBegin + updatesize;
  char* storeUpdateBegin = &memTracker[areaPtr][0] + updateOffset;
  char* diffBegin = updateBegin;
  char* diffEnd = updateEnd;
  char* storeDiffBegin = storeUpdateBegin;

  char* dataPtr;
  char* storeDataPtr;

  //**Lambda - Store diff
  auto StoreDiff = [&]() {
    if (diffBegin != diffEnd) {
      unsigned diffSize = (unsigned)(diffEnd - diffBegin);
      // Dump diff.
      uint64_t hash =
          CGits::Instance().ResourceManager2().put(RESOURCE_BUFFER, diffBegin, diffSize);
      uint64_t diffOffset = (uint64_t)diffBegin - (uint64_t)areaPtr;
      _updates.push_back(TData(areaPtr, hash, diffOffset));

      // Update stored memory area.
      memcpy(storeDiffBegin, diffBegin, diffSize);
    }
  };

  // Find and dump diffs.
  unsigned cmpBlockSize = 32;
  dataPtr = updateBegin;
  storeDataPtr = storeUpdateBegin;
  auto updateBack = updateEnd - cmpBlockSize;
  while (dataPtr <= updateBack) {
    // find diff
    if (memcmp(dataPtr, storeDataPtr, cmpBlockSize) != 0) {
      // get diff size
      diffBegin = dataPtr;
      storeDiffBegin = storeDataPtr;

      while (dataPtr <= updateBack && memcmp(dataPtr, storeDataPtr, cmpBlockSize) != 0) {
        dataPtr += cmpBlockSize;
        storeDataPtr += cmpBlockSize;
      }
      unsigned ending = (unsigned)(updateEnd - dataPtr);
      if (ending < cmpBlockSize) { // ending fix up
        dataPtr = updateEnd;
      }
      diffEnd = dataPtr;
      StoreDiff();
    } else {
      dataPtr += cmpBlockSize;
      storeDataPtr += cmpBlockSize;
    }
  }
  if (dataPtr < updateEnd) {
    diffBegin = dataPtr;
    storeDiffBegin = storeDataPtr;
    diffEnd = updateEnd;
    if (memcmp(diffBegin, storeDiffBegin, (unsigned)(diffEnd - diffBegin)) != 0) {
      StoreDiff();
    }
  }

  if (Configurator::Get().common.recorder.highIntegrity) {
    SD().WriteClientSizes();
  }
}

void gits::OpenGL::CDataUpdate::Apply() {
  for (auto& updateRef : _updates) {
    uint64_t areaPtrRec = updateRef.area;
    uint64_t updateOffset = updateRef.offset;
    auto& memTracker = SD()._memTracker;

    // Allocate memory if needed.
    if (memTracker.find(areaPtrRec) == memTracker.end()) {
      memTracker[areaPtrRec].resize((size_t)GetCArraySizeFromId(areaPtrRec), 0);
    }

    // Apply diff.
    char* dataPtr = &memTracker[areaPtrRec][0] + updateOffset;
    memcpy(dataPtr, &updateRef.playerCache[0], updateRef.playerCache.size());
  }
}

void gits::OpenGL::CDataUpdate::Write(CBinOStream& stream) const {
  stream << CGLuint((unsigned)_updates.size());
  for (auto& update : _updates) {
    stream << CGLuint64(update.area);
    stream << CGLuint64(update.hash);
    stream << CGLuint64(update.offset);
  }
}

void gits::OpenGL::CDataUpdate::Read(CBinIStream& stream) {
  CGLuint size;
  CGLuint64 area;
  CGLuint64 hash;
  CGLuint64 offset;
  stream >> size;
  for (uint32_t i = 0; i < *size; i++) {
    _updates.push_back(TData());
    stream >> area;
    _updates.back().area = *area;
    stream >> hash;
    _updates.back().hash = *hash;
    stream >> offset;
    _updates.back().offset = *offset;

    // Cache
    if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
      _updates.back().playerCache =
          std::move(gits::CGits::Instance().ResourceManager().get(_updates.back().hash));
    } else {
      _updates.back().playerCache =
          std::move(gits::CGits::Instance().ResourceManager2().get(_updates.back().hash));
    }
  }
}
uint64_t gits::OpenGL::CDataUpdate::Size() const {
  uint64_t count_header = sizeof(uint32_t);
  uint64_t per_update = sizeof(uint64_t) * 3; // area + hash + offset
  return count_header + per_update * _updates.size();
}

//************************************** CCoherentBufferUpdate ********************************************************

std::unordered_map<GLuint, GLenum> gits::OpenGL::CCoherentBufferUpdate::GetCurrentBuffers(
    TCoherentBufferData::UpdateType updateType, bool oncePerFrame) {
  if (oncePerFrame == true && (SD().GetCurrentSharedStateData().coherentBufferFrameNumber !=
                               CGits::Instance().CurrentFrame())) {
    SD().GetCurrentSharedStateData().coherentBufferUpdatedSet.clear();
    SD().GetCurrentSharedStateData().coherentBufferFrameNumber = CGits::Instance().CurrentFrame();
  }
  std::unordered_map<GLuint, GLenum> buffers;
  GLint attribs = 0;
  auto insert_buffer = [&](GLint buffer, GLenum target) {
    if ((oncePerFrame == false) ||
        (SD().GetCurrentSharedStateData().coherentBufferUpdatedSet.find(buffer) ==
         SD().GetCurrentSharedStateData().coherentBufferUpdatedSet.end())) {
      buffers.insert(std::pair<GLuint, GLenum>((GLuint)buffer, target));
      if (oncePerFrame == true) {
        SD().GetCurrentSharedStateData().coherentBufferUpdatedSet.insert(buffer);
      }
    }
  };
  auto add_buffer = [&](GLenum binding_target, GLenum target) {
    GLint buf = 0;
    drv.gl.glGetIntegerv(binding_target, &buf);
    if (buf != 0) {
      insert_buffer(buf, target);
    }
  };

  if (updateType == TCoherentBufferData::PER_DRAWCALL_UPDATE ||
      updateType == TCoherentBufferData::PER_FRAME_UPDATE) {
    auto can_query_vertex_attribs = [&]() {
      bool isVaoMandatory = curctx::IsEs2Plus() || (curctx::IsOgl() && SD().IsCurrentContextCore());
      GLint vao = 0;
      drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
      return !isVaoMandatory || vao != 0;
    };

    if (can_query_vertex_attribs()) {
      drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &attribs);

      for (int i = 0; i < attribs; ++i) {
        GLint enabled = GL_FALSE;
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        if (enabled == GL_TRUE) {
          GLint buf = 0;
          drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buf);
          insert_buffer(buf, GL_ARRAY_BUFFER);
        }
      }
      add_buffer(GL_ELEMENT_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER);
      auto& indexedTargets =
          SD().GetCurrentSharedStateData().IndexedBoundBuffers().TargetsInfo()[GL_UNIFORM_BUFFER];
      for (const auto& uniformBuff : indexedTargets) {
        if (uniformBuff.second.buffer != 0) {
          insert_buffer(uniformBuff.second.buffer, GL_UNIFORM_BUFFER);
        }
      }
    }
    GLint texUnit;
    GLint maxTexUnits;
    GLint tex_buffer_boundtex;
    GLuint texbuffer_buffer = 0;
    drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &texUnit);
    drv.gl.glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);

    for (int unit = GL_TEXTURE0; unit < GL_TEXTURE0 + maxTexUnits; unit++) {
      drv.gl.glActiveTexture(unit);
      drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_EXT, &tex_buffer_boundtex);
      CTextureStateObj* tex = SD().GetCurrentSharedStateData().Textures().Get(
          CTextureStateObj(tex_buffer_boundtex, GL_TEXTURE_BUFFER));
      if (tex != nullptr) {
        texbuffer_buffer = tex->Data().track.texbuffer_buffer;
      }
      if (texbuffer_buffer > 0) {
        insert_buffer(texbuffer_buffer, GL_TEXTURE_BUFFER_EXT);
      }
      texbuffer_buffer = 0;
    }
    drv.gl.glActiveTexture(texUnit);

    if ((curctx::IsOgl() && curctx::Version() >= 430) || curctx::IsEs31Plus()) {
      GLint shaderStorageBuffer;
      drv.gl.glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &shaderStorageBuffer);
      insert_buffer(shaderStorageBuffer, GL_SHADER_STORAGE_BUFFER);

      GLint maxShaderStorageBindings;
      drv.gl.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBindings);
      for (int i = 0; i < maxShaderStorageBindings; i++) {
        drv.gl.glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, i, &shaderStorageBuffer);
        insert_buffer(shaderStorageBuffer, GL_SHADER_STORAGE_BUFFER);
      }
    }
  }

  if (curctx::IsOgl() && updateType == TCoherentBufferData::TEXTURE_UPDATE) {
    add_buffer(GL_PIXEL_UNPACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER);
  }

  buffers.erase(0);
  return buffers;
}

void gits::OpenGL::CCoherentBufferUpdate::Diff(TCoherentBufferData::UpdateType updateType,
                                               TCoherentBufferData::UpdateMode updateMode,
                                               bool oncePerFrame) {

  GLint tex_buffer_bound;
  GLint array_buffer_bound;
  GLint shader_storage_buffer_bound = 0;
  drv.gl.glGetIntegerv(GL_TEXTURE_BUFFER_BINDING, &tex_buffer_bound);
  drv.gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_buffer_bound);
  if ((curctx::IsOgl() && curctx::Version() >= 430) || curctx::IsEs31Plus()) {
    drv.gl.glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &shader_storage_buffer_bound);
  }

  for (const auto& buff : GetCurrentBuffers(updateType, oncePerFrame)) {
    GLuint buffer = buff.first;
    GLenum target = buff.second;
    GLint size = 0;
    auto& mapping = SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().restore;
    GLint access = mapping.mapAccess;
    GLint length = mapping.mapLength;
    GLint offset = mapping.mapOffset;
    bool named_buffer = mapping.named;
    CBufferStateData::Restored::buffer_type type = mapping.type;

    if (mapping.mapped &&
        SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().track.coherentMapping) {
      if (!named_buffer) {
        drv.gl.glBindBuffer(target, buffer);
      }
      auto func_map = _glMapBuffer_wrap;
      auto func_unmap = _glUnmapBuffer_wrap;
      auto func_get = _glGetBufferParameteriv_wrap;

      if (named_buffer == false) {
        if (type == CBufferStateData::Restored::MAP_BUFFER_ARB) {
          func_map = _glMapBufferARB_wrap;
          func_unmap = _glUnmapBufferARB_wrap;
        } else if (type == CBufferStateData::Restored::MAP_BUFFER_OES) {
          func_map = _glMapBufferOES_wrap;
          func_unmap = _glUnmapBufferOES_wrap;
        }
      } else {
        if (type == CBufferStateData::Restored::MAP_BUFFER_EXT) {
          func_map = _glMapNamedBufferEXT_wrap;
          func_unmap = _glUnmapNamedBufferEXT_wrap;
          func_get = _glGetNamedBufferParameterivEXT_wrap;
        }
      }

      GLvoid* pointer = nullptr;
      if (length == -1 && offset == -1) {
        if (!(access & GL_MAP_READ_BIT)) {
          // Unmapping after glMapBuffer
          auto access_type = GL_READ_ONLY;
          // buffer is remapped with write permissions again, as we can read
          // write-only mapping - read it, and save
          if ((curctx::IsEs1() || curctx::IsEs2Plus()) &&
              ESBufferState() != TBuffersState::CAPTURE_ALWAYS) {
            access_type = GL_WRITE_ONLY;
          } else { // buffer is remapped with read permissions now - read it,
                   // and save
            access_type = GL_READ_ONLY;
          }

          func_unmap(buffer, target);
          func_get(buffer, target, GL_BUFFER_SIZE, &size);
          pointer = func_map(buffer, target, access_type);
        }
      } else {
        if (!(access & GL_MAP_READ_BIT)) {
          // Unmapping after glMapBufferRange
          // here buffer is remapped with read permissions now - read it, and
          // save
          func_unmap(buffer, target);
          if (named_buffer == false) {
            pointer = drv.gl.glMapBufferRange(target, offset, length, GL_MAP_READ_BIT);
          } else {
            pointer = drv.gl.glMapNamedBufferRangeEXT(buffer, offset, length, GL_MAP_READ_BIT);
          }
        }
      }
      if (pointer == nullptr) {
        auto func_get_buffer_pointer = _glGetBufferPointerv_wrap;
        if (named_buffer == false) {
          if (type == CBufferStateData::Restored::MAP_BUFFER_ARB) {
            func_get_buffer_pointer = _glGetBufferPointervARB_wrap;
          } else if (type == CBufferStateData::Restored::MAP_BUFFER_OES) {
            func_get_buffer_pointer = _glGetBufferPointervOES_wrap;
          }
        } else {
          if (type == CBufferStateData::Restored::MAP_BUFFER_EXT) {
            func_get_buffer_pointer = _glGetNamedBufferPointervEXT_wrap;
          }
        }

        func_get_buffer_pointer(buffer, target, GL_BUFFER_MAP_POINTER, &pointer);
      }

      // Populate _resource if data was changed.
      if (pointer != nullptr) {
        auto* bufferDataPtr = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
        if (bufferDataPtr == nullptr) {
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }

        // Optimized size of mapped buffer data dump
        if (Configurator::Get().opengl.recorder.optimizeBufferSize) {
          // Access to mapped memory is multiple times slower than access to cpu
          // memory so it is better to read it only once. Though temporary copy
          // of mapped memory is created there.
          auto mapMemSrc = pointer;
          GLsizeiptr tmpMapMemCpySize = 0;
          if (bufferDataPtr->Data().restore.mapLength == -1) {
            tmpMapMemCpySize = bufferDataPtr->Data().track.size;
          } else {
            tmpMapMemCpySize = bufferDataPtr->Data().restore.mapLength;
          }
          void* tmpMapMemCpy = malloc(tmpMapMemCpySize);
          void* tmpMapMemCpyDst = tmpMapMemCpy;
          // If map range flushing is used only flushed part of memory is being
          // copied to improve recorder performance
          if (bufferDataPtr->Data().restore.mapFlushRangeLength > 0) {
            mapMemSrc = (char*)mapMemSrc + bufferDataPtr->Data().restore.mapFlushRangeOffset;
            tmpMapMemCpyDst =
                (char*)tmpMapMemCpy + bufferDataPtr->Data().restore.mapFlushRangeOffset;
            tmpMapMemCpySize = bufferDataPtr->Data().restore.mapFlushRangeLength;
          }
          memcpy(tmpMapMemCpyDst, mapMemSrc, tmpMapMemCpySize);

          GLintptr mapOffset = 0;
          GLintptr buffOffset = 0;
          GLsizeiptr mapSize = 0;
          bufferDataPtr->CalculateMapChange(mapOffset, buffOffset, mapSize, tmpMapMemCpy);
          bufferDataPtr->TrackBufferData(buffOffset, mapSize, (char*)tmpMapMemCpy + mapOffset);
          offset = (int)mapOffset;
          length = (int)mapSize;
          if (mapSize != 0) {
            uint64_t hash = CGits::Instance().ResourceManager2().put(
                RESOURCE_BUFFER, ((GLubyte*)tmpMapMemCpy) + mapOffset, mapSize);
            _updates.push_back(TCoherentBufferData(hash, offset, buffer, length, target));
          }
          free(tmpMapMemCpy);
        }
        // Entire mapped buffer data dump
        else {
          if (bufferDataPtr->Data().restore.mapLength == -1) {
            length = bufferDataPtr->Data().track.size;
          } else {
            length = bufferDataPtr->Data().restore.mapLength;
          }

          offset = 0;
          if (length != 0) {
            uint64_t hash = CGits::Instance().ResourceManager2().put(RESOURCE_BUFFER,
                                                                     ((GLubyte*)pointer), length);
            _updates.push_back(TCoherentBufferData(hash, offset, buffer, length, target));
          }
        }
      }
    }
  }
  drv.gl.glBindBuffer(GL_TEXTURE_BUFFER_EXT, tex_buffer_bound);
  drv.gl.glBindBuffer(GL_ARRAY_BUFFER, array_buffer_bound);
  if ((curctx::IsOgl() && curctx::Version() >= 430) || curctx::IsEs31Plus()) {
    drv.gl.glBindBuffer(GL_SHADER_STORAGE_BUFFER, shader_storage_buffer_bound);
  }
}

void gits::OpenGL::CCoherentBufferUpdate::Apply() {

  GLint tex_buffer_bound;
  GLint array_buffer_bound;
  GLint shader_storage_buffer_bound = 0;
  drv.gl.glGetIntegerv(GL_TEXTURE_BUFFER_BINDING, &tex_buffer_bound);
  drv.gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_buffer_bound);
  if ((curctx::IsOgl() && curctx::Version() >= 430) || curctx::IsEs31Plus()) {
    drv.gl.glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &shader_storage_buffer_bound);
  }

  for (auto& updateRef : _updates) {
    GLvoid* pointer;
    GLuint recBuff = updateRef._buffer_name;

    GLuint playBuff;
    GLenum target = updateRef._target;
    GLuint offset = updateRef._offset;

    playBuff = CGLBuffer::GetMapping(recBuff);
    auto func_get_buffer_pointer = _glGetBufferPointerv_wrap;
    auto& restoredData = SD().GetCurrentSharedStateData().Buffers().Get(playBuff)->Data().restore;
    if (restoredData.named == false) {
      drv.gl.glBindBuffer(target, playBuff);
      if (restoredData.type == CBufferStateData::Restored::MAP_BUFFER_ARB) {
        func_get_buffer_pointer = _glGetBufferPointervARB_wrap;
      } else if (restoredData.type == CBufferStateData::Restored::MAP_BUFFER_OES) {
        func_get_buffer_pointer = _glGetBufferPointervOES_wrap;
      }
    } else {
      if (restoredData.type == CBufferStateData::Restored::MAP_BUFFER_EXT) {
        func_get_buffer_pointer = _glGetNamedBufferPointervEXT_wrap;
      }
    }

    func_get_buffer_pointer(playBuff, target, GL_BUFFER_MAP_POINTER, &pointer);

    if (playBuff == 0) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unmapping buffer zero");
    } else if (SD().GetCurrentSharedStateData().Buffers().Get(playBuff) == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer object");
    }
    if (!SD().GetCurrentSharedStateData().Buffers().Get(playBuff)->Data().restore.mapped) {
      LOG_WARNING << "Unmapping unmapped buffer";
    } else {
      std::memcpy(((GLubyte*)pointer) + offset, &updateRef.playerCache[0],
                  updateRef.playerCache.size());
      updateRef.playerCache.clear();
    }
  }
  drv.gl.glBindBuffer(GL_TEXTURE_BUFFER_EXT, tex_buffer_bound);
  drv.gl.glBindBuffer(GL_ARRAY_BUFFER, array_buffer_bound);
  if ((curctx::IsOgl() && curctx::Version() >= 430) || curctx::IsEs31Plus()) {
    drv.gl.glBindBuffer(GL_SHADER_STORAGE_BUFFER, shader_storage_buffer_bound);
  }
}

void gits::OpenGL::CCoherentBufferUpdate::Write(CBinOStream& stream) const {
  stream << CGLuint((unsigned)_updates.size());
  for (auto& update : _updates) {
    stream << CGLuint64(update._hash);
    stream << CGLuint(update._offset);
    stream << CGLuint(update._buffer_name);
    stream << CGLuint(update._length);
    stream << CGLuint(update._target);
  }
}

void gits::OpenGL::CCoherentBufferUpdate::Read(CBinIStream& stream) {
  CGLuint size;
  CGLuint64 hash;
  CGLuint offset;
  CGLuint buffer_name;
  CGLuint length;
  CGLuint target;
  stream >> size;
  for (uint32_t i = 0; i < *size; i++) {
    _updates.push_back(TCoherentBufferData());
    stream >> hash;
    _updates.back()._hash = *hash;
    stream >> offset;
    _updates.back()._offset = *offset;
    stream >> buffer_name;
    _updates.back()._buffer_name = *buffer_name;
    stream >> length;
    _updates.back()._length = *length;
    stream >> target;
    _updates.back()._target = *target;

    // Cache
    if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
      _updates.back().playerCache =
          std::move(gits::CGits::Instance().ResourceManager().get(_updates.back()._hash));
    } else {
      _updates.back().playerCache =
          std::move(gits::CGits::Instance().ResourceManager2().get(_updates.back()._hash));
    }
  }
}

uint64_t gits::OpenGL::CCoherentBufferUpdate::Size() const {
  uint64_t count_header = sizeof(uint32_t);
  uint64_t per_update = sizeof(uint64_t) + sizeof(uint32_t) * 4;
  return count_header + per_update * _updates.size();
}

/* ******************************** CGLSamplerType****************************** */
const char* gits::OpenGL::CGLSamplerType::NAME = "GLuint";

/* ********************************* G L C L A M P X ******************************* */

const char* gits::OpenGL::CGLclampx::NAME = "GLclampx";

gits::OpenGL::CGLclampx::CGLclampx() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLclampx::CGLclampx(GLclampx value) : CGLtype<GLtype, type>(value) {}

/* ********************************* G L F I X E D ******************************* */

const char* gits::OpenGL::CGLfixed::NAME = "GLfixed";

gits::OpenGL::CGLfixed::CGLfixed() : CGLtype<GLtype, type>() {}

gits::OpenGL::CGLfixed::CGLfixed(GLfixed value) : CGLtype<GLtype, type>(value) {}

/* ******************************** CGLsyncType ****************************** */
const char* gits::OpenGL::CGLsyncType::NAME = "GLsync";
gits::OpenGL::CGLsyncType::CGLsyncType() {}

gits::OpenGL::CGLsyncType::CGLsyncType(GLsync value) : CGLtype<GLtype, type>(value) {}

/*****************************************************************************/

gits::OpenGL::CGLIndirectCmds::CGLIndirectCmds() : ptr_value_(0), count_(0), buffer_(0) {}

gits::OpenGL::CGLIndirectCmds::CGLIndirectCmds(const void* ptr, uint32_t count) : count_(count) {
  drv.gl.glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &buffer_);
  if (buffer_ == 0) {
    const DrawArraysIndirectCommand* cmd_ptr = (DrawArraysIndirectCommand*)ptr;
    cmds_.assign(cmd_ptr, cmd_ptr + count);
  }
  ptr_value_ = reinterpret_cast<uintptr_t>(ptr);
}

const void* gits::OpenGL::CGLIndirectCmds::operator*() {
  if (buffer_ == 0) {
    return &cmds_[0];
  }
  return reinterpret_cast<const void*>(static_cast<uintptr_t>(ptr_value_));
}

void gits::OpenGL::CGLIndirectCmds::Write(CBinOStream& stream) const {
  write_to_stream(stream, ptr_value_);
  write_to_stream(stream, count_);
  write_to_stream(stream, buffer_);
  if (buffer_ == 0) {
    for (uint32_t i = 0; i < count_; ++i) {
      write_to_stream(stream, cmds_[i]);
    }
  }
}

void gits::OpenGL::CGLIndirectCmds::Read(CBinIStream& stream) {
  read_from_stream(stream, ptr_value_);
  read_from_stream(stream, count_);
  read_from_stream(stream, buffer_);
  if (buffer_ == 0) {
    if (count_ <= cmds_.max_size()) {
      cmds_.resize(count_);
      for (uint32_t i = 0; i < count_; ++i) {
        read_from_stream(stream, cmds_[i]);
      }
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
}

uint64_t gits::OpenGL::CGLIndirectCmds::Size() const {
  return sizeof(ptr_value_) + sizeof(count_) + sizeof(buffer_) +
         sizeof(DrawArraysIndirectCommand) * cmds_.size();
}

gits::OpenGL::CGLGenericResource::CGLGenericResource(const GLvoid* ptr,
                                                     size_t size,
                                                     GLenum sourceTarget,
                                                     TResourceType resourceType) {
  // Get currently bound source buffer
  if (CGits::Instance().IsStateRestoration()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  GLuint buffer = 0;
  if (sourceTarget != 0) {
    drv.gl.glGetIntegerv(GetBindingEnum(sourceTarget), (GLint*)&buffer);
  }

  *_isBuff = buffer != 0;
  if (buffer == 0) {
    _resource.reset(resourceType, ptr, size);
  } else {
    *_buffOffset = (GLuint64)ptr;
  }
}

gits::CBinaryResource::PointerProxy gits::OpenGL::CGLGenericResource::Value() {
  if (*_isBuff == 0) {
    return _resource.Data();
  } else {
    return CBinaryResource::PointerProxy((void*)*_buffOffset);
  }
}

gits::CBinaryResource::PointerProxy gits::OpenGL::CGLGenericResource::operator*() {
  return Value();
}

void gits::OpenGL::CGLGenericResource::Write(CBinOStream& stream) const {
  stream << _isBuff;
  if (*_isBuff == 0) {
    stream << _resource;
  } else {
    stream << _buffOffset;
  }
}

void gits::OpenGL::CGLGenericResource::Read(CBinIStream& stream) {
  stream >> _isBuff;
  if (*_isBuff == 0) {
    stream >> _resource;
  } else {
    stream >> _buffOffset;
  }
}

uint64_t gits::OpenGL::CGLGenericResource::Size() const {
  uint64_t total = _isBuff.Size();
  if (*_isBuff == 0) {
    total += _resource.Size();
  } else {
    total += _buffOffset.Size();
  }
  return total;
}

gits::OpenGL::CGLTexResource::CGLTexResource(
    GLenum target, GLenum format, GLenum type, GLsizei width, const GLvoid* ptr)
    : _resource(
          ptr, TexDataSize(format, type, width, 1, 1), GL_PIXEL_UNPACK_BUFFER, RESOURCE_TEXTURE) {}

gits::OpenGL::CGLTexResource::CGLTexResource(
    GLenum target, GLenum format, GLenum type, GLsizei width, GLsizei height, const GLvoid* ptr)
    : _resource(ptr,
                TexDataSize(format, type, width, height, 1),
                GL_PIXEL_UNPACK_BUFFER,
                RESOURCE_TEXTURE) {}

gits::OpenGL::CGLTexResource::CGLTexResource(GLenum target,
                                             GLenum format,
                                             GLenum type,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth,
                                             const GLvoid* ptr)
    : _resource(ptr,
                TexDataSize(format, type, width, height, depth),
                GL_PIXEL_UNPACK_BUFFER,
                RESOURCE_TEXTURE) {}

gits::OpenGL::CGLCompressedTexResource::CGLCompressedTexResource(GLenum target,
                                                                 GLsizei width,
                                                                 GLsizei size,
                                                                 const GLvoid* ptr)
    : _resource(
          ptr, CompressedTexDataSize(width, 1, 1, size), GL_PIXEL_UNPACK_BUFFER, RESOURCE_TEXTURE) {
}

gits::OpenGL::CGLCompressedTexResource::CGLCompressedTexResource(
    GLenum target, GLsizei width, GLsizei height, GLsizei size, const GLvoid* ptr)
    : _resource(ptr,
                CompressedTexDataSize(width, height, 1, size),
                GL_PIXEL_UNPACK_BUFFER,
                RESOURCE_TEXTURE) {}

gits::OpenGL::CGLCompressedTexResource::CGLCompressedTexResource(
    GLenum target, GLsizei width, GLsizei height, GLsizei depth, GLsizei size, const GLvoid* ptr)
    : _resource(ptr,
                CompressedTexDataSize(width, height, depth, size),
                GL_PIXEL_UNPACK_BUFFER,
                RESOURCE_TEXTURE) {}

gits::OpenGL::CGLClearTexResource::CGLClearTexResource(
    const void* data, GLuint texture, GLint level, GLenum format, GLenum type)
    : _isNullPtr(data == nullptr) {
  if (!*_isNullPtr) {
    _resource = std::make_unique<CGLGenericResource>(data, texelSize(format, type),
                                                     GL_PIXEL_UNPACK_BUFFER, RESOURCE_TEXTURE);
  }
}

gits::OpenGL::CGLBitmapResource::CGLBitmapResource(GLsizei width,
                                                   GLsizei height,
                                                   const GLubyte* bitmap)
    : _resource(bitmap, BitmapDataSize(width, height), GL_PIXEL_UNPACK_BUFFER, RESOURCE_TEXTURE) {}
