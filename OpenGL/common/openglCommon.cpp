// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openglCommon.h"
#include "log.h"
#include "tools.h"
#include "openglDrivers.h"
#include "openglEnums.h"
#include "stateDynamic.h"
#include <cmath>

namespace gits {
namespace OpenGL {
unsigned indexSize(GLenum type) {
  switch (type) {
  case GL_UNSIGNED_BYTE:
    return sizeof(GLubyte);
  case GL_UNSIGNED_SHORT:
    return sizeof(GLushort);
  case GL_UNSIGNED_INT:
    return sizeof(GLuint);
  default:
    LOG_ERROR << "Index type: '0x" << std::hex << type << "'!!!";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

unsigned texelElementsNum(GLenum format) {
  switch (format) {
  case GL_COLOR_INDEX:
  case GL_RED:
  case GL_RED_INTEGER:
  case GL_GREEN:
  case GL_GREEN_INTEGER:
  case GL_BLUE:
  case GL_BLUE_INTEGER:
  case GL_ALPHA:
  case GL_ALPHA_INTEGER:
  case GL_LUMINANCE:
  case GL_LUMINANCE_INTEGER_EXT:
  case GL_DEPTH_COMPONENT:
  case GL_DEPTH_STENCIL:
  case GL_STENCIL_INDEX:
  case GL_INTENSITY:
    return 1;
  case GL_YCBCR_422_APPLE:
  case GL_RG:
  case GL_RG_INTEGER:
  case GL_LUMINANCE_ALPHA:
  case GL_LUMINANCE_ALPHA_INTEGER_EXT:
    return 2;
  case GL_RGB:
  case GL_RGB_INTEGER:
  case GL_BGR:
  case GL_BGR_INTEGER:
    return 3;
  case GL_RGBA:
  case GL_RGBA_INTEGER:
  case GL_BGRA:
  case GL_BGRA_INTEGER:
  case GL_ABGR_EXT:
    return 4;
  default:
    LOG_ERROR << "Texture format: '0x" << std::hex << format << "'!!!";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

unsigned basicTexelSize(GLenum format, GLenum type) {
  unsigned dataNum = texelElementsNum(format);
  unsigned dataComp = 1;
  unsigned dataSize = 0;

  switch (type) {
  case GL_UNSIGNED_BYTE:
    dataSize = sizeof(GLubyte);
    break;
  case GL_BYTE:
    dataSize = sizeof(GLbyte);
    break;
  case GL_UNSIGNED_SHORT:
    dataSize = sizeof(GLushort);
    break;
  case GL_SHORT:
    dataSize = sizeof(GLshort);
    break;
  case GL_UNSIGNED_INT:
    dataSize = sizeof(GLuint);
    break;
  case GL_INT:
    dataSize = sizeof(GLint);
    break;
  case GL_FLOAT:
    dataSize = sizeof(GLfloat);
    break;
  case GL_HALF_FLOAT:
  case GL_HALF_FLOAT_OES:
    dataSize = sizeof(GLhalfNV);
    break;
  case GL_BITMAP:
    dataSize = sizeof(GLubyte);
    break;
  case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
    dataSize = sizeof(GLfloat) + sizeof(GLuint);
    break;
  default:
    LOG_ERROR << "Texture type: '0x" << std::hex << type << "'!!!";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return (dataNum / dataComp) * dataSize;
}

unsigned texelSize(GLenum format, GLenum type) {
  switch (type) {
  case GL_UNSIGNED_BYTE_3_3_2:
  case GL_UNSIGNED_BYTE_2_3_3_REV:
    return 1;
  case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
  case GL_UNSIGNED_SHORT_8_8_APPLE:
  case GL_UNSIGNED_SHORT_5_6_5:
  case GL_UNSIGNED_SHORT_5_6_5_REV:
  case GL_UNSIGNED_SHORT_4_4_4_4:
  case GL_UNSIGNED_SHORT_4_4_4_4_REV:
  case GL_UNSIGNED_SHORT_5_5_5_1:
  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    return 2;
  case GL_UNSIGNED_INT_24_8:
  case GL_UNSIGNED_INT_8_8_8_8:
  case GL_UNSIGNED_INT_8_8_8_8_REV:
  case GL_UNSIGNED_INT_5_9_9_9_REV:
  case GL_UNSIGNED_INT_10_10_10_2:
  case GL_UNSIGNED_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_10F_11F_11F_REV:
    return 4;
  default:
    return basicTexelSize(format, type);
  }
}

bool isTexelDataConsistent(GLenum type) {
  switch (type) {
  case GL_UNSIGNED_BYTE_3_3_2:
  case GL_UNSIGNED_BYTE_2_3_3_REV:
  case GL_UNSIGNED_SHORT_5_6_5:
  case GL_UNSIGNED_SHORT_5_6_5_REV:
  case GL_UNSIGNED_SHORT_5_5_5_1:
  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
  case GL_UNSIGNED_INT_10_10_10_2:
  case GL_UNSIGNED_INT_2_10_10_10_REV:
  case GL_UNSIGNED_INT_24_8:
  case GL_UNSIGNED_INT_10F_11F_11F_REV:
  case GL_UNSIGNED_INT_5_9_9_9_REV:
  case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
    return false;
  default:
    return true;
  }
}

const char* typeName(GLenum type) {
  switch (type) {
  case GL_BYTE:
    return "GLbyte";
  case GL_UNSIGNED_BYTE:
    return "GLubyte";
  case GL_SHORT:
    return "GLshort";
  case GL_UNSIGNED_SHORT:
    return "GLushort";
  case GL_INT:
    return "GLint";
  case GL_UNSIGNED_INT:
    return "GLuint";
  case GL_FLOAT:
    return "GLfloat";
  case GL_DOUBLE:
    return "GLdouble";
  case GL_HALF_FLOAT:
    return "GLhalf";
    break;
  default:
    LOG_ERROR << "Unknown GL type in 'typeName()' - " << type;
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + "Unknown GL type");
  }
}

const char* shaderTypeAbrvName(GLenum type) {
  switch (type) {
  case GL_VERTEX_SHADER:
    return "vert";
  case GL_TESS_CONTROL_SHADER:
    return "tesC";
  case GL_TESS_EVALUATION_SHADER:
    return "tesE";
  case GL_GEOMETRY_SHADER:
    return "geom";
  case GL_FRAGMENT_SHADER:
    return "frag";
  case GL_COMPUTE_SHADER:
    return "comp";
  default:
    LOG_WARNING << "Unknown shader type in 'shaderTypeAbrvName()' - " << type;
    return "unknown";
  }
}

GLenum GetBindingEnum(GLenum target) {
  typedef std::unordered_map<GLenum, GLenum> map_t;
  INIT_NEW_STATIC_OBJ(bindingEnumMap, map_t)
  if (bindingEnumMap.empty()) {
    // Buffers
    bindingEnumMap[GL_ARRAY_BUFFER] = GL_ARRAY_BUFFER_BINDING;
    bindingEnumMap[GL_ATOMIC_COUNTER_BUFFER] = GL_ATOMIC_COUNTER_BUFFER_BINDING;
    bindingEnumMap[GL_COPY_READ_BUFFER] = GL_COPY_READ_BUFFER_BINDING;
    bindingEnumMap[GL_COPY_WRITE_BUFFER] = GL_COPY_WRITE_BUFFER_BINDING;
    bindingEnumMap[GL_DRAW_INDIRECT_BUFFER] = GL_DRAW_INDIRECT_BUFFER_BINDING;
    bindingEnumMap[GL_ELEMENT_ARRAY_BUFFER] = GL_ELEMENT_ARRAY_BUFFER_BINDING;
    bindingEnumMap[GL_PIXEL_PACK_BUFFER] = GL_PIXEL_PACK_BUFFER_BINDING;
    bindingEnumMap[GL_PIXEL_UNPACK_BUFFER] = GL_PIXEL_UNPACK_BUFFER_BINDING;
    bindingEnumMap[GL_TEXTURE_BUFFER] = GL_TEXTURE_BINDING_BUFFER;
    bindingEnumMap[GL_TRANSFORM_FEEDBACK_BUFFER] = GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
    bindingEnumMap[GL_UNIFORM_BUFFER] = GL_UNIFORM_BUFFER_BINDING;
    bindingEnumMap[GL_SHADER_STORAGE_BUFFER] = GL_SHADER_STORAGE_BUFFER_BINDING;

    // Textures
    bindingEnumMap[GL_TEXTURE_1D] = GL_TEXTURE_BINDING_1D;
    bindingEnumMap[GL_TEXTURE_2D] = GL_TEXTURE_BINDING_2D;
    bindingEnumMap[GL_TEXTURE_3D] = GL_TEXTURE_BINDING_3D;
    bindingEnumMap[GL_TEXTURE_1D_ARRAY] = GL_TEXTURE_BINDING_1D_ARRAY;
    bindingEnumMap[GL_TEXTURE_2D_ARRAY] = GL_TEXTURE_BINDING_2D_ARRAY;
    bindingEnumMap[GL_TEXTURE_RECTANGLE] = GL_TEXTURE_BINDING_RECTANGLE;
    bindingEnumMap[GL_TEXTURE_EXTERNAL_OES] = GL_TEXTURE_BINDING_EXTERNAL_OES;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_POSITIVE_X] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_NEGATIVE_X] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_POSITIVE_Y] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_POSITIVE_Z] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z] = GL_TEXTURE_BINDING_CUBE_MAP;
    bindingEnumMap[GL_TEXTURE_CUBE_MAP_ARRAY] = GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
    bindingEnumMap[GL_TEXTURE_2D_MULTISAMPLE] = GL_TEXTURE_BINDING_2D_MULTISAMPLE;
    bindingEnumMap[GL_TEXTURE_2D_MULTISAMPLE_ARRAY] = GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;

    // FBOs
    bindingEnumMap[GL_DRAW_FRAMEBUFFER] = GL_DRAW_FRAMEBUFFER_BINDING;
    bindingEnumMap[GL_READ_FRAMEBUFFER] = GL_READ_FRAMEBUFFER_BINDING;
    bindingEnumMap[GL_FRAMEBUFFER] = GL_FRAMEBUFFER_BINDING;

    // RBO
    bindingEnumMap[GL_RENDERBUFFER] = GL_RENDERBUFFER_BINDING;
    bindingEnumMap[0] = 0;
  }
  // Special case. Not sure of the #define documented in
  // http://www.opengl.org/registry/specs/ARB/texture_buffer_object.txt to query
  // the binding point.
  if (target == GL_TEXTURE_BUFFER) {
    LOG_WARNING << "Buffer binding point for GL_TEXTURE_BUFFER queried";
  }

  map_t::iterator iter = bindingEnumMap.find(target);
  if (iter == bindingEnumMap.end()) {
    LOG_ERROR << "Unknown target enum: " << target << " to get binding enum.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return iter->second;
}

bool isTargetProxy(GLenum targetMode) {
  switch (targetMode) {
  case GL_PROXY_TEXTURE_1D:
  case GL_PROXY_TEXTURE_2D:
  case GL_PROXY_TEXTURE_3D:
  case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
  case GL_PROXY_TEXTURE_4D_SGIS:
  case GL_PROXY_TEXTURE_RECTANGLE:
  case GL_PROXY_TEXTURE_CUBE_MAP:
  case GL_PROXY_TEXTURE_1D_STACK_MESAX:
  case GL_PROXY_TEXTURE_2D_STACK_MESAX:
  case GL_PROXY_TEXTURE_1D_ARRAY:
  case GL_PROXY_TEXTURE_2D_ARRAY:
  case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
  case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
  case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
    return true;
  default:
    return false;
  }
}

bool isDepthFormat(GLenum internalFormat) {
  switch (internalFormat) {
  case GL_DEPTH_COMPONENT:
  case GL_DEPTH_COMPONENT16:
  case GL_DEPTH_COMPONENT24:
  case GL_DEPTH_COMPONENT32:
  case GL_DEPTH_STENCIL:
  case GL_DEPTH24_STENCIL8:
  case GL_DEPTH_COMPONENT32F:
  case GL_DEPTH32F_STENCIL8:
    return true;
  default:
    return false;
  }
}

bool isStencilFormat(GLenum internalFormat) {
  switch (internalFormat) {
  case GL_DEPTH_STENCIL:
  case GL_DEPTH24_STENCIL8:
  case GL_DEPTH_COMPONENT32F:
  case GL_DEPTH32F_STENCIL8:
    return true;
  default:
    return false;
  }
}

std::string drawModeToString(GLenum drawMode) {
  typedef std::map<GLenum, std::string> map_t;
  INIT_NEW_STATIC_OBJ(modesMap, map_t)
  if (modesMap.empty()) {
    modesMap[GL_POINTS] = "GL_POINTS";
    modesMap[GL_LINE_STRIP] = "GL_LINE_STRIP";
    modesMap[GL_LINE_LOOP] = "GL_LINE_LOOP";
    modesMap[GL_LINES] = "GL_LINES";
    modesMap[GL_TRIANGLE_STRIP] = "GL_TRIANGLE_STRIP";
    modesMap[GL_TRIANGLE_FAN] = "GL_TRIANGLE_FAN";
    modesMap[GL_TRIANGLES] = "GL_TRIANGLES";
    modesMap[GL_QUAD_STRIP] = "GL_QUAD_STRIP";
    modesMap[GL_QUADS] = "GL_QUADS";
    modesMap[GL_POLYGON] = "GL_POLYGON";
    modesMap[GL_PATCHES] = "GL_PATCHES";
  }
  return modesMap[drawMode];
}

bool getDataFromInternalFormat(GLenum internalFormat, GLint& retFormat, GLenum& retType) {
  typedef std::map<GLenum, std::pair<GLint, GLenum>> map_t;
  INIT_NEW_STATIC_OBJ(formatsMap, map_t)

  if (formatsMap.empty()) {
    formatsMap[GL_ALPHA4] = std::pair<GLint, GLenum>(GL_ALPHA, GL_UNSIGNED_BYTE);
    formatsMap[GL_ALPHA8] = std::pair<GLint, GLenum>(GL_ALPHA, GL_UNSIGNED_BYTE);
    formatsMap[GL_ALPHA12] = std::pair<GLint, GLenum>(GL_ALPHA, GL_UNSIGNED_SHORT);
    formatsMap[GL_ALPHA16] = std::pair<GLint, GLenum>(GL_ALPHA, GL_UNSIGNED_SHORT);
    formatsMap[GL_ALPHA] = formatsMap[GL_ALPHA16];
    formatsMap[GL_R8] = std::pair<GLint, GLenum>(GL_RED, GL_UNSIGNED_BYTE);
    formatsMap[GL_R8_SNORM] = std::pair<GLint, GLenum>(GL_RED, GL_BYTE);
    formatsMap[GL_R16] = std::pair<GLint, GLenum>(GL_RED, GL_UNSIGNED_SHORT);
    formatsMap[GL_R16_SNORM] = std::pair<GLint, GLenum>(GL_RED, GL_SHORT);
    formatsMap[GL_R] = formatsMap[GL_R16];
    formatsMap[GL_RED] = formatsMap[GL_R16];
    formatsMap[GL_RG8] = std::pair<GLint, GLenum>(GL_RG, GL_UNSIGNED_BYTE);
    formatsMap[GL_RG8_SNORM] = std::pair<GLint, GLenum>(GL_RG, GL_BYTE);
    formatsMap[GL_RG16] = std::pair<GLint, GLenum>(GL_RG, GL_UNSIGNED_SHORT);
    formatsMap[GL_RG16_SNORM] = std::pair<GLint, GLenum>(GL_RG, GL_SHORT);
    formatsMap[GL_RG] = formatsMap[GL_RG16];
    formatsMap[GL_R3_G3_B2] = std::pair<GLint, GLenum>(GL_RG, GL_UNSIGNED_BYTE_3_3_2);
    formatsMap[GL_RGB4] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGB5] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGB8] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGB8_SNORM] = std::pair<GLint, GLenum>(GL_RGB, GL_BYTE);
    formatsMap[GL_RGB10] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_SHORT);
    formatsMap[GL_RGB12] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_SHORT);
    formatsMap[GL_RGB16] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_SHORT);
    formatsMap[GL_RGB16_SNORM] = std::pair<GLint, GLenum>(GL_RGB, GL_SHORT);
    if (curctx::IsEs()) {
      // Common ES format and type
      formatsMap[GL_RGB] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_BYTE);
    } else {
      formatsMap[GL_RGB] = formatsMap[GL_RGB16];
    }
    formatsMap[GL_RGBA2] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGBA4] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGB5_A1] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGBA8] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_BYTE);
    formatsMap[GL_RGBA8_SNORM] = std::pair<GLint, GLenum>(GL_RGBA, GL_BYTE);
    formatsMap[GL_RGB10_A2] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_INT);
    formatsMap[GL_RGB10_A2UI] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA12] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_SHORT);
    formatsMap[GL_RGBA16] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_SHORT);
    formatsMap[GL_RGBA16_SNORM] = std::pair<GLint, GLenum>(GL_RGBA, GL_SHORT);
    if (curctx::IsEs()) {
      // Common ES format and type
      formatsMap[GL_RGBA] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_BYTE);
    } else {
      formatsMap[GL_RGBA] = formatsMap[GL_RGBA16];
    }
    formatsMap[GL_RGB565] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
    formatsMap[GL_SRGB8] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_BYTE);
    formatsMap[GL_SRGB] = formatsMap[GL_SRGB8];
    formatsMap[GL_SRGB8_ALPHA8] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_INT);
    formatsMap[GL_SRGB_ALPHA] = formatsMap[GL_SRGB8_ALPHA8];
    formatsMap[GL_R16F] = std::pair<GLint, GLenum>(GL_RED, GL_FLOAT);
    formatsMap[GL_RG16F] = std::pair<GLint, GLenum>(GL_RG, GL_FLOAT);
    formatsMap[GL_RGB16F] = std::pair<GLint, GLenum>(GL_RGB, GL_FLOAT);
    formatsMap[GL_RGBA16F] = std::pair<GLint, GLenum>(GL_RGBA, GL_HALF_FLOAT);
    formatsMap[GL_R32F] = std::pair<GLint, GLenum>(GL_RED, GL_FLOAT);
    formatsMap[GL_RG32F] = std::pair<GLint, GLenum>(GL_RG, GL_FLOAT);
    formatsMap[GL_RGB32F] = std::pair<GLint, GLenum>(GL_RGB, GL_FLOAT);
    formatsMap[GL_RGBA32F] = std::pair<GLint, GLenum>(GL_RGBA, GL_FLOAT);
    formatsMap[GL_R11F_G11F_B10F] =
        std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV);
    formatsMap[GL_RGB9_E5] = std::pair<GLint, GLenum>(GL_RGB, GL_FLOAT);
    formatsMap[GL_R8I] = std::pair<GLint, GLenum>(GL_RED_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_R8UI] = std::pair<GLint, GLenum>(GL_RED_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_R16I] = std::pair<GLint, GLenum>(GL_RED_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_R16UI] = std::pair<GLint, GLenum>(GL_RED_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_R32I] = std::pair<GLint, GLenum>(GL_RED_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_R32UI] = std::pair<GLint, GLenum>(GL_RED_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RG8I] = std::pair<GLint, GLenum>(GL_RG_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RG8UI] = std::pair<GLint, GLenum>(GL_RG_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RG16I] = std::pair<GLint, GLenum>(GL_RG_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RG16UI] = std::pair<GLint, GLenum>(GL_RG_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RG32I] = std::pair<GLint, GLenum>(GL_RG_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RG32UI] = std::pair<GLint, GLenum>(GL_RG_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGB8I] = std::pair<GLint, GLenum>(GL_RGB_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGB8UI] = std::pair<GLint, GLenum>(GL_RGB_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGB16I] = std::pair<GLint, GLenum>(GL_RGB_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGB16UI] = std::pair<GLint, GLenum>(GL_RGB_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGB32I] = std::pair<GLint, GLenum>(GL_RGB_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGB32UI] = std::pair<GLint, GLenum>(GL_RGB_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA8I] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA8UI] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA16I] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA16UI] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA32I] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_RGBA32UI] = std::pair<GLint, GLenum>(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
    formatsMap[GL_LUMINANCE4] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_BYTE);
    formatsMap[GL_LUMINANCE8] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_BYTE);
    formatsMap[GL_LUMINANCE12] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_SHORT);
    formatsMap[GL_LUMINANCE16] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_SHORT);
    formatsMap[GL_LUMINANCE] = formatsMap[GL_LUMINANCE16];
    formatsMap[GL_LUMINANCE4_ALPHA4] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
    formatsMap[GL_LUMINANCE6_ALPHA2] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
    formatsMap[GL_LUMINANCE8_ALPHA8] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT);
    formatsMap[GL_LUMINANCE12_ALPHA4] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT);
    formatsMap[GL_LUMINANCE12_ALPHA12] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_INT);
    formatsMap[GL_LUMINANCE16_ALPHA16] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_INT);
    formatsMap[GL_LUMINANCE_ALPHA] = formatsMap[GL_LUMINANCE16_ALPHA16];
    formatsMap[GL_INTENSITY4] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_BYTE);
    formatsMap[GL_INTENSITY8] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_BYTE);
    formatsMap[GL_INTENSITY12] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_SHORT);
    formatsMap[GL_INTENSITY16] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_SHORT);
    formatsMap[GL_INTENSITY] = formatsMap[GL_INTENSITY16];
    formatsMap[GL_SLUMINANCE] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_INT);
    formatsMap[GL_SLUMINANCE8] = std::pair<GLint, GLenum>(GL_LUMINANCE, GL_UNSIGNED_BYTE);
    formatsMap[GL_SLUMINANCE8_ALPHA8] =
        std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT);
    formatsMap[GL_SLUMINANCE_ALPHA] = std::pair<GLint, GLenum>(GL_LUMINANCE_ALPHA, GL_UNSIGNED_INT);
    formatsMap[GL_DEPTH_COMPONENT16] =
        std::pair<GLint, GLenum>(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    formatsMap[GL_DEPTH_COMPONENT24] =
        std::pair<GLint, GLenum>(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    formatsMap[GL_DEPTH_COMPONENT32] =
        std::pair<GLint, GLenum>(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    formatsMap[GL_DEPTH_COMPONENT32F] = std::pair<GLint, GLenum>(GL_DEPTH_COMPONENT, GL_FLOAT);
    formatsMap[GL_DEPTH_COMPONENT] = formatsMap[GL_DEPTH_COMPONENT32];
    formatsMap[GL_DEPTH24_STENCIL8] =
        std::pair<GLint, GLenum>(GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
    formatsMap[GL_DEPTH32F_STENCIL8] =
        std::pair<GLint, GLenum>(GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
    formatsMap[GL_DEPTH_STENCIL] = formatsMap[GL_DEPTH24_STENCIL8];
    formatsMap[1] = std::pair<GLint, GLenum>(GL_RED, GL_UNSIGNED_BYTE);
    formatsMap[2] = std::pair<GLint, GLenum>(GL_RG, GL_UNSIGNED_BYTE);
    formatsMap[3] = std::pair<GLint, GLenum>(GL_RGB, GL_UNSIGNED_BYTE);
    formatsMap[4] = std::pair<GLint, GLenum>(GL_RGBA, GL_UNSIGNED_BYTE);
  }
  std::map<GLenum, std::pair<GLint, GLenum>>::iterator it = formatsMap.find(internalFormat);
  if (it != formatsMap.end()) {
    retFormat = it->second.first;
    retType = it->second.second;
    return true;
  }
  return false;
}

GLenum GetSizeMapEnum(GLenum target) {
  static std::pair<GLenum, GLenum> tmparray[] = {
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_I_TO_I, GL_PIXEL_MAP_I_TO_I_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_S_TO_S, GL_PIXEL_MAP_S_TO_S_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_I_TO_R, GL_PIXEL_MAP_I_TO_R_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_I_TO_G, GL_PIXEL_MAP_I_TO_G_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_I_TO_B, GL_PIXEL_MAP_I_TO_B_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_I_TO_A, GL_PIXEL_MAP_I_TO_A_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_R_TO_R, GL_PIXEL_MAP_R_TO_R_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_G_TO_G, GL_PIXEL_MAP_G_TO_G_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_B_TO_B, GL_PIXEL_MAP_B_TO_B_SIZE),
      std::pair<GLenum, GLenum>(GL_PIXEL_MAP_A_TO_A, GL_PIXEL_MAP_A_TO_A_SIZE)};

  static std::map<GLenum, GLenum> sizeMapEnumMap(
      tmparray, tmparray + (sizeof(tmparray) / sizeof(tmparray[0])));
  std::map<GLenum, GLenum>::iterator iter = sizeMapEnumMap.find(target);

  if (iter != sizeMapEnumMap.end()) {
    return iter->second;
  } else {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void FboToImageMatch(GLenum fboAttachment, GLenum& format, GLenum& type) {
  switch (fboAttachment) {
  case GL_COLOR_ATTACHMENT0:
    format = GL_RGBA;
    type = GL_FLOAT;
    break;
  case GL_DEPTH_ATTACHMENT:
    format = GL_DEPTH_COMPONENT;
    type = GL_UNSIGNED_INT;
    break;
  case GL_STENCIL_ATTACHMENT:
    format = GL_STENCIL_INDEX;
    type = GL_UNSIGNED_BYTE;
    break;
  case GL_DEPTH_STENCIL_ATTACHMENT:
    format = GL_DEPTH_STENCIL;
    type = GL_UNSIGNED_INT_24_8;
    break;
  default:
    LOG_ERROR << "Unknown Fbo attachment: " << fboAttachment;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

GLenum InternalFormatToFboMatch(GLenum format) {
  typedef std::map<GLenum, GLenum> TFormatToFbo;
  INIT_NEW_STATIC_OBJ(formatToFboMap, TFormatToFbo)
  if (formatToFboMap.size() == 0) {
    formatToFboMap[GL_DEPTH24_STENCIL8] = GL_DEPTH_STENCIL_ATTACHMENT;
    formatToFboMap[GL_DEPTH32F_STENCIL8] = GL_DEPTH_STENCIL_ATTACHMENT;
    formatToFboMap[GL_DEPTH_COMPONENT] = GL_DEPTH_ATTACHMENT;
    formatToFboMap[GL_DEPTH_COMPONENT16] = GL_DEPTH_ATTACHMENT;
    formatToFboMap[GL_DEPTH_COMPONENT24] = GL_DEPTH_ATTACHMENT;
    formatToFboMap[GL_DEPTH_COMPONENT32] = GL_DEPTH_ATTACHMENT;
    formatToFboMap[GL_DEPTH_COMPONENT32F] = GL_DEPTH_ATTACHMENT;
    formatToFboMap[GL_DEPTH_STENCIL] = GL_DEPTH_STENCIL_ATTACHMENT;
    formatToFboMap[GL_R11F_G11F_B10F] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R16I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R16UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R32I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R32F] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R32UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R8I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R8UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RED] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG16I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG16UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG32I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG32UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG8I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG8UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB16I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB16UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB32I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB32UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB8I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB8UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB9_E5] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA16I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA16F] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA16UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA32F] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA32I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA32UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA8I] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA8UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_STENCIL_INDEX] = GL_STENCIL_ATTACHMENT;
    formatToFboMap[GL_STENCIL_INDEX1] = GL_STENCIL_ATTACHMENT;
    formatToFboMap[GL_STENCIL_INDEX16] = GL_STENCIL_ATTACHMENT;
    formatToFboMap[GL_STENCIL_INDEX4] = GL_STENCIL_ATTACHMENT;
    formatToFboMap[GL_STENCIL_INDEX8] = GL_STENCIL_ATTACHMENT;
    formatToFboMap[GL_ALPHA4] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_ALPHA8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_ALPHA12] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_ALPHA16] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R8_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R16] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R16_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG8_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG16] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG16_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_R3_G3_B2] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB4] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB5] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB565] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB8_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB10] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB12] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB16] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB16_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA2] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA4] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB5_A1] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA8_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB10_A2] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB10_A2UI] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA12] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA16] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGBA16_SNORM] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_SRGB8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_SRGB8_ALPHA8] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RG32F] = GL_COLOR_ATTACHMENT0;
    formatToFboMap[GL_RGB16F] = GL_COLOR_ATTACHMENT0;
  }

  if (formatToFboMap.find(format) != formatToFboMap.end()) {
    return formatToFboMap[format];
  } else {
    LOG_ERROR << "Unknown Texture Format: " << format;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

GLenum FboToBlitMask(GLenum attachment) {
  switch (attachment) {
  case GL_COLOR_ATTACHMENT0:
    return GL_COLOR_BUFFER_BIT;
  case GL_DEPTH_ATTACHMENT:
    return GL_DEPTH_BUFFER_BIT;
  case GL_STENCIL_ATTACHMENT:
    return GL_STENCIL_BUFFER_BIT;
  case GL_DEPTH_STENCIL_ATTACHMENT:
    return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
  default:
    LOG_ERROR << "Unknown Fbo attachment: " << attachment;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

const char* DrawBufferToSuffixStr(GLenum buffer) {
  switch (buffer) {
  case GL_NONE:
    return "none";
  case GL_FRONT_LEFT:
    return "_frontl";
  case GL_FRONT_RIGHT:
    return "_frontr";
  case GL_BACK_LEFT:
    return "_backl";
  case GL_BACK_RIGHT:
    return "_backr";
  case GL_FRONT:
    return "_front";
  case GL_BACK:
    return "_back";
  case GL_LEFT:
    return "_left";
  case GL_RIGHT:
    return "_right";
  case GL_FRONT_AND_BACK:
    return "_frontback";
  case GL_COLOR_ATTACHMENT0:
    return "_att0";
  case GL_COLOR_ATTACHMENT0 + 1:
    return "_att1";
  case GL_COLOR_ATTACHMENT0 + 2:
    return "_att2";
  case GL_COLOR_ATTACHMENT0 + 3:
    return "_att3";
  case GL_COLOR_ATTACHMENT0 + 4:
    return "_att4";
  case GL_COLOR_ATTACHMENT0 + 5:
    return "_att5";
  case GL_COLOR_ATTACHMENT0 + 6:
    return "_att6";
  case GL_COLOR_ATTACHMENT0 + 7:
    return "_att7";
  case GL_COLOR_ATTACHMENT0 + 8:
    return "_att8";
  case GL_COLOR_ATTACHMENT0 + 9:
    return "_att9";
  case GL_COLOR_ATTACHMENT0 + 10:
    return "_att10";
  case GL_COLOR_ATTACHMENT0 + 11:
    return "_att11";
  case GL_COLOR_ATTACHMENT0 + 12:
    return "_att12";
  case GL_COLOR_ATTACHMENT0 + 13:
    return "_att13";
  case GL_COLOR_ATTACHMENT0 + 14:
    return "_att14";
  case GL_COLOR_ATTACHMENT0 + 15:
    return "_att15";
  case GL_DEPTH_ATTACHMENT:
    return "_depth";
  case GL_STENCIL_ATTACHMENT:
    return "_stencil";
  default:
    LOG_ERROR << "Unknown buffer: " << buffer;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

bool IsTextureCompressed(GLenum internalFormat) {
  switch (internalFormat) {
  case GL_COMPRESSED_ALPHA:
  case GL_COMPRESSED_INTENSITY:
  case GL_COMPRESSED_LUMINANCE:
  case GL_COMPRESSED_LUMINANCE_ALPHA:
  case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
  case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
  case GL_COMPRESSED_R11_EAC:
  case GL_COMPRESSED_RED:
  case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
  case GL_COMPRESSED_RED_RGTC1:
  case GL_COMPRESSED_RG:
  case GL_COMPRESSED_RG11_EAC:
  case GL_COMPRESSED_RGB:
  case GL_COMPRESSED_RGB8_ETC2:
  case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
  case GL_COMPRESSED_RGBA:
  case GL_COMPRESSED_RGBA8_ETC2_EAC:
  case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
  case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
  case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
  case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
  case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
  case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
  case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
  case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
  case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
  case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
  case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
  case GL_COMPRESSED_RGBA_FXT1_3DFX:
  case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
  case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
  case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
  case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
  case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
  case GL_COMPRESSED_RGB_FXT1_3DFX:
  case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
  case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
  case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
  case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
  case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
  case GL_COMPRESSED_SIGNED_R11_EAC:
  case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:
  case GL_COMPRESSED_SIGNED_RED_RGTC1:
  case GL_COMPRESSED_SIGNED_RG11_EAC:
  case GL_COMPRESSED_SLUMINANCE:
  case GL_COMPRESSED_SLUMINANCE_ALPHA:
  case GL_COMPRESSED_SRGB:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
  case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
  case GL_COMPRESSED_SRGB8_ETC2:
  case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
  case GL_COMPRESSED_SRGB_ALPHA:
  case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
  case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
  case GL_ETC1_RGB8_OES:
    return true;
  default:
    return false;
  }
}

unsigned DataTypeSize(GLenum type) {
  switch (type) {
  case GL_BYTE:
    return sizeof(GLbyte);
  case GL_UNSIGNED_BYTE:
    return sizeof(GLubyte);
  case GL_SHORT:
    return sizeof(GLshort);
  case GL_UNSIGNED_SHORT:
    return sizeof(GLushort);
  case GL_FIXED:
    return sizeof(GLfixed);
  case GL_INT:
    return sizeof(GLint);
  case GL_UNSIGNED_INT:
    return sizeof(GLuint);
  case GL_FLOAT:
    return sizeof(GLfloat);
  case GL_HALF_FLOAT:
    return sizeof(GLhalfNV);
  case GL_DOUBLE:
    return sizeof(GLdouble);
    // INTERLEAVED ARRAYS FORMATS
  case GL_V2F:
    return 2 * sizeof(GLfloat);
  case GL_V3F:
    return 3 * sizeof(GLfloat);
  case GL_C4UB_V2F:
    return 4 * sizeof(GLubyte) + 2 * sizeof(GLfloat);
  case GL_C4UB_V3F:
    return 4 * sizeof(GLubyte) + 3 * sizeof(GLfloat);
  case GL_C3F_V3F:
    return 6 * sizeof(GLfloat);
  case GL_N3F_V3F:
    return 6 * sizeof(GLfloat);
  case GL_C4F_N3F_V3F:
    return 10 * sizeof(GLfloat);
  case GL_T2F_V3F:
    return 5 * sizeof(GLfloat);
  case GL_T4F_V4F:
    return 8 * sizeof(GLfloat);
  case GL_T2F_C4UB_V3F:
    return 4 * sizeof(GLubyte) + 5 * sizeof(GLfloat);
  case GL_T2F_C3F_V3F:
    return 8 * sizeof(GLfloat);
  case GL_T2F_N3F_V3F:
    return 8 * sizeof(GLfloat);
  case GL_T2F_C4F_N3F_V3F:
    return 12 * sizeof(GLfloat);
  case GL_T4F_C4F_N3F_V4F:
    return 15 * sizeof(GLfloat);
  // following types are specific to display lists, but fit here nicely
  case GL_2_BYTES:
    return 2 * sizeof(GLbyte);
  case GL_3_BYTES:
    return 3 * sizeof(GLbyte);
  case GL_4_BYTES:
    return 4 * sizeof(GLbyte);
  default:
    LOG_ERROR << "Unknown pointer type GLenum: 0x" << std::hex << type << "!!!";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

} // namespace OpenGL
} // namespace gits
