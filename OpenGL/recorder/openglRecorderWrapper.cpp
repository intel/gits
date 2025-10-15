// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglRecorderWrapper.cpp
 *
 * @brief Definition of OpenGL recorder wrapper.
 */

#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "openglRecorderWrapper.h"
#include "openglLibraryRecorder.h"
#include "recorder.h"
#include "gits.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "exception.h"
#include "log.h"
#include "tools.h"
#include "opengl_apis_iface.h"

#include "gitsFunctions.h"

#include "eglFunctions.h"
#include "wglFunctions.h"
#include "glxFunctions.h"
#include "openglCommon.h"
#include "openglTools.h"
#include "stateTracking.h"
#include "openglState.h"
#include "openglRecorderSubWrappers.h"
#include "openglRecorderConditions.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <filesystem>

static gits::OpenGL::CRecorderWrapper* wrapper = nullptr;

gits::OpenGL::IRecorderWrapper* STDCALL GITSRecorderOpenGL() {
  if (wrapper == nullptr) {
    try {
      // library not set - perform initialization
      gits::CGits::Instance().apis.UseApi3dIface(
          std::shared_ptr<gits::ApisIface::Api3d>(new gits::OpenGL::OpenGLApi()));
      gits::CRecorder& recorder = gits::CRecorder::Instance();
      wrapper = new gits::OpenGL::CRecorderWrapper(recorder);
      auto library = new gits::OpenGL::CLibraryRecorder;
      recorder.Register(std::shared_ptr<gits::CLibrary>(library)); // TODO: VS2010 workaround
    } catch (const std::exception& ex) {
      LOG_ERROR << "Cannot initialize recorder: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return wrapper;
}

namespace gits {

DrawCallWrapperPrePost::DrawCallWrapperPrePost(gits::CRecorder& rec) : _recorder(rec) {
  gits::CGits::Instance().DrawCountUp();
  const auto& frameRangeCfg = Configurator::Get().opengl.recorder;
  if (frameRangeCfg.oglSingleDraw.number == gits::CGits::Instance().CurrentDrawCount()) {
    gits::OpenGL::SD().EraseNonCurrentData();
  }
  _recorder.DrawBegin();
  if (gits::OpenGL::SD().GetCurrentSharedStateData().coherentBufferMapping == true &&
      (gits::OpenGL::SD().GetCurrentContext() != nullptr)) {
    _recorder.Schedule(new gits::OpenGL::CgitsCoherentBufferMapping(
        gits::OpenGL::CCoherentBufferUpdate::TCoherentBufferData::PER_DRAWCALL_UPDATE,
        gits::OpenGL::CCoherentBufferUpdate::TCoherentBufferData::UPDATE_BOUND,
        Configurator::Get().opengl.recorder.coherentMapUpdatePerFrame));
  }
}
DrawCallWrapperPrePost::~DrawCallWrapperPrePost() {
  using namespace gits;
  using namespace OpenGL;
  try {
    if (Configurator::Get()
            .opengl.recorder.dumpDrawsFromFrames[gits::CGits::Instance().CurrentFrame()]) {
      //Dump screenshots after drawcalls
      std::filesystem::path path = Configurator::Get().common.recorder.dumpPath /
                                   "gitsScreenshots" / "gitsRecorder" / "draws";
      capture_drawbuffer(
          path, "drawcall-" + std::to_string(CGits::Instance().CurrentDrawCount()) + "-pre", true);
    }
    _recorder.DrawEnd();
  } catch (...) {
    topmost_exception_handler("DrawCallWrapperPrePost::~DrawCallWrapperPrePost");
  }
}

} // namespace gits

namespace gits {
namespace OpenGL {

CRecorderWrapper::CRecorderWrapper(CRecorder& recorder) : _recorder(recorder) {
  drv.add_terminate_event([] { CRecorder::Dispose(); });
}

void CRecorderWrapper::StreamFinishedEvent(std::function<void()> event) {
  _recorder.RegisterDisposeEvent(std::move(event));
}

void CRecorderWrapper::EndFramePost() const {
  _recorder.EndFramePost();
}

void CRecorderWrapper::CloseRecorderIfRequired() {
  if (_recorder.IsMarkedForDeletion()) {
    _recorder.Close();
  }
}
std::recursive_mutex& CRecorderWrapper::GetInterceptorMutex() const {
  return _recorder.GetMutex();
}

CDrivers& CRecorderWrapper::Drivers() const {
  return drv;
}

void CRecorderWrapper::PreSwap() const {
  if (Record() &&
      Configurator::Get().opengl.recorder.dumpScreenshots[gits::CGits::Instance().CurrentFrame()]) {
    FrameBufferSave(gits::CGits::Instance().CurrentFrame());
  }
}

void CRecorderWrapper::eglGetError(EGLint return_value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetError(return_value));
  }
}

// Key EGL functions are ALWAYS recorded - this is done to simplify state
// restoration, as context's existence can be assumed.
void CRecorderWrapper::eglGetDisplay(EGLDisplay return_value,
                                     EGLNativeDisplayType display_id) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglGetDisplay(return_value, display_id), true);
}

void CRecorderWrapper::eglInitialize(EGLBoolean return_value,
                                     EGLDisplay dpy,
                                     EGLint* major,
                                     EGLint* minor) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglInitialize(return_value, dpy, major, minor), true);
}

void CRecorderWrapper::eglTerminate(EGLBoolean return_value, EGLDisplay dpy) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglTerminate(return_value, dpy), true);
}

void CRecorderWrapper::eglQueryString(const char* return_value, EGLDisplay dpy, EGLint name) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQueryString(return_value, dpy, name));
  }
}

void CRecorderWrapper::eglGetConfigs(EGLBoolean return_value,
                                     EGLDisplay dpy,
                                     EGLConfig* configs,
                                     EGLint config_size,
                                     EGLint* num_config) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetConfigs(return_value, dpy, configs, config_size, num_config));
  }
}

void CRecorderWrapper::eglChooseConfig(EGLBoolean return_value,
                                       EGLDisplay dpy,
                                       const EGLint* attrib_list,
                                       EGLConfig* configs,
                                       EGLint config_size,
                                       EGLint* num_config) const {
  GITS_REC_ENTRY_EGL
  if (_recorder.Running()) {
    _recorder.Schedule(
        new CeglChooseConfig(return_value, dpy, attrib_list, configs, config_size, num_config));
  }
}

void CRecorderWrapper::eglGetConfigAttrib(EGLBoolean return_value,
                                          EGLDisplay dpy,
                                          EGLConfig config,
                                          EGLint attribute,
                                          EGLint* value) const {
  GITS_REC_ENTRY_EGL
  if (_recorder.Running()) {
    _recorder.Schedule(new CeglGetConfigAttrib(return_value, dpy, config, attribute, value));
  }
}

void CRecorderWrapper::eglCreateWindowSurface(EGLSurface return_value,
                                              EGLDisplay dpy,
                                              EGLConfig config,
                                              EGLNativeWindowType win,
                                              const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreateWindowSurface(return_value, dpy, config, win, attrib_list),
                     true);
}

void CRecorderWrapper::eglCreatePbufferSurface(EGLSurface return_value,
                                               EGLDisplay dpy,
                                               EGLConfig config,
                                               const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreatePbufferSurface(return_value, dpy, config, attrib_list), true);
}

void CRecorderWrapper::eglCreatePixmapSurface(EGLSurface return_value,
                                              EGLDisplay dpy,
                                              EGLConfig config,
                                              EGLNativePixmapType pixmap,
                                              const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreatePixmapSurface(return_value, dpy, config, pixmap, attrib_list),
                     true);
}

void CRecorderWrapper::eglDestroySurface(EGLBoolean return_value,
                                         EGLDisplay dpy,
                                         EGLSurface surface) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglDestroySurface(return_value, dpy, surface), true);
}

void CRecorderWrapper::eglQuerySurface(EGLBoolean return_value,
                                       EGLDisplay dpy,
                                       EGLSurface surface,
                                       EGLint attribute,
                                       EGLint* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQuerySurface(return_value, dpy, surface, attribute, value));
  }
}

void CRecorderWrapper::eglBindAPI(EGLBoolean return_value, EGLenum api) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglBindAPI(return_value, api), true);
}

void CRecorderWrapper::eglQueryAPI(EGLenum return_value) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglQueryAPI(return_value));
}

void CRecorderWrapper::eglWaitClient(EGLBoolean return_value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglWaitClient(return_value));
  }
}

void CRecorderWrapper::eglReleaseThread(EGLBoolean return_value) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglReleaseThread(return_value), true);
}

void CRecorderWrapper::eglCreatePbufferFromClientBuffer(EGLSurface return_value,
                                                        EGLDisplay dpy,
                                                        EGLenum buftype,
                                                        EGLClientBuffer buffer,
                                                        EGLConfig config,
                                                        const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreatePbufferFromClientBuffer(return_value, dpy, buftype, buffer,
                                                           config, attrib_list),
                     true);
}

void CRecorderWrapper::eglSurfaceAttrib(EGLBoolean return_value,
                                        EGLDisplay dpy,
                                        EGLSurface surface,
                                        EGLint attribute,
                                        EGLint value) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglSurfaceAttrib(return_value, dpy, surface, attribute, value), true);
}

void CRecorderWrapper::eglBindTexImage(EGLBoolean return_value,
                                       EGLDisplay dpy,
                                       EGLSurface surface,
                                       EGLint buffer) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglBindTexImage(return_value, dpy, surface, buffer));
  }
}

void CRecorderWrapper::eglReleaseTexImage(EGLBoolean return_value,
                                          EGLDisplay dpy,
                                          EGLSurface surface,
                                          EGLint buffer) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglReleaseTexImage(return_value, dpy, surface, buffer));
  }
}

void CRecorderWrapper::eglSwapInterval(EGLBoolean return_value,
                                       EGLDisplay dpy,
                                       EGLint interval) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglSwapInterval(return_value, dpy, interval), true);
}

void CRecorderWrapper::eglCreateContext(EGLContext return_value,
                                        EGLDisplay dpy,
                                        EGLConfig config,
                                        EGLContext share_context,
                                        const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreateContext(return_value, dpy, config, share_context, attrib_list),
                     true);
}

void CRecorderWrapper::eglDestroyContext(EGLBoolean return_value,
                                         EGLDisplay dpy,
                                         EGLContext ctx) const {
  GITS_REC_ENTRY_EGL

  SD().RemoveContext((void*)ctx);
  _recorder.Schedule(new CeglDestroyContext(return_value, dpy, ctx), true);

  static uint32_t deleted_context = 0;
  if (++deleted_context == Configurator::Get().opengl.recorder.all.exitDeleteContext) {
    _recorder.Close();
  }
}

void CRecorderWrapper::eglMakeCurrent(EGLBoolean return_value,
                                      EGLDisplay dpy,
                                      EGLSurface draw,
                                      EGLSurface read,
                                      EGLContext ctx) const {
  GITS_REC_ENTRY_EGL
  SD().SetCurrentContext((void*)ctx);

  _recorder.Schedule(new CeglMakeCurrent(return_value, dpy, draw, read, ctx), true);
}

void CRecorderWrapper::eglGetCurrentContext(EGLContext return_value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetCurrentContext(return_value));
  }
}

void CRecorderWrapper::eglGetCurrentSurface(EGLSurface return_value, EGLint readdraw) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetCurrentSurface(return_value, readdraw));
  }
}

void CRecorderWrapper::eglGetCurrentDisplay(EGLDisplay return_value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetCurrentDisplay(return_value));
  }
}

void CRecorderWrapper::eglQueryContext(EGLBoolean return_value,
                                       EGLDisplay dpy,
                                       EGLContext ctx,
                                       EGLint attribute,
                                       EGLint* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQueryContext(return_value, dpy, ctx, attribute, value));
  }
}

void CRecorderWrapper::eglWaitGL(EGLBoolean return_value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglWaitGL(return_value));
  }
}

void CRecorderWrapper::eglWaitNative(EGLBoolean return_value, EGLint engine) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglWaitNative(return_value, engine));
  }
}

void CRecorderWrapper::eglSwapBuffers(EGLBoolean return_value,
                                      EGLDisplay dpy,
                                      EGLSurface surface) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglSwapBuffers(return_value, dpy, surface));
  }
  _recorder.FrameEnd();
}

void CRecorderWrapper::eglCopyBuffers(EGLBoolean return_value,
                                      EGLDisplay dpy,
                                      EGLSurface surface,
                                      EGLNativePixmapType target) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCopyBuffers(return_value, dpy, surface, target));
  }
}

void CRecorderWrapper::eglGetProcAddress(void* return_value, const char* procname) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(
        new CeglGetProcAddress(reinterpret_cast<GLsizeiptr>(return_value), procname));
  }
}

void CRecorderWrapper::eglClientWaitSyncKHR(EGLDisplay dpy,
                                            EGLSyncKHR sync,
                                            EGLint flags,
                                            EGLTimeKHR timeout) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglClientWaitSyncKHR(dpy, sync, flags, timeout));
  }
}

void CRecorderWrapper::eglClientWaitSyncNV(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglClientWaitSyncNV(sync, flags, timeout));
  }
}

void CRecorderWrapper::eglCreateDRMImageMESA(EGLDisplay dpy, const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreateDRMImageMESA(dpy, attrib_list));
  }
}

void CRecorderWrapper::eglCreateFenceSyncNV(EGLDisplay dpy,
                                            EGLenum condition,
                                            const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreateFenceSyncNV(dpy, condition, attrib_list));
  }
}

void CRecorderWrapper::eglCreateImageKHR(EGLImageKHR return_value,
                                         EGLDisplay dpy,
                                         EGLContext ctx,
                                         EGLenum target,
                                         EGLClientBuffer buffer,
                                         const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreateImageKHR(return_value, dpy, ctx, target, buffer, attrib_list),
                     true);
}

void CRecorderWrapper::eglCreatePixmapSurfaceHI(EGLDisplay dpy,
                                                EGLConfig config,
                                                void* pixmap) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreatePixmapSurfaceHI(dpy, config, pixmap));
  }
}

void CRecorderWrapper::eglCreateStreamFromFileDescriptorKHR(
    EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreateStreamFromFileDescriptorKHR(dpy, file_descriptor));
  }
}

void CRecorderWrapper::eglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreateStreamKHR(dpy, attrib_list));
  }
}

void CRecorderWrapper::eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy,
                                                         EGLConfig config,
                                                         EGLStreamKHR stream,
                                                         const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreateStreamProducerSurfaceKHR(dpy, config, stream, attrib_list));
  }
}

void CRecorderWrapper::eglCreateSyncKHR(EGLSyncKHR return_value,
                                        EGLDisplay dpy,
                                        EGLenum type,
                                        const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglCreateSyncKHR(return_value, dpy, type, attrib_list));
  }
}

void CRecorderWrapper::eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglDestroyImageKHR(dpy, image), true);
}

void CRecorderWrapper::eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglDestroyStreamKHR(dpy, stream));
  }
}

void CRecorderWrapper::eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglDestroySyncKHR(dpy, sync));
  }
}

void CRecorderWrapper::eglDestroySyncNV(EGLSyncNV sync) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglDestroySyncNV(sync));
  }
}

void CRecorderWrapper::eglExportDRMImageMESA(
    EGLDisplay dpy, EGLImageKHR image, EGLint* name, EGLint* handle, EGLint* stride) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglExportDRMImageMESA(dpy, image, name, handle, stride));
  }
}

void CRecorderWrapper::eglFenceNV(EGLSyncNV sync) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglFenceNV(sync));
  }
}

void CRecorderWrapper::eglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetStreamFileDescriptorKHR(dpy, stream));
  }
}

void CRecorderWrapper::eglGetSyncAttribKHR(EGLDisplay dpy,
                                           EGLSyncKHR sync,
                                           EGLint attribute,
                                           EGLint* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetSyncAttribKHR(dpy, sync, attribute, value));
  }
}

void CRecorderWrapper::eglGetSyncAttribNV(EGLSyncNV sync, EGLint attribute, EGLint* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetSyncAttribNV(sync, attribute, value));
  }
}

void CRecorderWrapper::eglGetSystemTimeFrequencyNV() const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetSystemTimeFrequencyNV());
  }
}

void CRecorderWrapper::eglGetSystemTimeNV() const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglGetSystemTimeNV());
  }
}

void CRecorderWrapper::eglLockSurfaceKHR(EGLDisplay display,
                                         EGLSurface surface,
                                         const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglLockSurfaceKHR(display, surface, attrib_list));
  }
}

void CRecorderWrapper::eglPostSubBufferNV(
    EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglPostSubBufferNV(dpy, surface, x, y, width, height));
  }
}

void CRecorderWrapper::eglQueryStreamKHR(EGLDisplay dpy,
                                         EGLStreamKHR stream,
                                         EGLenum attribute,
                                         EGLint* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQueryStreamKHR(dpy, stream, attribute, value));
  }
}

void CRecorderWrapper::eglQueryStreamTimeKHR(EGLDisplay dpy,
                                             EGLStreamKHR stream,
                                             EGLenum attribute,
                                             EGLTimeKHR* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQueryStreamTimeKHR(dpy, stream, attribute, value));
  }
}

void CRecorderWrapper::eglQueryStreamu64KHR(EGLDisplay dpy,
                                            EGLStreamKHR stream,
                                            EGLenum attribute,
                                            EGLuint64KHR* value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQueryStreamu64KHR(dpy, stream, attribute, value));
  }
}

void CRecorderWrapper::eglQuerySurfacePointerANGLE(EGLDisplay dpy,
                                                   EGLSurface surface,
                                                   EGLint attribute,
                                                   void** value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglQuerySurfacePointerANGLE(dpy, surface, attribute, value));
  }
}

void CRecorderWrapper::eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglSignalSyncKHR(dpy, sync, mode));
  }
}

void CRecorderWrapper::eglSignalSyncNV(EGLSyncNV sync, EGLenum mode) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglSignalSyncNV(sync, mode));
  }
}

void CRecorderWrapper::eglStreamAttribKHR(EGLDisplay dpy,
                                          EGLStreamKHR stream,
                                          EGLenum attribute,
                                          EGLint value) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglStreamAttribKHR(dpy, stream, attribute, value));
  }
}

void CRecorderWrapper::eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglStreamConsumerAcquireKHR(dpy, stream));
  }
}

void CRecorderWrapper::eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy,
                                                             EGLStreamKHR stream) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglStreamConsumerGLTextureExternalKHR(dpy, stream));
  }
}

void CRecorderWrapper::eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglStreamConsumerReleaseKHR(dpy, stream));
  }
}

void CRecorderWrapper::eglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglUnlockSurfaceKHR(display, surface));
  }
}

void CRecorderWrapper::eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) const {
  GITS_REC_ENTRY_EGL
  if (Record()) {
    _recorder.Schedule(new CeglWaitSyncKHR(dpy, sync, flags));
  }
}

void CRecorderWrapper::eglSetBlobCacheFuncsANDROID(EGLDisplay dpy,
                                                   EGLSetBlobFuncANDROID set,
                                                   EGLGetBlobFuncANDROID get) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglSetBlobCacheFuncsANDROID(dpy, set, get), true);
}

void CRecorderWrapper::eglGetPlatformDisplayEXT(EGLDisplay return_value,
                                                EGLenum platform,
                                                Display* native_display,
                                                const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(
      new CeglGetPlatformDisplayEXT(return_value, platform, native_display, attrib_list));
}

void CRecorderWrapper::eglCreatePlatformWindowSurfaceEXT(EGLSurface return_value,
                                                         EGLDisplay dpy,
                                                         EGLConfig config,
                                                         EGLNativeWindowType native_window,
                                                         const EGLint* attrib_list) const {
  GITS_REC_ENTRY_EGL
  _recorder.Schedule(new CeglCreatePlatformWindowSurfaceEXT(return_value, dpy, config,
                                                            native_window, attrib_list));
}

void CRecorderWrapper::UpdateMappedTextures() const {
  auto toUpdate = SD().GetCurrentSharedStateData().GetMappedTextures().SaveAllTexturesToFile();
  for (auto& tex : toUpdate) {
    _recorder.Schedule(new CUpdateMappedTexture(tex.texture, tex.level, tex.hash, tex.size));
  }
}

bool CRecorderWrapper::Record() const {
  return _recorder.Running() && (SD().GetCurrentContext() != nullptr);
}

void CRecorderWrapper::TrackThread() const {
  int threadId = _recorder.TrackThread();
  if (threadId >= 0) {
    _recorder.Schedule(new CTokenMakeCurrentThread(threadId));
  }
}

#if defined GITS_PLATFORM_WINDOWS

void CRecorderWrapper::wglChoosePixelFormat(bool return_value,
                                            HDC hdc,
                                            const PIXELFORMATDESCRIPTOR* ppfd) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglChoosePixelFormat(return_value, hdc, ppfd));
}

void CRecorderWrapper::wglSetPixelFormat(bool return_value,
                                         HDC hdc,
                                         int format,
                                         const PIXELFORMATDESCRIPTOR* ppfd) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSetPixelFormat(return_value, hdc, format, ppfd), true);
}

namespace detail {
std::map<int, int> getCurrentPixelAttributes(HDC hdc, HGLRC hglrc) {
  static const int attributes[] = {WGL_DRAW_TO_WINDOW_ARB,   WGL_DRAW_TO_BITMAP_ARB,
                                   WGL_ACCELERATION_ARB,     WGL_SWAP_LAYER_BUFFERS_ARB,
                                   WGL_SWAP_METHOD_ARB,      WGL_SHARE_DEPTH_ARB,
                                   WGL_SHARE_STENCIL_ARB,    WGL_SHARE_ACCUM_ARB,
                                   WGL_SUPPORT_GDI_ARB,      WGL_SUPPORT_OPENGL_ARB,
                                   WGL_DOUBLE_BUFFER_ARB,    WGL_STEREO_ARB,
                                   WGL_PIXEL_TYPE_ARB,       WGL_COLOR_BITS_ARB,
                                   WGL_RED_BITS_ARB,         WGL_GREEN_BITS_ARB,
                                   WGL_BLUE_BITS_ARB,        WGL_ALPHA_BITS_ARB,
                                   WGL_ACCUM_BITS_ARB,       WGL_ACCUM_RED_BITS_ARB,
                                   WGL_ACCUM_GREEN_BITS_ARB, WGL_ACCUM_BLUE_BITS_ARB,
                                   WGL_ACCUM_ALPHA_BITS_ARB, WGL_DEPTH_BITS_ARB,
                                   WGL_STENCIL_BITS_ARB,     WGL_AUX_BUFFERS_ARB,
                                   WGL_SAMPLE_BUFFERS_ARB,   WGL_SAMPLES_ARB};
  const int attribsCount = sizeof(attributes) / sizeof(attributes[0]);
  static int values[attribsCount];

  HGLRC origCtx = (HGLRC)drv.wgl.wglGetCurrentContext();
  HDC origDC = (HDC)drv.wgl.wglGetCurrentDC();

  drv.wgl.wglMakeCurrent(hdc, hglrc);
  typedef BOOL(STDCALL wglGetPixelFormatAttribivARB_t)(HDC, int, int, int, const int*, int*);
  static wglGetPixelFormatAttribivARB_t* wglGetPixelFormatAttribivARB = 0;
  if (wglGetPixelFormatAttribivARB == 0) {
    wglGetPixelFormatAttribivARB =
        (wglGetPixelFormatAttribivARB_t*)drv.wgl.wglGetProcAddress("wglGetPixelFormatAttribivARB");
  }

  int pixelFormat = ::GetPixelFormat(hdc);
  if (!wglGetPixelFormatAttribivARB(hdc, pixelFormat, 0, attribsCount, attributes, values)) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  drv.wgl.wglMakeCurrent(origDC, origCtx);

  std::map<int, int> attribsMap;
  for (int i = 0; i < attribsCount; ++i) {
    attribsMap[attributes[i]] = values[i];
  }
  return attribsMap;
}
} // namespace detail

void CRecorderWrapper::wglCreateContext(HGLRC hglrc, HDC hdc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateContext(hglrc, hdc), true);
}

void CRecorderWrapper::wglCreateLayerContext(HGLRC return_value, HDC hdc, int iLayerPlane) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateLayerContext(return_value, hdc, iLayerPlane), true);
}

void CRecorderWrapper::wglGetPixelFormat(int return_value, HDC hdc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetPixelFormat(return_value, hdc));
}

void CRecorderWrapper::wglDescribePixelFormat(
    int return_value, HDC hdc, int format, unsigned nBytes, PIXELFORMATDESCRIPTOR* ppfd) const {}

void CRecorderWrapper::wglMakeCurrent(bool return_value, HDC hdc, HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL
#ifdef GITS_PLATFORM_WINDOWS
  static int lastScreenWidth = 0;
  static int lastScreenHeight = 0;
  int currentScreenWidth = GetSystemMetrics(SM_CXSCREEN);
  int currentScreenHeight = GetSystemMetrics(SM_CYSCREEN);

  if ((currentScreenWidth * currentScreenHeight) > 0 &&
      (currentScreenWidth != lastScreenWidth || currentScreenHeight != lastScreenHeight)) {
    _recorder.Schedule(new CTokenScreenResolution(currentScreenWidth, currentScreenHeight), true);
  }

  lastScreenWidth = currentScreenWidth;
  lastScreenHeight = currentScreenHeight;
#endif

  SD().SetCurrentContext((void*)hglrc);

#ifdef GITS_PLATFORM_WINDOWS
  int windowThread = CStateDynamicNative::Get().GetWindowThread(WindowFromDC(hdc));
  int currentThread = CGits::Instance().CurrentThreadId();
  if (windowThread != currentThread) {
    LOG_WARNING
        << "Scheduling thread switch for window params update (WA for thread affine winapi)";

    HWND hwnd = WindowFromDC(hdc);
    _recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(windowThread));
    _recorder.Schedule(new ChelperWglUpdateWindow(hwnd));
    _recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(currentThread));
  }
#endif
  _recorder.Schedule(new CwglMakeCurrent(return_value, hdc, hglrc), true);
}

void CRecorderWrapper::wglDeleteContext(bool retVal, HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL

  SD().RemoveContext((void*)hglrc);
  _recorder.Schedule(new CwglDeleteContext(retVal, hglrc), true);

  static int deleted_context = 0;
  if (++deleted_context == Configurator::Get().opengl.recorder.all.exitDeleteContext) {
    _recorder.Close();
  }
}

void CRecorderWrapper::wglGetCurrentContext(HGLRC return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetCurrentContext(return_value));
}

void CRecorderWrapper::wglSwapLayerBuffers(bool return_value, HDC hdc, unsigned plane) const {
  GITS_REC_ENTRY_WGL

  if (Record()) {
    UpdateMappedTextures();
  }

  if (Record()) {
    int windowThread = CStateDynamicNative::Get().GetWindowThread(WindowFromDC(hdc));
    int currentThread = CGits::Instance().CurrentThreadId();
    if (windowThread != currentThread) {
      LOG_WARNING << "Current thread does not match thread which created window: "
                  << WindowFromDC(hdc);
      LOG_WARNING
          << "Scheduling thread switch for window params update (WA for thread affine winapi)";

      HWND hwnd = WindowFromDC(hdc);
      _recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(windowThread));
      _recorder.Schedule(new ChelperWglUpdateWindow(hwnd));
      _recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(currentThread));
    }
    _recorder.Schedule(new CwglSwapLayerBuffers(return_value, hdc, plane));
  }
  _recorder.FrameEnd();
}

void CRecorderWrapper::wglSwapBuffers(bool return_value, HDC hdc) const {
  GITS_REC_ENTRY_WGL

  if (Record()) {
    UpdateMappedTextures();
  }

  if (Record()) {
    int windowThread = CStateDynamicNative::Get().GetWindowThread(WindowFromDC(hdc));
    int currentThread = CGits::Instance().CurrentThreadId();
    if (windowThread != currentThread) {
      LOG_WARNING << "Current thread does not match thread which created window: "
                  << WindowFromDC(hdc);
      LOG_WARNING
          << "Scheduling thread switch for window params update (WA for thread affine winapi)";

      HWND hwnd = WindowFromDC(hdc);
      _recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(windowThread));
      _recorder.Schedule(new ChelperWglUpdateWindow(hwnd));
      _recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(currentThread));
    }
    _recorder.Schedule(new CwglSwapBuffers(return_value, hdc));
  }
  _recorder.FrameEnd();
}

void CRecorderWrapper::wglSwapMultipleBuffers(bool return_value, GLuint buffers, HDC* hdc) const {
  GITS_REC_ENTRY_WGL
  if (Record()) {
    _recorder.Schedule(new CwglSwapMultipleBuffers(return_value, buffers, hdc));
  }
  _recorder.FrameEnd();
}

namespace detail {
void getCurrentFontInfo(HDC hdc, std::string& str, std::vector<int>& args) {
  str.resize(256, 0);
  const int strsize = GetTextFace(hdc, 256, &str.at(0));
  str.resize(strsize);

  TEXTMETRIC tm;
  GetTextMetrics(hdc, &tm);
  args.push_back(tm.tmHeight);
  args.push_back(tm.tmAveCharWidth);
  args.push_back(0); //nEscapement
  args.push_back(0); //nOrientation
  args.push_back(tm.tmWeight);
  args.push_back(tm.tmItalic);
  args.push_back(tm.tmUnderlined);
  args.push_back(tm.tmStruckOut);
  args.push_back(tm.tmCharSet);
  args.push_back(OUT_DEFAULT_PRECIS);
  args.push_back(CLIP_DEFAULT_PRECIS);
  args.push_back(DEFAULT_QUALITY);
  args.push_back(tm.tmPitchAndFamily);
}

void fillFontParameters(HDC hdc,
                        DWORD first,
                        DWORD count,
                        DWORD listBase,
                        CFunction::TId api,
                        std::string& str,
                        std::vector<int>& args) {
  getCurrentFontInfo(hdc, str, args);
  args.push_back(first);
  args.push_back(count);
  args.push_back(listBase);

  if (api == CFunction::ID_WGL_USE_FONT_BITMAPS_A || api == CFunction::ID_WGL_USE_FONT_OUTLINES_A) {
    args.push_back(1); //is narrow
  } else {
    args.push_back(0); //is wide char version
  }

  if (api == CFunction::ID_WGL_USE_FONT_BITMAPS_A || api == CFunction::ID_WGL_USE_FONT_BITMAPS_W) {
    args.push_back(1); //is bitmapped version
  } else {
    args.push_back(0); //is outline
  }
}
} // namespace detail

void CRecorderWrapper::wglUseFontBitmapsA(
    bool return_value, HDC hdc, DWORD first, DWORD count, DWORD listBase) const {
  std::string str;
  std::vector<int> args;
  detail::getCurrentFontInfo(hdc, str, args);

  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglUseFontBitmapsA(return_value, hdc, first, count, listBase,
                                             std::move(str), std::move(args)),
                     true);
}

void CRecorderWrapper::wglUseFontBitmapsW(
    bool return_value, HDC hdc, DWORD first, DWORD count, DWORD listBase) const {
  std::string str;
  std::vector<int> args;
  detail::getCurrentFontInfo(hdc, str, args);

  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglUseFontBitmapsW(return_value, hdc, first, count, listBase,
                                             std::move(str), std::move(args)),
                     true);
}

void CRecorderWrapper::wglUseFontOutlinesA(bool return_value,
                                           HDC hdc,
                                           DWORD first,
                                           DWORD count,
                                           DWORD listBase,
                                           float deviation,
                                           float extrusion,
                                           int format,
                                           LPGLYPHMETRICSFLOAT lpgmf) const {
  std::string str;
  std::vector<int> args;
  detail::getCurrentFontInfo(hdc, str, args);

  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglUseFontOutlinesA(return_value, hdc, first, count, listBase, deviation,
                                              extrusion, format, lpgmf, std::move(str),
                                              std::move(args)),
                     true);
}

void CRecorderWrapper::wglUseFontOutlinesW(bool return_value,
                                           HDC hdc,
                                           DWORD first,
                                           DWORD count,
                                           DWORD listBase,
                                           float deviation,
                                           float extrusion,
                                           int format,
                                           LPGLYPHMETRICSFLOAT lpgmf) const {
  std::string str;
  std::vector<int> args;
  detail::getCurrentFontInfo(hdc, str, args);

  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglUseFontOutlinesW(return_value, hdc, first, count, listBase, deviation,
                                              extrusion, format, lpgmf, std::move(str),
                                              std::move(args)),
                     true);
}

void CRecorderWrapper::wglShareLists(bool return_value, HGLRC hglrc1, HGLRC hglrc2) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglShareLists(return_value, hglrc1, hglrc2), true);
}

void CRecorderWrapper::wglGetCurrentDC(HDC return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetCurrentDC(return_value));
}

void CRecorderWrapper::wglCopyContext(BOOL return_value,
                                      HGLRC hglrcSrc,
                                      HGLRC hglrcDst,
                                      UINT mask) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCopyContext(return_value, hglrcSrc, hglrcDst, mask), true);
}

void CRecorderWrapper::wglGetProcAddress(LPCSTR lpszProc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetProcAddress(lpszProc));
}

void CRecorderWrapper::wglGetDefaultProcAddress(LPCSTR lpszProc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetDefaultProcAddress(lpszProc));
}

void CRecorderWrapper::wglGetLayerPaletteEntries(
    HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr));
}

void CRecorderWrapper::wglCreateBufferRegionARB(HANDLE return_value,
                                                HDC hDC,
                                                int iLayerPlane,
                                                UINT uType) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateBufferRegionARB(return_value, hDC, iLayerPlane, uType), true);
}

void CRecorderWrapper::wglDeleteBufferRegionARB(HANDLE hRegion) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDeleteBufferRegionARB(hRegion), true);
}

void CRecorderWrapper::wglSaveBufferRegionARB(
    BOOL return_value, HANDLE hRegion, int x, int y, int width, int height) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSaveBufferRegionARB(return_value, hRegion, x, y, width, height), true);
}

void CRecorderWrapper::wglRestoreBufferRegionARB(BOOL return_value,
                                                 HANDLE hRegion,
                                                 int x,
                                                 int y,
                                                 int width,
                                                 int height,
                                                 int xSrc,
                                                 int ySrc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglRestoreBufferRegionARB(return_value, hRegion, x, y, width, height, xSrc, ySrc));
}

void CRecorderWrapper::wglGetExtensionsStringARB(const char* return_value, HDC hdc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetExtensionsStringARB(return_value, hdc));
}

void CRecorderWrapper::wglGetPixelFormatAttribivARB(BOOL return_value,
                                                    HDC hdc,
                                                    int iPixelFormat,
                                                    int iLayerPlane,
                                                    UINT nAttributes,
                                                    const int* piAttributes,
                                                    int* piValues) const {
  GITS_REC_ENTRY_WGL
  //_recorder.Schedule(new CwglGetPixelFormatAttribivARB(return_value, hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues));
}

void CRecorderWrapper::wglGetPixelFormatAttribfvARB(BOOL return_value,
                                                    HDC hdc,
                                                    int iPixelFormat,
                                                    int iLayerPlane,
                                                    UINT nAttributes,
                                                    const int* piAttributes,
                                                    FLOAT* pfValues) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetPixelFormatAttribfvARB(return_value, hdc, iPixelFormat, iLayerPlane,
                                                       nAttributes, piAttributes, pfValues));
}

void CRecorderWrapper::wglChoosePixelFormatARB(BOOL return_value,
                                               HDC hdc,
                                               const int* piAttribIList,
                                               const FLOAT* pfAttribFList,
                                               UINT nMaxFormats,
                                               int* piFormats,
                                               UINT* nNumFormats) const {}

void CRecorderWrapper::wglMakeContextCurrentARB(BOOL return_value,
                                                HDC hDrawDC,
                                                HDC hReadDC,
                                                HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglMakeContextCurrentARB(return_value, hDrawDC, hReadDC, hglrc), true);
}

void CRecorderWrapper::wglGetCurrentReadDCARB(HDC return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetCurrentReadDCARB(return_value));
}

void CRecorderWrapper::wglCreatePbufferARB(HPBUFFERARB return_value,
                                           HDC hDC,
                                           int iPixelFormat,
                                           int iWidth,
                                           int iHeight,
                                           const int* piAttribList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglCreatePbufferARB(return_value, hDC, iPixelFormat, iWidth, iHeight, piAttribList),
      true);
}

void CRecorderWrapper::wglGetPbufferDCARB(HDC return_value, HPBUFFERARB hPbuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetPbufferDCARB(return_value, hPbuffer), true);
}

void CRecorderWrapper::wglReleasePbufferDCARB(int return_value,
                                              HPBUFFERARB hPbuffer,
                                              HDC hDC) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleasePbufferDCARB(return_value, hPbuffer, hDC), true);
}

void CRecorderWrapper::wglDestroyPbufferARB(BOOL return_value, HPBUFFERARB hPbuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDestroyPbufferARB(return_value, hPbuffer), true);
}

void CRecorderWrapper::wglQueryPbufferARB(BOOL return_value,
                                          HPBUFFERARB hPbuffer,
                                          int iAttribute,
                                          int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQueryPbufferARB(return_value, hPbuffer, iAttribute, piValue));
}

void CRecorderWrapper::wglBindTexImageARB(BOOL return_value,
                                          HPBUFFERARB hPbuffer,
                                          int iBuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBindTexImageARB(return_value, hPbuffer, iBuffer), true);
}

void CRecorderWrapper::wglReleaseTexImageARB(BOOL return_value,
                                             HPBUFFERARB hPbuffer,
                                             int iBuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleaseTexImageARB(return_value, hPbuffer, iBuffer), true);
}

void CRecorderWrapper::wglSetPbufferAttribARB(BOOL return_value,
                                              HPBUFFERARB hPbuffer,
                                              const int* piAttribList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSetPbufferAttribARB(return_value, hPbuffer, piAttribList), true);
}

void CRecorderWrapper::wglCreateContextAttribsARB(HGLRC return_value,
                                                  HDC hDC,
                                                  HGLRC hShareContext,
                                                  const int* attribList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateContextAttribsARB(return_value, hDC, hShareContext, attribList),
                     true);
}

void CRecorderWrapper::wglCreateDisplayColorTableEXT(GLboolean return_value, GLushort id) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateDisplayColorTableEXT(return_value, id), true);
}

void CRecorderWrapper::wglLoadDisplayColorTableEXT(GLboolean return_value,
                                                   const GLushort* table,
                                                   GLuint length) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglLoadDisplayColorTableEXT(return_value, table, length), true);
}

void CRecorderWrapper::wglBindDisplayColorTableEXT(GLboolean return_value, GLushort id) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBindDisplayColorTableEXT(return_value, id), true);
}

void CRecorderWrapper::wglDestroyDisplayColorTableEXT(GLushort id) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDestroyDisplayColorTableEXT(id), true);
}

void CRecorderWrapper::wglGetExtensionsStringEXT(const char* return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetExtensionsStringEXT(return_value));
}

void CRecorderWrapper::wglMakeContextCurrentEXT(BOOL return_value,
                                                HDC hDrawDC,
                                                HDC hReadDC,
                                                HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglMakeContextCurrentEXT(return_value, hDrawDC, hReadDC, hglrc), true);
}

void CRecorderWrapper::wglGetCurrentReadDCEXT(HDC return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetCurrentReadDCEXT(return_value), true);
}

void CRecorderWrapper::wglCreatePbufferEXT(HPBUFFEREXT return_value,
                                           HDC hDC,
                                           int iPixelFormat,
                                           int iWidth,
                                           int iHeight,
                                           const int* piAttribList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglCreatePbufferEXT(return_value, hDC, iPixelFormat, iWidth, iHeight, piAttribList),
      true);
}

void CRecorderWrapper::wglGetPbufferDCEXT(HDC return_value, HPBUFFEREXT hPbuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetPbufferDCEXT(return_value, hPbuffer), true);
}

void CRecorderWrapper::wglReleasePbufferDCEXT(int return_value,
                                              HPBUFFEREXT hPbuffer,
                                              HDC hDC) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleasePbufferDCEXT(return_value, hPbuffer, hDC), true);
}

void CRecorderWrapper::wglDestroyPbufferEXT(BOOL return_value, HPBUFFEREXT hPbuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDestroyPbufferEXT(return_value, hPbuffer), true);
}

void CRecorderWrapper::wglQueryPbufferEXT(BOOL return_value,
                                          HPBUFFEREXT hPbuffer,
                                          int iAttribute,
                                          int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQueryPbufferEXT(return_value, hPbuffer, iAttribute, piValue));
}

void CRecorderWrapper::wglGetPixelFormatAttribivEXT(BOOL return_value,
                                                    HDC hdc,
                                                    int iPixelFormat,
                                                    int iLayerPlane,
                                                    UINT nAttributes,
                                                    int* piAttributes,
                                                    int* piValues) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetPixelFormatAttribivEXT(return_value, hdc, iPixelFormat, iLayerPlane,
                                                       nAttributes, piAttributes, piValues));
}

void CRecorderWrapper::wglGetPixelFormatAttribfvEXT(BOOL return_value,
                                                    HDC hdc,
                                                    int iPixelFormat,
                                                    int iLayerPlane,
                                                    UINT nAttributes,
                                                    int* piAttributes,
                                                    FLOAT* pfValues) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetPixelFormatAttribfvEXT(return_value, hdc, iPixelFormat, iLayerPlane,
                                                       nAttributes, piAttributes, pfValues));
}

void CRecorderWrapper::wglChoosePixelFormatEXT(BOOL return_value,
                                               HDC hdc,
                                               const int* piAttribIList,
                                               const FLOAT* pfAttribFList,
                                               UINT nMaxFormats,
                                               int* piFormats,
                                               UINT* nNumFormats) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglChoosePixelFormatEXT(return_value, hdc, piAttribIList, pfAttribFList,
                                                  nMaxFormats, piFormats, nNumFormats));
}

void CRecorderWrapper::wglSwapIntervalEXT(BOOL return_value, int interval) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSwapIntervalEXT(return_value, interval), true);
}

void CRecorderWrapper::wglGetSwapIntervalEXT(int return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetSwapIntervalEXT(return_value), true);
}

void CRecorderWrapper::wglAllocateMemoryNV(
    void* return_value, GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglAllocateMemoryNV(return_value, size, readfreq, writefreq, priority),
                     true);
}

void CRecorderWrapper::wglFreeMemoryNV(void* pointer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglFreeMemoryNV(pointer), true);
}

void CRecorderWrapper::wglGetSyncValuesOML(
    BOOL return_value, HDC hdc, INT64* ust, INT64* msc, INT64* sbc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetSyncValuesOML(return_value, hdc, ust, msc, sbc), true);
}

void CRecorderWrapper::wglGetMscRateOML(BOOL return_value,
                                        HDC hdc,
                                        INT32* numerator,
                                        INT32* denominator) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetMscRateOML(return_value, hdc, numerator, denominator), true);
}

void CRecorderWrapper::wglSwapBuffersMscOML(
    INT64 return_value, HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSwapBuffersMscOML(return_value, hdc, target_msc, divisor, remainder),
                     true);
}

void CRecorderWrapper::wglSwapLayerBuffersMscOML(INT64 return_value,
                                                 HDC hdc,
                                                 int fuPlanes,
                                                 INT64 target_msc,
                                                 INT64 divisor,
                                                 INT64 remainder) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglSwapLayerBuffersMscOML(return_value, hdc, fuPlanes, target_msc, divisor, remainder),
      true);
}

void CRecorderWrapper::wglWaitForMscOML(BOOL return_value,
                                        HDC hdc,
                                        INT64 target_msc,
                                        INT64 divisor,
                                        INT64 remainder,
                                        INT64* ust,
                                        INT64* msc,
                                        INT64* sbc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglWaitForMscOML(return_value, hdc, target_msc, divisor, remainder, ust, msc, sbc),
      true);
}

void CRecorderWrapper::wglWaitForSbcOML(
    BOOL return_value, HDC hdc, INT64 target_sbc, INT64* ust, INT64* msc, INT64* sbc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglWaitForSbcOML(return_value, hdc, target_sbc, ust, msc, sbc), true);
}

void CRecorderWrapper::wglGetDigitalVideoParametersI3D(BOOL return_value,
                                                       HDC hDC,
                                                       int iAttribute,
                                                       int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetDigitalVideoParametersI3D(return_value, hDC, iAttribute, piValue),
                     true);
}

void CRecorderWrapper::wglSetDigitalVideoParametersI3D(BOOL return_value,
                                                       HDC hDC,
                                                       int iAttribute,
                                                       const int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSetDigitalVideoParametersI3D(return_value, hDC, iAttribute, piValue),
                     true);
}

void CRecorderWrapper::wglGetGammaTableParametersI3D(BOOL return_value,
                                                     HDC hDC,
                                                     int iAttribute,
                                                     int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGammaTableParametersI3D(return_value, hDC, iAttribute, piValue),
                     true);
}

void CRecorderWrapper::wglSetGammaTableParametersI3D(BOOL return_value,
                                                     HDC hDC,
                                                     int iAttribute,
                                                     const int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSetGammaTableParametersI3D(return_value, hDC, iAttribute, piValue),
                     true);
}

void CRecorderWrapper::wglGetGammaTableI3D(BOOL return_value,
                                           HDC hDC,
                                           int iEntries,
                                           USHORT* puRed,
                                           USHORT* puGreen,
                                           USHORT* puBlue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGammaTableI3D(return_value, hDC, iEntries, puRed, puGreen, puBlue),
                     true);
}

void CRecorderWrapper::wglSetGammaTableI3D(BOOL return_value,
                                           HDC hDC,
                                           int iEntries,
                                           const USHORT* puRed,
                                           const USHORT* puGreen,
                                           const USHORT* puBlue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSetGammaTableI3D(return_value, hDC, iEntries, puRed, puGreen, puBlue),
                     true);
}

void CRecorderWrapper::wglEnableGenlockI3D(BOOL return_value, HDC hDC) const {
  GITS_REC_ENTRY_WGL
  if (Record()) {
    _recorder.Schedule(new CwglEnableGenlockI3D(return_value, hDC), true);
  }
}

void CRecorderWrapper::wglDisableGenlockI3D(BOOL return_value, HDC hDC) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDisableGenlockI3D(return_value, hDC), true);
}

void CRecorderWrapper::wglIsEnabledGenlockI3D(BOOL return_value, HDC hDC, BOOL* pFlag) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglIsEnabledGenlockI3D(return_value, hDC, pFlag), true);
}

void CRecorderWrapper::wglGenlockSourceI3D(BOOL return_value, HDC hDC, UINT uSource) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGenlockSourceI3D(return_value, hDC, uSource), true);
}

void CRecorderWrapper::wglGetGenlockSourceI3D(BOOL return_value, HDC hDC, UINT* uSource) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGenlockSourceI3D(return_value, hDC, uSource), true);
}

void CRecorderWrapper::wglGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT uEdge) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGenlockSourceEdgeI3D(return_value, hDC, uEdge), true);
}

void CRecorderWrapper::wglGetGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT* uEdge) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGenlockSourceEdgeI3D(return_value, hDC, uEdge), true);
}

void CRecorderWrapper::wglGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT uRate) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGenlockSampleRateI3D(return_value, hDC, uRate), true);
}

void CRecorderWrapper::wglGetGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT* uRate) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGenlockSampleRateI3D(return_value, hDC, uRate), true);
}

void CRecorderWrapper::wglGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT uDelay) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGenlockSourceDelayI3D(return_value, hDC, uDelay), true);
}

void CRecorderWrapper::wglGetGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT* uDelay) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGenlockSourceDelayI3D(return_value, hDC, uDelay), true);
}

void CRecorderWrapper::wglQueryGenlockMaxSourceDelayI3D(BOOL return_value,
                                                        HDC hDC,
                                                        UINT* uMaxLineDelay,
                                                        UINT* uMaxPixelDelay) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglQueryGenlockMaxSourceDelayI3D(return_value, hDC, uMaxLineDelay, uMaxPixelDelay),
      true);
}

void CRecorderWrapper::wglCreateImageBufferI3D(LPVOID return_value,
                                               HDC hDC,
                                               DWORD dwSize,
                                               UINT uFlags) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateImageBufferI3D(return_value, hDC, dwSize, uFlags), true);
}

void CRecorderWrapper::wglDestroyImageBufferI3D(BOOL return_value, HDC hDC, LPVOID pAddress) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDestroyImageBufferI3D(return_value, hDC, pAddress), true);
}

void CRecorderWrapper::wglAssociateImageBufferEventsI3D(BOOL return_value,
                                                        HDC hDC,
                                                        const HANDLE* pEvent,
                                                        const LPVOID* pAddress,
                                                        const DWORD* pSize,
                                                        UINT count) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglAssociateImageBufferEventsI3D(return_value, hDC, pEvent, pAddress, pSize, count),
      true);
}

void CRecorderWrapper::wglReleaseImageBufferEventsI3D(BOOL return_value,
                                                      HDC hDC,
                                                      const LPVOID* pAddress,
                                                      UINT count) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleaseImageBufferEventsI3D(return_value, hDC, pAddress, count), true);
}

void CRecorderWrapper::wglEnableFrameLockI3D(BOOL return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEnableFrameLockI3D(return_value), true);
}

void CRecorderWrapper::wglDisableFrameLockI3D(BOOL return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDisableFrameLockI3D(return_value), true);
}

void CRecorderWrapper::wglIsEnabledFrameLockI3D(BOOL return_value, BOOL* pFlag) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglIsEnabledFrameLockI3D(return_value, pFlag), true);
}

void CRecorderWrapper::wglQueryFrameLockMasterI3D(BOOL return_value, BOOL* pFlag) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQueryFrameLockMasterI3D(return_value, pFlag), true);
}

void CRecorderWrapper::wglGetFrameUsageI3D(BOOL return_value, float* pUsage) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetFrameUsageI3D(return_value, pUsage), true);
}

void CRecorderWrapper::wglBeginFrameTrackingI3D(BOOL return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBeginFrameTrackingI3D(return_value), true);
}

void CRecorderWrapper::wglEndFrameTrackingI3D(BOOL return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEndFrameTrackingI3D(return_value), true);
}

void CRecorderWrapper::wglQueryFrameTrackingI3D(BOOL return_value,
                                                DWORD* pFrameCount,
                                                DWORD* pMissedFrames,
                                                float* pLastMissedUsage) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglQueryFrameTrackingI3D(return_value, pFrameCount, pMissedFrames, pLastMissedUsage),
      true);
}

void CRecorderWrapper::wglSetStereoEmitterState3DL(BOOL return_value, HDC hDC, UINT uState) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglSetStereoEmitterState3DL(return_value, hDC, uState), true);
}

void CRecorderWrapper::wglEnumerateVideoDevicesNV(int return_value,
                                                  HDC hDC,
                                                  HVIDEOOUTPUTDEVICENV* phDeviceList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEnumerateVideoDevicesNV(return_value, hDC, phDeviceList), true);
}

void CRecorderWrapper::wglBindVideoDeviceNV(BOOL return_value,
                                            HDC hDC,
                                            unsigned uVideoSlot,
                                            HVIDEOOUTPUTDEVICENV hVideoDevice,
                                            const int* piAttribList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglBindVideoDeviceNV(return_value, hDC, uVideoSlot, hVideoDevice, piAttribList), true);
}

void CRecorderWrapper::wglQueryCurrentContextNV(BOOL return_value,
                                                int iAttribute,
                                                int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQueryCurrentContextNV(return_value, iAttribute, piValue), true);
}

void CRecorderWrapper::wglGetVideoDeviceNV(BOOL return_value,
                                           HDC hDC,
                                           int numDevices,
                                           HPVIDEODEV* hVideoDevice) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetVideoDeviceNV(return_value, hDC, numDevices, hVideoDevice), true);
}

void CRecorderWrapper::wglReleaseVideoDeviceNV(BOOL return_value, HPVIDEODEV hVideoDevice) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleaseVideoDeviceNV(return_value, hVideoDevice), true);
}

void CRecorderWrapper::wglBindVideoImageNV(BOOL return_value,
                                           HPVIDEODEV hVideoDevice,
                                           HPBUFFERARB hPbuffer,
                                           int iVideoBuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBindVideoImageNV(return_value, hVideoDevice, hPbuffer, iVideoBuffer),
                     true);
}

void CRecorderWrapper::wglReleaseVideoImageNV(BOOL return_value,
                                              HPBUFFERARB hPbuffer,
                                              int iVideoBuffer) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleaseVideoImageNV(return_value, hPbuffer, iVideoBuffer), true);
}

void CRecorderWrapper::wglSendPbufferToVideoNV(BOOL return_value,
                                               HPBUFFERARB hPbuffer,
                                               int iBufferType,
                                               unsigned long* pulCounterPbuffer,
                                               BOOL bBlock) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglSendPbufferToVideoNV(return_value, hPbuffer, iBufferType, pulCounterPbuffer, bBlock),
      true);
}

void CRecorderWrapper::wglGetVideoInfoNV(BOOL return_value,
                                         HPVIDEODEV hpVideoDevice,
                                         unsigned long* pulCounterOutputPbuffer,
                                         unsigned long* pulCounterOutputVideo) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetVideoInfoNV(return_value, hpVideoDevice, pulCounterOutputPbuffer,
                                            pulCounterOutputVideo),
                     true);
}

void CRecorderWrapper::wglJoinSwapGroupNV(BOOL return_value, HDC hDC, GLuint group) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglJoinSwapGroupNV(return_value, hDC, group), true);
}

void CRecorderWrapper::wglBindSwapBarrierNV(BOOL return_value, GLuint group, GLuint barrier) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBindSwapBarrierNV(return_value, group, barrier), true);
}

void CRecorderWrapper::wglQuerySwapGroupNV(BOOL return_value,
                                           HDC hDC,
                                           GLuint* group,
                                           GLuint* barrier) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQuerySwapGroupNV(return_value, hDC, group, barrier), true);
}

void CRecorderWrapper::wglQueryMaxSwapGroupsNV(BOOL return_value,
                                               HDC hDC,
                                               GLuint* maxGroups,
                                               GLuint* maxBarriers) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQueryMaxSwapGroupsNV(return_value, hDC, maxGroups, maxBarriers), true);
}

void CRecorderWrapper::wglQueryFrameCountNV(BOOL return_value, HDC hDC, GLuint* count) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglQueryFrameCountNV(return_value, hDC, count), true);
}

void CRecorderWrapper::wglResetFrameCountNV(BOOL return_value, HDC hDC) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglResetFrameCountNV(return_value, hDC), true);
}

void CRecorderWrapper::wglEnumGpusNV(BOOL return_value, UINT iGpuIndex, HGPUNV* phGpu) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEnumGpusNV(return_value, iGpuIndex, phGpu), true);
}

void CRecorderWrapper::wglEnumGpuDevicesNV(BOOL return_value,
                                           HGPUNV hGpu,
                                           UINT iDeviceIndex,
                                           PGPU_DEVICE lpGpuDevice) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEnumGpuDevicesNV(return_value, hGpu, iDeviceIndex, lpGpuDevice), true);
}

void CRecorderWrapper::wglCreateAffinityDCNV(HDC return_value, const HGPUNV* phGpuList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateAffinityDCNV(return_value, phGpuList), true);
}

void CRecorderWrapper::wglEnumGpusFromAffinityDCNV(BOOL return_value,
                                                   HDC hAffinityDC,
                                                   UINT iGpuIndex,
                                                   HGPUNV* hGpu) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEnumGpusFromAffinityDCNV(return_value, hAffinityDC, iGpuIndex, hGpu),
                     true);
}

void CRecorderWrapper::wglDeleteDCNV(BOOL return_value, HDC hdc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDeleteDCNV(return_value, hdc), true);
}

void CRecorderWrapper::wglGetGPUIDsAMD(UINT return_value, UINT maxCount, UINT* ids) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGPUIDsAMD(return_value, maxCount, ids), true);
}

void CRecorderWrapper::wglGetGPUInfoAMD(
    INT return_value, UINT id, int property, GLenum dataType, UINT size, void* data) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetGPUInfoAMD(return_value, id, property, dataType, size, data), true);
}

void CRecorderWrapper::wglGetContextGPUIDAMD(UINT return_value, HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetContextGPUIDAMD(return_value, hglrc), true);
}

void CRecorderWrapper::wglCreateAssociatedContextAMD(HGLRC return_value, UINT id) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCreateAssociatedContextAMD(return_value, id), true);
}

void CRecorderWrapper::wglCreateAssociatedContextAttribsAMD(HGLRC return_value,
                                                            UINT id,
                                                            HGLRC hShareContext,
                                                            const int* attribList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglCreateAssociatedContextAttribsAMD(return_value, id, hShareContext, attribList), true);
}

void CRecorderWrapper::wglDeleteAssociatedContextAMD(BOOL return_value, HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglDeleteAssociatedContextAMD(return_value, hglrc), true);
}

void CRecorderWrapper::wglMakeAssociatedContextCurrentAMD(BOOL return_value, HGLRC hglrc) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglMakeAssociatedContextCurrentAMD(return_value, hglrc), true);
}

void CRecorderWrapper::wglGetCurrentAssociatedContextAMD(HGLRC return_value) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglGetCurrentAssociatedContextAMD(return_value), true);
}

void CRecorderWrapper::wglBlitContextFramebufferAMD(HGLRC dstCtx,
                                                    GLint srcX0,
                                                    GLint srcY0,
                                                    GLint srcX1,
                                                    GLint srcY1,
                                                    GLint dstX0,
                                                    GLint dstY0,
                                                    GLint dstX1,
                                                    GLint dstY1,
                                                    GLbitfield mask,
                                                    GLenum filter) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBlitContextFramebufferAMD(dstCtx, srcX0, srcY0, srcX1, srcY1, dstX0,
                                                       dstY0, dstX1, dstY1, mask, filter),
                     true);
}

void CRecorderWrapper::wglBindVideoCaptureDeviceNV(BOOL return_value,
                                                   UINT uVideoSlot,
                                                   HVIDEOINPUTDEVICENV hDevice) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglBindVideoCaptureDeviceNV(return_value, uVideoSlot, hDevice), true);
}

void CRecorderWrapper::wglEnumerateVideoCaptureDevicesNV(UINT return_value,
                                                         HDC hDc,
                                                         HVIDEOINPUTDEVICENV* phDeviceList) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglEnumerateVideoCaptureDevicesNV(return_value, hDc, phDeviceList));
}

void CRecorderWrapper::wglLockVideoCaptureDeviceNV(BOOL return_value,
                                                   HDC hDc,
                                                   HVIDEOINPUTDEVICENV hDevice) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglLockVideoCaptureDeviceNV(return_value, hDc, hDevice));
}

void CRecorderWrapper::wglQueryVideoCaptureDeviceNV(
    BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice, int iAttribute, int* piValue) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(
      new CwglQueryVideoCaptureDeviceNV(return_value, hDc, hDevice, iAttribute, piValue), true);
}

void CRecorderWrapper::wglReleaseVideoCaptureDeviceNV(BOOL return_value,
                                                      HDC hDc,
                                                      HVIDEOINPUTDEVICENV hDevice) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglReleaseVideoCaptureDeviceNV(return_value, hDc, hDevice), true);
}

void CRecorderWrapper::wglCopyImageSubDataNV(BOOL return_value,
                                             HGLRC hSrcRC,
                                             GLuint srcName,
                                             GLenum srcTarget,
                                             GLint srcLevel,
                                             GLint srcX,
                                             GLint srcY,
                                             GLint srcZ,
                                             HGLRC hDstRC,
                                             GLuint dstName,
                                             GLenum dstTarget,
                                             GLint dstLevel,
                                             GLint dstX,
                                             GLint dstY,
                                             GLint dstZ,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth) const {
  GITS_REC_ENTRY_WGL
  _recorder.Schedule(new CwglCopyImageSubDataNV(return_value, hSrcRC, srcName, srcTarget, srcLevel,
                                                srcX, srcY, srcZ, hDstRC, dstName, dstTarget,
                                                dstLevel, dstX, dstY, dstZ, width, height, depth),
                     true);
}

#endif

void CRecorderWrapper::glXChooseVisual(XVisualInfo* return_value,
                                       Display* dpy,
                                       int screen,
                                       int* attribList) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXChooseVisual(return_value, dpy, screen, attribList), true);
}

void CRecorderWrapper::glXCreateContext(GLXContext return_value,
                                        Display* dpy,
                                        XVisualInfo* vis,
                                        GLXContext shareList,
                                        Bool direct) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCreateContext(return_value, dpy, vis, shareList, direct), true);
}

void CRecorderWrapper::glXDestroyContext(Display* dpy, GLXContext ctx) const {
  GITS_REC_ENTRY_GLX

  SD().RemoveContext((void*)ctx);
  _recorder.Schedule(new CglXDestroyContext(dpy, ctx), true);

  static uint32_t deleted_context = 0;
  if (++deleted_context == Configurator::Get().opengl.recorder.all.exitDeleteContext) {
    _recorder.Close();
  }
}

void CRecorderWrapper::glXMakeCurrent(Bool return_value,
                                      Display* dpy,
                                      GLXDrawable drawable,
                                      GLXContext ctx) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXMakeCurrent(return_value, dpy, drawable, ctx), true);
}

void CRecorderWrapper::glXCopyContext(Display* dpy,
                                      GLXContext src,
                                      GLXContext dst,
                                      unsigned long mask) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCopyContext(dpy, src, dst, mask), true);
}

void CRecorderWrapper::glXSwapBuffers(Display* dpy, GLXDrawable drawable) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXSwapBuffers(dpy, drawable));
  _recorder.FrameEnd();
}

void CRecorderWrapper::glXCreateGLXPixmap(GLXPixmap return_value,
                                          Display* dpy,
                                          XVisualInfo* visual,
                                          Pixmap pixmap) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCreateGLXPixmap(return_value, dpy, visual, pixmap), true);
}

void CRecorderWrapper::glXDestroyGLXPixmap(Display* dpy, GLXPixmap pixmap) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXDestroyGLXPixmap(dpy, pixmap), true);
}

void CRecorderWrapper::glXQueryExtension(Bool return_value,
                                         Display* dpy,
                                         int* errorb,
                                         int* event) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryExtension(return_value, dpy, errorb, event));
}

void CRecorderWrapper::glXQueryVersion(Bool return_value, Display* dpy, int* maj, int* min) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryVersion(return_value, dpy, maj, min));
}

void CRecorderWrapper::glXIsDirect(Bool return_value, Display* dpy, GLXContext ctx) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXIsDirect(return_value, dpy, ctx));
}

void CRecorderWrapper::glXGetConfig(
    int return_value, Display* dpy, XVisualInfo* visual, int attrib, int* value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetConfig(return_value, dpy, visual, attrib, value), true);
}

void CRecorderWrapper::glXGetCurrentContext(GLXContext return_value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetCurrentContext(return_value));
}

void CRecorderWrapper::glXGetCurrentDrawable(GLXDrawable return_value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetCurrentDrawable(return_value));
}

void CRecorderWrapper::glXWaitGL() const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXWaitGL());
}

void CRecorderWrapper::glXWaitX() const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXWaitX());
}

void CRecorderWrapper::glXUseXFont(Font font, int first, int count, int list) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXUseXFont(font, first, count, list), true);
}

void CRecorderWrapper::glXQueryExtensionsString(const char* return_value,
                                                Display* dpy,
                                                int screen) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryExtensionsString(return_value, dpy, screen));
}

void CRecorderWrapper::glXQueryServerString(const char* return_value,
                                            Display* dpy,
                                            int screen,
                                            int name) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryServerString(return_value, dpy, screen, name));
}

void CRecorderWrapper::glXGetClientString(const char* return_value, Display* dpy, int name) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetClientString(return_value, dpy, name));
}

void CRecorderWrapper::glXGetCurrentDisplay(Display* return_value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetCurrentDisplay(return_value));
}

void CRecorderWrapper::glXChooseFBConfig(
    GLXFBConfig* return_value, Display* dpy, int screen, const int* attribList, int* nitems) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXChooseFBConfig(return_value, dpy, screen, attribList, nitems));
}

void CRecorderWrapper::glXGetFBConfigAttrib(
    int return_value, Display* dpy, GLXFBConfig config, int attribute, int* value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetFBConfigAttrib(return_value, dpy, config, attribute, value));
}

void CRecorderWrapper::glXGetFBConfigs(GLXFBConfig* return_value,
                                       Display* dpy,
                                       int screen,
                                       int* nelements) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetFBConfigs(return_value, dpy, screen, nelements));
}

void CRecorderWrapper::glXGetVisualFromFBConfig(XVisualInfo* return_value,
                                                Display* dpy,
                                                GLXFBConfig config) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetVisualFromFBConfig(return_value, dpy, config));
}

void CRecorderWrapper::glXCreateWindow(GLXWindow return_value,
                                       Display* dpy,
                                       GLXFBConfig config,
                                       Window win,
                                       const int* attribList) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCreateWindow(return_value, dpy, config, win, attribList), true);
}

void CRecorderWrapper::glXDestroyWindow(Display* dpy, GLXWindow window) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXDestroyWindow(dpy, window), true);
}

void CRecorderWrapper::glXCreatePixmap(GLXPixmap return_value,
                                       Display* dpy,
                                       GLXFBConfig config,
                                       Pixmap pixmap,
                                       const int* attribList) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCreatePixmap(return_value, dpy, config, pixmap, attribList), true);
}

void CRecorderWrapper::glXDestroyPixmap(Display* dpy, GLXPixmap pixmap) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXDestroyPixmap(dpy, pixmap), true);
}

void CRecorderWrapper::glXCreatePbuffer(GLXPbuffer return_value,
                                        Display* dpy,
                                        GLXFBConfig config,
                                        const int* attribList) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCreatePbuffer(return_value, dpy, config, attribList), true);
}

void CRecorderWrapper::glXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXDestroyPbuffer(dpy, pbuf), true);
}

void CRecorderWrapper::glXQueryDrawable(Display* dpy,
                                        GLXDrawable draw,
                                        int attribute,
                                        unsigned int* value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryDrawable(dpy, draw, attribute, value));
}

void CRecorderWrapper::glXCreateNewContext(GLXContext return_value,
                                           Display* dpy,
                                           GLXFBConfig config,
                                           int renderType,
                                           GLXContext shareList,
                                           Bool direct) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(
      new CglXCreateNewContext(return_value, dpy, config, renderType, shareList, direct), true);
}

void CRecorderWrapper::glXMakeContextCurrent(
    Bool return_value, Display* dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXMakeContextCurrent(return_value, dpy, draw, read, ctx), true);
}

void CRecorderWrapper::glXGetCurrentReadDrawable(GLXDrawable return_value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetCurrentReadDrawable(return_value));
}

void CRecorderWrapper::glXQueryContext(
    int return_value, Display* dpy, GLXContext ctx, int attribute, int* value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryContext(return_value, dpy, ctx, attribute, value));
}

void CRecorderWrapper::glXSelectEvent(Display* dpy,
                                      GLXDrawable drawable,
                                      unsigned long mask) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXSelectEvent(dpy, drawable, mask));
}

void CRecorderWrapper::glXGetSelectedEvent(Display* dpy,
                                           GLXDrawable drawable,
                                           unsigned long* mask) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetSelectedEvent(dpy, drawable, mask));
}

void CRecorderWrapper::glXGetProcAddressARB(void* return_value, const GLubyte* func_name) const {}

void CRecorderWrapper::glXGetProcAddress(void* return_value, const GLubyte* func_name) const {}

void CRecorderWrapper::glXAllocateMemoryNV(
    void* return_value, GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXAllocateMemoryNV(return_value, size, readfreq, writefreq, priority),
                     true);
}

void CRecorderWrapper::glXFreeMemoryNV(GLvoid* pointer) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXFreeMemoryNV(pointer), true);
}

void CRecorderWrapper::glXBindTexImageARB(Bool return_value,
                                          Display* dpy,
                                          GLXPbuffer pbuffer,
                                          int buffer) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXBindTexImageARB(return_value, dpy, pbuffer, buffer), true);
}

void CRecorderWrapper::glXReleaseTexImageARB(Bool return_value,
                                             Display* dpy,
                                             GLXPbuffer pbuffer,
                                             int buffer) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXReleaseTexImageARB(return_value, dpy, pbuffer, buffer), true);
}

void CRecorderWrapper::glXDrawableAttribARB(Bool return_value,
                                            Display* dpy,
                                            GLXDrawable draw,
                                            const int* attribList) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXDrawableAttribARB(return_value, dpy, draw, attribList), true);
}

void CRecorderWrapper::glXGetFrameUsageMESA(int return_value,
                                            Display* dpy,
                                            GLXDrawable drawable,
                                            float* usage) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetFrameUsageMESA(return_value, dpy, drawable, usage));
}

void CRecorderWrapper::glXBeginFrameTrackingMESA(int return_value,
                                                 Display* dpy,
                                                 GLXDrawable drawable) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXBeginFrameTrackingMESA(return_value, dpy, drawable), true);
}

void CRecorderWrapper::glXEndFrameTrackingMESA(int return_value,
                                               Display* dpy,
                                               GLXDrawable drawable) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXEndFrameTrackingMESA(return_value, dpy, drawable), true);
}

void CRecorderWrapper::glXQueryFrameTrackingMESA(int return_value,
                                                 Display* dpy,
                                                 GLXDrawable drawable,
                                                 int64_t* swapCount,
                                                 int64_t* missedFrames,
                                                 float* lastMissedUsage) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXQueryFrameTrackingMESA(return_value, dpy, drawable, swapCount,
                                                    missedFrames, lastMissedUsage));
}

void CRecorderWrapper::glXSwapIntervalMESA(int return_value, unsigned int interval) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXSwapIntervalMESA(return_value, interval), true);
}

void CRecorderWrapper::glXGetSwapIntervalMESA(int return_value) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXGetSwapIntervalMESA(return_value));
}

void CRecorderWrapper::glXBindTexImageEXT(Display* dpy,
                                          GLXDrawable drawable,
                                          int buffer,
                                          const int* attrib_list) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXBindTexImageEXT(dpy, drawable, buffer, attrib_list), true);
}

void CRecorderWrapper::glXReleaseTexImageEXT(Display* dpy, GLXDrawable drawable, int buffer) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXReleaseTexImageEXT(dpy, drawable, buffer), true);
}

void CRecorderWrapper::glXCreateContextAttribsARB(GLXContext return_value,
                                                  Display* dpy,
                                                  GLXFBConfig config,
                                                  GLXContext share_context,
                                                  Bool direct,
                                                  const int* attrib_list) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXCreateContextAttribsARB(return_value, dpy, config, share_context,
                                                     direct, attrib_list),
                     true);
}

void CRecorderWrapper::glXSwapIntervalSGI(int return_value, int interval) const {
  GITS_REC_ENTRY_GLX
  _recorder.Schedule(new CglXSwapIntervalSGI(return_value, interval), true);
}

} //namespace OpenGL
} //namespace gits
