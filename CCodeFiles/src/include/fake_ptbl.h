// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   fakePtbl.h
*
* Fake portability header is used in ccode where portability is not supported in oposite to gitsPlayer. 
* It contains list of all portable API versions which are simply passing params to original APIs.
*/

#pragma once
#include "tools.h"
#include "windowing.h"
#include "openglDrivers.h"

#if defined GITS_PLATFORM_WINDOWS
#include <Windows.h>
#include <windowsx.h>
#elif defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <GL/glx.h>
#undef True
#undef XVisualInfo
#endif

namespace gits {
//***************** Windowing Functions Interface ****************************

inline win_ptr_t ptblCreateWin(GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWin(width, height, x, y, show);
}

inline win_ptr_t ptblCreateWin(
    GLXFBConfig pf, XVisualInfo* xinfo, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWin(pf, xinfo, width, height, x, y, show);
}

inline win_ptr_t ptblCreateWin(
    EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show) {
  return CreateWin(pf, width, height, x, y, show);
}

inline void ptblRemoveWin(win_ptr_t handle) {
  RemoveWin(handle);
}

inline void ptblResizeWin(win_ptr_t handle, int width, int height) {
  ResizeWin(handle, width, height);
}
inline void ptblMoveWin(win_ptr_t handle, int x, int y) {
  MoveWin(handle, x, y);
}
inline void ptblWinVisibility(win_ptr_t handle, bool show) {
  WinVisibility(handle, show);
}
inline void ptblWinTitle(win_ptr_t handle, const std::string& title) {
  WinTitle(handle, title);
}
inline win_handle_t ptblGetWinHandle(win_ptr_t win) {
  return GetWinHandle(win);
}

namespace OpenGL {

//****************** WGL Portable Fake interface *************************
inline int ptbl_wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglChoosePixelFormat(hdc, ppfd);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline BOOL ptbl_wglChoosePixelFormatARB(HDC hdc,
                                         const int* piAttribIList,
                                         const FLOAT* pfAttribFList,
                                         UINT nMaxFormats,
                                         int* piFormats,
                                         UINT* nNumFormats) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglChoosePixelFormatARB(hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats,
                                         nNumFormats);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline BOOL ptbl_wglSetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* pfd) {
#ifdef GITS_PLATFORM_WINDOWS
  return SetPixelFormat(hdc, format, pfd);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}
inline HGLRC ptbl_wglCreateContext(HDC hdc) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglCreateContext(hdc);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline HGLRC ptbl_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int* attribList) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglCreateContextAttribsARB(hDC, hShareContext, attribList);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline BOOL ptbl_wglMakeCurrent(HDC hdc, HGLRC hglrc) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglMakeCurrent(hdc, hglrc);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline BOOL ptbl_wglSwapBuffers(HDC hdc) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglSwapBuffers(hdc);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline BOOL ptbl_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglShareLists(hglrc1, hglrc2);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline HPBUFFERARB ptbl_wglCreatePbufferARB(
    HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglCreatePbufferARB(hDC, iPixelFormat, iWidth, iHeight, piAttribList);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline HDC ptbl_wglGetPbufferDCARB(HPBUFFERARB hPbuffer) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglGetPbufferDCARB(hPbuffer);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline BOOL ptbl_wglDeleteContext(HGLRC hglrc) {
#ifdef GITS_PLATFORM_WINDOWS
  return drv.wgl.wglDeleteContext(hglrc);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline HDC ptblGetDC(HWND hwnd) {
#ifdef GITS_PLATFORM_WINDOWS
  return GetDC(hwnd);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline HWND ptblWindowFromDC(HDC dc) {
#ifdef GITS_PLATFORM_WINDOWS
  return WindowFromDC(dc);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline int ptblReleaseDC(HWND hwnd, HDC hdc) {
#ifdef GITS_PLATFORM_WINDOWS
  return ReleaseDC(hwnd, hdc);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

//****************** EGL Portable Fake interface *************************
inline EGLSurface ptbl_eglCreateWindowSurface(EGLDisplay dpy,
                                              EGLConfig config,
                                              EGLNativeWindowType win,
                                              const EGLint* attrib_list) {
  return drv.egl.eglCreateWindowSurface(dpy, config, win, attrib_list);
}

inline EGLContext ptbl_eglCreateContext(EGLDisplay dpy,
                                        EGLConfig config,
                                        EGLContext share_context,
                                        const EGLint* attrib_list) {
  return drv.egl.eglCreateContext(dpy, config, share_context, attrib_list);
}

inline EGLBoolean ptbl_eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
  return drv.egl.eglDestroyContext(dpy, ctx);
}

inline EGLBoolean ptbl_eglMakeCurrent(EGLDisplay dpy,
                                      EGLSurface draw,
                                      EGLSurface read,
                                      EGLContext ctx) {
  return drv.egl.eglMakeCurrent(dpy, draw, read, ctx);
}

inline EGLBoolean ptbl_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  return drv.egl.eglSwapBuffers(dpy, surface);
}

inline EGLSurface ptbl_eglCreatePbufferSurface(EGLDisplay dpy,
                                               EGLConfig config,
                                               const EGLint* attrib_list) {
  return drv.egl.eglCreatePbufferSurface(dpy, config, attrib_list);
}

inline EGLBoolean ptbl_eglChooseConfig(EGLDisplay dpy,
                                       const EGLint* attrib_list,
                                       EGLConfig* configs,
                                       EGLint config_size,
                                       EGLint* num_config) {
  return drv.egl.eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

inline EGLBoolean ptbl_eglBindAPI(EGLenum api) {
  return drv.egl.eglBindAPI(api);
}

inline EGLenum ptbl_eglQueryAPI() {
  return drv.egl.eglQueryAPI();
}

//****************** GLX Portable Fake interface *************************
inline XVisualInfo* ptbl_glXChooseVisual(Display* dpy, int screen, int* attribList) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXChooseVisual(dpy, screen, attribList);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline GLXFBConfig* ptbl_glXChooseFBConfig(Display* dpy,
                                           int screen,
                                           const int* attribList,
                                           int* nitems) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXChooseFBConfig(dpy, screen, attribList, nitems);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline XVisualInfo* ptbl_glXGetVisualFromFBConfig(Display* dpy, GLXFBConfig config) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXGetVisualFromFBConfig(dpy, config);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline GLXContext ptbl_glXCreateContext(Display* dpy,
                                        XVisualInfo* vis,
                                        GLXContext shareList,
                                        Bool direct) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXCreateContext(dpy, vis, shareList, direct);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline GLXContext ptbl_glXCreateNewContext(
    Display* dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXCreateNewContext(dpy, config, renderType, shareList, direct);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline GLXContext ptbl_glXCreateContextAttribsARB(Display* dpy,
                                                  GLXFBConfig config,
                                                  GLXContext share_context,
                                                  Bool direct,
                                                  const int* attrib_list) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXCreateContextAttribsARB(dpy, config, share_context, direct, attrib_list);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline void ptbl_glXDestroyContext(Display* dpy, GLXContext ctx) {
#ifdef GITS_PLATFORM_X11
  drv.glx.glXDestroyContext(dpy, ctx);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline Bool ptbl_glXMakeCurrent(Display* dpy, GLXDrawable drawable, GLXContext ctx) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXMakeCurrent(dpy, drawable, ctx);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline Bool ptbl_glXMakeContextCurrent(Display* dpy,
                                       GLXDrawable draw,
                                       GLXDrawable read,
                                       GLXContext ctx) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXMakeContextCurrent(dpy, draw, read, ctx);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline void ptbl_glXSwapBuffers(Display* dpy, GLXDrawable drawable) {
#ifdef GITS_PLATFORM_X11
  drv.glx.glXSwapBuffers(dpy, drawable);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline GLXPbuffer ptbl_glXCreatePbuffer(Display* dpy, GLXFBConfig config, const int* attribList) {
#ifdef GITS_PLATFORM_X11
  return drv.glx.glXCreatePbuffer(dpy, config, attribList);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline void ptbl_XFree(GLXFBConfig* fbconf) {
#ifdef GITS_PLATFORM_X11
  XFree(fbconf);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

inline int ptbl_DefaultScreen(Display* display) {
#ifdef GITS_PLATFORM_X11
  return DefaultScreen(display);
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

} // namespace OpenGL
} // namespace gits
