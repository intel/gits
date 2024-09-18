// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglRecorderWrapper.h
 *
 * @brief Declaration of OpenGL recorder wrapper.
 */

#pragma once

#include "openglRecorderWrapperIface.h"
#include "tools_lite.h"
#include <vector>
#include <functional>

namespace gits {
class CRecorder;
class DrawCallWrapperPrePost : private gits::noncopyable {
private:
  CRecorder& _recorder;

public:
  DrawCallWrapperPrePost(gits::CRecorder& rec);
  DrawCallWrapperPrePost(const DrawCallWrapperPrePost& other) = delete;
  DrawCallWrapperPrePost& operator=(const DrawCallWrapperPrePost& other) = delete;
  ~DrawCallWrapperPrePost();
};
} // namespace gits
#define DRAWCALL_WRAPPER_PRE_POST DrawCallWrapperPrePost drawCallWrapperPrePost(_recorder);

#define GITS_REC_ENTRY     TrackThread();
#define GITS_REC_ENTRY_EGL GITS_REC_ENTRY
#define GITS_REC_ENTRY_WGL GITS_REC_ENTRY
#define GITS_REC_ENTRY_GLX GITS_REC_ENTRY
#define GITS_REC_ENTRY_GL  GITS_REC_ENTRY

namespace gits {

class CRecorder;

namespace OpenGL {

class CRecorderWrapper : public IRecorderWrapper {
  CRecorder& _recorder;

  //Function used to check if tokens should be recorder. Only gl functions that should run
  // differently are glGetString and glGetStringf.
  bool Record() const;

  void FrameEnd() const;
  void UpdateMappedTextures() const;
  void TrackThread() const;

public:
  CRecorderWrapper(const CRecorderWrapper& ref) = delete;
  ~CRecorderWrapper() = default;
  CRecorderWrapper& operator=(const CRecorderWrapper& ref) = delete;
  void StreamFinishedEvent(std::function<void()> e);
  CRecorderWrapper(CRecorder& recorder);

  void EndFramePost() const override;
  void CloseRecorderIfRequired() override;
  std::recursive_mutex& GetInterceptorMutex() const override;
  CDrivers& Drivers() const override;
  void PreSwap() const;

  void GLInitialize(CGlDriver::TApiType api) const override {
    drv.gl.Initialize(api);
  }
  void EGLInitialize() const override {
    drv.egl.Used(true);
  }

#include "openglRecorderWrapperAuto.h"

#if defined GITS_PLATFORM_WINDOWS
  void wglChoosePixelFormat(bool return_value,
                            HDC hdc,
                            const PIXELFORMATDESCRIPTOR* ppfd) const override;
  void wglSetPixelFormat(bool return_value,
                         HDC hdc,
                         int format,
                         const PIXELFORMATDESCRIPTOR* ppfd) const override;
  void wglCreateContext(HGLRC hglrc, HDC hdc) const override;
  void wglCreateLayerContext(HGLRC return_value, HDC hdc, int iLayerPlane) const override;
  void wglGetPixelFormat(int return_value, HDC hdc) const override;
  void wglDescribePixelFormat(int return_value,
                              HDC hdc,
                              int format,
                              unsigned nBytes,
                              PIXELFORMATDESCRIPTOR* ppfd) const override;
  void wglCopyContext(BOOL return_value, HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) const override;
  void wglMakeCurrent(bool return_value, HDC hdc, HGLRC hglrc) const override;
  void wglDeleteContext(bool retVal, HGLRC hglrc) const override;
  void wglSwapBuffers(bool return_value, HDC hdc) const override;
  void wglSwapMultipleBuffers(bool return_value, GLuint buffer, HDC* hdc) const override;
  void wglSwapLayerBuffers(bool return_value, HDC hdc, unsigned plane) const override;
  void wglUseFontBitmapsA(
      bool return_value, HDC hdc, DWORD first, DWORD count, DWORD listBase) const override;
  void wglUseFontOutlinesA(bool return_value,
                           HDC hdc,
                           DWORD first,
                           DWORD count,
                           DWORD listBase,
                           float deviation,
                           float extrusion,
                           int format,
                           LPGLYPHMETRICSFLOAT lpgmf) const override;
  void wglUseFontBitmapsW(
      bool return_value, HDC hdc, DWORD first, DWORD count, DWORD listBase) const override;
  void wglUseFontOutlinesW(bool return_value,
                           HDC hdc,
                           DWORD first,
                           DWORD count,
                           DWORD listBase,
                           float deviation,
                           float extrusion,
                           int format,
                           LPGLYPHMETRICSFLOAT lpgmf) const override;
  void wglShareLists(bool return_value, HGLRC hglrc1, HGLRC hglrc2) const override;
  void wglGetCurrentDC(HDC return_value) const override;
  void wglGetCurrentContext(HGLRC return_value) const override;
  void wglGetProcAddress(LPCSTR lpszProc) const override;
  void wglGetDefaultProcAddress(LPCSTR lpszProc) const override;
  void wglGetLayerPaletteEntries(
      HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr) const override;

  void wglCreateBufferRegionARB(HANDLE return_value,
                                HDC hDC,
                                int iLayerPlane,
                                UINT uType) const override;
  void wglDeleteBufferRegionARB(HANDLE hRegion) const override;
  void wglSaveBufferRegionARB(
      BOOL return_value, HANDLE hRegion, int x, int y, int width, int height) const override;
  void wglRestoreBufferRegionARB(BOOL return_value,
                                 HANDLE hRegion,
                                 int x,
                                 int y,
                                 int width,
                                 int height,
                                 int xSrc,
                                 int ySrc) const override;
  void wglGetExtensionsStringARB(const char* return_value, HDC hdc) const override;
  void wglGetPixelFormatAttribivARB(BOOL return_value,
                                    HDC hdc,
                                    int iPixelFormat,
                                    int iLayerPlane,
                                    UINT nAttributes,
                                    const int* piAttributes,
                                    int* piValues) const override;
  void wglGetPixelFormatAttribfvARB(BOOL return_value,
                                    HDC hdc,
                                    int iPixelFormat,
                                    int iLayerPlane,
                                    UINT nAttributes,
                                    const int* piAttributes,
                                    FLOAT* pfValues) const override;
  void wglChoosePixelFormatARB(BOOL return_value,
                               HDC hdc,
                               const int* piAttribIList,
                               const FLOAT* pfAttribFList,
                               UINT nMaxFormats,
                               int* piFormats,
                               UINT* nNumFormats) const override;
  void wglMakeContextCurrentARB(BOOL return_value,
                                HDC hDrawDC,
                                HDC hReadDC,
                                HGLRC hglrc) const override;
  void wglGetCurrentReadDCARB(HDC return_value) const override;
  void wglCreatePbufferARB(HPBUFFERARB return_value,
                           HDC hDC,
                           int iPixelFormat,
                           int iWidth,
                           int iHeight,
                           const int* piAttribList) const override;
  void wglGetPbufferDCARB(HDC return_value, HPBUFFERARB hPbuffer) const override;
  void wglReleasePbufferDCARB(int return_value, HPBUFFERARB hPbuffer, HDC hDC) const override;
  void wglDestroyPbufferARB(BOOL return_value, HPBUFFERARB hPbuffer) const override;
  void wglQueryPbufferARB(BOOL return_value,
                          HPBUFFERARB hPbuffer,
                          int iAttribute,
                          int* piValue) const override;
  void wglBindTexImageARB(BOOL return_value, HPBUFFERARB hPbuffer, int iBuffer) const override;
  void wglReleaseTexImageARB(BOOL return_value, HPBUFFERARB hPbuffer, int iBuffer) const override;
  void wglSetPbufferAttribARB(BOOL return_value,
                              HPBUFFERARB hPbuffer,
                              const int* piAttribList) const override;
  void wglCreateContextAttribsARB(HGLRC return_value,
                                  HDC hDC,
                                  HGLRC hShareContext,
                                  const int* attribList) const override;
  void wglCreateDisplayColorTableEXT(GLboolean return_value, GLushort id) const override;
  void wglLoadDisplayColorTableEXT(GLboolean return_value,
                                   const GLushort* table,
                                   GLuint length) const override;
  void wglBindDisplayColorTableEXT(GLboolean return_value, GLushort id) const override;
  void wglDestroyDisplayColorTableEXT(GLushort id) const override;
  void wglGetExtensionsStringEXT(const char* return_value) const override;
  void wglMakeContextCurrentEXT(BOOL return_value,
                                HDC hDrawDC,
                                HDC hReadDC,
                                HGLRC hglrc) const override;
  void wglGetCurrentReadDCEXT(HDC return_value) const override;
  void wglCreatePbufferEXT(HPBUFFEREXT return_value,
                           HDC hDC,
                           int iPixelFormat,
                           int iWidth,
                           int iHeight,
                           const int* piAttribList) const override;
  void wglGetPbufferDCEXT(HDC return_value, HPBUFFEREXT hPbuffer) const override;
  void wglReleasePbufferDCEXT(int return_value, HPBUFFEREXT hPbuffer, HDC hDC) const override;
  void wglDestroyPbufferEXT(BOOL return_value, HPBUFFEREXT hPbuffer) const override;
  void wglQueryPbufferEXT(BOOL return_value,
                          HPBUFFEREXT hPbuffer,
                          int iAttribute,
                          int* piValue) const override;
  void wglGetPixelFormatAttribivEXT(BOOL return_value,
                                    HDC hdc,
                                    int iPixelFormat,
                                    int iLayerPlane,
                                    UINT nAttributes,
                                    int* piAttributes,
                                    int* piValues) const override;
  void wglGetPixelFormatAttribfvEXT(BOOL return_value,
                                    HDC hdc,
                                    int iPixelFormat,
                                    int iLayerPlane,
                                    UINT nAttributes,
                                    int* piAttributes,
                                    FLOAT* pfValues) const override;
  void wglChoosePixelFormatEXT(BOOL return_value,
                               HDC hdc,
                               const int* piAttribIList,
                               const FLOAT* pfAttribFList,
                               UINT nMaxFormats,
                               int* piFormats,
                               UINT* nNumFormats) const override;
  void wglSwapIntervalEXT(BOOL return_value, int interval) const override;
  void wglGetSwapIntervalEXT(int return_value) const override;
  void wglAllocateMemoryNV(void* return_value,
                           GLsizei size,
                           GLfloat readfreq,
                           GLfloat writefreq,
                           GLfloat priority) const override;
  void wglFreeMemoryNV(void* pointer) const override;
  void wglGetSyncValuesOML(
      BOOL return_value, HDC hdc, INT64* ust, INT64* msc, INT64* sbc) const override;
  void wglGetMscRateOML(BOOL return_value,
                        HDC hdc,
                        INT32* numerator,
                        INT32* denominator) const override;
  void wglSwapBuffersMscOML(
      INT64 return_value, HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder) const override;
  void wglSwapLayerBuffersMscOML(INT64 return_value,
                                 HDC hdc,
                                 int fuPlanes,
                                 INT64 target_msc,
                                 INT64 divisor,
                                 INT64 remainder) const override;
  void wglWaitForMscOML(BOOL return_value,
                        HDC hdc,
                        INT64 target_msc,
                        INT64 divisor,
                        INT64 remainder,
                        INT64* ust,
                        INT64* msc,
                        INT64* sbc) const override;
  void wglWaitForSbcOML(BOOL return_value,
                        HDC hdc,
                        INT64 target_sbc,
                        INT64* ust,
                        INT64* msc,
                        INT64* sbc) const override;
  void wglGetDigitalVideoParametersI3D(BOOL return_value,
                                       HDC hDC,
                                       int iAttribute,
                                       int* piValue) const override;
  void wglSetDigitalVideoParametersI3D(BOOL return_value,
                                       HDC hDC,
                                       int iAttribute,
                                       const int* piValue) const override;
  void wglGetGammaTableParametersI3D(BOOL return_value,
                                     HDC hDC,
                                     int iAttribute,
                                     int* piValue) const override;
  void wglSetGammaTableParametersI3D(BOOL return_value,
                                     HDC hDC,
                                     int iAttribute,
                                     const int* piValue) const override;
  void wglGetGammaTableI3D(BOOL return_value,
                           HDC hDC,
                           int iEntries,
                           USHORT* puRed,
                           USHORT* puGreen,
                           USHORT* puBlue) const override;
  void wglSetGammaTableI3D(BOOL return_value,
                           HDC hDC,
                           int iEntries,
                           const USHORT* puRed,
                           const USHORT* puGreen,
                           const USHORT* puBlue) const override;
  void wglEnableGenlockI3D(BOOL return_value, HDC hDC) const override;
  void wglDisableGenlockI3D(BOOL return_value, HDC hDC) const override;
  void wglIsEnabledGenlockI3D(BOOL return_value, HDC hDC, BOOL* pFlag) const override;
  void wglGenlockSourceI3D(BOOL return_value, HDC hDC, UINT uSource) const override;
  void wglGetGenlockSourceI3D(BOOL return_value, HDC hDC, UINT* uSource) const override;
  void wglGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT uEdge) const override;
  void wglGetGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT* uEdge) const override;
  void wglGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT uRate) const override;
  void wglGetGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT* uRate) const override;
  void wglGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT uDelay) const override;
  void wglGetGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT* uDelay) const override;
  void wglQueryGenlockMaxSourceDelayI3D(BOOL return_value,
                                        HDC hDC,
                                        UINT* uMaxLineDelay,
                                        UINT* uMaxPixelDelay) const override;
  void wglCreateImageBufferI3D(void* return_value,
                               HDC hDC,
                               DWORD dwSize,
                               UINT uFlags) const override;
  void wglDestroyImageBufferI3D(BOOL return_value, HDC hDC, LPVOID pAddress) const override;
  void wglAssociateImageBufferEventsI3D(BOOL return_value,
                                        HDC hDC,
                                        const HANDLE* pEvent,
                                        const LPVOID* pAddress,
                                        const DWORD* pSize,
                                        UINT count) const override;
  void wglReleaseImageBufferEventsI3D(BOOL return_value,
                                      HDC hDC,
                                      const LPVOID* pAddress,
                                      UINT count) const override;
  void wglEnableFrameLockI3D(BOOL return_value) const override;
  void wglDisableFrameLockI3D(BOOL return_value) const override;
  void wglIsEnabledFrameLockI3D(BOOL return_value, BOOL* pFlag) const override;
  void wglQueryFrameLockMasterI3D(BOOL return_value, BOOL* pFlag) const override;
  void wglGetFrameUsageI3D(BOOL return_value, float* pUsage) const override;
  void wglBeginFrameTrackingI3D(BOOL return_value) const override;
  void wglEndFrameTrackingI3D(BOOL return_value) const override;
  void wglQueryFrameTrackingI3D(BOOL return_value,
                                DWORD* pFrameCount,
                                DWORD* pMissedFrames,
                                float* pLastMissedUsage) const override;
  void wglSetStereoEmitterState3DL(BOOL return_value, HDC hDC, UINT uState) const override;
  void wglEnumerateVideoDevicesNV(int return_value,
                                  HDC hDC,
                                  HVIDEOOUTPUTDEVICENV* phDeviceList) const override;
  void wglBindVideoDeviceNV(BOOL return_value,
                            HDC hDC,
                            unsigned uVideoSlot,
                            HVIDEOOUTPUTDEVICENV hVideoDevice,
                            const int* piAttribList) const override;
  void wglQueryCurrentContextNV(BOOL return_value, int iAttribute, int* piValue) const override;
  void wglGetVideoDeviceNV(BOOL return_value,
                           HDC hDC,
                           int numDevices,
                           HPVIDEODEV* hVideoDevice) const override;
  void wglReleaseVideoDeviceNV(BOOL return_value, HPVIDEODEV hVideoDevice) const override;
  void wglBindVideoImageNV(BOOL return_value,
                           HPVIDEODEV hVideoDevice,
                           HPBUFFERARB hPbuffer,
                           int iVideoBuffer) const override;
  void wglReleaseVideoImageNV(BOOL return_value,
                              HPBUFFERARB hPbuffer,
                              int iVideoBuffer) const override;
  void wglSendPbufferToVideoNV(BOOL return_value,
                               HPBUFFERARB hPbuffer,
                               int iBufferType,
                               unsigned long* pulCounterPbuffer,
                               BOOL bBlock) const override;
  void wglGetVideoInfoNV(BOOL return_value,
                         HPVIDEODEV hpVideoDevice,
                         unsigned long* pulCounterOutputPbuffer,
                         unsigned long* pulCounterOutputVideo) const override;
  void wglJoinSwapGroupNV(BOOL return_value, HDC hDC, GLuint group) const override;
  void wglBindSwapBarrierNV(BOOL return_value, GLuint group, GLuint barrier) const override;
  void wglQuerySwapGroupNV(BOOL return_value,
                           HDC hDC,
                           GLuint* group,
                           GLuint* barrier) const override;
  void wglQueryMaxSwapGroupsNV(BOOL return_value,
                               HDC hDC,
                               GLuint* maxGroups,
                               GLuint* maxBarriers) const override;
  void wglQueryFrameCountNV(BOOL return_value, HDC hDC, GLuint* count) const override;
  void wglResetFrameCountNV(BOOL return_value, HDC hDC) const override;
  void wglEnumGpusNV(BOOL return_value, UINT iGpuIndex, HGPUNV* phGpu) const override;
  void wglEnumGpuDevicesNV(BOOL return_value,
                           HGPUNV hGpu,
                           UINT iDeviceIndex,
                           PGPU_DEVICE lpGpuDevice) const override;
  void wglCreateAffinityDCNV(HDC return_value, const HGPUNV* phGpuList) const override;
  void wglEnumGpusFromAffinityDCNV(BOOL return_value,
                                   HDC hAffinityDC,
                                   UINT iGpuIndex,
                                   HGPUNV* hGpu) const override;
  void wglDeleteDCNV(BOOL return_value, HDC hdc) const override;
  void wglGetGPUIDsAMD(UINT return_value, UINT maxCount, UINT* ids) const override;
  void wglGetGPUInfoAMD(INT return_value,
                        UINT id,
                        int property,
                        GLenum dataType,
                        UINT size,
                        void* data) const override;
  void wglGetContextGPUIDAMD(UINT return_value, HGLRC hglrc) const override;
  void wglCreateAssociatedContextAMD(HGLRC return_value, UINT id) const override;
  void wglCreateAssociatedContextAttribsAMD(HGLRC return_value,
                                            UINT id,
                                            HGLRC hShareContext,
                                            const int* attribList) const override;
  void wglDeleteAssociatedContextAMD(BOOL return_value, HGLRC hglrc) const override;
  void wglMakeAssociatedContextCurrentAMD(BOOL return_value, HGLRC hglrc) const override;
  void wglGetCurrentAssociatedContextAMD(HGLRC return_value) const override;
  void wglBlitContextFramebufferAMD(HGLRC dstCtx,
                                    GLint srcX0,
                                    GLint srcY0,
                                    GLint srcX1,
                                    GLint srcY1,
                                    GLint dstX0,
                                    GLint dstY0,
                                    GLint dstX1,
                                    GLint dstY1,
                                    GLbitfield mask,
                                    GLenum filter) const override;
  void wglBindVideoCaptureDeviceNV(BOOL return_value,
                                   UINT uVideoSlot,
                                   HVIDEOINPUTDEVICENV hDevice) const override;
  void wglEnumerateVideoCaptureDevicesNV(UINT return_value,
                                         HDC hDc,
                                         HVIDEOINPUTDEVICENV* phDeviceList) const override;
  void wglLockVideoCaptureDeviceNV(BOOL return_value,
                                   HDC hDc,
                                   HVIDEOINPUTDEVICENV hDevice) const override;
  void wglQueryVideoCaptureDeviceNV(BOOL return_value,
                                    HDC hDc,
                                    HVIDEOINPUTDEVICENV hDevice,
                                    int iAttribute,
                                    int* piValue) const override;
  void wglReleaseVideoCaptureDeviceNV(BOOL return_value,
                                      HDC hDc,
                                      HVIDEOINPUTDEVICENV hDevice) const override;
  void wglCopyImageSubDataNV(BOOL return_value,
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
                             GLsizei depth) const override;
#endif

  void glXChooseVisual(XVisualInfo* return_value,
                       Display* dpy,
                       int screen,
                       int* attribList) const override;
  void glXCreateContext(GLXContext return_value,
                        Display* dpy,
                        XVisualInfo* vis,
                        GLXContext shareList,
                        Bool direct) const override;
  void glXDestroyContext(Display* dpy, GLXContext ctx) const override;
  void glXMakeCurrent(Bool return_value,
                      Display* dpy,
                      GLXDrawable drawable,
                      GLXContext ctx) const override;
  void glXCopyContext(Display* dpy,
                      GLXContext src,
                      GLXContext dst,
                      unsigned long mask) const override;
  void glXSwapBuffers(Display* dpy, GLXDrawable drawable) const override;
  void glXCreateGLXPixmap(GLXPixmap return_value,
                          Display* dpy,
                          XVisualInfo* visual,
                          Pixmap pixmap) const override;
  void glXDestroyGLXPixmap(Display* dpy, GLXPixmap pixmap) const override;
  void glXQueryExtension(Bool return_value, Display* dpy, int* errorb, int* event) const override;
  void glXQueryVersion(Bool return_value, Display* dpy, int* maj, int* min) const override;
  void glXIsDirect(Bool return_value, Display* dpy, GLXContext ctx) const override;
  void glXGetConfig(
      int return_value, Display* dpy, XVisualInfo* visual, int attrib, int* value) const override;
  void glXGetCurrentContext(GLXContext return_value) const override;
  void glXGetCurrentDrawable(GLXDrawable return_value) const override;
  void glXWaitGL() const override;
  void glXWaitX() const override;
  void glXUseXFont(Font font, int first, int count, int list) const override;
  void glXQueryExtensionsString(const char* return_value, Display* dpy, int screen) const override;
  void glXQueryServerString(const char* return_value,
                            Display* dpy,
                            int screen,
                            int name) const override;
  void glXGetClientString(const char* return_value, Display* dpy, int name) const override;
  void glXGetCurrentDisplay(Display* return_value) const override;
  void glXChooseFBConfig(GLXFBConfig* return_value,
                         Display* dpy,
                         int screen,
                         const int* attribList,
                         int* nitems) const override;
  void glXGetFBConfigAttrib(
      int return_value, Display* dpy, GLXFBConfig config, int attribute, int* value) const override;
  void glXGetFBConfigs(GLXFBConfig* return_value,
                       Display* dpy,
                       int screen,
                       int* nelements) const override;
  void glXGetVisualFromFBConfig(XVisualInfo* return_value,
                                Display* dpy,
                                GLXFBConfig config) const override;
  void glXCreateWindow(GLXWindow return_value,
                       Display* dpy,
                       GLXFBConfig config,
                       Window win,
                       const int* attribList) const override;
  void glXDestroyWindow(Display* dpy, GLXWindow window) const override;
  void glXCreatePixmap(GLXPixmap return_value,
                       Display* dpy,
                       GLXFBConfig config,
                       Pixmap pixmap,
                       const int* attribList) const override;
  void glXDestroyPixmap(Display* dpy, GLXPixmap pixmap) const override;
  void glXCreatePbuffer(GLXPbuffer return_value,
                        Display* dpy,
                        GLXFBConfig config,
                        const int* attribList) const override;
  void glXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf) const override;
  void glXQueryDrawable(Display* dpy,
                        GLXDrawable draw,
                        int attribute,
                        unsigned int* value) const override;
  void glXCreateNewContext(GLXContext return_value,
                           Display* dpy,
                           GLXFBConfig config,
                           int renderType,
                           GLXContext shareList,
                           Bool direct) const override;
  void glXMakeContextCurrent(Bool return_value,
                             Display* dpy,
                             GLXDrawable draw,
                             GLXDrawable read,
                             GLXContext ctx) const override;
  void glXGetCurrentReadDrawable(GLXDrawable return_value) const override;
  void glXQueryContext(
      int return_value, Display* dpy, GLXContext ctx, int attribute, int* value) const override;
  void glXSelectEvent(Display* dpy, GLXDrawable drawable, unsigned long mask) const override;
  void glXGetSelectedEvent(Display* dpy, GLXDrawable drawable, unsigned long* mask) const override;
  void glXGetProcAddressARB(void* return_value, const GLubyte*) const override;
  void glXGetProcAddress(void* return_value, const GLubyte*) const override;
  void glXAllocateMemoryNV(void* return_value,
                           GLsizei size,
                           GLfloat readfreq,
                           GLfloat writefreq,
                           GLfloat priority) const override;
  void glXFreeMemoryNV(GLvoid* pointer) const override;
  void glXBindTexImageARB(Bool return_value,
                          Display* dpy,
                          GLXPbuffer pbuffer,
                          int buffer) const override;
  void glXReleaseTexImageARB(Bool return_value,
                             Display* dpy,
                             GLXPbuffer pbuffer,
                             int buffer) const override;
  void glXDrawableAttribARB(Bool return_value,
                            Display* dpy,
                            GLXDrawable draw,
                            const int* attribList) const override;
  void glXGetFrameUsageMESA(int return_value,
                            Display* dpy,
                            GLXDrawable drawable,
                            float* usage) const override;
  void glXBeginFrameTrackingMESA(int return_value,
                                 Display* dpy,
                                 GLXDrawable drawable) const override;
  void glXEndFrameTrackingMESA(int return_value, Display* dpy, GLXDrawable drawable) const override;
  void glXQueryFrameTrackingMESA(int return_value,
                                 Display* dpy,
                                 GLXDrawable drawable,
                                 int64_t* swapCount,
                                 int64_t* missedFrames,
                                 float* lastMissedUsage) const override;
  void glXSwapIntervalMESA(int return_value, unsigned int interval) const override;
  void glXGetSwapIntervalMESA(int return_value) const override;
  void glXBindTexImageEXT(Display* dpy,
                          GLXDrawable drawable,
                          int buffer,
                          const int* attrib_list) const override;
  void glXReleaseTexImageEXT(Display* dpy, GLXDrawable drawable, int buffer) const override;
  void glXCreateContextAttribsARB(GLXContext return_value,
                                  Display* dpy,
                                  GLXFBConfig config,
                                  GLXContext share_context,
                                  Bool direct,
                                  const int* attrib_list) const override;
  void glXSwapIntervalSGI(int return_value, int interval) const override;

  void eglGetError(EGLint return_value) const override;
  void eglGetDisplay(EGLDisplay return_value, EGLNativeDisplayType display_id) const override;
  void eglInitialize(EGLBoolean return_value,
                     EGLDisplay dpy,
                     EGLint* major,
                     EGLint* minor) const override;
  void eglTerminate(EGLBoolean return_value, EGLDisplay dpy) const override;
  void eglQueryString(const char* return_value, EGLDisplay dpy, EGLint name) const override;
  void eglGetConfigs(EGLBoolean return_value,
                     EGLDisplay dpy,
                     EGLConfig* configs,
                     EGLint config_size,
                     EGLint* num_config) const override;
  void eglChooseConfig(EGLBoolean return_value,
                       EGLDisplay dpy,
                       const EGLint* attrib_list,
                       EGLConfig* configs,
                       EGLint config_size,
                       EGLint* num_config) const override;
  void eglGetConfigAttrib(EGLBoolean return_value,
                          EGLDisplay dpy,
                          EGLConfig config,
                          EGLint attribute,
                          EGLint* value) const override;
  void eglCreateWindowSurface(EGLSurface return_value,
                              EGLDisplay dpy,
                              EGLConfig config,
                              EGLNativeWindowType win,
                              const EGLint* attrib_list) const override;
  void eglCreatePbufferSurface(EGLSurface return_value,
                               EGLDisplay dpy,
                               EGLConfig config,
                               const EGLint* attrib_list) const override;
  void eglCreatePixmapSurface(EGLSurface return_value,
                              EGLDisplay dpy,
                              EGLConfig config,
                              EGLNativePixmapType pixmap,
                              const EGLint* attrib_list) const override;
  void eglDestroySurface(EGLBoolean return_value,
                         EGLDisplay dpy,
                         EGLSurface surface) const override;
  void eglQuerySurface(EGLBoolean return_value,
                       EGLDisplay dpy,
                       EGLSurface surface,
                       EGLint attribute,
                       EGLint* value) const override;
  void eglBindAPI(EGLBoolean return_value, EGLenum api) const override;
  void eglQueryAPI(EGLenum return_value) const override;
  void eglWaitClient(EGLBoolean return_value) const override;
  void eglReleaseThread(EGLBoolean return_value) const override;
  void eglCreatePbufferFromClientBuffer(EGLSurface return_value,
                                        EGLDisplay dpy,
                                        EGLenum buftype,
                                        EGLClientBuffer buffer,
                                        EGLConfig config,
                                        const EGLint* attrib_list) const override;
  void eglSurfaceAttrib(EGLBoolean return_value,
                        EGLDisplay dpy,
                        EGLSurface surface,
                        EGLint attribute,
                        EGLint value) const override;
  void eglBindTexImage(EGLBoolean return_value,
                       EGLDisplay dpy,
                       EGLSurface surface,
                       EGLint buffer) const override;
  void eglReleaseTexImage(EGLBoolean return_value,
                          EGLDisplay dpy,
                          EGLSurface surface,
                          EGLint buffer) const override;
  void eglSwapInterval(EGLBoolean return_value, EGLDisplay dpy, EGLint interval) const override;
  void eglCreateContext(EGLContext return_value,
                        EGLDisplay dpy,
                        EGLConfig config,
                        EGLContext share_context,
                        const EGLint* attrib_list) const override;
  void eglDestroyContext(EGLBoolean return_value, EGLDisplay dpy, EGLContext ctx) const override;
  void eglMakeCurrent(EGLBoolean return_value,
                      EGLDisplay dpy,
                      EGLSurface draw,
                      EGLSurface read,
                      EGLContext ctx) const override;
  void eglGetCurrentContext(EGLContext return_value) const override;
  void eglGetCurrentSurface(EGLSurface return_value, EGLint readdraw) const override;
  void eglGetCurrentDisplay(EGLDisplay return_value) const override;
  void eglQueryContext(EGLBoolean return_value,
                       EGLDisplay dpy,
                       EGLContext ctx,
                       EGLint attribute,
                       EGLint* value) const override;
  void eglWaitGL(EGLBoolean return_value) const override;
  void eglWaitNative(EGLBoolean return_value, EGLint engine) const override;
  void eglSwapBuffers(EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface) const override;
  void eglCopyBuffers(EGLBoolean return_value,
                      EGLDisplay dpy,
                      EGLSurface surface,
                      EGLNativePixmapType target) const override;
  void eglGetProcAddress(void* return_value, const char* procname) const override;
  void eglClientWaitSyncKHR(EGLDisplay dpy,
                            EGLSyncKHR sync,
                            EGLint flags,
                            EGLTimeKHR timeout) const override;
  void eglClientWaitSyncNV(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout) const override;
  void eglCreateDRMImageMESA(EGLDisplay dpy, const EGLint* attrib_list) const override;
  void eglCreateFenceSyncNV(EGLDisplay dpy,
                            EGLenum condition,
                            const EGLint* attrib_list) const override;
  void eglCreateImageKHR(EGLImageKHR return_value,
                         EGLDisplay dpy,
                         EGLContext ctx,
                         EGLenum target,
                         EGLClientBuffer buffer,
                         const EGLint* attrib_list) const override;
  void eglCreatePixmapSurfaceHI(EGLDisplay dpy, EGLConfig config, void* pixmap) const override;
  void eglCreateStreamFromFileDescriptorKHR(
      EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor) const override;
  void eglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list) const override;
  void eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy,
                                         EGLConfig config,
                                         EGLStreamKHR stream,
                                         const EGLint* attrib_list) const override;
  void eglCreateSyncKHR(EGLSyncKHR return_value,
                        EGLDisplay dpy,
                        EGLenum type,
                        const EGLint* attrib_list) const override;
  void eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) const override;
  void eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream) const override;
  void eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) const override;
  void eglDestroySyncNV(EGLSyncNV sync) const override;
  void eglExportDRMImageMESA(EGLDisplay dpy,
                             EGLImageKHR image,
                             EGLint* name,
                             EGLint* handle,
                             EGLint* stride) const override;
  void eglFenceNV(EGLSyncNV sync) const override;
  void eglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream) const override;
  void eglGetSyncAttribKHR(EGLDisplay dpy,
                           EGLSyncKHR sync,
                           EGLint attribute,
                           EGLint* value) const override;
  void eglGetSyncAttribNV(EGLSyncNV sync, EGLint attribute, EGLint* value) const override;
  void eglGetSystemTimeFrequencyNV() const override;
  void eglGetSystemTimeNV() const override;
  void eglLockSurfaceKHR(EGLDisplay display,
                         EGLSurface surface,
                         const EGLint* attrib_list) const override;
  void eglPostSubBufferNV(EGLDisplay dpy,
                          EGLSurface surface,
                          EGLint x,
                          EGLint y,
                          EGLint width,
                          EGLint height) const override;
  void eglQueryStreamKHR(EGLDisplay dpy,
                         EGLStreamKHR stream,
                         EGLenum attribute,
                         EGLint* value) const override;
  void eglQueryStreamTimeKHR(EGLDisplay dpy,
                             EGLStreamKHR stream,
                             EGLenum attribute,
                             EGLTimeKHR* value) const override;
  void eglQueryStreamu64KHR(EGLDisplay dpy,
                            EGLStreamKHR stream,
                            EGLenum attribute,
                            EGLuint64KHR* value) const override;
  void eglQuerySurfacePointerANGLE(EGLDisplay dpy,
                                   EGLSurface surface,
                                   EGLint attribute,
                                   void** value) const override;
  void eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode) const override;
  void eglSignalSyncNV(EGLSyncNV sync, EGLenum mode) const override;
  void eglStreamAttribKHR(EGLDisplay dpy,
                          EGLStreamKHR stream,
                          EGLenum attribute,
                          EGLint value) const override;
  void eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream) const override;
  void eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy, EGLStreamKHR stream) const override;
  void eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream) const override;
  void eglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface) const override;
  void eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) const override;
  void eglSetBlobCacheFuncsANDROID(EGLDisplay dpy,
                                   EGLSetBlobFuncANDROID set,
                                   EGLGetBlobFuncANDROID get) const override;
  void eglGetPlatformDisplayEXT(EGLDisplay return_value,
                                EGLenum platform,
                                Display* native_display,
                                const EGLint* attrib_list) const override;
  void eglCreatePlatformWindowSurfaceEXT(EGLSurface return_value,
                                         EGLDisplay dpy,
                                         EGLConfig config,
                                         EGLNativeWindowType native_window,
                                         const EGLint* attrib_list) const override;
};
} // namespace OpenGL
} // namespace gits
