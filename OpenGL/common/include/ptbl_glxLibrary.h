// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptbl_glxLibrary.h
*
*/
#include "ptblLibrary.h"

namespace gits {
namespace OpenGL {

//****************** Portable interface *************************
XVisualInfo* ptbl_glXChooseVisual(Display* dpy, int screen, int* attribList);
GLXFBConfig* ptbl_glXChooseFBConfig(Display* dpy, int screen, const int* attribList, int* nitems);
XVisualInfo* ptbl_glXGetVisualFromFBConfig(Display* dpy, GLXFBConfig config);
GLXContext ptbl_glXCreateContext(Display* dpy, XVisualInfo* vis, GLXContext shareList, Bool direct);
GLXContext ptbl_glXCreateNewContext(
    Display* dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct);
GLXContext ptbl_glXCreateContextAttribsARB(Display* dpy,
                                           GLXFBConfig config,
                                           GLXContext share_context,
                                           Bool direct,
                                           const int* attrib_list);
void ptbl_glXDestroyContext(Display* dpy, GLXContext ctx);
Bool ptbl_glXMakeCurrent(Display* dpy, GLXDrawable drawable, GLXContext ctx);
Bool ptbl_glXMakeContextCurrent(Display* dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
void ptbl_glXSwapBuffers(Display* dpy, GLXDrawable drawable);
GLXPbuffer ptbl_glXCreatePbuffer(Display* dpy, GLXFBConfig config, const int* attribList);
void ptbl_XFree(GLXFBConfig* fbconf);
int ptbl_DefaultScreen(Display* display);

//****************** Portable execution *************************
void execSetContextGLX(PtblHandle ctx);
void execDelContextGLX(PtblHandle ctx);
void execBufferSwapGLX(PtblHandle ctx);

//****************** Helper functions ***************************
typedef std::vector<int> GLXPFAttribs;
typedef std::vector<int> GLXCtxParams;
GLXPFAttribs PtblToGLXARBAttribs(const PtblPFAttribs& ptblattribs);
PtblPFAttribs GLXToPtblAttribs(const int* glxattribs);
PtblPFAttribs XVisualToPtblAttribs(const int* glxattribs);
GLXCtxParams GetUpdatedGLXCtxParams(const int* ptblparams);
PtblCtxParams GLXToPtblCtxParams(const int* glxparams);
GLXCtxParams PtblToGLXCtxParams(const PtblCtxParams& ptblparams);
} // namespace OpenGL
} // namespace gits