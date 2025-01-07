// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

/*
  The purpose of this file is to contain all the utilities and functions
  related to OpenGL API that do no interactions with the driver. These
  include translation of shader type enum to string, or computation of
  number of components from format enum.
*/

#ifndef BUILD_FOR_CCODE
#include "gits.h"
#endif
#include "openglTypes.h"
#include <string>
#include <unordered_map>

namespace gits {
namespace OpenGL {
unsigned indexSize(GLenum type);
unsigned texelComponentsNum(GLenum format);
unsigned texelSize(GLenum format, GLenum type);
unsigned texelElementsNum(GLenum format);
bool isTexelDataConsistent(GLenum type);
const char* typeName(GLenum type);
const char* shaderTypeAbrvName(GLenum type);
GLenum GetBindingEnum(GLenum target);
bool isTargetProxy(GLenum targetMode);
bool isDepthFormat(GLenum internalFormat);
bool isStencilFormat(GLenum internalFormat);
std::string drawModeToString(GLenum drawMode);
bool getDataFromInternalFormat(GLenum internalFormat, GLint& retFormat, GLenum& retType);
GLenum GetSizeMapEnum(GLenum target);
void FboToImageMatch(GLenum fboAttachment, GLenum& format, GLenum& type);
GLenum InternalFormatToFboMatch(GLenum format);
GLenum FboToBlitMask(GLenum attachment);
const char* DrawBufferToSuffixStr(GLenum buffer);
bool IsTextureCompressed(GLenum internalFormat);
const char* GetGLEnumString(GLenum glenum);
unsigned DataTypeSize(GLenum type);
} // namespace OpenGL
} // namespace gits
