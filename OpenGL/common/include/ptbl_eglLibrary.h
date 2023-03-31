// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptbl_eglLibrary.h
*
*/

#pragma once
#include "openglTypes.h"
#include "ptblLibrary.h"

namespace gits {
namespace OpenGL {
//****************** Portable interface *************************
EGLSurface ptbl_eglCreateWindowSurface(EGLDisplay dpy,
                                       EGLConfig config,
                                       EGLNativeWindowType win,
                                       const EGLint* attrib_list);
EGLContext ptbl_eglCreateContext(EGLDisplay dpy,
                                 EGLConfig config,
                                 EGLContext share_context,
                                 const EGLint* attrib_list);
EGLBoolean ptbl_eglDestroyContext(EGLDisplay dpy, EGLContext ctx);
EGLBoolean ptbl_eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLBoolean ptbl_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
EGLSurface ptbl_eglCreatePbufferSurface(EGLDisplay dpy,
                                        EGLConfig config,
                                        const EGLint* attrib_list);
EGLBoolean ptbl_eglChooseConfig(EGLDisplay dpy,
                                const EGLint* attrib_list,
                                EGLConfig* configs,
                                EGLint config_size,
                                EGLint* num_config);
EGLBoolean ptbl_eglBindAPI(EGLenum api);
EGLenum ptbl_eglQueryAPI();
EGLConfig ptblFindConfigEGL(EGLDisplay dpy, const std::vector<EGLint>& attribs);
EGLDisplay ptbl_GetEGLDisplay();

//****************** Portable execution *************************
void execSetContextEGL(PtblHandle ctx);
void execDelContextEGL(PtblHandle ctx);
void execBufferSwapEGL(PtblHandle ctx);

//****************** Helper functions ***************************
typedef std::vector<int> EGLPFAttribs;
typedef std::vector<int> EGLCtxAttribs;
EGLPFAttribs PtblToEGLAttribs(const PtblPFAttribs& ptblattribs);
PtblPFAttribs EGLToPtblAttribs(const EGLint* eglattribs);
std::vector<int> GetUpdatedEGLCtxParams(const int* params);
PtblCtxParams EGLToPtblCtxParams(const EGLint* eglparams);
EGLCtxAttribs PtblToEGLCtxParams(const PtblCtxParams& ptblparams);
} // namespace OpenGL
} // namespace gits