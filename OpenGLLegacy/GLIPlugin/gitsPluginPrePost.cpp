// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gitsPluginPrePost.h
 *
 * @brief
 */

#include "gitsPluginPrePost.h"

namespace gits {
namespace OpenGL {
void* get_proc_address(const char* name);
}
} // namespace gits

std::recursive_mutex globalMutex;
// Avoid recording API - recursive functions.
thread_local uint32_t recursionDepth = 0;
const uint32_t disableDepth = 1000;

void CloseRecorderIfRequired() {
  gits::OpenGL::CGitsPlugin::RecorderWrapper().CloseRecorderIfRequired();
}

void EndFramePost() {
  gits::OpenGL::CGitsPlugin::RecorderWrapper().EndFramePost();
}

void PrePostDisableGL() {
  recursionDepth = disableDepth;
}

void post_gits_wrapper() {}

void entry() {
  gits::OpenGL::CGitsPlugin::Initialize();
}

namespace {
// When Mesas libGL is loaded to the application, the first thing it does is to
// load itself globally into the process. When working with LD_LIBRARY_PATH the
// library it loads is GITS lib instead and Mesa symbols fail to get pushed to
// global symbol namespace. DRI drivers loaded after that fail to find required
// symbols and error out fatally. Here we emulate Mesa behavior by pushing Mesa
// into global namespace after we have loaded.
void spillGL() {
#ifdef GITS_PLATFORM_X11
  CALL_ONCE[] {
    std::string path = (gits::OpenGL::CGitsPlugin::Configuration().common.shared.libGL).string();
    void* handleLibGL = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handleLibGL == nullptr) {
      LOG_ERROR << "Failed to load GL library: " << path;
      exit(1);
    }
  };
#endif
}

void spillEGL() {
#ifdef GITS_PLATFORM_X11
  CALL_ONCE[] {
    std::string path = (gits::OpenGL::CGitsPlugin::Configuration().common.shared.libEGL).string();
    void* handleLibEGL = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handleLibEGL == nullptr) {
      LOG_ERROR << "Failed to load EGL library: " << path;
      exit(1);
    }
  };
#endif
}
} // namespace

#define GITS_ENTRY_WGL GITS_ENTRY
#define GITS_ENTRY_GLX GITS_ENTRY spillGL();
#define GITS_ENTRY_EGL                                                                             \
  GITS_ENTRY spillEGL();                                                                           \
  wrapper.EGLInitialize();

extern "C" {

GLAPI int GLAPIENTRY GITSIdentificationToken() {
  static int i = 0;
  return ++i;
}

GLAPI EGLint GLAPIENTRY eglGetError() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetError();
  GITS_WRAPPER_PRE
  wrapper.eglGetError(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLDisplay GLAPIENTRY eglGetDisplay(EGLNativeDisplayType arg0) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetDisplay(arg0);
  GITS_WRAPPER_PRE
  wrapper.eglGetDisplay(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglInitialize(EGLDisplay arg0, EGLint* arg1, EGLint* arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglInitialize(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.eglInitialize(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglTerminate(EGLDisplay arg0) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglTerminate(arg0);
  GITS_WRAPPER_PRE
  wrapper.eglTerminate(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI const char* GLAPIENTRY eglQueryString(EGLDisplay arg0, EGLint arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryString(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.eglQueryString(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglGetConfigs(EGLDisplay arg0,
                                          EGLConfig* arg1,
                                          EGLint arg2,
                                          EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetConfigs(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglGetConfigs(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY
eglChooseConfig(EGLDisplay arg0, const EGLint* arg1, EGLConfig* arg2, EGLint arg3, EGLint* arg4) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglChooseConfig(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.eglChooseConfig(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglGetConfigAttrib(EGLDisplay arg0,
                                               EGLConfig arg1,
                                               EGLint arg2,
                                               EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetConfigAttrib(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglGetConfigAttrib(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreateWindowSurface(EGLDisplay arg0,
                                                   EGLConfig arg1,
                                                   EGLNativeWindowType arg2,
                                                   const EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreateWindowSurface(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglCreateWindowSurface(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreatePbufferSurface(EGLDisplay arg0,
                                                    EGLConfig arg1,
                                                    const EGLint* arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreatePbufferSurface(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.eglCreatePbufferSurface(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreatePixmapSurface(EGLDisplay arg0,
                                                   EGLConfig arg1,
                                                   EGLNativePixmapType arg2,
                                                   const EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreatePixmapSurface(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglCreatePixmapSurface(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglDestroySurface(EGLDisplay arg0, EGLSurface arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDestroySurface(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.eglDestroySurface(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQuerySurface(EGLDisplay arg0,
                                            EGLSurface arg1,
                                            EGLint arg2,
                                            EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQuerySurface(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglQuerySurface(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglBindAPI(EGLenum arg0) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglBindAPI(arg0);
  GITS_WRAPPER_PRE
  wrapper.eglBindAPI(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLenum GLAPIENTRY eglQueryAPI() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryAPI();
  GITS_WRAPPER_PRE
  wrapper.eglQueryAPI(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglWaitClient() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglWaitClient();
  GITS_WRAPPER_PRE
  wrapper.eglWaitClient(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglReleaseThread() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglReleaseThread();
  GITS_WRAPPER_PRE
  wrapper.eglReleaseThread(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreatePbufferFromClientBuffer(
    EGLDisplay arg0, EGLenum arg1, EGLClientBuffer arg2, EGLConfig arg3, const EGLint* arg4) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglCreatePbufferFromClientBuffer(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.eglCreatePbufferFromClientBuffer(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglSurfaceAttrib(EGLDisplay arg0,
                                             EGLSurface arg1,
                                             EGLint arg2,
                                             EGLint arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglSurfaceAttrib(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglSurfaceAttrib(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglBindTexImage(EGLDisplay arg0, EGLSurface arg1, EGLint arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglBindTexImage(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.eglBindTexImage(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglReleaseTexImage(EGLDisplay arg0, EGLSurface arg1, EGLint arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglReleaseTexImage(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.eglReleaseTexImage(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglSwapInterval(EGLDisplay arg0, EGLint arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglSwapInterval(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.eglSwapInterval(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLContext GLAPIENTRY eglCreateContext(EGLDisplay arg0,
                                             EGLConfig arg1,
                                             EGLContext arg2,
                                             const EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreateContext(arg0, arg1, arg2, arg3);
  switch (wrapper.Drivers().egl.eglQueryAPI()) {
  case EGL_OPENGL_API:
    wrapper.GLInitialize(gits::OpenGL::CGlDriver::API_GL);
    break;
  case EGL_OPENGL_ES_API: {
    // as of EGL1.3 only version can be passed in context attribs for ES
    // look in attrib list for api version to initialize
    gits::OpenGL::CGlDriver::TApiType api = gits::OpenGL::CGlDriver::API_GLES1;
    if (arg3 && arg3[0] == EGL_CONTEXT_CLIENT_VERSION && (arg3[1] == 2 || arg3[1] == 3)) {
      api = gits::OpenGL::CGlDriver::API_GLES2;
    }
    wrapper.GLInitialize(api);
  } break;
  default:
    LOG_ERROR << "Attempted to create unsupported by GITS context type:"
              << wrapper.Drivers().egl.eglQueryAPI();
    abort();
  }
  GITS_WRAPPER_PRE
  wrapper.eglCreateContext(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglDestroyContext(EGLDisplay arg0, EGLContext arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDestroyContext(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.eglDestroyContext(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglMakeCurrent(EGLDisplay arg0,
                                           EGLSurface arg1,
                                           EGLSurface arg2,
                                           EGLContext arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglMakeCurrent(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglMakeCurrent(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLContext GLAPIENTRY eglGetCurrentContext() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetCurrentContext();
  GITS_WRAPPER_PRE
  wrapper.eglGetCurrentContext(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglGetCurrentSurface(EGLint arg0) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetCurrentSurface(arg0);
  GITS_WRAPPER_PRE
  wrapper.eglGetCurrentSurface(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLDisplay GLAPIENTRY eglGetCurrentDisplay() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetCurrentDisplay();
  GITS_WRAPPER_PRE
  wrapper.eglGetCurrentDisplay(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQueryContext(EGLDisplay arg0,
                                            EGLContext arg1,
                                            EGLint arg2,
                                            EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryContext(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglQueryContext(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglWaitGL() {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglWaitGL();
  GITS_WRAPPER_PRE
  wrapper.eglWaitGL(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglWaitNative(EGLint arg0) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglWaitNative(arg0);
  GITS_WRAPPER_PRE
  wrapper.eglWaitNative(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglSwapBuffers(EGLDisplay arg0, EGLSurface arg1) {
  GITS_ENTRY_EGL
  if (recursionDepth <= 1) {
    wrapper.PreSwap();
  }
  auto return_value = wrapper.Drivers().egl.eglSwapBuffers(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.eglSwapBuffers(return_value, arg0, arg1);
  EndFramePost();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglCopyBuffers(EGLDisplay arg0,
                                           EGLSurface arg1,
                                           EGLNativePixmapType arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCopyBuffers(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.eglCopyBuffers(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void* GLAPIENTRY eglGetProcAddress(const char* arg0) {
  GITS_ENTRY_EGL
  // wrapper.Drivers().egl.eglGetProcAddress(arg0);
  auto return_value = gits::OpenGL::get_proc_address(arg0);
  GITS_WRAPPER_PRE
  wrapper.eglGetProcAddress(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLint GLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy,
                                             EGLSyncKHR sync,
                                             EGLint flags,
                                             EGLTimeKHR timeout) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglClientWaitSyncKHR(dpy, sync, flags, timeout);
  GITS_WRAPPER_PRE
  wrapper.eglClientWaitSyncKHR(dpy, sync, flags, timeout);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLint GLAPIENTRY eglClientWaitSyncNV(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglClientWaitSyncNV(sync, flags, timeout);
  GITS_WRAPPER_PRE
  wrapper.eglClientWaitSyncNV(sync, flags, timeout);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLImageKHR GLAPIENTRY eglCreateDRMImageMESA(EGLDisplay dpy, const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreateDRMImageMESA(dpy, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglCreateDRMImageMESA(dpy, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSyncNV GLAPIENTRY eglCreateFenceSyncNV(EGLDisplay dpy,
                                                EGLenum condition,
                                                const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreateFenceSyncNV(dpy, condition, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglCreateFenceSyncNV(dpy, condition, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLImageKHR GLAPIENTRY eglCreateImageKHR(EGLDisplay dpy,
                                               EGLContext ctx,
                                               EGLenum target,
                                               EGLClientBuffer buffer,
                                               const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglCreateImageKHR(return_value, dpy, ctx, target, buffer, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreatePixmapSurfaceHI(EGLDisplay dpy,
                                                     EGLConfig config,
                                                     void* pixmap) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreatePixmapSurfaceHI(dpy, config, pixmap);
  GITS_WRAPPER_PRE
  wrapper.eglCreatePixmapSurfaceHI(dpy, config, pixmap);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLStreamKHR GLAPIENTRY
eglCreateStreamFromFileDescriptorKHR(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglCreateStreamFromFileDescriptorKHR(dpy, file_descriptor);
  GITS_WRAPPER_PRE
  wrapper.eglCreateStreamFromFileDescriptorKHR(dpy, file_descriptor);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLStreamKHR GLAPIENTRY eglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreateStreamKHR(dpy, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglCreateStreamKHR(dpy, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy,
                                                              EGLConfig config,
                                                              EGLStreamKHR stream,
                                                              const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglCreateStreamProducerSurfaceKHR(dpy, config, stream, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglCreateStreamProducerSurfaceKHR(dpy, config, stream, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSyncKHR GLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy,
                                             EGLenum type,
                                             const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglCreateSyncKHR(dpy, type, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglCreateSyncKHR(return_value, dpy, type, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDestroyImageKHR(dpy, image);
  GITS_WRAPPER_PRE
  wrapper.eglDestroyImageKHR(dpy, image);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDestroyStreamKHR(dpy, stream);
  GITS_WRAPPER_PRE
  wrapper.eglDestroyStreamKHR(dpy, stream);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDestroySyncKHR(dpy, sync);
  GITS_WRAPPER_PRE
  wrapper.eglDestroySyncKHR(dpy, sync);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglDestroySyncNV(EGLSyncNV sync) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDestroySyncNV(sync);
  GITS_WRAPPER_PRE
  wrapper.eglDestroySyncNV(sync);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglExportDRMImageMESA(
    EGLDisplay dpy, EGLImageKHR image, EGLint* name, EGLint* handle, EGLint* stride) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglExportDRMImageMESA(dpy, image, name, handle, stride);
  GITS_WRAPPER_PRE
  wrapper.eglExportDRMImageMESA(dpy, image, name, handle, stride);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglFenceNV(EGLSyncNV sync) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglFenceNV(sync);
  GITS_WRAPPER_PRE
  wrapper.eglFenceNV(sync);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLNativeFileDescriptorKHR GLAPIENTRY eglGetStreamFileDescriptorKHR(EGLDisplay dpy,
                                                                          EGLStreamKHR stream) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetStreamFileDescriptorKHR(dpy, stream);
  GITS_WRAPPER_PRE
  wrapper.eglGetStreamFileDescriptorKHR(dpy, stream);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglGetSyncAttribKHR(EGLDisplay dpy,
                                                EGLSyncKHR sync,
                                                EGLint attribute,
                                                EGLint* value) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetSyncAttribKHR(dpy, sync, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglGetSyncAttribKHR(dpy, sync, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglGetSyncAttribNV(EGLSyncNV sync, EGLint attribute, EGLint* value) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetSyncAttribNV(sync, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglGetSyncAttribNV(sync, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

// Those functions are not implemented in libGLES_intel7.so. It cause crash in
// Houdini method with Basemark X.
/*
GLAPI EGLuint64NV GLAPIENTRY eglGetSystemTimeFrequencyNV()
{
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetSystemTimeFrequencyNV();
  GITS_WRAPPER_PRE
  wrapper.eglGetSystemTimeFrequencyNV();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLuint64NV GLAPIENTRY eglGetSystemTimeNV()
{
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetSystemTimeNV();
  GITS_WRAPPER_PRE
  wrapper.eglGetSystemTimeNV();
  GITS_WRAPPER_POST
  return return_value;
}
*/
GLAPI EGLBoolean GLAPIENTRY eglLockSurfaceKHR(EGLDisplay display,
                                              EGLSurface surface,
                                              const EGLint* attrib_list) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglLockSurfaceKHR(display, surface, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.eglLockSurfaceKHR(display, surface, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglPostSubBufferNV(
    EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglPostSubBufferNV(dpy, surface, x, y, width, height);
  GITS_WRAPPER_PRE
  wrapper.eglPostSubBufferNV(dpy, surface, x, y, width, height);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQueryStreamKHR(EGLDisplay dpy,
                                              EGLStreamKHR stream,
                                              EGLenum attribute,
                                              EGLint* value) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryStreamKHR(dpy, stream, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglQueryStreamKHR(dpy, stream, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQueryStreamTimeKHR(EGLDisplay dpy,
                                                  EGLStreamKHR stream,
                                                  EGLenum attribute,
                                                  EGLTimeKHR* value) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryStreamTimeKHR(dpy, stream, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglQueryStreamTimeKHR(dpy, stream, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQueryStreamu64KHR(EGLDisplay dpy,
                                                 EGLStreamKHR stream,
                                                 EGLenum attribute,
                                                 EGLuint64KHR* value) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryStreamu64KHR(dpy, stream, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglQueryStreamu64KHR(dpy, stream, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQuerySurfacePointerANGLE(EGLDisplay dpy,
                                                        EGLSurface surface,
                                                        EGLint attribute,
                                                        void** value) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglQuerySurfacePointerANGLE(dpy, surface, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglQuerySurfacePointerANGLE(dpy, surface, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglSignalSyncKHR(dpy, sync, mode);
  GITS_WRAPPER_PRE
  wrapper.eglSignalSyncKHR(dpy, sync, mode);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglSignalSyncNV(EGLSyncNV sync, EGLenum mode) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglSignalSyncNV(sync, mode);
  GITS_WRAPPER_PRE
  wrapper.eglSignalSyncNV(sync, mode);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglStreamAttribKHR(EGLDisplay dpy,
                                               EGLStreamKHR stream,
                                               EGLenum attribute,
                                               EGLint value) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglStreamAttribKHR(dpy, stream, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.eglStreamAttribKHR(dpy, stream, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglStreamConsumerAcquireKHR(dpy, stream);
  GITS_WRAPPER_PRE
  wrapper.eglStreamConsumerAcquireKHR(dpy, stream);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy,
                                                                  EGLStreamKHR stream) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglStreamConsumerGLTextureExternalKHR(dpy, stream);
  GITS_WRAPPER_PRE
  wrapper.eglStreamConsumerGLTextureExternalKHR(dpy, stream);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglStreamConsumerReleaseKHR(dpy, stream);
  GITS_WRAPPER_PRE
  wrapper.eglStreamConsumerReleaseKHR(dpy, stream);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglUnlockSurfaceKHR(display, surface);
  GITS_WRAPPER_PRE
  wrapper.eglUnlockSurfaceKHR(display, surface);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLint GLAPIENTRY eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglWaitSyncKHR(dpy, sync, flags);
  GITS_WRAPPER_PRE
  wrapper.eglWaitSyncKHR(dpy, sync, flags);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglSetSwapRectangleANDROID(
    EGLDisplay arg0, EGLSurface arg1, EGLint arg2, EGLint arg3, EGLint arg4, EGLint arg5) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglSetSwapRectangleANDROID(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(
      wrapper.eglSetSwapRectangleANDROID(arg0, arg1, arg2, arg3, arg4, arg5);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLClientBuffer GLAPIENTRY eglGetRenderBufferANDROID(EGLDisplay arg0, EGLSurface arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetRenderBufferANDROID(arg0, arg1);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglGetRenderBufferANDROID(arg0, arg1);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLint GLAPIENTRY eglDupNativeFenceFDANDROID(EGLDisplay arg0, EGLSyncKHR arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglDupNativeFenceFDANDROID(arg0, arg1);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglDupNativeFenceFDANDROID(arg0, arg1);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLint GLAPIENTRY eglWaitSyncANDROID(EGLDisplay arg0, EGLSyncKHR arg1, EGLint arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglWaitSyncANDROID(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglWaitSyncANDROID(arg0, arg1, arg2);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLDisplay GLAPIENTRY eglGetPlatformDisplayEXT(EGLenum arg0, void* arg1, const EGLint* arg2) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglGetPlatformDisplayEXT(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.eglGetPlatformDisplayEXT(return_value, arg0, static_cast<Display*>(arg1), arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLSurface GLAPIENTRY eglCreatePlatformWindowSurfaceEXT(EGLDisplay arg0,
                                                              EGLConfig arg1,
                                                              void* arg2,
                                                              const EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value =
      wrapper.Drivers().egl.eglCreatePlatformWindowSurfaceEXT(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.eglCreatePlatformWindowSurfaceEXT(return_value, arg0, arg1,
                                            static_cast<EGLNativeWindowType>(arg2), arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglBindWaylandDisplayWL(EGLDisplay arg0, struct wl_display* arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglBindWaylandDisplayWL(arg0, arg1);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglBindWaylandDisplayWL(arg0, arg1);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglUnbindWaylandDisplayWL(EGLDisplay arg0, struct wl_display* arg1) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglUnbindWaylandDisplayWL(arg0, arg1);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglUnbindWaylandDisplayWL(arg0, arg1);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI EGLBoolean GLAPIENTRY eglQueryWaylandBufferWL(EGLDisplay arg0,
                                                    struct wl_buffer* arg1,
                                                    EGLint arg2,
                                                    EGLint* arg3) {
  GITS_ENTRY_EGL
  auto return_value = wrapper.Drivers().egl.eglQueryWaylandBufferWL(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglQueryWaylandBufferWL(arg0, arg1);)
  GITS_WRAPPER_POST
  return return_value;
}

// This function causes crash during recording using app_process
// instrumentation, when eglInitialize is called after eglTerminate (WA for
// GFXBench 4.0 Car Chase subcapturing).
/*
GLAPI void GLAPIENTRY eglSetBlobCacheFuncsANDROID(EGLDisplay dpy,
EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get)
{
  GITS_ENTRY_EGL
  wrapper.Drivers().egl.eglSetBlobCacheFuncsANDROID(dpy, set, get);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.eglSetBlobCacheFuncsANDROID(dpy, set,
get)); GITS_WRAPPER_POST
}*/

#if defined GITS_PLATFORM_WINDOWS
GLAPI BOOL GLAPIENTRY wglCopyContext(HGLRC arg0, HGLRC arg1, UINT arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCopyContext(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglCopyContext(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglCreateContext(HDC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateContext(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglCreateContext(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglCreateLayerContext(HDC arg0, int arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateLayerContext(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglCreateLayerContext(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDeleteContext(HGLRC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDeleteContext(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDeleteContext(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglGetCurrentContext() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetCurrentContext();
  GITS_WRAPPER_PRE
  wrapper.wglGetCurrentContext(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HDC GLAPIENTRY wglGetCurrentDC() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetCurrentDC();
  GITS_WRAPPER_PRE
  wrapper.wglGetCurrentDC(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void* GLAPIENTRY wglGetDefaultProcAddress(const char* arg0) {
  GITS_ENTRY_WGL
  auto return_value = gits::OpenGL::get_proc_address(arg0);
  auto return_value_drv = wrapper.Drivers().wgl.wglGetDefaultProcAddress(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetDefaultProcAddress(arg0);
  GITS_WRAPPER_POST
  if (return_value_drv == NULL) {
    return NULL;
  }
  return return_value;
}

GLAPI int GLAPIENTRY wglGetPixelFormat(HDC arg0) {
  gits::OpenGL::CGitsPlugin::Initialize();
#ifdef GITS_PLATFORM_WINDOWS
  if (gits::OpenGL::CGitsPlugin::Configuration().opengl.recorder.mtDriverWA) {
    // wglDescribePixelFormat, like wglGetPixelFormat, is used in threaded
    // drivers off the 'main' thread when wglMakeCurrent is called, which causes
    // deadlock due to GITS api mutex. Because the apis are fairly
    // insignificant, we do not record them at all, and instead always forward
    // to the driver. We do not synchronize on the either, which introduces two
    // races. One in CGitsPlugin::Initialize(), and one in first call of the
    // function in wrapper.Drivers().wgl.wglFunction.

    gits::OpenGL::IRecorderWrapper& wrapper = gits::OpenGL::CGitsPlugin::RecorderWrapper();
    return wrapper.Drivers().wgl.wglGetPixelFormat(arg0);
  }
#endif
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetPixelFormat(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetPixelFormat(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void* GLAPIENTRY wglGetProcAddress(const char* arg0) {
  GITS_ENTRY_WGL
  auto return_value = gits::OpenGL::get_proc_address(arg0);
  auto return_value_drv = wrapper.Drivers().wgl.wglGetProcAddress(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetProcAddress(arg0);
  GITS_WRAPPER_POST
  if (return_value_drv == NULL) {
    return NULL;
  } else if (return_value == NULL) {
    return return_value_drv;
  }
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglMakeCurrent(HDC arg0, HGLRC arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglMakeCurrent(arg0, arg1);
  if (arg0 && arg1) {
    wrapper.GLInitialize(gits::OpenGL::CGlDriver::API_GL);
  }
  GITS_WRAPPER_PRE
  wrapper.wglMakeCurrent(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglRealizeLayerPalette(HDC arg0, int arg1, BOOL arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglRealizeLayerPalette(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.wglRealizeLayerPalette(arg0, arg1, arg2);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglShareLists(HGLRC arg0, HGLRC arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglShareLists(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglShareLists(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSwapBuffers(HDC arg0) {
  GITS_ENTRY_WGL
  if (recursionDepth <= 1) {
    wrapper.PreSwap();
  }
  auto return_value = wrapper.Drivers().wgl.wglSwapBuffers(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglSwapBuffers(return_value, arg0);
  EndFramePost();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSwapMultipleBuffers(UINT arg0, const WGLSWAP* arg1) {
  GITS_ENTRY_WGL
  if (recursionDepth <= 1) {
    wrapper.PreSwap();
  }
  auto return_value = wrapper.Drivers().wgl.wglSwapMultipleBuffers(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglSwapMultipleBuffers(return_value, arg0, (HDC*)arg1);
  EndFramePost();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSwapLayerBuffers(HDC arg0, UINT arg1) {
  GITS_ENTRY_WGL
  if (recursionDepth <= 1) {
    wrapper.PreSwap();
  }
  auto return_value = wrapper.Drivers().wgl.wglSwapLayerBuffers(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglSwapLayerBuffers(return_value, arg0, arg1);
  EndFramePost();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglUseFontBitmapsA(HDC arg0, DWORD arg1, DWORD arg2, DWORD arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglUseFontBitmapsA(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglUseFontBitmapsA(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglUseFontBitmapsW(HDC arg0, DWORD arg1, DWORD arg2, DWORD arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglUseFontBitmapsW(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglUseFontBitmapsW(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY wglChoosePixelFormat(HDC arg0, const PIXELFORMATDESCRIPTOR* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglChoosePixelFormat(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglChoosePixelFormat(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglDescribeLayerPlane(HDC arg0, int arg1, int arg2, UINT arg3, LPLAYERPLANEDESCRIPTOR arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDescribeLayerPlane(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.wglDescribeLayerPlane(arg0, arg1, arg2, arg3, arg4);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY wglDescribePixelFormat(HDC arg0,
                                            int arg1,
                                            UINT arg2,
                                            PIXELFORMATDESCRIPTOR* arg3) {
  gits::OpenGL::CGitsPlugin::Initialize();
#ifdef GITS_PLATFORM_WINDOWS
  if (gits::OpenGL::CGitsPlugin::Configuration().opengl.recorder.mtDriverWA) {
    // wglDescribePixelFormat, like wglGetPixelFormat, is used in threaded
    // drivers off the 'main' thread when wglMakeCurrent is called, which causes
    // deadlock due to GITS api mutex. Because the apis are fairly
    // insignificant, we do not record them at all, and instead always forward
    // to the driver. We do not synchronize on the either, which introduces two
    // races. One in CGitsPlugin::Initialize(), and one in first call of the
    // function in wrapper.Drivers().wgl.wglFunction.
    gits::OpenGL::IRecorderWrapper& wrapper = gits::OpenGL::CGitsPlugin::RecorderWrapper();
    return wrapper.Drivers().wgl.wglDescribePixelFormat(arg0, arg1, arg2, arg3);
  }
#endif
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDescribePixelFormat(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.wglDescribePixelFormat(return_value, arg0, arg1, arg2, arg3);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY
wglGetLayerPaletteEntries(HDC arg0, int arg1, int arg2, int arg3, COLORREF* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetLayerPaletteEntries(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglGetLayerPaletteEntries(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY
wglSetLayerPaletteEntries(HDC arg0, int arg1, int arg2, int arg3, const COLORREF* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetLayerPaletteEntries(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.wglSetLayerPaletteEntries(arg0, arg1, arg2, arg3, arg4);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSetPixelFormat(HDC arg0, int arg1, const PIXELFORMATDESCRIPTOR* arg2) {
  gits::OpenGL::CGitsPlugin::Initialize();
#ifdef GITS_PLATFORM_WINDOWS
  if (gits::OpenGL::CGitsPlugin::Configuration().opengl.recorder.mtDriverWA) {
    bool deadlock = false;
    Timer deadlock_timer;
    deadlock_timer.Start();
    while (globalMutex.try_lock() == false) {
      if (deadlock_timer.Get() / 1e6 >= 1000) {
        deadlock = true;
        break;
      }
    }
    if (!deadlock) {
      globalMutex.unlock();
    }

    if (deadlock) {
      LOG_WARNING << "Deadlock detected in " << __FUNCTION__
                  << ", function will not be processed by GITS.";
      gits::OpenGL::IRecorderWrapper& wrapper = gits::OpenGL::CGitsPlugin::RecorderWrapper();
      return wrapper.Drivers().wgl.wglSetPixelFormat(arg0, arg1, arg2);
    }
  }
#endif
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetPixelFormat(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglSetPixelFormat(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglUseFontOutlinesA(HDC arg0,
                                          UINT arg1,
                                          UINT arg2,
                                          UINT arg3,
                                          FLOAT arg4,
                                          FLOAT arg5,
                                          int arg6,
                                          LPGLYPHMETRICSFLOAT arg7) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglUseFontOutlinesA(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  GITS_WRAPPER_PRE
  wrapper.wglUseFontOutlinesA(return_value, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglUseFontOutlinesW(HDC arg0,
                                          UINT arg1,
                                          UINT arg2,
                                          UINT arg3,
                                          FLOAT arg4,
                                          FLOAT arg5,
                                          int arg6,
                                          LPGLYPHMETRICSFLOAT arg7) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglUseFontOutlinesW(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  GITS_WRAPPER_PRE
  wrapper.wglUseFontOutlinesW(return_value, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HANDLE GLAPIENTRY wglCreateBufferRegionARB(HDC arg0, int arg1, UINT arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateBufferRegionARB(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglCreateBufferRegionARB(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY wglDeleteBufferRegionARB(HANDLE arg0) {
  GITS_ENTRY_WGL
  wrapper.Drivers().wgl.wglDeleteBufferRegionARB(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDeleteBufferRegionARB(arg0);
  GITS_WRAPPER_POST
}

GLAPI BOOL GLAPIENTRY wglSaveBufferRegionARB(HANDLE arg0, int arg1, int arg2, int arg3, int arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSaveBufferRegionARB(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglSaveBufferRegionARB(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglRestoreBufferRegionARB(HANDLE arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglRestoreBufferRegionARB(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
  GITS_WRAPPER_PRE
  wrapper.wglRestoreBufferRegionARB(return_value, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI const char* GLAPIENTRY wglGetExtensionsStringARB(HDC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetExtensionsStringARB(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetExtensionsStringARB(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribivARB(HDC arg0, int arg1, int arg2, UINT arg3, const int* arg4, int* arg5) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglGetPixelFormatAttribivARB(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  wrapper.wglGetPixelFormatAttribivARB(return_value, arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetPixelFormatAttribfvARB(
    HDC arg0, int arg1, int arg2, UINT arg3, const int* arg4, FLOAT* arg5) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglGetPixelFormatAttribfvARB(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  wrapper.wglGetPixelFormatAttribfvARB(return_value, arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglChoosePixelFormatARB(
    HDC arg0, const int* arg1, const FLOAT* arg2, UINT arg3, int* arg4, UINT* arg5) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglChoosePixelFormatARB(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  wrapper.wglChoosePixelFormatARB(return_value, arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglMakeContextCurrentARB(HDC arg0, HDC arg1, HGLRC arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglMakeContextCurrentARB(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglMakeContextCurrentARB(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HDC GLAPIENTRY wglGetCurrentReadDCARB() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetCurrentReadDCARB();
  GITS_WRAPPER_PRE
  wrapper.wglGetCurrentReadDCARB(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HPBUFFERARB GLAPIENTRY
wglCreatePbufferARB(HDC arg0, int arg1, int arg2, int arg3, const int* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreatePbufferARB(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglCreatePbufferARB(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HDC GLAPIENTRY wglGetPbufferDCARB(HPBUFFERARB arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetPbufferDCARB(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetPbufferDCARB(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY wglReleasePbufferDCARB(HPBUFFERARB arg0, HDC arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleasePbufferDCARB(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglReleasePbufferDCARB(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDestroyPbufferARB(HPBUFFERARB arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDestroyPbufferARB(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDestroyPbufferARB(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryPbufferARB(HPBUFFERARB arg0, int arg1, int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryPbufferARB(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglQueryPbufferARB(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglBindTexImageARB(HPBUFFERARB arg0, int arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBindTexImageARB(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglBindTexImageARB(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglReleaseTexImageARB(HPBUFFERARB arg0, int arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleaseTexImageARB(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglReleaseTexImageARB(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSetPbufferAttribARB(HPBUFFERARB arg0, const int* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetPbufferAttribARB(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglSetPbufferAttribARB(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglCreateContextAttribsARB(HDC arg0, HGLRC arg1, const int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateContextAttribsARB(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglCreateContextAttribsARB(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLboolean GLAPIENTRY wglCreateDisplayColorTableEXT(GLushort arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateDisplayColorTableEXT(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglCreateDisplayColorTableEXT(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLboolean GLAPIENTRY wglLoadDisplayColorTableEXT(const GLushort* arg0, GLuint arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglLoadDisplayColorTableEXT(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglLoadDisplayColorTableEXT(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLboolean GLAPIENTRY wglBindDisplayColorTableEXT(GLushort arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBindDisplayColorTableEXT(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglBindDisplayColorTableEXT(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY wglDestroyDisplayColorTableEXT(GLushort arg0) {
  GITS_ENTRY_WGL
  wrapper.Drivers().wgl.wglDestroyDisplayColorTableEXT(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDestroyDisplayColorTableEXT(arg0);
  GITS_WRAPPER_POST
}

GLAPI const char* GLAPIENTRY wglGetExtensionsStringEXT() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetExtensionsStringEXT();
  GITS_WRAPPER_PRE
  wrapper.wglGetExtensionsStringEXT(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglMakeContextCurrentEXT(HDC arg0, HDC arg1, HGLRC arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglMakeContextCurrentEXT(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglMakeContextCurrentEXT(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HDC GLAPIENTRY wglGetCurrentReadDCEXT() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetCurrentReadDCEXT();
  GITS_WRAPPER_PRE
  wrapper.wglGetCurrentReadDCEXT(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HPBUFFEREXT GLAPIENTRY
wglCreatePbufferEXT(HDC arg0, int arg1, int arg2, int arg3, const int* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreatePbufferEXT(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglCreatePbufferEXT(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HDC GLAPIENTRY wglGetPbufferDCEXT(HPBUFFEREXT arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetPbufferDCEXT(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetPbufferDCEXT(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY wglReleasePbufferDCEXT(HPBUFFEREXT arg0, HDC arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleasePbufferDCEXT(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglReleasePbufferDCEXT(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDestroyPbufferEXT(HPBUFFEREXT arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDestroyPbufferEXT(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDestroyPbufferEXT(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryPbufferEXT(HPBUFFEREXT arg0, int arg1, int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryPbufferEXT(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglQueryPbufferEXT(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribivEXT(HDC arg0, int arg1, int arg2, UINT arg3, int* arg4, int* arg5) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglGetPixelFormatAttribivEXT(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  wrapper.wglGetPixelFormatAttribivEXT(return_value, arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribfvEXT(HDC arg0, int arg1, int arg2, UINT arg3, int* arg4, FLOAT* arg5) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglGetPixelFormatAttribfvEXT(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  wrapper.wglGetPixelFormatAttribfvEXT(return_value, arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglChoosePixelFormatEXT(
    HDC arg0, const int* arg1, const FLOAT* arg2, UINT arg3, int* arg4, UINT* arg5) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglChoosePixelFormatEXT(arg0, arg1, arg2, arg3, arg4, arg5);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(
      wrapper.wglChoosePixelFormatEXT(return_value, arg0, arg1, arg2, arg3, arg4, arg5);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSwapIntervalEXT(int arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSwapIntervalEXT(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglSwapIntervalEXT(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY wglGetSwapIntervalEXT() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetSwapIntervalEXT();
  GITS_WRAPPER_PRE
  wrapper.wglGetSwapIntervalEXT(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void* GLAPIENTRY wglAllocateMemoryNV(GLsizei arg0, GLfloat arg1, GLfloat arg2, GLfloat arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglAllocateMemoryNV(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglAllocateMemoryNV(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY wglFreeMemoryNV(void* arg0) {
  GITS_ENTRY_WGL
  wrapper.Drivers().wgl.wglFreeMemoryNV(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglFreeMemoryNV(arg0);
  GITS_WRAPPER_POST
}

GLAPI BOOL GLAPIENTRY wglGetSyncValuesOML(HDC arg0, INT64* arg1, INT64* arg2, INT64* arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetSyncValuesOML(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglGetSyncValuesOML(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetMscRateOML(HDC arg0, INT32* arg1, INT32* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetMscRateOML(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglGetMscRateOML(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI INT64 GLAPIENTRY wglSwapBuffersMscOML(HDC arg0, INT64 arg1, INT64 arg2, INT64 arg3) {
  GITS_ENTRY_WGL
  if (recursionDepth <= 1) {
    wrapper.PreSwap();
  }
  auto return_value = wrapper.Drivers().wgl.wglSwapBuffersMscOML(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglSwapBuffersMscOML(return_value, arg0, arg1, arg2, arg3);
  EndFramePost();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI INT64 GLAPIENTRY
wglSwapLayerBuffersMscOML(HDC arg0, int arg1, INT64 arg2, INT64 arg3, INT64 arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSwapLayerBuffersMscOML(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglSwapLayerBuffersMscOML(return_value, arg0, arg1, arg2, arg3, arg4);
  EndFramePost();
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglWaitForMscOML(
    HDC arg0, INT64 arg1, INT64 arg2, INT64 arg3, INT64* arg4, INT64* arg5, INT64* arg6) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglWaitForMscOML(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
  GITS_WRAPPER_PRE
  wrapper.wglWaitForMscOML(return_value, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglWaitForSbcOML(HDC arg0, INT64 arg1, INT64* arg2, INT64* arg3, INT64* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglWaitForSbcOML(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglWaitForSbcOML(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetDigitalVideoParametersI3D(HDC arg0, int arg1, int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetDigitalVideoParametersI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglGetDigitalVideoParametersI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSetDigitalVideoParametersI3D(HDC arg0, int arg1, const int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetDigitalVideoParametersI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglSetDigitalVideoParametersI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetGammaTableParametersI3D(HDC arg0, int arg1, int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGammaTableParametersI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglGetGammaTableParametersI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSetGammaTableParametersI3D(HDC arg0, int arg1, const int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetGammaTableParametersI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglSetGammaTableParametersI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY
wglGetGammaTableI3D(HDC arg0, int arg1, USHORT* arg2, USHORT* arg3, USHORT* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGammaTableI3D(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglGetGammaTableI3D(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSetGammaTableI3D(
    HDC arg0, int arg1, const USHORT* arg2, const USHORT* arg3, const USHORT* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetGammaTableI3D(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglSetGammaTableI3D(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglEnableGenlockI3D(HDC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnableGenlockI3D(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglEnableGenlockI3D(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDisableGenlockI3D(HDC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDisableGenlockI3D(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDisableGenlockI3D(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglIsEnabledGenlockI3D(HDC arg0, BOOL* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglIsEnabledGenlockI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglIsEnabledGenlockI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGenlockSourceI3D(HDC arg0, UINT arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGenlockSourceI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGenlockSourceI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetGenlockSourceI3D(HDC arg0, UINT* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGenlockSourceI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGetGenlockSourceI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGenlockSourceEdgeI3D(HDC arg0, UINT arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGenlockSourceEdgeI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGenlockSourceEdgeI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetGenlockSourceEdgeI3D(HDC arg0, UINT* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGenlockSourceEdgeI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGetGenlockSourceEdgeI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGenlockSampleRateI3D(HDC arg0, UINT arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGenlockSampleRateI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGenlockSampleRateI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetGenlockSampleRateI3D(HDC arg0, UINT* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGenlockSampleRateI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGetGenlockSampleRateI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGenlockSourceDelayI3D(HDC arg0, UINT arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGenlockSourceDelayI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGenlockSourceDelayI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetGenlockSourceDelayI3D(HDC arg0, UINT* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGenlockSourceDelayI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGetGenlockSourceDelayI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryGenlockMaxSourceDelayI3D(HDC arg0, UINT* arg1, UINT* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryGenlockMaxSourceDelayI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglQueryGenlockMaxSourceDelayI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI LPVOID GLAPIENTRY wglCreateImageBufferI3D(HDC arg0, DWORD arg1, UINT arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateImageBufferI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglCreateImageBufferI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDestroyImageBufferI3D(HDC arg0, LPVOID arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDestroyImageBufferI3D(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglDestroyImageBufferI3D(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglAssociateImageBufferEventsI3D(
    HDC arg0, const HANDLE* arg1, const LPVOID* arg2, const DWORD* arg3, UINT arg4) {
  GITS_ENTRY_WGL
  auto return_value =
      wrapper.Drivers().wgl.wglAssociateImageBufferEventsI3D(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglAssociateImageBufferEventsI3D(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglReleaseImageBufferEventsI3D(HDC arg0, const LPVOID* arg1, UINT arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleaseImageBufferEventsI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglReleaseImageBufferEventsI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglEnableFrameLockI3D() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnableFrameLockI3D();
  GITS_WRAPPER_PRE
  wrapper.wglEnableFrameLockI3D(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDisableFrameLockI3D() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDisableFrameLockI3D();
  GITS_WRAPPER_PRE
  wrapper.wglDisableFrameLockI3D(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglIsEnabledFrameLockI3D(BOOL* arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglIsEnabledFrameLockI3D(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglIsEnabledFrameLockI3D(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryFrameLockMasterI3D(BOOL* arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryFrameLockMasterI3D(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglQueryFrameLockMasterI3D(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetFrameUsageI3D(float* arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetFrameUsageI3D(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetFrameUsageI3D(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglBeginFrameTrackingI3D() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBeginFrameTrackingI3D();
  GITS_WRAPPER_PRE
  wrapper.wglBeginFrameTrackingI3D(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglEndFrameTrackingI3D() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEndFrameTrackingI3D();
  GITS_WRAPPER_PRE
  wrapper.wglEndFrameTrackingI3D(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryFrameTrackingI3D(DWORD* arg0, DWORD* arg1, float* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryFrameTrackingI3D(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglQueryFrameTrackingI3D(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSetStereoEmitterState3DL(HDC arg0, UINT arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSetStereoEmitterState3DL(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglSetStereoEmitterState3DL(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY wglEnumerateVideoDevicesNV(HDC arg0, HVIDEOOUTPUTDEVICENV* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnumerateVideoDevicesNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglEnumerateVideoDevicesNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglBindVideoDeviceNV(HDC arg0,
                                           unsigned arg1,
                                           HVIDEOOUTPUTDEVICENV arg2,
                                           const int* arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBindVideoDeviceNV(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglBindVideoDeviceNV(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryCurrentContextNV(int arg0, int* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryCurrentContextNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglQueryCurrentContextNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetVideoDeviceNV(HDC arg0, int arg1, HPVIDEODEV* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetVideoDeviceNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglGetVideoDeviceNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglReleaseVideoDeviceNV(HPVIDEODEV arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleaseVideoDeviceNV(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglReleaseVideoDeviceNV(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglBindVideoImageNV(HPVIDEODEV arg0, HPBUFFERARB arg1, int arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBindVideoImageNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglBindVideoImageNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglReleaseVideoImageNV(HPBUFFERARB arg0, int arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleaseVideoImageNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglReleaseVideoImageNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglSendPbufferToVideoNV(HPBUFFERARB arg0,
                                              int arg1,
                                              unsigned long* arg2,
                                              BOOL arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglSendPbufferToVideoNV(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglSendPbufferToVideoNV(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglGetVideoInfoNV(HPVIDEODEV arg0, unsigned long* arg1, unsigned long* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetVideoInfoNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglGetVideoInfoNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglJoinSwapGroupNV(HDC arg0, GLuint arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglJoinSwapGroupNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglJoinSwapGroupNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglBindSwapBarrierNV(GLuint arg0, GLuint arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBindSwapBarrierNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglBindSwapBarrierNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQuerySwapGroupNV(HDC arg0, GLuint* arg1, GLuint* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQuerySwapGroupNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglQuerySwapGroupNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryMaxSwapGroupsNV(HDC arg0, GLuint* arg1, GLuint* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryMaxSwapGroupsNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglQueryMaxSwapGroupsNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryFrameCountNV(HDC arg0, GLuint* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryFrameCountNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglQueryFrameCountNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglResetFrameCountNV(HDC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglResetFrameCountNV(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglResetFrameCountNV(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglEnumGpusNV(UINT arg0, HGPUNV* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnumGpusNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglEnumGpusNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglEnumGpuDevicesNV(HGPUNV arg0, UINT arg1, PGPU_DEVICE arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnumGpuDevicesNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglEnumGpuDevicesNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HDC GLAPIENTRY wglCreateAffinityDCNV(const HGPUNV* arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateAffinityDCNV(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglCreateAffinityDCNV(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglEnumGpusFromAffinityDCNV(HDC arg0, UINT arg1, HGPUNV* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnumGpusFromAffinityDCNV(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglEnumGpusFromAffinityDCNV(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDeleteDCNV(HDC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDeleteDCNV(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDeleteDCNV(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI UINT GLAPIENTRY wglGetGPUIDsAMD(UINT arg0, UINT* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGPUIDsAMD(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglGetGPUIDsAMD(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI INT GLAPIENTRY wglGetGPUInfoAMD(UINT arg0, int arg1, GLenum arg2, UINT arg3, void* arg4) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetGPUInfoAMD(arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_PRE
  wrapper.wglGetGPUInfoAMD(return_value, arg0, arg1, arg2, arg3, arg4);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI UINT GLAPIENTRY wglGetContextGPUIDAMD(HGLRC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetContextGPUIDAMD(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglGetContextGPUIDAMD(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglCreateAssociatedContextAMD(UINT arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateAssociatedContextAMD(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglCreateAssociatedContextAMD(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglCreateAssociatedContextAttribsAMD(UINT arg0,
                                                            HGLRC arg1,
                                                            const int* arg2) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCreateAssociatedContextAttribsAMD(arg0, arg1, arg2);
  GITS_WRAPPER_PRE
  wrapper.wglCreateAssociatedContextAttribsAMD(return_value, arg0, arg1, arg2);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglDeleteAssociatedContextAMD(HGLRC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglDeleteAssociatedContextAMD(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglDeleteAssociatedContextAMD(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglMakeAssociatedContextCurrentAMD(HGLRC arg0) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglMakeAssociatedContextCurrentAMD(arg0);
  GITS_WRAPPER_PRE
  wrapper.wglMakeAssociatedContextCurrentAMD(return_value, arg0);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI HGLRC GLAPIENTRY wglGetCurrentAssociatedContextAMD() {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglGetCurrentAssociatedContextAMD();
  GITS_WRAPPER_PRE
  wrapper.wglGetCurrentAssociatedContextAMD(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY wglBlitContextFramebufferAMD(HGLRC arg0,
                                                   GLint arg1,
                                                   GLint arg2,
                                                   GLint arg3,
                                                   GLint arg4,
                                                   GLint arg5,
                                                   GLint arg6,
                                                   GLint arg7,
                                                   GLint arg8,
                                                   GLbitfield arg9,
                                                   GLenum arg10) {
  GITS_ENTRY_WGL
  wrapper.Drivers().wgl.wglBlitContextFramebufferAMD(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
                                                     arg8, arg9, arg10);
  GITS_WRAPPER_PRE
  wrapper.wglBlitContextFramebufferAMD(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,
                                       arg10);
  GITS_WRAPPER_POST
}

GLAPI BOOL GLAPIENTRY wglBindVideoCaptureDeviceNV(UINT arg0, HVIDEOINPUTDEVICENV arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglBindVideoCaptureDeviceNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglBindVideoCaptureDeviceNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI UINT GLAPIENTRY wglEnumerateVideoCaptureDevicesNV(HDC arg0, HVIDEOINPUTDEVICENV* arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglEnumerateVideoCaptureDevicesNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglEnumerateVideoCaptureDevicesNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglLockVideoCaptureDeviceNV(HDC arg0, HVIDEOINPUTDEVICENV arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglLockVideoCaptureDeviceNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglLockVideoCaptureDeviceNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglQueryVideoCaptureDeviceNV(HDC arg0,
                                                   HVIDEOINPUTDEVICENV arg1,
                                                   int arg2,
                                                   int* arg3) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglQueryVideoCaptureDeviceNV(arg0, arg1, arg2, arg3);
  GITS_WRAPPER_PRE
  wrapper.wglQueryVideoCaptureDeviceNV(return_value, arg0, arg1, arg2, arg3);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglReleaseVideoCaptureDeviceNV(HDC arg0, HVIDEOINPUTDEVICENV arg1) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglReleaseVideoCaptureDeviceNV(arg0, arg1);
  GITS_WRAPPER_PRE
  wrapper.wglReleaseVideoCaptureDeviceNV(return_value, arg0, arg1);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI BOOL GLAPIENTRY wglCopyImageSubDataNV(HGLRC arg0,
                                            GLuint arg1,
                                            GLenum arg2,
                                            GLint arg3,
                                            GLint arg4,
                                            GLint arg5,
                                            GLint arg6,
                                            HGLRC arg7,
                                            GLuint arg8,
                                            GLenum arg9,
                                            GLint arg10,
                                            GLint arg11,
                                            GLint arg12,
                                            GLint arg13,
                                            GLsizei arg14,
                                            GLsizei arg15,
                                            GLsizei arg16) {
  GITS_ENTRY_WGL
  auto return_value = wrapper.Drivers().wgl.wglCopyImageSubDataNV(
      arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14,
      arg15, arg16);
  GITS_WRAPPER_PRE
  wrapper.wglCopyImageSubDataNV(return_value, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8,
                                arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16);
  GITS_WRAPPER_POST
  return return_value;
}

#elif defined GITS_PLATFORM_X11

GLAPI XVisualInfo* GLAPIENTRY glXChooseVisual(Display* dpy, int screen, int* attribList) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXChooseVisual(dpy, screen, attribList);
  GITS_WRAPPER_PRE
  wrapper.glXChooseVisual(return_value, dpy, screen, attribList);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXContext GLAPIENTRY glXCreateContext(Display* dpy,
                                             XVisualInfo* vis,
                                             GLXContext shareList,
                                             Bool direct) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXCreateContext(dpy, vis, shareList, direct);
  GITS_WRAPPER_PRE
  wrapper.glXCreateContext(return_value, dpy, vis, shareList, direct);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXDestroyContext(Display* dpy, GLXContext ctx) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXDestroyContext(dpy, ctx);
  GITS_WRAPPER_PRE
  wrapper.glXDestroyContext(dpy, ctx);
  GITS_WRAPPER_POST
}

GLAPI Bool GLAPIENTRY glXMakeCurrent(Display* dpy, GLXDrawable drawable, GLXContext ctx) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXMakeCurrent(dpy, drawable, ctx);
  GITS_WRAPPER_PRE
  wrapper.glXMakeCurrent(return_value, dpy, drawable, ctx);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXCopyContext(Display* dpy,
                                     GLXContext src,
                                     GLXContext dst,
                                     unsigned long mask) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXCopyContext(dpy, src, dst, mask);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXCopyContext(dpy, src, dst, mask);)
  GITS_WRAPPER_POST
}

GLAPI void GLAPIENTRY glXSwapBuffers(Display* dpy, GLXDrawable drawable) {
  GITS_ENTRY_GLX
  if (recursionDepth <= 1) {
    wrapper.PreSwap();
  }
  wrapper.Drivers().glx.glXSwapBuffers(dpy, drawable);
  GITS_WRAPPER_PRE
  wrapper.glXSwapBuffers(dpy, drawable);
  GITS_WRAPPER_POST
}

GLAPI GLXPixmap GLAPIENTRY glXCreateGLXPixmap(Display* dpy, XVisualInfo* visual, Pixmap pixmap) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXCreateGLXPixmap(dpy, visual, pixmap);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXCreateGLXPixmap(return_value, dpy, visual, pixmap);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXDestroyGLXPixmap(Display* dpy, GLXPixmap pixmap) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXDestroyGLXPixmap(dpy, pixmap);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXDestroyGLXPixmap(dpy, pixmap);)
  GITS_WRAPPER_POST
}

GLAPI Bool GLAPIENTRY glXQueryExtension(Display* dpy, int* errorb, int* event) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXQueryExtension(dpy, errorb, event);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXQueryExtension(return_value, dpy, errorb, event);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI Bool GLAPIENTRY glXQueryVersion(Display* dpy, int* maj, int* min) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXQueryVersion(dpy, maj, min);
  GITS_WRAPPER_PRE
  wrapper.glXQueryVersion(return_value, dpy, maj, min);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI Bool GLAPIENTRY glXIsDirect(Display* dpy, GLXContext ctx) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXIsDirect(dpy, ctx);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXIsDirect(return_value, dpy, ctx);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXGetConfig(Display* dpy, XVisualInfo* visual, int attrib, int* value) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetConfig(dpy, visual, attrib, value);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXGetConfig(return_value, dpy, visual, attrib, value);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXContext GLAPIENTRY glXGetCurrentContext() {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetCurrentContext();
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXGetCurrentContext(return_value);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXDrawable GLAPIENTRY glXGetCurrentDrawable() {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetCurrentDrawable();
  GITS_WRAPPER_PRE
  wrapper.glXGetCurrentDrawable(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXWaitGL() {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXWaitGL();
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXWaitGL();)
  GITS_WRAPPER_POST
}

GLAPI void GLAPIENTRY glXWaitX() {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXWaitX();
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXWaitX();)
  GITS_WRAPPER_POST
}

GLAPI void GLAPIENTRY glXUseXFont(Font font, int first, int count, int list) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXUseXFont(font, first, count, list);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXUseXFont(font, first, count, list);)
  GITS_WRAPPER_POST
}

GLAPI const char* GLAPIENTRY glXQueryExtensionsString(Display* dpy, int screen) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXQueryExtensionsString(dpy, screen);
  GITS_WRAPPER_PRE
  wrapper.glXQueryExtensionsString(return_value, dpy, screen);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI const char* GLAPIENTRY glXQueryServerString(Display* dpy, int screen, int name) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXQueryServerString(dpy, screen, name);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXQueryServerString(return_value, dpy, screen, name);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI const char* GLAPIENTRY glXGetClientString(Display* dpy, int name) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetClientString(dpy, name);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXGetClientString(return_value, dpy, name);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI Display* GLAPIENTRY glXGetCurrentDisplay() {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetCurrentDisplay();
  GITS_WRAPPER_PRE
  wrapper.glXGetCurrentDisplay(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXFBConfig* GLAPIENTRY glXChooseFBConfig(Display* dpy,
                                                int screen,
                                                const int* attribList,
                                                int* nitems) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXChooseFBConfig(dpy, screen, attribList, nitems);
  GITS_WRAPPER_PRE
  wrapper.glXChooseFBConfig(return_value, dpy, screen, attribList, nitems);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXGetFBConfigAttrib(Display* dpy,
                                          GLXFBConfig config,
                                          int attribute,
                                          int* value) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetFBConfigAttrib(dpy, config, attribute, value);
  GITS_WRAPPER_PRE
  wrapper.glXGetFBConfigAttrib(return_value, dpy, config, attribute, value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXFBConfig* GLAPIENTRY glXGetFBConfigs(Display* dpy, int screen, int* nelements) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetFBConfigs(dpy, screen, nelements);
  GITS_WRAPPER_PRE
  wrapper.glXGetFBConfigs(return_value, dpy, screen, nelements);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI XVisualInfo* GLAPIENTRY glXGetVisualFromFBConfig(Display* dpy, GLXFBConfig config) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetVisualFromFBConfig(dpy, config);
  GITS_WRAPPER_PRE
  wrapper.glXGetVisualFromFBConfig(return_value, dpy, config);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXWindow GLAPIENTRY glXCreateWindow(Display* dpy,
                                           GLXFBConfig config,
                                           Window win,
                                           const int* attribList) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXCreateWindow(dpy, config, win, attribList);
  GITS_WRAPPER_PRE
  wrapper.glXCreateWindow(return_value, dpy, config, win, attribList);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXDestroyWindow(Display* dpy, GLXWindow window) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXDestroyWindow(dpy, window);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXDestroyWindow(dpy, window);)
  GITS_WRAPPER_POST
}

GLAPI GLXPixmap GLAPIENTRY glXCreatePixmap(Display* dpy,
                                           GLXFBConfig config,
                                           Pixmap pixmap,
                                           const int* attribList) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXCreatePixmap(dpy, config, pixmap, attribList);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(
      wrapper.glXCreatePixmap(return_value, dpy, config, pixmap, attribList);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXDestroyPixmap(Display* dpy, GLXPixmap pixmap) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXDestroyPixmap(dpy, pixmap);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXDestroyPixmap(dpy, pixmap);)
  GITS_WRAPPER_POST
}

GLAPI GLXPbuffer GLAPIENTRY glXCreatePbuffer(Display* dpy,
                                             GLXFBConfig config,
                                             const int* attribList) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXCreatePbuffer(dpy, config, attribList);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXCreatePbuffer(return_value, dpy, config, attribList);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXDestroyPbuffer(dpy, pbuf);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXDestroyPbuffer(dpy, pbuf);)
  GITS_WRAPPER_POST
}

GLAPI void GLAPIENTRY glXQueryDrawable(Display* dpy,
                                       GLXDrawable draw,
                                       int attribute,
                                       unsigned int* value) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXQueryDrawable(dpy, draw, attribute, value);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXQueryDrawable(dpy, draw, attribute, value);)
  GITS_WRAPPER_POST
}

GLAPI GLXContext GLAPIENTRY glXCreateNewContext(
    Display* dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct) {
  GITS_ENTRY_GLX
  auto return_value =
      wrapper.Drivers().glx.glXCreateNewContext(dpy, config, renderType, shareList, direct);
  GITS_WRAPPER_PRE
  wrapper.glXCreateNewContext(return_value, dpy, config, renderType, shareList, direct);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI Bool GLAPIENTRY glXMakeContextCurrent(Display* dpy,
                                            GLXDrawable draw,
                                            GLXDrawable read,
                                            GLXContext ctx) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXMakeContextCurrent(dpy, draw, read, ctx);
  GITS_WRAPPER_PRE
  wrapper.glXMakeContextCurrent(return_value, dpy, draw, read, ctx);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI GLXDrawable GLAPIENTRY glXGetCurrentReadDrawable() {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetCurrentReadDrawable();
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXGetCurrentReadDrawable(return_value, arg0);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXQueryContext(Display* dpy, GLXContext ctx, int attribute, int* value) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXQueryContext(dpy, ctx, attribute, value);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXQueryContext(return_value, dpy, ctx, attribute, value);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXSelectEvent(Display* dpy, GLXDrawable drawable, unsigned long mask) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXSelectEvent(dpy, drawable, mask);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXSelectEvent(dpy, drawable, mask);)
  GITS_WRAPPER_POST
}

GLAPI void GLAPIENTRY glXGetSelectedEvent(Display* dpy, GLXDrawable drawable, unsigned long* mask) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXGetSelectedEvent(dpy, drawable, mask);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXGetSelectedEvent(dpy, drawable, mask);)
  GITS_WRAPPER_POST
}

GLAPI void* GLAPIENTRY glXGetProcAddressARB(const GLubyte* procname) {
  GITS_ENTRY_GLX
  auto return_value = get_proc_address((const char*)procname);
  GITS_WRAPPER_PRE
  wrapper.glXGetProcAddressARB(return_value, procname);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void* GLAPIENTRY glXGetProcAddress(const GLubyte* procname) {
  GITS_ENTRY_GLX
  auto return_value = get_proc_address((const char*)procname);
  GITS_WRAPPER_PRE
  wrapper.glXGetProcAddress(return_value, procname);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void* GLAPIENTRY glXAllocateMemoryNV(GLsizei size,
                                           GLfloat readfreq,
                                           GLfloat writefreq,
                                           GLfloat priority) {
  GITS_ENTRY_GLX
  auto return_value =
      wrapper.Drivers().glx.glXAllocateMemoryNV(size, readfreq, writefreq, priority);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(
      wrapper.glXAllocateMemoryNV(return_value, size, readfreq, writefreq, priority);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXFreeMemoryNV(GLvoid* pointer) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXFreeMemoryNV(pointer);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXFreeMemoryNV(pointer);)
  GITS_WRAPPER_POST
}

GLAPI Bool GLAPIENTRY glXBindTexImageARB(Display* dpy, GLXPbuffer pbuffer, int buffer) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXBindTexImageARB(dpy, pbuffer, buffer);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXBindTexImageARB(return_value, dpy, pbuffer, buffer);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI Bool GLAPIENTRY glXReleaseTexImageARB(Display* dpy, GLXPbuffer pbuffer, int buffer) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXReleaseTexImageARB(dpy, pbuffer, buffer);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXReleaseTexImageARB(return_value, dpy, pbuffer, buffer);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI Bool GLAPIENTRY glXDrawableAttribARB(Display* dpy, GLXDrawable draw, const int* attribList) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXDrawableAttribARB(dpy, draw, attribList);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXDrawableAttribARB(return_value, dpy, draw, attribList);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXGetFrameUsageMESA(Display* dpy, GLXDrawable drawable, float* usage) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetFrameUsageMESA(dpy, drawable, usage);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXGetFrameUsageMESA(return_value, dpy, drawable, usage);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXBeginFrameTrackingMESA(Display* dpy, GLXDrawable drawable) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXBeginFrameTrackingMESA(dpy, drawable);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXBeginFrameTrackingMESA(return_value, dpy, drawable);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXEndFrameTrackingMESA(Display* dpy, GLXDrawable drawable) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXEndFrameTrackingMESA(dpy, drawable);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXEndFrameTrackingMESA(return_value, dpy, drawable);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXQueryFrameTrackingMESA(Display* dpy,
                                               GLXDrawable drawable,
                                               int64_t* swapCount,
                                               int64_t* missedFrames,
                                               float* lastMissedUsage) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXQueryFrameTrackingMESA(
      dpy, drawable, swapCount, missedFrames, lastMissedUsage);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXQueryFrameTrackingMESA(
      return_value, dpy, drawable, swapCount, missedFrames, lastMissedUsage);)
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXSwapIntervalMESA(unsigned int interval) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXSwapIntervalMESA(interval);
  GITS_WRAPPER_PRE
  wrapper.glXSwapIntervalMESA(return_value, interval);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXGetSwapIntervalMESA() {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXGetSwapIntervalMESA();
  GITS_WRAPPER_PRE
  wrapper.glXGetSwapIntervalMESA(return_value);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI void GLAPIENTRY glXBindTexImageEXT(Display* dpy,
                                         GLXDrawable drawable,
                                         int buffer,
                                         const int* attrib_list) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXBindTexImageEXT(dpy, drawable, buffer, attrib_list);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXBindTexImageEXT(dpy, drawable, buffer, attrib_list);)
  GITS_WRAPPER_POST
}

GLAPI void GLAPIENTRY glXReleaseTexImageEXT(Display* dpy, GLXDrawable drawable, int buffer) {
  GITS_ENTRY_GLX
  wrapper.Drivers().glx.glXReleaseTexImageEXT(dpy, drawable, buffer);
  GITS_WRAPPER_PRE
  GITS_WRAPPER_NOT_SUPPORTED(wrapper.glXReleaseTexImageEXT(dpy, drawable, buffer);)
  GITS_WRAPPER_POST
}

GLAPI GLXContext GLAPIENTRY glXCreateContextAttribsARB(Display* dpy,
                                                       GLXFBConfig config,
                                                       GLXContext share_context,
                                                       Bool direct,
                                                       const int* attrib_list) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXCreateContextAttribsARB(dpy, config, share_context,
                                                                       direct, attrib_list);
  GITS_WRAPPER_PRE
  wrapper.glXCreateContextAttribsARB(return_value, dpy, config, share_context, direct, attrib_list);
  GITS_WRAPPER_POST
  return return_value;
}

GLAPI int GLAPIENTRY glXSwapIntervalSGI(int interval) {
  GITS_ENTRY_GLX
  auto return_value = wrapper.Drivers().glx.glXSwapIntervalSGI(interval);
  GITS_WRAPPER_PRE
  wrapper.glXSwapIntervalSGI(return_value, interval);
  GITS_WRAPPER_POST
  return return_value;
}
#endif
} //extern "C"
