// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <map>
#include <stdexcept>

#include "log.h"
#include "openglTools.h"
#include "dynamic_linker.h"

#ifndef VOID_T_DEFINED
#define VOID_T_DEFINED
typedef struct void_type_tag {
}* void_t;
#endif

typedef struct wl_display wl_display;
typedef struct wl_buffer wl_buffer;

namespace gits {
struct Tracer {
  template <class T>
  NOINLINE void trace_ret(T r) {
    using namespace gits::OpenGL;
    LOG_TRACE_RAW << " = " << ToStr(r) << "\n";
  }

  NOINLINE void trace_ret(void_t r) {
    LOG_TRACE_RAW << "\n";
  }

  void print_args(plog::Record& s) {}

  template <class T>
  NOINLINE void print_args(plog::Record& s, T t) {
    using namespace gits::OpenGL;
    s << ToStr(t);
  }

  template <class Head, class... Rest>
  NOINLINE void print_args(plog::Record& s, Head h, Rest... r) {
    using namespace gits::OpenGL;
    s << ToStr(h) << ", ";
    print_args(s, r...);
  }

  Tracer(const char* nameStr) : name(nameStr) {}

  template <class... Args>
  NOINLINE void trace(Args... args) {
    auto log = plog::Record(plog::debug, PLOG_GET_FUNC(), __LINE__, PLOG_GET_FILE(),
                            PLOG_GET_THIS(), GITS_LOG_INSTANCE_ID_RAW);
    log << name << "(";
    print_args(log, args...);
    log << ")";
    plog::get<GITS_LOG_INSTANCE_ID_RAW>()->operator+=(log);
  }

private:
  const char* name;
};
} // namespace gits
#include "glDrivers.h"

#define EGL_FUNCTIONS(a)                                                                           \
  EGL_FUNCTION(a, EGLBoolean, eglBindAPI, (EGLenum api), (api))                                    \
  EGL_FUNCTION(a, EGLBoolean, eglBindTexImage,                                                     \
               (EGLDisplay dpy, EGLSurface surface, EGLint buffer), (dpy, surface, buffer))        \
  EGL_FUNCTION(a, EGLBoolean, eglChooseConfig,                                                     \
               (EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, \
                EGLint* num_config),                                                               \
               (dpy, attrib_list, configs, config_size, num_config))                               \
  EGL_FUNCTION(a, EGLint, eglClientWaitSyncKHR,                                                    \
               (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout),                \
               (dpy, sync, flags, timeout))                                                        \
  EGL_FUNCTION(a, EGLint, eglClientWaitSyncNV, (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout),  \
               (sync, flags, timeout))                                                             \
  EGL_FUNCTION(a, EGLBoolean, eglCopyBuffers,                                                      \
               (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target),                   \
               (dpy, surface, target))                                                             \
  EGL_FUNCTION(                                                                                    \
      a, EGLContext, eglCreateContext,                                                             \
      (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list),     \
      (dpy, config, share_context, attrib_list))                                                   \
  EGL_FUNCTION(a, EGLImageKHR, eglCreateDRMImageMESA, (EGLDisplay dpy, const EGLint* attrib_list), \
               (dpy, attrib_list))                                                                 \
  EGL_FUNCTION(a, EGLSyncNV, eglCreateFenceSyncNV,                                                 \
               (EGLDisplay dpy, EGLenum condition, const EGLint* attrib_list),                     \
               (dpy, condition, attrib_list))                                                      \
  EGL_FUNCTION(a, EGLImageKHR, eglCreateImageKHR,                                                  \
               (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer,            \
                const EGLint* attrib_list),                                                        \
               (dpy, ctx, target, buffer, attrib_list))                                            \
  EGL_FUNCTION(a, EGLSurface, eglCreatePixmapSurfaceHI,                                            \
               (EGLDisplay dpy, EGLConfig config, void* pixmap), (dpy, config, pixmap))            \
  EGL_FUNCTION(a, EGLSurface, eglCreatePbufferFromClientBuffer,                                    \
               (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config,         \
                const EGLint* attrib_list),                                                        \
               (dpy, buftype, buffer, config, attrib_list))                                        \
  EGL_FUNCTION(a, EGLSurface, eglCreatePbufferSurface,                                             \
               (EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list),                      \
               (dpy, config, attrib_list))                                                         \
  EGL_FUNCTION(                                                                                    \
      a, EGLSurface, eglCreatePixmapSurface,                                                       \
      (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint* attrib_list),   \
      (dpy, config, pixmap, attrib_list))                                                          \
  EGL_FUNCTION(a, EGLStreamKHR, eglCreateStreamFromFileDescriptorKHR,                              \
               (EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor),                       \
               (dpy, file_descriptor))                                                             \
  EGL_FUNCTION(a, EGLStreamKHR, eglCreateStreamKHR, (EGLDisplay dpy, const EGLint* attrib_list),   \
               (dpy, attrib_list))                                                                 \
  EGL_FUNCTION(a, EGLSurface, eglCreateStreamProducerSurfaceKHR,                                   \
               (EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint* attrib_list), \
               (dpy, config, stream, attrib_list))                                                 \
  EGL_FUNCTION(a, EGLSyncKHR, eglCreateSyncKHR,                                                    \
               (EGLDisplay dpy, EGLenum type, const EGLint* attrib_list),                          \
               (dpy, type, attrib_list))                                                           \
  EGL_FUNCTION(                                                                                    \
      a, EGLSurface, eglCreateWindowSurface,                                                       \
      (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint* attrib_list),      \
      (dpy, config, win, attrib_list))                                                             \
  EGL_FUNCTION(a, EGLBoolean, eglDestroyContext, (EGLDisplay dpy, EGLContext ctx), (dpy, ctx))     \
  EGL_FUNCTION(a, EGLBoolean, eglDestroyImageKHR, (EGLDisplay dpy, EGLImageKHR image),             \
               (dpy, image))                                                                       \
  EGL_FUNCTION(a, EGLBoolean, eglDestroyStreamKHR, (EGLDisplay dpy, EGLStreamKHR stream),          \
               (dpy, stream))                                                                      \
  EGL_FUNCTION(a, EGLBoolean, eglDestroySurface, (EGLDisplay dpy, EGLSurface surface),             \
               (dpy, surface))                                                                     \
  EGL_FUNCTION(a, EGLBoolean, eglDestroySyncKHR, (EGLDisplay dpy, EGLSyncKHR sync), (dpy, sync))   \
  EGL_FUNCTION(a, EGLBoolean, eglDestroySyncNV, (EGLSyncNV sync), (sync))                          \
  EGL_FUNCTION(                                                                                    \
      a, EGLBoolean, eglExportDRMImageMESA,                                                        \
      (EGLDisplay dpy, EGLImageKHR image, EGLint * name, EGLint * handle, EGLint * stride),        \
      (dpy, image, name, handle, stride))                                                          \
  EGL_FUNCTION(a, EGLBoolean, eglFenceNV, (EGLSyncNV sync), (sync))                                \
  EGL_FUNCTION(a, EGLBoolean, eglGetConfigAttrib,                                                  \
               (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint * value),               \
               (dpy, config, attribute, value))                                                    \
  EGL_FUNCTION(a, EGLBoolean, eglGetConfigs,                                                       \
               (EGLDisplay dpy, EGLConfig * configs, EGLint config_size, EGLint * num_config),     \
               (dpy, configs, config_size, num_config))                                            \
  EGL_FUNCTION(a, EGLContext, eglGetCurrentContext, (), ())                                        \
  EGL_FUNCTION(a, EGLDisplay, eglGetCurrentDisplay, (), ())                                        \
  EGL_FUNCTION(a, EGLSurface, eglGetCurrentSurface, (EGLint readdraw), (readdraw))                 \
  EGL_FUNCTION(a, EGLDisplay, eglGetDisplay, (EGLNativeDisplayType display_id), (display_id))      \
  EGL_FUNCTION(a, EGLint, eglGetError, (), ())                                                     \
  EGL_FUNCTION(a, void*, eglGetProcAddress, (const char* procname), (procname))                    \
  EGL_FUNCTION(a, EGLNativeFileDescriptorKHR, eglGetStreamFileDescriptorKHR,                       \
               (EGLDisplay dpy, EGLStreamKHR stream), (dpy, stream))                               \
  EGL_FUNCTION(a, EGLBoolean, eglGetSyncAttribKHR,                                                 \
               (EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint * value),                \
               (dpy, sync, attribute, value))                                                      \
  EGL_FUNCTION(a, EGLBoolean, eglGetSyncAttribNV,                                                  \
               (EGLSyncNV sync, EGLint attribute, EGLint * value), (sync, attribute, value))       \
  EGL_FUNCTION(a, EGLuint64NV, eglGetSystemTimeFrequencyNV, (), ())                                \
  EGL_FUNCTION(a, EGLuint64NV, eglGetSystemTimeNV, (), ())                                         \
  EGL_FUNCTION(a, EGLBoolean, eglInitialize, (EGLDisplay dpy, EGLint * major, EGLint * minor),     \
               (dpy, major, minor))                                                                \
  EGL_FUNCTION(a, EGLBoolean, eglLockSurfaceKHR,                                                   \
               (EGLDisplay display, EGLSurface surface, const EGLint* attrib_list),                \
               (display, surface, attrib_list))                                                    \
  EGL_FUNCTION(a, EGLBoolean, eglMakeCurrent,                                                      \
               (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx),                 \
               (dpy, draw, read, ctx))                                                             \
  EGL_FUNCTION(                                                                                    \
      a, EGLBoolean, eglPostSubBufferNV,                                                           \
      (EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height),       \
      (dpy, surface, x, y, width, height))                                                         \
  EGL_FUNCTION(a, EGLenum, eglQueryAPI, (), ())                                                    \
  EGL_FUNCTION(a, EGLBoolean, eglQueryContext,                                                     \
               (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint * value),                 \
               (dpy, ctx, attribute, value))                                                       \
  EGL_FUNCTION(a, EGLBoolean, eglQueryStreamKHR,                                                   \
               (EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint * value),           \
               (dpy, stream, attribute, value))                                                    \
  EGL_FUNCTION(a, EGLBoolean, eglQueryStreamTimeKHR,                                               \
               (EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR * value),       \
               (dpy, stream, attribute, value))                                                    \
  EGL_FUNCTION(a, EGLBoolean, eglQueryStreamu64KHR,                                                \
               (EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR * value),     \
               (dpy, stream, attribute, value))                                                    \
  EGL_FUNCTION(a, const char*, eglQueryString, (EGLDisplay dpy, EGLint name), (dpy, name))         \
  EGL_FUNCTION(a, EGLBoolean, eglQuerySurface,                                                     \
               (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint * value),             \
               (dpy, surface, attribute, value))                                                   \
  EGL_FUNCTION(a, EGLBoolean, eglQuerySurfacePointerANGLE,                                         \
               (EGLDisplay dpy, EGLSurface surface, EGLint attribute, void** value),               \
               (dpy, surface, attribute, value))                                                   \
  EGL_FUNCTION(a, EGLBoolean, eglReleaseTexImage,                                                  \
               (EGLDisplay dpy, EGLSurface surface, EGLint buffer), (dpy, surface, buffer))        \
  EGL_FUNCTION(a, EGLBoolean, eglReleaseThread, (), ())                                            \
  EGL_FUNCTION(a, EGLBoolean, eglSignalSyncKHR, (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode),   \
               (dpy, sync, mode))                                                                  \
  EGL_FUNCTION(a, EGLBoolean, eglSignalSyncNV, (EGLSyncNV sync, EGLenum mode), (sync, mode))       \
  EGL_FUNCTION(a, EGLBoolean, eglStreamAttribKHR,                                                  \
               (EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value),             \
               (dpy, stream, attribute, value))                                                    \
  EGL_FUNCTION(a, EGLBoolean, eglStreamConsumerAcquireKHR, (EGLDisplay dpy, EGLStreamKHR stream),  \
               (dpy, stream))                                                                      \
  EGL_FUNCTION(a, EGLBoolean, eglStreamConsumerGLTextureExternalKHR,                               \
               (EGLDisplay dpy, EGLStreamKHR stream), (dpy, stream))                               \
  EGL_FUNCTION(a, EGLBoolean, eglStreamConsumerReleaseKHR, (EGLDisplay dpy, EGLStreamKHR stream),  \
               (dpy, stream))                                                                      \
  EGL_FUNCTION(a, EGLBoolean, eglSurfaceAttrib,                                                    \
               (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value),               \
               (dpy, surface, attribute, value))                                                   \
  EGL_FUNCTION(a, EGLBoolean, eglSwapBuffers, (EGLDisplay dpy, EGLSurface surface),                \
               (dpy, surface))                                                                     \
  EGL_FUNCTION(a, EGLBoolean, eglSwapInterval, (EGLDisplay dpy, EGLint interval), (dpy, interval)) \
  EGL_FUNCTION(a, EGLBoolean, eglTerminate, (EGLDisplay dpy), (dpy))                               \
  EGL_FUNCTION(a, EGLBoolean, eglUnlockSurfaceKHR, (EGLDisplay display, EGLSurface surface),       \
               (display, surface))                                                                 \
  EGL_FUNCTION(a, EGLBoolean, eglWaitClient, (), ())                                               \
  EGL_FUNCTION(a, EGLBoolean, eglWaitGL, (), ())                                                   \
  EGL_FUNCTION(a, EGLBoolean, eglWaitNative, (EGLint engine), (engine))                            \
  EGL_FUNCTION(a, EGLint, eglWaitSyncKHR, (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags),         \
               (dpy, sync, flags))                                                                 \
  EGL_FUNCTION(                                                                                    \
      a, EGLBoolean, eglSetSwapRectangleANDROID,                                                   \
      (EGLDisplay arg0, EGLSurface arg1, EGLint arg2, EGLint arg3, EGLint arg4, EGLint arg5),      \
      (arg0, arg1, arg2, arg3, arg4, arg5))                                                        \
  EGL_FUNCTION(a, EGLClientBuffer, eglGetRenderBufferANDROID, (EGLDisplay arg0, EGLSurface arg1),  \
               (arg0, arg1))                                                                       \
  EGL_FUNCTION(a, EGLint, eglDupNativeFenceFDANDROID, (EGLDisplay arg0, EGLSyncKHR arg1),          \
               (arg0, arg1))                                                                       \
  EGL_FUNCTION(a, EGLint, eglWaitSyncANDROID, (EGLDisplay arg0, EGLSyncKHR arg1, EGLint arg2),     \
               (arg0, arg1, arg2))                                                                 \
  EGL_FUNCTION(a, void_t, eglSetBlobCacheFuncsANDROID,                                             \
               (EGLDisplay dpy, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get),             \
               (dpy, set, get))                                                                    \
  EGL_FUNCTION(a, EGLDisplay, eglGetPlatformDisplayEXT,                                            \
               (EGLenum arg0, void* arg1, const EGLint* arg2), (arg0, arg1, arg2))                 \
  EGL_FUNCTION(a, EGLSurface, eglCreatePlatformWindowSurfaceEXT,                                   \
               (EGLDisplay arg0, EGLConfig arg1, void* arg2, const EGLint* arg3),                  \
               (arg0, arg1, arg2, arg3))                                                           \
  EGL_FUNCTION(a, EGLBoolean, eglBindWaylandDisplayWL,                                             \
               (EGLDisplay arg0, struct wl_display * arg1), (arg0, arg1))                          \
  EGL_FUNCTION(a, EGLBoolean, eglUnbindWaylandDisplayWL,                                           \
               (EGLDisplay arg0, struct wl_display * arg1), (arg0, arg1))                          \
  EGL_FUNCTION(a, EGLBoolean, eglQueryWaylandBufferWL,                                             \
               (EGLDisplay arg0, struct wl_buffer * arg1, EGLint arg2, EGLint * arg3),             \
               (arg0, arg1, arg2, arg3))

#define WGL_FUNCTIONS(a)                                                                           \
  WGL_FUNCTION(a, int, wglChoosePixelFormat, (HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd),         \
               (hdc, ppfd))                                                                        \
  WGL_FUNCTION(a, BOOL, wglCopyContext, (HGLRC hdc, HGLRC format, UINT ppfd), (hdc, format, ppfd)) \
  WGL_FUNCTION(a, HGLRC, wglCreateContext, (HDC hdc), (hdc))                                       \
  WGL_FUNCTION(a, HGLRC, wglCreateLayerContext, (HDC hdc, int iLayerPlane), (hdc, iLayerPlane))    \
  WGL_FUNCTION(a, BOOL, wglDeleteContext, (HGLRC hglrc), (hglrc))                                  \
  WGL_FUNCTION(a, BOOL, wglDescribeLayerPlane,                                                     \
               (HDC hdc, int arg1, int arg2, UINT arg3, LPLAYERPLANEDESCRIPTOR arg4),              \
               (hdc, arg1, arg2, arg3, arg4))                                                      \
  WGL_FUNCTION(a, int, wglDescribePixelFormat,                                                     \
               (HDC hdc, int format, unsigned nBytes, PIXELFORMATDESCRIPTOR* ppfd),                \
               (hdc, format, nBytes, ppfd))                                                        \
  WGL_FUNCTION(a, HGLRC, wglGetCurrentContext, (), ())                                             \
  WGL_FUNCTION(a, HDC, wglGetCurrentDC, (), ())                                                    \
  WGL_FUNCTION(a, void*, wglGetDefaultProcAddress, (LPCSTR lpszProc), (lpszProc))                  \
  WGL_FUNCTION(a, int, wglGetLayerPaletteEntries,                                                  \
               (HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr),                \
               (hdc, iLayerPlane, iStart, cEntries, pcr))                                          \
  WGL_FUNCTION(a, int, wglGetPixelFormat, (HDC hdc), (hdc))                                        \
  WGL_FUNCTION(a, void*, wglGetProcAddress, (LPCSTR lpszProc), (lpszProc))                         \
  WGL_FUNCTION(a, BOOL, wglMakeCurrent, (HDC hdc, HGLRC hglrc), (hdc, hglrc))                      \
  WGL_FUNCTION(a, BOOL, wglRealizeLayerPalette, (HDC arg0, int arg1, BOOL arg2),                   \
               (arg0, arg1, arg2))                                                                 \
  WGL_FUNCTION(a, int, wglSetLayerPaletteEntries,                                                  \
               (HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF* pcr),          \
               (hdc, iLayerPlane, iStart, cEntries, pcr))                                          \
  WGL_FUNCTION(a, BOOL, wglSetPixelFormat,                                                         \
               (HDC hdc, int format, const PIXELFORMATDESCRIPTOR* ppfd), (hdc, format, ppfd))      \
  WGL_FUNCTION(a, BOOL, wglShareLists, (HGLRC hglrc1, HGLRC hglrc2), (hglrc1, hglrc2))             \
  WGL_FUNCTION(a, BOOL, wglSwapBuffers, (HDC hdc), (hdc))                                          \
  WGL_FUNCTION(a, BOOL, wglSwapLayerBuffers, (HDC hdc, UINT plane), (hdc, plane))                  \
  WGL_FUNCTION(a, DWORD, wglSwapMultipleBuffers, (UINT buffer, const WGLSWAP* hdc), (buffer, hdc)) \
  WGL_FUNCTION(a, BOOL, wglUseFontBitmapsA, (HDC hdc, DWORD first, DWORD count, DWORD listBase),   \
               (hdc, first, count, listBase))                                                      \
  WGL_FUNCTION(a, BOOL, wglUseFontBitmapsW, (HDC hdc, DWORD first, DWORD count, DWORD listBase),   \
               (hdc, first, count, listBase))                                                      \
  WGL_FUNCTION(a, BOOL, wglUseFontOutlinesA,                                                       \
               (HDC hdc, DWORD first, DWORD count, DWORD listBase, float deviation,                \
                float extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf),                           \
               (hdc, first, count, listBase, deviation, extrusion, format, lpgmf))                 \
  WGL_FUNCTION(a, BOOL, wglUseFontOutlinesW,                                                       \
               (HDC hdc, DWORD first, DWORD count, DWORD listBase, float deviation,                \
                float extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf),                           \
               (hdc, first, count, listBase, deviation, extrusion, format, lpgmf))

#define WGL_EXT_FUNCTIONS(a)                                                                       \
  WGL_EXT_FUNCTION(a, void*, wglAllocateMemoryNV,                                                  \
                   (GLsizei arg0, GLfloat arg1, GLfloat arg2, GLfloat arg3),                       \
                   (arg0, arg1, arg2, arg3))                                                       \
  WGL_EXT_FUNCTION(                                                                                \
      a, BOOL, wglAssociateImageBufferEventsI3D,                                                   \
      (HDC arg0, const HANDLE* arg1, const LPVOID* arg2, const DWORD* arg3, UINT arg4),            \
      (arg0, arg1, arg2, arg3, arg4))                                                              \
  WGL_EXT_FUNCTION(a, BOOL, wglBeginFrameTrackingI3D, (), ())                                      \
  WGL_EXT_FUNCTION(a, GLboolean, wglBindDisplayColorTableEXT, (GLushort arg0), (arg0))             \
  WGL_EXT_FUNCTION(a, BOOL, wglBindSwapBarrierNV, (GLuint arg0, GLuint arg1), (arg0, arg1))        \
  WGL_EXT_FUNCTION(a, BOOL, wglBindTexImageARB, (HPBUFFERARB arg0, int arg1), (arg0, arg1))        \
  WGL_EXT_FUNCTION(a, BOOL, wglBindVideoCaptureDeviceNV, (UINT arg0, HVIDEOINPUTDEVICENV arg1),    \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, BOOL, wglBindVideoDeviceNV,                                                  \
                   (HDC arg0, unsigned int arg1, HVIDEOOUTPUTDEVICENV arg2, const int* arg3),      \
                   (arg0, arg1, arg2, arg3))                                                       \
  WGL_EXT_FUNCTION(a, BOOL, wglBindVideoImageNV, (HPVIDEODEV arg0, HPBUFFERARB arg1, int arg2),    \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, void_t, wglBlitContextFramebufferAMD,                                        \
                   (HGLRC arg0, GLint arg1, GLint arg2, GLint arg3, GLint arg4, GLint arg5,        \
                    GLint arg6, GLint arg7, GLint arg8, GLbitfield arg9, GLenum arg10),            \
                   (arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10))            \
  WGL_EXT_FUNCTION(                                                                                \
      a, BOOL, wglChoosePixelFormatARB,                                                            \
      (HDC arg0, const int* arg1, const FLOAT* arg2, UINT arg3, int* arg4, UINT* arg5),            \
      (arg0, arg1, arg2, arg3, arg4, arg5))                                                        \
  WGL_EXT_FUNCTION(                                                                                \
      a, BOOL, wglChoosePixelFormatEXT,                                                            \
      (HDC arg0, const int* arg1, const FLOAT* arg2, UINT arg3, int* arg4, UINT* arg5),            \
      (arg0, arg1, arg2, arg3, arg4, arg5))                                                        \
  WGL_EXT_FUNCTION(a, BOOL, wglCopyImageSubDataNV,                                                 \
                   (HGLRC arg0, GLuint arg1, GLenum arg2, GLint arg3, GLint arg4, GLint arg5,      \
                    GLint arg6, HGLRC arg7, GLuint arg8, GLenum arg9, GLint arg10, GLint arg11,    \
                    GLint arg12, GLint arg13, GLsizei arg14, GLsizei arg15, GLsizei arg16),        \
                   (arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11,      \
                    arg12, arg13, arg14, arg15, arg16))                                            \
  WGL_EXT_FUNCTION(a, HDC, wglCreateAffinityDCNV, (const HGPUNV* arg0), (arg0))                    \
  WGL_EXT_FUNCTION(a, HGLRC, wglCreateAssociatedContextAMD, (UINT arg0), (arg0))                   \
  WGL_EXT_FUNCTION(a, HGLRC, wglCreateAssociatedContextAttribsAMD,                                 \
                   (UINT arg0, HGLRC arg1, const int* arg2), (arg0, arg1, arg2))                   \
  WGL_EXT_FUNCTION(a, HANDLE, wglCreateBufferRegionARB, (HDC arg0, int arg1, UINT arg2),           \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, HGLRC, wglCreateContextAttribsARB, (HDC arg0, HGLRC arg1, const int* arg2),  \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, GLboolean, wglCreateDisplayColorTableEXT, (GLushort arg0), (arg0))           \
  WGL_EXT_FUNCTION(a, LPVOID, wglCreateImageBufferI3D, (HDC arg0, DWORD arg1, UINT arg2),          \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, HPBUFFERARB, wglCreatePbufferARB,                                            \
                   (HDC arg0, int arg1, int arg2, int arg3, const int* arg4),                      \
                   (arg0, arg1, arg2, arg3, arg4))                                                 \
  WGL_EXT_FUNCTION(a, HPBUFFEREXT, wglCreatePbufferEXT,                                            \
                   (HDC arg0, int arg1, int arg2, int arg3, const int* arg4),                      \
                   (arg0, arg1, arg2, arg3, arg4))                                                 \
  WGL_EXT_FUNCTION(a, BOOL, wglDXCloseDeviceNV, (HANDLE arg0), (arg0))                             \
  WGL_EXT_FUNCTION(a, BOOL, wglDXLockObjectsNV, (HANDLE arg0, GLint arg1, HANDLE * arg2),          \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglDXObjectAccessNV, (HANDLE arg0, GLenum arg1), (arg0, arg1))         \
  WGL_EXT_FUNCTION(a, HANDLE, wglDXOpenDeviceNV, (void* arg0), (arg0))                             \
  WGL_EXT_FUNCTION(a, HANDLE, wglDXRegisterObjectNV,                                               \
                   (HANDLE arg0, void* arg1, GLuint arg2, GLenum arg3, GLenum arg4),               \
                   (arg0, arg1, arg2, arg3, arg4))                                                 \
  WGL_EXT_FUNCTION(a, BOOL, wglDXSetResourceShareHandleNV, (void* arg0, HANDLE arg1),              \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, BOOL, wglDXUnlockObjectsNV, (HANDLE arg0, GLint arg1, HANDLE * arg2),        \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglDXUnregisterObjectNV, (HANDLE arg0, HANDLE arg1), (arg0, arg1))     \
  WGL_EXT_FUNCTION(a, BOOL, wglDeleteAssociatedContextAMD, (HGLRC arg0), (arg0))                   \
  WGL_EXT_FUNCTION(a, void_t, wglDeleteBufferRegionARB, (HANDLE arg0), (arg0))                     \
  WGL_EXT_FUNCTION(a, BOOL, wglDeleteDCNV, (HDC arg0), (arg0))                                     \
  WGL_EXT_FUNCTION(a, void_t, wglDestroyDisplayColorTableEXT, (GLushort arg0), (arg0))             \
  WGL_EXT_FUNCTION(a, BOOL, wglDestroyImageBufferI3D, (HDC arg0, LPVOID arg1), (arg0, arg1))       \
  WGL_EXT_FUNCTION(a, BOOL, wglDestroyPbufferARB, (HPBUFFERARB arg0), (arg0))                      \
  WGL_EXT_FUNCTION(a, BOOL, wglDestroyPbufferEXT, (HPBUFFEREXT arg0), (arg0))                      \
  WGL_EXT_FUNCTION(a, BOOL, wglDisableFrameLockI3D, (), ())                                        \
  WGL_EXT_FUNCTION(a, BOOL, wglDisableGenlockI3D, (HDC arg0), (arg0))                              \
  WGL_EXT_FUNCTION(a, BOOL, wglEnableFrameLockI3D, (), ())                                         \
  WGL_EXT_FUNCTION(a, BOOL, wglEnableGenlockI3D, (HDC arg0), (arg0))                               \
  WGL_EXT_FUNCTION(a, BOOL, wglEndFrameTrackingI3D, (), ())                                        \
  WGL_EXT_FUNCTION(a, BOOL, wglEnumGpuDevicesNV, (HGPUNV arg0, UINT arg1, PGPU_DEVICE arg2),       \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglEnumGpusFromAffinityDCNV, (HDC arg0, UINT arg1, HGPUNV * arg2),     \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglEnumGpusNV, (UINT arg0, HGPUNV * arg1), (arg0, arg1))               \
  WGL_EXT_FUNCTION(a, UINT, wglEnumerateVideoCaptureDevicesNV,                                     \
                   (HDC arg0, HVIDEOINPUTDEVICENV * arg1), (arg0, arg1))                           \
  WGL_EXT_FUNCTION(a, int, wglEnumerateVideoDevicesNV, (HDC arg0, HVIDEOOUTPUTDEVICENV * arg1),    \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, void_t, wglFreeMemoryNV, (void* arg0), (arg0))                               \
  WGL_EXT_FUNCTION(a, BOOL, wglGenlockSampleRateI3D, (HDC arg0, UINT arg1), (arg0, arg1))          \
  WGL_EXT_FUNCTION(a, BOOL, wglGenlockSourceDelayI3D, (HDC arg0, UINT arg1), (arg0, arg1))         \
  WGL_EXT_FUNCTION(a, BOOL, wglGenlockSourceEdgeI3D, (HDC arg0, UINT arg1), (arg0, arg1))          \
  WGL_EXT_FUNCTION(a, BOOL, wglGenlockSourceI3D, (HDC arg0, UINT arg1), (arg0, arg1))              \
  WGL_EXT_FUNCTION(a, UINT, wglGetContextGPUIDAMD, (HGLRC arg0), (arg0))                           \
  WGL_EXT_FUNCTION(a, HGLRC, wglGetCurrentAssociatedContextAMD, (), ())                            \
  WGL_EXT_FUNCTION(a, HDC, wglGetCurrentReadDCARB, (), ())                                         \
  WGL_EXT_FUNCTION(a, HDC, wglGetCurrentReadDCEXT, (), ())                                         \
  WGL_EXT_FUNCTION(a, BOOL, wglGetDigitalVideoParametersI3D, (HDC arg0, int arg1, int* arg2),      \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, const char*, wglGetExtensionsStringARB, (HDC arg0), (arg0))                  \
  WGL_EXT_FUNCTION(a, const char*, wglGetExtensionsStringEXT, (), ())                              \
  WGL_EXT_FUNCTION(a, BOOL, wglGetFrameUsageI3D, (float* arg0), (arg0))                            \
  WGL_EXT_FUNCTION(a, UINT, wglGetGPUIDsAMD, (UINT arg0, UINT * arg1), (arg0, arg1))               \
  WGL_EXT_FUNCTION(a, INT, wglGetGPUInfoAMD,                                                       \
                   (UINT arg0, int arg1, GLenum arg2, UINT arg3, void* arg4),                      \
                   (arg0, arg1, arg2, arg3, arg4))                                                 \
  WGL_EXT_FUNCTION(a, BOOL, wglGetGammaTableI3D,                                                   \
                   (HDC arg0, int arg1, USHORT* arg2, USHORT* arg3, USHORT* arg4),                 \
                   (arg0, arg1, arg2, arg3, arg4))                                                 \
  WGL_EXT_FUNCTION(a, BOOL, wglGetGammaTableParametersI3D, (HDC arg0, int arg1, int* arg2),        \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglGetGenlockSampleRateI3D, (HDC arg0, UINT * arg1), (arg0, arg1))     \
  WGL_EXT_FUNCTION(a, BOOL, wglGetGenlockSourceDelayI3D, (HDC arg0, UINT * arg1), (arg0, arg1))    \
  WGL_EXT_FUNCTION(a, BOOL, wglGetGenlockSourceEdgeI3D, (HDC arg0, UINT * arg1), (arg0, arg1))     \
  WGL_EXT_FUNCTION(a, BOOL, wglGetGenlockSourceI3D, (HDC arg0, UINT * arg1), (arg0, arg1))         \
  WGL_EXT_FUNCTION(a, BOOL, wglGetMscRateOML, (HDC arg0, INT32 * arg1, INT32 * arg2),              \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, HDC, wglGetPbufferDCARB, (HPBUFFERARB arg0), (arg0))                         \
  WGL_EXT_FUNCTION(a, HDC, wglGetPbufferDCEXT, (HPBUFFEREXT arg0), (arg0))                         \
  WGL_EXT_FUNCTION(a, BOOL, wglGetPixelFormatAttribfvARB,                                          \
                   (HDC arg0, int arg1, int arg2, UINT arg3, const int* arg4, FLOAT* arg5),        \
                   (arg0, arg1, arg2, arg3, arg4, arg5))                                           \
  WGL_EXT_FUNCTION(a, BOOL, wglGetPixelFormatAttribfvEXT,                                          \
                   (HDC arg0, int arg1, int arg2, UINT arg3, int* arg4, FLOAT* arg5),              \
                   (arg0, arg1, arg2, arg3, arg4, arg5))                                           \
  WGL_EXT_FUNCTION(a, BOOL, wglGetPixelFormatAttribivARB,                                          \
                   (HDC arg0, int arg1, int arg2, UINT arg3, const int* arg4, int* arg5),          \
                   (arg0, arg1, arg2, arg3, arg4, arg5))                                           \
  WGL_EXT_FUNCTION(a, BOOL, wglGetPixelFormatAttribivEXT,                                          \
                   (HDC arg0, int arg1, int arg2, UINT arg3, int* arg4, int* arg5),                \
                   (arg0, arg1, arg2, arg3, arg4, arg5))                                           \
  WGL_EXT_FUNCTION(a, int, wglGetSwapIntervalEXT, (), ())                                          \
  WGL_EXT_FUNCTION(a, BOOL, wglGetSyncValuesOML,                                                   \
                   (HDC arg0, INT64 * arg1, INT64 * arg2, INT64 * arg3), (arg0, arg1, arg2, arg3)) \
  WGL_EXT_FUNCTION(a, BOOL, wglGetVideoDeviceNV, (HDC arg0, int arg1, HPVIDEODEV* arg2),           \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglGetVideoInfoNV,                                                     \
                   (HPVIDEODEV arg0, unsigned long* arg1, unsigned long* arg2),                    \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglIsEnabledFrameLockI3D, (BOOL * arg0), (arg0))                       \
  WGL_EXT_FUNCTION(a, BOOL, wglIsEnabledGenlockI3D, (HDC arg0, BOOL * arg1), (arg0, arg1))         \
  WGL_EXT_FUNCTION(a, BOOL, wglJoinSwapGroupNV, (HDC arg0, GLuint arg1), (arg0, arg1))             \
  WGL_EXT_FUNCTION(a, GLboolean, wglLoadDisplayColorTableEXT, (const GLushort* arg0, GLuint arg1), \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, BOOL, wglLockVideoCaptureDeviceNV, (HDC arg0, HVIDEOINPUTDEVICENV arg1),     \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, BOOL, wglMakeAssociatedContextCurrentAMD, (HGLRC arg0), (arg0))              \
  WGL_EXT_FUNCTION(a, BOOL, wglMakeContextCurrentARB, (HDC arg0, HDC arg1, HGLRC arg2),            \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglMakeContextCurrentEXT, (HDC arg0, HDC arg1, HGLRC arg2),            \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryCurrentContextNV, (int arg0, int* arg1), (arg0, arg1))         \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryFrameCountNV, (HDC arg0, GLuint * arg1), (arg0, arg1))         \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryFrameLockMasterI3D, (BOOL * arg0), (arg0))                     \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryFrameTrackingI3D, (DWORD * arg0, DWORD * arg1, float* arg2),   \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryGenlockMaxSourceDelayI3D,                                      \
                   (HDC arg0, UINT * arg1, UINT * arg2), (arg0, arg1, arg2))                       \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryMaxSwapGroupsNV, (HDC arg0, GLuint * arg1, GLuint * arg2),     \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryPbufferARB, (HPBUFFERARB arg0, int arg1, int* arg2),           \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryPbufferEXT, (HPBUFFEREXT arg0, int arg1, int* arg2),           \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglQuerySwapGroupNV, (HDC arg0, GLuint * arg1, GLuint * arg2),         \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglQueryVideoCaptureDeviceNV,                                          \
                   (HDC arg0, HVIDEOINPUTDEVICENV arg1, int arg2, int* arg3),                      \
                   (arg0, arg1, arg2, arg3))                                                       \
  WGL_EXT_FUNCTION(a, BOOL, wglReleaseImageBufferEventsI3D,                                        \
                   (HDC arg0, const LPVOID* arg1, UINT arg2), (arg0, arg1, arg2))                  \
  WGL_EXT_FUNCTION(a, int, wglReleasePbufferDCARB, (HPBUFFERARB arg0, HDC arg1), (arg0, arg1))     \
  WGL_EXT_FUNCTION(a, int, wglReleasePbufferDCEXT, (HPBUFFEREXT arg0, HDC arg1), (arg0, arg1))     \
  WGL_EXT_FUNCTION(a, BOOL, wglReleaseTexImageARB, (HPBUFFERARB arg0, int arg1), (arg0, arg1))     \
  WGL_EXT_FUNCTION(a, BOOL, wglReleaseVideoCaptureDeviceNV, (HDC arg0, HVIDEOINPUTDEVICENV arg1),  \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, BOOL, wglReleaseVideoDeviceNV, (HPVIDEODEV arg0), (arg0))                    \
  WGL_EXT_FUNCTION(a, BOOL, wglReleaseVideoImageNV, (HPBUFFERARB arg0, int arg1), (arg0, arg1))    \
  WGL_EXT_FUNCTION(a, BOOL, wglResetFrameCountNV, (HDC arg0), (arg0))                              \
  WGL_EXT_FUNCTION(a, BOOL, wglRestoreBufferRegionARB,                                             \
                   (HANDLE arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6),      \
                   (arg0, arg1, arg2, arg3, arg4, arg5, arg6))                                     \
  WGL_EXT_FUNCTION(a, BOOL, wglSaveBufferRegionARB,                                                \
                   (HANDLE arg0, int arg1, int arg2, int arg3, int arg4),                          \
                   (arg0, arg1, arg2, arg3, arg4))                                                 \
  WGL_EXT_FUNCTION(a, BOOL, wglSendPbufferToVideoNV,                                               \
                   (HPBUFFERARB arg0, int arg1, unsigned long* arg2, BOOL arg3),                   \
                   (arg0, arg1, arg2, arg3))                                                       \
  WGL_EXT_FUNCTION(a, BOOL, wglSetDigitalVideoParametersI3D,                                       \
                   (HDC arg0, int arg1, const int* arg2), (arg0, arg1, arg2))                      \
  WGL_EXT_FUNCTION(                                                                                \
      a, BOOL, wglSetGammaTableI3D,                                                                \
      (HDC arg0, int arg1, const USHORT* arg2, const USHORT* arg3, const USHORT* arg4),            \
      (arg0, arg1, arg2, arg3, arg4))                                                              \
  WGL_EXT_FUNCTION(a, BOOL, wglSetGammaTableParametersI3D, (HDC arg0, int arg1, const int* arg2),  \
                   (arg0, arg1, arg2))                                                             \
  WGL_EXT_FUNCTION(a, BOOL, wglSetPbufferAttribARB, (HPBUFFERARB arg0, const int* arg1),           \
                   (arg0, arg1))                                                                   \
  WGL_EXT_FUNCTION(a, BOOL, wglSetStereoEmitterState3DL, (HDC arg0, UINT arg1), (arg0, arg1))      \
  WGL_EXT_FUNCTION(a, INT64, wglSwapBuffersMscOML, (HDC arg0, INT64 arg1, INT64 arg2, INT64 arg3), \
                   (arg0, arg1, arg2, arg3))                                                       \
  WGL_EXT_FUNCTION(a, BOOL, wglSwapIntervalEXT, (int arg0), (arg0))                                \
  WGL_EXT_FUNCTION(a, INT64, wglSwapLayerBuffersMscOML,                                            \
                   (HDC hdc, INT fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder),      \
                   (hdc, fuPlanes, target_msc, divisor, remainder))                                \
  WGL_EXT_FUNCTION(                                                                                \
      a, BOOL, wglWaitForMscOML,                                                                   \
      (HDC arg0, INT64 arg1, INT64 arg2, INT64 arg3, INT64 * arg4, INT64 * arg5, INT64 * arg6),    \
      (arg0, arg1, arg2, arg3, arg4, arg5, arg6))                                                  \
  WGL_EXT_FUNCTION(a, BOOL, wglWaitForSbcOML,                                                      \
                   (HDC arg0, INT64 arg1, INT64 * arg2, INT64 * arg3, INT64 * arg4),               \
                   (arg0, arg1, arg2, arg3, arg4))

#define GLX_FUNCTIONS(a)                                                                           \
  GLX_FUNCTION(a, void*, glXAllocateMemoryNV,                                                      \
               (GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority),              \
               (size, readfreq, writefreq, priority))                                              \
  GLX_FUNCTION(a, int, glXBeginFrameTrackingMESA, (Display * dpy, GLXDrawable drawable),           \
               (dpy, drawable))                                                                    \
  GLX_FUNCTION(a, int, glXBindChannelToWindowSGIX,                                                 \
               (Display * display, int screen, int channel, Window window),                        \
               (display, screen, channel, window))                                                 \
  GLX_FUNCTION(a, int, glXBindHyperpipeSGIX, (Display * dpy, int hpId), (dpy, hpId))               \
  GLX_FUNCTION(a, Bool, glXBindSwapBarrierNV, (Display * dpy, GLuint group, GLuint barrier),       \
               (dpy, group, barrier))                                                              \
  GLX_FUNCTION(a, void_t, glXBindSwapBarrierSGIX,                                                  \
               (Display * dpy, GLXDrawable drawable, int barrier), (dpy, drawable, barrier))       \
  GLX_FUNCTION(a, Bool, glXBindTexImageARB, (Display * dpy, GLXPbuffer pbuffer, int buffer),       \
               (dpy, pbuffer, buffer))                                                             \
  GLX_FUNCTION(a, void_t, glXBindTexImageEXT,                                                      \
               (Display * dpy, GLXDrawable drawable, int buffer, const int* attrib_list),          \
               (dpy, drawable, buffer, attrib_list))                                               \
  GLX_FUNCTION(a, int, glXBindVideoCaptureDeviceNV,                                                \
               (Display * dpy, unsigned int video_capture_slot, GLXVideoCaptureDeviceNV device),   \
               (dpy, video_capture_slot, device))                                                  \
  GLX_FUNCTION(                                                                                    \
      a, int, glXBindVideoDeviceNV,                                                                \
      (Display * dpy, unsigned int video_slot, unsigned int video_device, const int* attrib_list), \
      (dpy, video_slot, video_device, attrib_list))                                                \
  GLX_FUNCTION(a, int, glXBindVideoImageNV,                                                        \
               (Display * dpy, GLXVideoDeviceNV VideoDevice, GLXPbuffer pbuf, int iVideoBuffer),   \
               (dpy, VideoDevice, pbuf, iVideoBuffer))                                             \
  GLX_FUNCTION(a, int, glXChannelRectSGIX,                                                         \
               (Display * display, int screen, int channel, int x, int y, int w, int h),           \
               (display, screen, channel, x, y, w, h))                                             \
  GLX_FUNCTION(a, int, glXChannelRectSyncSGIX,                                                     \
               (Display * display, int screen, int channel, GLenum synctype),                      \
               (display, screen, channel, synctype))                                               \
  GLX_FUNCTION(a, GLXFBConfig*, glXChooseFBConfig,                                                 \
               (Display * dpy, int screen, const int* attribList, int* nitems),                    \
               (dpy, screen, attribList, nitems))                                                  \
  GLX_FUNCTION(a, GLXFBConfigSGIX*, glXChooseFBConfigSGIX,                                         \
               (Display * dpy, int screen, int* attrib_list, int* nelements),                      \
               (dpy, screen, attrib_list, nelements))                                              \
  GLX_FUNCTION(a, XVisualInfo*, glXChooseVisual, (Display * dpy, int screen, int* attribList),     \
               (dpy, screen, attribList))                                                          \
  GLX_FUNCTION(a, void_t, glXCopyContext,                                                          \
               (Display * dpy, GLXContext src, GLXContext dst, unsigned long mask),                \
               (dpy, src, dst, mask))                                                              \
  GLX_FUNCTION(a, void_t, glXCopySubBufferMESA,                                                    \
               (Display * dpy, GLXDrawable drawable, int x, int y, int width, int height),         \
               (dpy, drawable, x, y, width, height))                                               \
  GLX_FUNCTION(a, GLXContext, glXCreateContext,                                                    \
               (Display * dpy, XVisualInfo * vis, GLXContext shareList, Bool direct),              \
               (dpy, vis, shareList, direct))                                                      \
  GLX_FUNCTION(a, GLXContext, glXCreateContextAttribsARB,                                          \
               (Display * dpy, GLXFBConfig config, GLXContext share_context, Bool direct,          \
                const int* attrib_list),                                                           \
               (dpy, config, share_context, direct, attrib_list))                                  \
  GLX_FUNCTION(a, GLXContext, glXCreateContextWithConfigSGIX,                                      \
               (Display * dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list,     \
                Bool direct),                                                                      \
               (dpy, config, render_type, share_list, direct))                                     \
  GLX_FUNCTION(a, GLXPbufferSGIX, glXCreateGLXPbufferSGIX,                                         \
               (Display * dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height,    \
                int* attrib_list),                                                                 \
               (dpy, config, width, height, attrib_list))                                          \
  GLX_FUNCTION(a, GLXPixmap, glXCreateGLXPixmap,                                                   \
               (Display * dpy, XVisualInfo * visual, Pixmap pixmap), (dpy, visual, pixmap))        \
  GLX_FUNCTION(a, GLXPixmap, glXCreateGLXPixmapMESA,                                               \
               (Display * dpy, XVisualInfo * visual, Pixmap pixmap, Colormap cmap),                \
               (dpy, visual, pixmap, cmap))                                                        \
  GLX_FUNCTION(a, GLXPixmap, glXCreateGLXPixmapWithConfigSGIX,                                     \
               (Display * dpy, GLXFBConfigSGIX config, Pixmap pixmap), (dpy, config, pixmap))      \
  GLX_FUNCTION(                                                                                    \
      a, GLXContext, glXCreateNewContext,                                                          \
      (Display * dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct),      \
      (dpy, config, renderType, shareList, direct))                                                \
  GLX_FUNCTION(a, GLXPbuffer, glXCreatePbuffer,                                                    \
               (Display * dpy, GLXFBConfig config, const int* attribList),                         \
               (dpy, config, attribList))                                                          \
  GLX_FUNCTION(a, GLXPixmap, glXCreatePixmap,                                                      \
               (Display * dpy, GLXFBConfig config, Pixmap pixmap, const int* attribList),          \
               (dpy, config, pixmap, attribList))                                                  \
  GLX_FUNCTION(a, GLXWindow, glXCreateWindow,                                                      \
               (Display * dpy, GLXFBConfig config, Window win, const int* attribList),             \
               (dpy, config, win, attribList))                                                     \
  GLX_FUNCTION(a, void_t, glXCushionSGI, (Display * dpy, Window window, float cushion),            \
               (dpy, window, cushion))                                                             \
  GLX_FUNCTION(a, void_t, glXDestroyContext, (Display * dpy, GLXContext ctx), (dpy, ctx))          \
  GLX_FUNCTION(a, void_t, glXDestroyGLXPbufferSGIX, (Display * dpy, GLXPbufferSGIX pbuf),          \
               (dpy, pbuf))                                                                        \
  GLX_FUNCTION(a, void_t, glXDestroyGLXPixmap, (Display * dpy, GLXPixmap pixmap), (dpy, pixmap))   \
  GLX_FUNCTION(a, int, glXDestroyHyperpipeConfigSGIX, (Display * dpy, int hpId), (dpy, hpId))      \
  GLX_FUNCTION(a, void_t, glXDestroyPbuffer, (Display * dpy, GLXPbuffer pbuf), (dpy, pbuf))        \
  GLX_FUNCTION(a, void_t, glXDestroyPixmap, (Display * dpy, GLXPixmap pixmap), (dpy, pixmap))      \
  GLX_FUNCTION(a, void_t, glXDestroyWindow, (Display * dpy, GLXWindow window), (dpy, window))      \
  GLX_FUNCTION(a, Bool, glXDrawableAttribARB,                                                      \
               (Display * dpy, GLXDrawable draw, const int* attribList), (dpy, draw, attribList))  \
  GLX_FUNCTION(a, int, glXEndFrameTrackingMESA, (Display * dpy, GLXDrawable drawable),             \
               (dpy, drawable))                                                                    \
  GLX_FUNCTION(a, GLXVideoCaptureDeviceNV*, glXEnumerateVideoCaptureDevicesNV,                     \
               (Display * dpy, int screen, int* nelements), (dpy, screen, nelements))              \
  GLX_FUNCTION(a, unsigned int*, glXEnumerateVideoDevicesNV,                                       \
               (Display * dpy, int screen, int* nelements), (dpy, screen, nelements))              \
  GLX_FUNCTION(a, void_t, glXFreeContextEXT, (Display * dpy, GLXContext context), (dpy, context))  \
  GLX_FUNCTION(a, void_t, glXFreeMemoryNV, (GLvoid * pointer), (pointer))                          \
  GLX_FUNCTION(a, unsigned int, glXGetAGPOffsetMESA, (const void* pointer), (pointer))             \
  GLX_FUNCTION(a, const char*, glXGetClientString, (Display * dpy, int name), (dpy, name))         \
  GLX_FUNCTION(a, int, glXGetConfig,                                                               \
               (Display * dpy, XVisualInfo * visual, int attrib, int* value),                      \
               (dpy, visual, attrib, value))                                                       \
  GLX_FUNCTION(a, GLXContextID, glXGetContextIDEXT, (const GLXContext context), (context))         \
  GLX_FUNCTION(a, GLXContext, glXGetCurrentContext, (), ())                                        \
  GLX_FUNCTION(a, Display*, glXGetCurrentDisplay, (), ())                                          \
  GLX_FUNCTION(a, Display*, glXGetCurrentDisplayEXT, (), ())                                       \
  GLX_FUNCTION(a, GLXDrawable, glXGetCurrentDrawable, (), ())                                      \
  GLX_FUNCTION(a, GLXDrawable, glXGetCurrentReadDrawable, (), ())                                  \
  GLX_FUNCTION(a, GLXDrawable, glXGetCurrentReadDrawableSGI, (), ())                               \
  GLX_FUNCTION(a, int, glXGetFBConfigAttrib,                                                       \
               (Display * dpy, GLXFBConfig config, int attribute, int* value),                     \
               (dpy, config, attribute, value))                                                    \
  GLX_FUNCTION(a, int, glXGetFBConfigAttribSGIX,                                                   \
               (Display * dpy, GLXFBConfigSGIX config, int attribute, int* value),                 \
               (dpy, config, attribute, value))                                                    \
  GLX_FUNCTION(a, GLXFBConfigSGIX, glXGetFBConfigFromVisualSGIX,                                   \
               (Display * dpy, XVisualInfo * vis), (dpy, vis))                                     \
  GLX_FUNCTION(a, GLXFBConfig*, glXGetFBConfigs, (Display * dpy, int screen, int* nelements),      \
               (dpy, screen, nelements))                                                           \
  GLX_FUNCTION(a, int, glXGetFrameUsageMESA, (Display * dpy, GLXDrawable drawable, float* usage),  \
               (dpy, drawable, usage))                                                             \
  GLX_FUNCTION(a, Bool, glXGetMscRateOML,                                                          \
               (Display * dpy, GLXDrawable drawable, int32_t * numerator, int32_t * denominator),  \
               (dpy, drawable, numerator, denominator))                                            \
  GLX_FUNCTION(a, void*, glXGetProcAddress, (const GLubyte* procname), (procname))                 \
  GLX_FUNCTION(a, void*, glXGetProcAddressARB, (const GLubyte* arg0), (arg0))                      \
  GLX_FUNCTION(a, void_t, glXGetSelectedEvent,                                                     \
               (Display * dpy, GLXDrawable drawable, unsigned long* mask), (dpy, drawable, mask))  \
  GLX_FUNCTION(a, void_t, glXGetSelectedEventSGIX,                                                 \
               (Display * dpy, GLXDrawable drawable, unsigned long* mask), (dpy, drawable, mask))  \
  GLX_FUNCTION(a, int, glXGetSwapIntervalMESA, (), ())                                             \
  GLX_FUNCTION(a, Bool, glXGetSyncValuesOML,                                                       \
               (Display * dpy, GLXDrawable drawable, int64_t * ust, int64_t * msc, int64_t * sbc), \
               (dpy, drawable, ust, msc, sbc))                                                     \
  GLX_FUNCTION(a, int, glXGetTransparentIndexSUN,                                                  \
               (Display * dpy, Window overlay, Window underlay, long* pTransparentIndex),          \
               (dpy, overlay, underlay, pTransparentIndex))                                        \
  GLX_FUNCTION(a, int, glXGetVideoDeviceNV,                                                        \
               (Display * dpy, int screen, int numVideoDevices, GLXVideoDeviceNV* pVideoDevice),   \
               (dpy, screen, numVideoDevices, pVideoDevice))                                       \
  GLX_FUNCTION(a, int, glXGetVideoInfoNV,                                                          \
               (Display * dpy, int screen, GLXVideoDeviceNV VideoDevice,                           \
                unsigned long* pulCounterOutputPbuffer, unsigned long* pulCounterOutputVideo),     \
               (dpy, screen, VideoDevice, pulCounterOutputPbuffer, pulCounterOutputVideo))         \
  GLX_FUNCTION(a, int, glXGetVideoSyncSGI, (unsigned int* count), (count))                         \
  GLX_FUNCTION(a, XVisualInfo*, glXGetVisualFromFBConfig, (Display * dpy, GLXFBConfig config),     \
               (dpy, config))                                                                      \
  GLX_FUNCTION(a, XVisualInfo*, glXGetVisualFromFBConfigSGIX,                                      \
               (Display * dpy, GLXFBConfigSGIX config), (dpy, config))                             \
  GLX_FUNCTION(a, int, glXHyperpipeAttribSGIX,                                                     \
               (Display * dpy, int timeSlice, int attrib, int size, void* attribList),             \
               (dpy, timeSlice, attrib, size, attribList))                                         \
  GLX_FUNCTION(a, int, glXHyperpipeConfigSGIX,                                                     \
               (Display * dpy, int networkId, int npipes, void* cfg, int* hpId),                   \
               (dpy, networkId, npipes, cfg, hpId))                                                \
  GLX_FUNCTION(a, GLXContext, glXImportContextEXT, (Display * dpy, GLXContextID contextID),        \
               (dpy, contextID))                                                                   \
  GLX_FUNCTION(a, Bool, glXIsDirect, (Display * dpy, GLXContext ctx), (dpy, ctx))                  \
  GLX_FUNCTION(a, Bool, glXJoinSwapGroupNV, (Display * dpy, GLXDrawable drawable, GLuint group),   \
               (dpy, drawable, group))                                                             \
  GLX_FUNCTION(a, void_t, glXJoinSwapGroupSGIX,                                                    \
               (Display * dpy, GLXDrawable drawable, GLXDrawable member), (dpy, drawable, member)) \
  GLX_FUNCTION(a, void_t, glXLockVideoCaptureDeviceNV,                                             \
               (Display * dpy, GLXVideoCaptureDeviceNV device), (dpy, device))                     \
  GLX_FUNCTION(a, Bool, glXMakeContextCurrent,                                                     \
               (Display * dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx),                \
               (dpy, draw, read, ctx))                                                             \
  GLX_FUNCTION(a, Bool, glXMakeCurrent, (Display * dpy, GLXDrawable drawable, GLXContext ctx),     \
               (dpy, drawable, ctx))                                                               \
  GLX_FUNCTION(a, Bool, glXMakeCurrentReadSGI,                                                     \
               (Display * dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx),                \
               (dpy, draw, read, ctx))                                                             \
  GLX_FUNCTION(a, int, glXQueryChannelDeltasSGIX,                                                  \
               (Display * display, int screen, int channel, int* x, int* y, int* w, int* h),       \
               (display, screen, channel, x, y, w, h))                                             \
  GLX_FUNCTION(a, int, glXQueryChannelRectSGIX,                                                    \
               (Display * display, int screen, int channel, int* dx, int* dy, int* dw, int* dh),   \
               (display, screen, channel, dx, dy, dw, dh))                                         \
  GLX_FUNCTION(a, int, glXQueryContext,                                                            \
               (Display * dpy, GLXContext ctx, int attribute, int* value),                         \
               (dpy, ctx, attribute, value))                                                       \
  GLX_FUNCTION(a, int, glXQueryContextInfoEXT,                                                     \
               (Display * dpy, GLXContext context, int attribute, int* value),                     \
               (dpy, context, attribute, value))                                                   \
  GLX_FUNCTION(a, void_t, glXQueryDrawable,                                                        \
               (Display * dpy, GLXDrawable draw, int attribute, unsigned int* value),              \
               (dpy, draw, attribute, value))                                                      \
  GLX_FUNCTION(a, Bool, glXQueryExtension, (Display * dpy, int* errorb, int* event),               \
               (dpy, errorb, event))                                                               \
  GLX_FUNCTION(a, const char*, glXQueryExtensionsString, (Display * dpy, int screen),              \
               (dpy, screen))                                                                      \
  GLX_FUNCTION(a, Bool, glXQueryFrameCountNV, (Display * dpy, int screen, GLuint* count),          \
               (dpy, screen, count))                                                               \
  GLX_FUNCTION(a, int, glXQueryFrameTrackingMESA,                                                  \
               (Display * dpy, GLXDrawable drawable, int64_t * swapCount, int64_t * missedFrames,  \
                float* lastMissedUsage),                                                           \
               (dpy, drawable, swapCount, missedFrames, lastMissedUsage))                          \
  GLX_FUNCTION(a, int, glXQueryGLXPbufferSGIX,                                                     \
               (Display * dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int* value),           \
               (dpy, pbuf, attribute, value))                                                      \
  GLX_FUNCTION(a, int, glXQueryHyperpipeAttribSGIX,                                                \
               (Display * dpy, int timeSlice, int attrib, int size, void* returnAttribList),       \
               (dpy, timeSlice, attrib, size, returnAttribList))                                   \
  GLX_FUNCTION(a, int, glXQueryHyperpipeBestAttribSGIX,                                            \
               (Display * dpy, int timeSlice, int attrib, int size, void* attribList,              \
                void* returnAttribList),                                                           \
               (dpy, timeSlice, attrib, size, attribList, returnAttribList))                       \
  GLX_FUNCTION(a, void*, glXQueryHyperpipeConfigSGIX, (Display * dpy, int hpId, int* npipes),      \
               (dpy, hpId, npipes))                                                                \
  GLX_FUNCTION(a, void*, glXQueryHyperpipeNetworkSGIX, (Display * dpy, int* npipes),               \
               (dpy, npipes))                                                                      \
  GLX_FUNCTION(a, Bool, glXQueryMaxSwapBarriersSGIX, (Display * dpy, int screen, int* max),        \
               (dpy, screen, max))                                                                 \
  GLX_FUNCTION(a, Bool, glXQueryMaxSwapGroupsNV,                                                   \
               (Display * dpy, int screen, GLuint* maxGroups, GLuint* maxBarriers),                \
               (dpy, screen, maxGroups, maxBarriers))                                              \
  GLX_FUNCTION(a, const char*, glXQueryServerString, (Display * dpy, int screen, int name),        \
               (dpy, screen, name))                                                                \
  GLX_FUNCTION(a, Bool, glXQuerySwapGroupNV,                                                       \
               (Display * dpy, GLXDrawable drawable, GLuint * group, GLuint * barrier),            \
               (dpy, drawable, group, barrier))                                                    \
  GLX_FUNCTION(a, Bool, glXQueryVersion, (Display * dpy, int* maj, int* min), (dpy, maj, min))     \
  GLX_FUNCTION(a, int, glXQueryVideoCaptureDeviceNV,                                               \
               (Display * dpy, GLXVideoCaptureDeviceNV device, int attribute, int* value),         \
               (dpy, device, attribute, value))                                                    \
  GLX_FUNCTION(a, Bool, glXReleaseBuffersMESA, (Display * dpy, GLXDrawable drawable),              \
               (dpy, drawable))                                                                    \
  GLX_FUNCTION(a, Bool, glXReleaseTexImageARB, (Display * dpy, GLXPbuffer pbuffer, int buffer),    \
               (dpy, pbuffer, buffer))                                                             \
  GLX_FUNCTION(a, void_t, glXReleaseTexImageEXT,                                                   \
               (Display * dpy, GLXDrawable drawable, int buffer), (dpy, drawable, buffer))         \
  GLX_FUNCTION(a, void_t, glXReleaseVideoCaptureDeviceNV,                                          \
               (Display * dpy, GLXVideoCaptureDeviceNV device), (dpy, device))                     \
  GLX_FUNCTION(a, int, glXReleaseVideoDeviceNV,                                                    \
               (Display * dpy, int screen, GLXVideoDeviceNV VideoDevice),                          \
               (dpy, screen, VideoDevice))                                                         \
  GLX_FUNCTION(a, int, glXReleaseVideoImageNV, (Display * dpy, GLXPbuffer pbuf), (dpy, pbuf))      \
  GLX_FUNCTION(a, Bool, glXResetFrameCountNV, (Display * dpy, int screen), (dpy, screen))          \
  GLX_FUNCTION(a, void_t, glXSelectEvent,                                                          \
               (Display * dpy, GLXDrawable drawable, unsigned long mask), (dpy, drawable, mask))   \
  GLX_FUNCTION(a, void_t, glXSelectEventSGIX,                                                      \
               (Display * dpy, GLXDrawable drawable, unsigned long mask), (dpy, drawable, mask))   \
  GLX_FUNCTION(a, int, glXSendPbufferToVideoNV,                                                    \
               (Display * dpy, GLXPbuffer pbuf, int iBufferType, unsigned long* pulCounterPbuffer, \
                GLboolean bBlock),                                                                 \
               (dpy, pbuf, iBufferType, pulCounterPbuffer, bBlock))                                \
  GLX_FUNCTION(a, Bool, glXSet3DfxModeMESA, (int mode), (mode))                                    \
  GLX_FUNCTION(a, void_t, glXSwapBuffers, (Display * dpy, GLXDrawable drawable), (dpy, drawable))  \
  GLX_FUNCTION(a, int64_t, glXSwapBuffersMscOML,                                                   \
               (Display * dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor,          \
                int64_t remainder),                                                                \
               (dpy, drawable, target_msc, divisor, remainder))                                    \
  GLX_FUNCTION(a, void_t, glXSwapIntervalEXT, (Display * dpy, GLXDrawable drawable, int interval), \
               (dpy, drawable, interval))                                                          \
  GLX_FUNCTION(a, int, glXSwapIntervalMESA, (unsigned int interval), (interval))                   \
  GLX_FUNCTION(a, int, glXSwapIntervalSGI, (int interval), (interval))                             \
  GLX_FUNCTION(a, void_t, glXUseXFont, (Font font, int first, int count, int list),                \
               (font, first, count, list))                                                         \
  GLX_FUNCTION(a, Bool, glXWaitForMscOML,                                                          \
               (Display * dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor,          \
                int64_t remainder, int64_t * ust, int64_t * msc, int64_t * sbc),                   \
               (dpy, drawable, target_msc, divisor, remainder, ust, msc, sbc))                     \
  GLX_FUNCTION(a, Bool, glXWaitForSbcOML,                                                          \
               (Display * dpy, GLXDrawable drawable, int64_t target_sbc, int64_t * ust,            \
                int64_t * msc, int64_t * sbc),                                                     \
               (dpy, drawable, target_sbc, ust, msc, sbc))                                         \
  GLX_FUNCTION(a, void_t, glXWaitGL, (), ())                                                       \
  GLX_FUNCTION(a, int, glXWaitVideoSyncSGI, (int divisor, int remainder, unsigned int* count),     \
               (divisor, remainder, count))                                                        \
  GLX_FUNCTION(a, void_t, glXWaitX, (), ())

namespace gits {
namespace OpenGL {
#define EGL_FUNCTION(a, b, c, d, e) a##EGL_FUNCTION(b, c, d, e)
#define DECLARE_PTR_EGL_FUNCTION(a, b, c, d)                                                       \
  a(STDCALL* b) c;                                                                                 \
  a(STDCALL* shd_##b) c;

class CEglDriver {
public:
  CEglDriver();
  CEglDriver(const CEglDriver& other) = delete;
  CEglDriver& operator=(const CEglDriver& other) = delete;
  ~CEglDriver();
  dl::SharedObject Library();
  void Used(bool value) {
    _egl_used = value;
  }
  bool Used() const {
    return _egl_used;
  }

  EGL_FUNCTIONS(DECLARE_PTR_)
private:
  bool _initialized;
  dl::SharedObject _lib_egl;
  bool _egl_used;
};

#undef DECLARE_PTR_EGL_FUNCTION

#define WGL_FUNCTION(a, b, c, d, e) a##WGL_FUNCTION(b, c, d, e)
#define DECLARE_PTR_WGL_FUNCTION(a, b, c, d)                                                       \
  a(STDCALL* b) c;                                                                                 \
  a(STDCALL* shd_##b) c;

#define WGL_EXT_FUNCTION(a, b, c, d, e) a##WGL_EXT_FUNCTION(b, c, d, e)
#define DECLARE_PTR_WGL_EXT_FUNCTION(a, b, c, d)                                                   \
  a(STDCALL* b) c;                                                                                 \
  a(STDCALL* shd_##b) c;

#if defined GITS_PLATFORM_WINDOWS
class CWglDriver {
public:
  CWglDriver();
  CWglDriver(const CWglDriver& other) = delete;
  CWglDriver& operator=(const CWglDriver& other) = delete;
  ~CWglDriver();
  dl::SharedObject Library();

  WGL_FUNCTIONS(DECLARE_PTR_)
  WGL_EXT_FUNCTIONS(DECLARE_PTR_)

private:
  bool _initialized;
  dl::SharedObject _lib;
};
#endif

#undef DECLARE_PTR_WGL_FUNCTION
#undef DECLARE_PTR_WGL_EXT_FUNCTION

#if defined GITS_PLATFORM_X11

#define GLX_FUNCTION(a, b, c, d, e) a##GLX_FUNCTION(b, c, d, e)
#define DECLARE_PTR_GLX_FUNCTION(a, b, c, d)                                                       \
  a(STDCALL* b) c;                                                                                 \
  a(STDCALL* shd_##b) c;

class CGlxDriver {
public:
  CGlxDriver();
  dl::SharedObject Library();

  GLX_FUNCTIONS(DECLARE_PTR_)
private:
  bool _initialized;
  dl::SharedObject _lib;
};

#undef DECLARE_PTR_GLX_FUNCTION

#endif

#define GL_FUNCTION(a, b, c, d, e) a##GL_FUNCTION(b, c, d, e)
#define DECLARE_PTR_GL_FUNCTION(a, b, c, e)                                                        \
  a(STDCALL* b) c;                                                                                 \
  a(STDCALL* shd_##b) c;

#define DRAW_FUNCTION(a, b, c, d, e) GL_FUNCTION(a, b, c, d, e)

#define DECLARE_PTR_GL_DRAW_FUNCTION(a, b, c, e)                                                   \
  a(STDCALL* b) c;                                                                                 \
  a(STDCALL* shd_##b) c;

class CGlDriver {
public:
  CGlDriver();
  CGlDriver(const CGlDriver& other) = delete;
  CGlDriver& operator=(const CGlDriver& other) = delete;
  ~CGlDriver();
  enum TApiType {
    API_GL,
    API_GLES1,
    API_GLES2,
    API_NULL
  };

  void Initialize(TApiType api) WEAK;

  TApiType Api() const {
    return _api;
  }
  dl::SharedObject Library() const {
    return _lib;
  }

  bool HasExtension(const std::string& extension) const;
  bool CanReadWriteonlyMappings() const;

  GL_FUNCTIONS(DECLARE_PTR_)
  DRAW_FUNCTIONS(DECLARE_PTR_)
private:
  TApiType _api;
  bool _initialized;
  dl::SharedObject _lib;
};

#undef DECLARE_PTR_GL_FUNCTION
#undef DECLARE_PTR_GL_DRAW_FUNCTION

struct CDrivers {
  CEglDriver egl;
#if defined GITS_PLATFORM_WINDOWS
  CWglDriver wgl;
#elif defined GITS_PLATFORM_X11
  CGlxDriver glx;
#endif

  CGlDriver gl;

  bool traceGLAPIBypass{false};

  void add_terminate_event(std::function<void()> e) {
    events_.push_back(e);
  }

  NOINLINE void trigger_terminate_event() {
    for (auto& e : events_) {
      e();
    }
  }

private:
  std::vector<std::function<void()>> events_;
};

extern CDrivers drv WEAK;
#if defined GITS_PLATFORM_X11
extern "C" GLXContext glXGetCurrentContext();
#endif

void* load_egl_or_native(const char* name);
bool check_gl_function_availability(const char* name);

} // namespace OpenGL
} // namespace gits
