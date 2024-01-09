// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   opengl_apis_iface.cpp
 *
 * @brief Definition of class that passes generic opengl data to the lower, generic layers.
 *
 */

#include "include/openglLibrary.h"
#include "opengl_apis_iface.h"

namespace gits {
namespace OpenGL {
void OpenGLApi::Play_SwapAfterPrepare() const {
#if defined GITS_PLATFORM_WINDOWS
  if (drv.gl.Api() == CGlDriver::API_GLES1 || drv.gl.Api() == CGlDriver::API_GLES2) {
    drv.egl.eglSwapBuffers(drv.egl.eglGetCurrentDisplay(), drv.egl.eglGetCurrentSurface(EGL_DRAW));
  } else if (drv.gl.Api() == CGlDriver::API_GL || curctx::IsOgl()) {
    drv.wgl.wglSwapBuffers(drv.wgl.wglGetCurrentDC());
  } else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
#endif
}
} // namespace OpenGL
} //namespace gits
