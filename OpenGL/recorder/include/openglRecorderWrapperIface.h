// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglRecorderWrapperIface.h
 * 
 * @brief Declaration of OpenGL recorder wrapper interface.
 */

#pragma once

#include "recorderIface.h"
#include "openglTypes.h"
#include "openglDrivers.h"
#include "tools.h"

namespace gits {

namespace OpenGL {

class IRecorderWrapper {
public:
  virtual void StreamFinishedEvent(std::function<void()>) = 0;
  virtual void EndFramePost() const = 0;
  virtual void CloseRecorderIfRequired() = 0;
  virtual CDrivers& Drivers() const = 0;
  virtual void PreSwap() const = 0;

  virtual void GLInitialize(CGlDriver::TApiType api) const = 0;
  virtual void EGLInitialize() const = 0;

#include "openglRecorderWrapperIfaceAuto.h"
#if defined GITS_PLATFORM_WINDOWS
  virtual void wglChoosePixelFormat(bool return_value,
                                    HDC hdc,
                                    const PIXELFORMATDESCRIPTOR* ppfd) const = 0;
  virtual void wglSetPixelFormat(bool return_value,
                                 HDC hdc,
                                 int format,
                                 const PIXELFORMATDESCRIPTOR* ppfd) const = 0;
  virtual void wglCreateContext(HGLRC hglrc, HDC hdc) const = 0;
  virtual void wglCreateLayerContext(HGLRC return_value, HDC hdc, int iLayerPlane) const = 0;
  virtual void wglGetPixelFormat(int return_value, HDC hdc) const = 0;
  virtual void wglDescribePixelFormat(int return_value,
                                      HDC hdc,
                                      int format,
                                      unsigned nBytes,
                                      PIXELFORMATDESCRIPTOR* ppfd) const = 0;
  virtual void wglCopyContext(BOOL return_value,
                              HGLRC hglrcSrc,
                              HGLRC hglrcDst,
                              UINT mask) const = 0;
  virtual void wglMakeCurrent(bool return_value, HDC hdc, HGLRC hglrc) const = 0;
  virtual void wglDeleteContext(bool retVal, HGLRC hglrc) const = 0;
  virtual void wglSwapBuffers(bool return_value, HDC hdc) const = 0;
  virtual void wglSwapMultipleBuffers(bool return_value, GLuint buffer, HDC* hdc) const = 0;
  virtual void wglSwapLayerBuffers(bool return_value, HDC hdc, unsigned plane) const = 0;
  virtual void wglUseFontBitmapsA(
      bool return_value, HDC hdc, DWORD first, DWORD count, DWORD listBase) const = 0;
  virtual void wglUseFontOutlinesA(bool return_value,
                                   HDC hdc,
                                   DWORD first,
                                   DWORD count,
                                   DWORD listBase,
                                   float deviation,
                                   float extrusion,
                                   int format,
                                   LPGLYPHMETRICSFLOAT lpgmf) const = 0;
  virtual void wglUseFontBitmapsW(
      bool return_value, HDC hdc, DWORD first, DWORD count, DWORD listBase) const = 0;
  virtual void wglUseFontOutlinesW(bool return_value,
                                   HDC hdc,
                                   DWORD first,
                                   DWORD count,
                                   DWORD listBase,
                                   float deviation,
                                   float extrusion,
                                   int format,
                                   LPGLYPHMETRICSFLOAT lpgmf) const = 0;
  virtual void wglShareLists(bool return_value, HGLRC hglrc1, HGLRC hglrc2) const = 0;
  virtual void wglGetCurrentDC(HDC return_value) const = 0;
  virtual void wglGetCurrentContext(HGLRC return_value) const = 0;
  virtual void wglGetProcAddress(LPCSTR lpszProc) const = 0;
  virtual void wglGetDefaultProcAddress(LPCSTR lpszProc) const = 0;
  virtual void wglGetLayerPaletteEntries(
      HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr) const = 0;

  virtual void wglCreateBufferRegionARB(HANDLE return_value,
                                        HDC hDC,
                                        int iLayerPlane,
                                        UINT uType) const = 0;
  virtual void wglDeleteBufferRegionARB(HANDLE hRegion) const = 0;
  virtual void wglSaveBufferRegionARB(
      BOOL return_value, HANDLE hRegion, int x, int y, int width, int height) const = 0;
  virtual void wglRestoreBufferRegionARB(BOOL return_value,
                                         HANDLE hRegion,
                                         int x,
                                         int y,
                                         int width,
                                         int height,
                                         int xSrc,
                                         int ySrc) const = 0;
  virtual void wglGetExtensionsStringARB(const char* return_value, HDC hdc) const = 0;
  virtual void wglGetPixelFormatAttribivARB(BOOL return_value,
                                            HDC hdc,
                                            int iPixelFormat,
                                            int iLayerPlane,
                                            UINT nAttributes,
                                            const int* piAttributes,
                                            int* piValues) const = 0;
  virtual void wglGetPixelFormatAttribfvARB(BOOL return_value,
                                            HDC hdc,
                                            int iPixelFormat,
                                            int iLayerPlane,
                                            UINT nAttributes,
                                            const int* piAttributes,
                                            FLOAT* pfValues) const = 0;
  virtual void wglChoosePixelFormatARB(BOOL return_value,
                                       HDC hdc,
                                       const int* piAttribIList,
                                       const FLOAT* pfAttribFList,
                                       UINT nMaxFormats,
                                       int* piFormats,
                                       UINT* nNumFormats) const = 0;
  virtual void wglMakeContextCurrentARB(BOOL return_value,
                                        HDC hDrawDC,
                                        HDC hReadDC,
                                        HGLRC hglrc) const = 0;
  virtual void wglGetCurrentReadDCARB(HDC return_value) const = 0;
  virtual void wglCreatePbufferARB(HPBUFFERARB return_value,
                                   HDC hDC,
                                   int iPixelFormat,
                                   int iWidth,
                                   int iHeight,
                                   const int* piAttribList) const = 0;
  virtual void wglGetPbufferDCARB(HDC return_value, HPBUFFERARB hPbuffer) const = 0;
  virtual void wglReleasePbufferDCARB(int return_value, HPBUFFERARB hPbuffer, HDC hDC) const = 0;
  virtual void wglDestroyPbufferARB(BOOL return_value, HPBUFFERARB hPbuffer) const = 0;
  virtual void wglQueryPbufferARB(BOOL return_value,
                                  HPBUFFERARB hPbuffer,
                                  int iAttribute,
                                  int* piValue) const = 0;
  virtual void wglBindTexImageARB(BOOL return_value, HPBUFFERARB hPbuffer, int iBuffer) const = 0;
  virtual void wglReleaseTexImageARB(BOOL return_value,
                                     HPBUFFERARB hPbuffer,
                                     int iBuffer) const = 0;
  virtual void wglSetPbufferAttribARB(BOOL return_value,
                                      HPBUFFERARB hPbuffer,
                                      const int* piAttribList) const = 0;
  virtual void wglCreateContextAttribsARB(HGLRC return_value,
                                          HDC hDC,
                                          HGLRC hShareContext,
                                          const int* attribList) const = 0;
  virtual void wglCreateDisplayColorTableEXT(GLboolean return_value, GLushort id) const = 0;
  virtual void wglLoadDisplayColorTableEXT(GLboolean return_value,
                                           const GLushort* table,
                                           GLuint length) const = 0;
  virtual void wglBindDisplayColorTableEXT(GLboolean return_value, GLushort id) const = 0;
  virtual void wglDestroyDisplayColorTableEXT(GLushort id) const = 0;
  virtual void wglGetExtensionsStringEXT(const char* return_value) const = 0;
  virtual void wglMakeContextCurrentEXT(BOOL return_value,
                                        HDC hDrawDC,
                                        HDC hReadDC,
                                        HGLRC hglrc) const = 0;
  virtual void wglGetCurrentReadDCEXT(HDC return_value) const = 0;
  virtual void wglCreatePbufferEXT(HPBUFFEREXT return_value,
                                   HDC hDC,
                                   int iPixelFormat,
                                   int iWidth,
                                   int iHeight,
                                   const int* piAttribList) const = 0;
  virtual void wglGetPbufferDCEXT(HDC return_value, HPBUFFEREXT hPbuffer) const = 0;
  virtual void wglReleasePbufferDCEXT(int return_value, HPBUFFEREXT hPbuffer, HDC hDC) const = 0;
  virtual void wglDestroyPbufferEXT(BOOL return_value, HPBUFFEREXT hPbuffer) const = 0;
  virtual void wglQueryPbufferEXT(BOOL return_value,
                                  HPBUFFEREXT hPbuffer,
                                  int iAttribute,
                                  int* piValue) const = 0;
  virtual void wglGetPixelFormatAttribivEXT(BOOL return_value,
                                            HDC hdc,
                                            int iPixelFormat,
                                            int iLayerPlane,
                                            UINT nAttributes,
                                            int* piAttributes,
                                            int* piValues) const = 0;
  virtual void wglGetPixelFormatAttribfvEXT(BOOL return_value,
                                            HDC hdc,
                                            int iPixelFormat,
                                            int iLayerPlane,
                                            UINT nAttributes,
                                            int* piAttributes,
                                            FLOAT* pfValues) const = 0;
  virtual void wglChoosePixelFormatEXT(BOOL return_value,
                                       HDC hdc,
                                       const int* piAttribIList,
                                       const FLOAT* pfAttribFList,
                                       UINT nMaxFormats,
                                       int* piFormats,
                                       UINT* nNumFormats) const = 0;
  virtual void wglSwapIntervalEXT(BOOL return_value, int interval) const = 0;
  virtual void wglGetSwapIntervalEXT(int return_value) const = 0;
  virtual void wglAllocateMemoryNV(void* return_value,
                                   GLsizei size,
                                   GLfloat readfreq,
                                   GLfloat writefreq,
                                   GLfloat priority) const = 0;
  virtual void wglFreeMemoryNV(void* pointer) const = 0;
  virtual void wglGetSyncValuesOML(
      BOOL return_value, HDC hdc, INT64* ust, INT64* msc, INT64* sbc) const = 0;
  virtual void wglGetMscRateOML(BOOL return_value,
                                HDC hdc,
                                INT32* numerator,
                                INT32* denominator) const = 0;
  virtual void wglSwapBuffersMscOML(
      INT64 return_value, HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder) const = 0;
  virtual void wglSwapLayerBuffersMscOML(INT64 return_value,
                                         HDC hdc,
                                         int fuPlanes,
                                         INT64 target_msc,
                                         INT64 divisor,
                                         INT64 remainder) const = 0;
  virtual void wglWaitForMscOML(BOOL return_value,
                                HDC hdc,
                                INT64 target_msc,
                                INT64 divisor,
                                INT64 remainder,
                                INT64* ust,
                                INT64* msc,
                                INT64* sbc) const = 0;
  virtual void wglWaitForSbcOML(
      BOOL return_value, HDC hdc, INT64 target_sbc, INT64* ust, INT64* msc, INT64* sbc) const = 0;
  virtual void wglGetDigitalVideoParametersI3D(BOOL return_value,
                                               HDC hDC,
                                               int iAttribute,
                                               int* piValue) const = 0;
  virtual void wglSetDigitalVideoParametersI3D(BOOL return_value,
                                               HDC hDC,
                                               int iAttribute,
                                               const int* piValue) const = 0;
  virtual void wglGetGammaTableParametersI3D(BOOL return_value,
                                             HDC hDC,
                                             int iAttribute,
                                             int* piValue) const = 0;
  virtual void wglSetGammaTableParametersI3D(BOOL return_value,
                                             HDC hDC,
                                             int iAttribute,
                                             const int* piValue) const = 0;
  virtual void wglGetGammaTableI3D(BOOL return_value,
                                   HDC hDC,
                                   int iEntries,
                                   USHORT* puRed,
                                   USHORT* puGreen,
                                   USHORT* puBlue) const = 0;
  virtual void wglSetGammaTableI3D(BOOL return_value,
                                   HDC hDC,
                                   int iEntries,
                                   const USHORT* puRed,
                                   const USHORT* puGreen,
                                   const USHORT* puBlue) const = 0;
  virtual void wglEnableGenlockI3D(BOOL return_value, HDC hDC) const = 0;
  virtual void wglDisableGenlockI3D(BOOL return_value, HDC hDC) const = 0;
  virtual void wglIsEnabledGenlockI3D(BOOL return_value, HDC hDC, BOOL* pFlag) const = 0;
  virtual void wglGenlockSourceI3D(BOOL return_value, HDC hDC, UINT uSource) const = 0;
  virtual void wglGetGenlockSourceI3D(BOOL return_value, HDC hDC, UINT* uSource) const = 0;
  virtual void wglGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT uEdge) const = 0;
  virtual void wglGetGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT* uEdge) const = 0;
  virtual void wglGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT uRate) const = 0;
  virtual void wglGetGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT* uRate) const = 0;
  virtual void wglGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT uDelay) const = 0;
  virtual void wglGetGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT* uDelay) const = 0;
  virtual void wglQueryGenlockMaxSourceDelayI3D(BOOL return_value,
                                                HDC hDC,
                                                UINT* uMaxLineDelay,
                                                UINT* uMaxPixelDelay) const = 0;
  virtual void wglCreateImageBufferI3D(void* return_value,
                                       HDC hDC,
                                       DWORD dwSize,
                                       UINT uFlags) const = 0;
  virtual void wglDestroyImageBufferI3D(BOOL return_value, HDC hDC, LPVOID pAddress) const = 0;
  virtual void wglAssociateImageBufferEventsI3D(BOOL return_value,
                                                HDC hDC,
                                                const HANDLE* pEvent,
                                                const LPVOID* pAddress,
                                                const DWORD* pSize,
                                                UINT count) const = 0;
  virtual void wglReleaseImageBufferEventsI3D(BOOL return_value,
                                              HDC hDC,
                                              const LPVOID* pAddress,
                                              UINT count) const = 0;
  virtual void wglEnableFrameLockI3D(BOOL return_value) const = 0;
  virtual void wglDisableFrameLockI3D(BOOL return_value) const = 0;
  virtual void wglIsEnabledFrameLockI3D(BOOL return_value, BOOL* pFlag) const = 0;
  virtual void wglQueryFrameLockMasterI3D(BOOL return_value, BOOL* pFlag) const = 0;
  virtual void wglGetFrameUsageI3D(BOOL return_value, float* pUsage) const = 0;
  virtual void wglBeginFrameTrackingI3D(BOOL return_value) const = 0;
  virtual void wglEndFrameTrackingI3D(BOOL return_value) const = 0;
  virtual void wglQueryFrameTrackingI3D(BOOL return_value,
                                        DWORD* pFrameCount,
                                        DWORD* pMissedFrames,
                                        float* pLastMissedUsage) const = 0;
  virtual void wglSetStereoEmitterState3DL(BOOL return_value, HDC hDC, UINT uState) const = 0;
  virtual void wglEnumerateVideoDevicesNV(int return_value,
                                          HDC hDC,
                                          HVIDEOOUTPUTDEVICENV* phDeviceList) const = 0;
  virtual void wglBindVideoDeviceNV(BOOL return_value,
                                    HDC hDC,
                                    unsigned uVideoSlot,
                                    HVIDEOOUTPUTDEVICENV hVideoDevice,
                                    const int* piAttribList) const = 0;
  virtual void wglQueryCurrentContextNV(BOOL return_value, int iAttribute, int* piValue) const = 0;
  virtual void wglGetVideoDeviceNV(BOOL return_value,
                                   HDC hDC,
                                   int numDevices,
                                   HPVIDEODEV* hVideoDevice) const = 0;
  virtual void wglReleaseVideoDeviceNV(BOOL return_value, HPVIDEODEV hVideoDevice) const = 0;
  virtual void wglBindVideoImageNV(BOOL return_value,
                                   HPVIDEODEV hVideoDevice,
                                   HPBUFFERARB hPbuffer,
                                   int iVideoBuffer) const = 0;
  virtual void wglReleaseVideoImageNV(BOOL return_value,
                                      HPBUFFERARB hPbuffer,
                                      int iVideoBuffer) const = 0;
  virtual void wglSendPbufferToVideoNV(BOOL return_value,
                                       HPBUFFERARB hPbuffer,
                                       int iBufferType,
                                       unsigned long* pulCounterPbuffer,
                                       BOOL bBlock) const = 0;
  virtual void wglGetVideoInfoNV(BOOL return_value,
                                 HPVIDEODEV hpVideoDevice,
                                 unsigned long* pulCounterOutputPbuffer,
                                 unsigned long* pulCounterOutputVideo) const = 0;
  virtual void wglJoinSwapGroupNV(BOOL return_value, HDC hDC, GLuint group) const = 0;
  virtual void wglBindSwapBarrierNV(BOOL return_value, GLuint group, GLuint barrier) const = 0;
  virtual void wglQuerySwapGroupNV(BOOL return_value,
                                   HDC hDC,
                                   GLuint* group,
                                   GLuint* barrier) const = 0;
  virtual void wglQueryMaxSwapGroupsNV(BOOL return_value,
                                       HDC hDC,
                                       GLuint* maxGroups,
                                       GLuint* maxBarriers) const = 0;
  virtual void wglQueryFrameCountNV(BOOL return_value, HDC hDC, GLuint* count) const = 0;
  virtual void wglResetFrameCountNV(BOOL return_value, HDC hDC) const = 0;
  virtual void wglEnumGpusNV(BOOL return_value, UINT iGpuIndex, HGPUNV* phGpu) const = 0;
  virtual void wglEnumGpuDevicesNV(BOOL return_value,
                                   HGPUNV hGpu,
                                   UINT iDeviceIndex,
                                   PGPU_DEVICE lpGpuDevice) const = 0;
  virtual void wglCreateAffinityDCNV(HDC return_value, const HGPUNV* phGpuList) const = 0;
  virtual void wglEnumGpusFromAffinityDCNV(BOOL return_value,
                                           HDC hAffinityDC,
                                           UINT iGpuIndex,
                                           HGPUNV* hGpu) const = 0;
  virtual void wglDeleteDCNV(BOOL return_value, HDC hdc) const = 0;
  virtual void wglGetGPUIDsAMD(UINT return_value, UINT maxCount, UINT* ids) const = 0;
  virtual void wglGetGPUInfoAMD(
      INT return_value, UINT id, int property, GLenum dataType, UINT size, void* data) const = 0;
  virtual void wglGetContextGPUIDAMD(UINT return_value, HGLRC hglrc) const = 0;
  virtual void wglCreateAssociatedContextAMD(HGLRC return_value, UINT id) const = 0;
  virtual void wglCreateAssociatedContextAttribsAMD(HGLRC return_value,
                                                    UINT id,
                                                    HGLRC hShareContext,
                                                    const int* attribList) const = 0;
  virtual void wglDeleteAssociatedContextAMD(BOOL return_value, HGLRC hglrc) const = 0;
  virtual void wglMakeAssociatedContextCurrentAMD(BOOL return_value, HGLRC hglrc) const = 0;
  virtual void wglGetCurrentAssociatedContextAMD(HGLRC return_value) const = 0;
  virtual void wglBlitContextFramebufferAMD(HGLRC dstCtx,
                                            GLint srcX0,
                                            GLint srcY0,
                                            GLint srcX1,
                                            GLint srcY1,
                                            GLint dstX0,
                                            GLint dstY0,
                                            GLint dstX1,
                                            GLint dstY1,
                                            GLbitfield mask,
                                            GLenum filter) const = 0;
  virtual void wglBindVideoCaptureDeviceNV(BOOL return_value,
                                           UINT uVideoSlot,
                                           HVIDEOINPUTDEVICENV hDevice) const = 0;
  virtual void wglEnumerateVideoCaptureDevicesNV(UINT return_value,
                                                 HDC hDc,
                                                 HVIDEOINPUTDEVICENV* phDeviceList) const = 0;
  virtual void wglLockVideoCaptureDeviceNV(BOOL return_value,
                                           HDC hDc,
                                           HVIDEOINPUTDEVICENV hDevice) const = 0;
  virtual void wglQueryVideoCaptureDeviceNV(BOOL return_value,
                                            HDC hDc,
                                            HVIDEOINPUTDEVICENV hDevice,
                                            int iAttribute,
                                            int* piValue) const = 0;
  virtual void wglReleaseVideoCaptureDeviceNV(BOOL return_value,
                                              HDC hDc,
                                              HVIDEOINPUTDEVICENV hDevice) const = 0;
  virtual void wglCopyImageSubDataNV(BOOL return_value,
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
                                     GLsizei depth) const = 0;
#endif

  virtual void glXChooseVisual(XVisualInfo* return_value,
                               Display* dpy,
                               int screen,
                               int* attribList) const = 0;
  virtual void glXCreateContext(GLXContext return_value,
                                Display* dpy,
                                XVisualInfo* vis,
                                GLXContext shareList,
                                Bool direct) const = 0;
  virtual void glXDestroyContext(Display* dpy, GLXContext ctx) const = 0;
  virtual void glXMakeCurrent(Bool return_value,
                              Display* dpy,
                              GLXDrawable drawable,
                              GLXContext ctx) const = 0;
  virtual void glXCopyContext(Display* dpy,
                              GLXContext src,
                              GLXContext dst,
                              unsigned long mask) const = 0;
  virtual void glXSwapBuffers(Display* dpy, GLXDrawable drawable) const = 0;
  virtual void glXCreateGLXPixmap(GLXPixmap return_value,
                                  Display* dpy,
                                  XVisualInfo* visual,
                                  Pixmap pixmap) const = 0;
  virtual void glXDestroyGLXPixmap(Display* dpy, GLXPixmap pixmap) const = 0;
  virtual void glXQueryExtension(Bool return_value,
                                 Display* dpy,
                                 int* errorb,
                                 int* event) const = 0;
  virtual void glXQueryVersion(Bool return_value, Display* dpy, int* maj, int* min) const = 0;
  virtual void glXIsDirect(Bool return_value, Display* dpy, GLXContext ctx) const = 0;
  virtual void glXGetConfig(
      int return_value, Display* dpy, XVisualInfo* visual, int attrib, int* value) const = 0;
  virtual void glXGetCurrentContext(GLXContext return_value) const = 0;
  virtual void glXGetCurrentDrawable(GLXDrawable return_value) const = 0;
  virtual void glXWaitGL() const = 0;
  virtual void glXWaitX() const = 0;
  virtual void glXUseXFont(Font font, int first, int count, int list) const = 0;
  virtual void glXQueryExtensionsString(const char* return_value,
                                        Display* dpy,
                                        int screen) const = 0;
  virtual void glXQueryServerString(const char* return_value,
                                    Display* dpy,
                                    int screen,
                                    int name) const = 0;
  virtual void glXGetClientString(const char* return_value, Display* dpy, int name) const = 0;
  virtual void glXGetCurrentDisplay(Display* return_value) const = 0;
  virtual void glXChooseFBConfig(GLXFBConfig* return_value,
                                 Display* dpy,
                                 int screen,
                                 const int* attribList,
                                 int* nitems) const = 0;
  virtual void glXGetFBConfigAttrib(
      int return_value, Display* dpy, GLXFBConfig config, int attribute, int* value) const = 0;
  virtual void glXGetFBConfigs(GLXFBConfig* return_value,
                               Display* dpy,
                               int screen,
                               int* nelements) const = 0;
  virtual void glXGetVisualFromFBConfig(XVisualInfo* return_value,
                                        Display* dpy,
                                        GLXFBConfig config) const = 0;
  virtual void glXCreateWindow(GLXWindow return_value,
                               Display* dpy,
                               GLXFBConfig config,
                               Window win,
                               const int* attribList) const = 0;
  virtual void glXDestroyWindow(Display* dpy, GLXWindow window) const = 0;
  virtual void glXCreatePixmap(GLXPixmap return_value,
                               Display* dpy,
                               GLXFBConfig config,
                               Pixmap pixmap,
                               const int* attribList) const = 0;
  virtual void glXDestroyPixmap(Display* dpy, GLXPixmap pixmap) const = 0;
  virtual void glXCreatePbuffer(GLXPbuffer return_value,
                                Display* dpy,
                                GLXFBConfig config,
                                const int* attribList) const = 0;
  virtual void glXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf) const = 0;
  virtual void glXQueryDrawable(Display* dpy,
                                GLXDrawable draw,
                                int attribute,
                                unsigned int* value) const = 0;
  virtual void glXCreateNewContext(GLXContext return_value,
                                   Display* dpy,
                                   GLXFBConfig config,
                                   int renderType,
                                   GLXContext shareList,
                                   Bool direct) const = 0;
  virtual void glXMakeContextCurrent(Bool return_value,
                                     Display* dpy,
                                     GLXDrawable draw,
                                     GLXDrawable read,
                                     GLXContext ctx) const = 0;
  virtual void glXGetCurrentReadDrawable(GLXDrawable return_value) const = 0;
  virtual void glXQueryContext(
      int return_value, Display* dpy, GLXContext ctx, int attribute, int* value) const = 0;
  virtual void glXSelectEvent(Display* dpy, GLXDrawable drawable, unsigned long mask) const = 0;
  virtual void glXGetSelectedEvent(Display* dpy,
                                   GLXDrawable drawable,
                                   unsigned long* mask) const = 0;
  virtual void glXGetProcAddressARB(void* return_value, const GLubyte*) const = 0;
  virtual void glXGetProcAddress(void* return_value, const GLubyte*) const = 0;
  virtual void glXAllocateMemoryNV(void* return_value,
                                   GLsizei size,
                                   GLfloat readfreq,
                                   GLfloat writefreq,
                                   GLfloat priority) const = 0;
  virtual void glXFreeMemoryNV(GLvoid* pointer) const = 0;
  virtual void glXBindTexImageARB(Bool return_value,
                                  Display* dpy,
                                  GLXPbuffer pbuffer,
                                  int buffer) const = 0;
  virtual void glXReleaseTexImageARB(Bool return_value,
                                     Display* dpy,
                                     GLXPbuffer pbuffer,
                                     int buffer) const = 0;
  virtual void glXDrawableAttribARB(Bool return_value,
                                    Display* dpy,
                                    GLXDrawable draw,
                                    const int* attribList) const = 0;
  virtual void glXGetFrameUsageMESA(int return_value,
                                    Display* dpy,
                                    GLXDrawable drawable,
                                    float* usage) const = 0;
  virtual void glXBeginFrameTrackingMESA(int return_value,
                                         Display* dpy,
                                         GLXDrawable drawable) const = 0;
  virtual void glXEndFrameTrackingMESA(int return_value,
                                       Display* dpy,
                                       GLXDrawable drawable) const = 0;
  virtual void glXQueryFrameTrackingMESA(int return_value,
                                         Display* dpy,
                                         GLXDrawable drawable,
                                         int64_t* swapCount,
                                         int64_t* missedFrames,
                                         float* lastMissedUsage) const = 0;
  virtual void glXSwapIntervalMESA(int return_value, unsigned int interval) const = 0;
  virtual void glXGetSwapIntervalMESA(int return_value) const = 0;
  virtual void glXBindTexImageEXT(Display* dpy,
                                  GLXDrawable drawable,
                                  int buffer,
                                  const int* attrib_list) const = 0;
  virtual void glXReleaseTexImageEXT(Display* dpy, GLXDrawable drawable, int buffer) const = 0;
  virtual void glXCreateContextAttribsARB(GLXContext return_value,
                                          Display* dpy,
                                          GLXFBConfig config,
                                          GLXContext share_context,
                                          Bool direct,
                                          const int* attrib_list) const = 0;
  virtual void glXSwapIntervalSGI(int return_value, int interval) const = 0;

  virtual void eglGetError(EGLint return_value) const = 0;
  virtual void eglGetDisplay(EGLDisplay return_value, EGLNativeDisplayType display_id) const = 0;
  virtual void eglInitialize(EGLBoolean return_value,
                             EGLDisplay dpy,
                             EGLint* major,
                             EGLint* minor) const = 0;
  virtual void eglTerminate(EGLBoolean return_value, EGLDisplay dpy) const = 0;
  virtual void eglQueryString(const char* return_value, EGLDisplay dpy, EGLint name) const = 0;
  virtual void eglGetConfigs(EGLBoolean return_value,
                             EGLDisplay dpy,
                             EGLConfig* configs,
                             EGLint config_size,
                             EGLint* num_config) const = 0;
  virtual void eglChooseConfig(EGLBoolean return_value,
                               EGLDisplay dpy,
                               const EGLint* attrib_list,
                               EGLConfig* configs,
                               EGLint config_size,
                               EGLint* num_config) const = 0;
  virtual void eglGetConfigAttrib(EGLBoolean return_value,
                                  EGLDisplay dpy,
                                  EGLConfig config,
                                  EGLint attribute,
                                  EGLint* value) const = 0;
  virtual void eglCreateWindowSurface(EGLSurface return_value,
                                      EGLDisplay dpy,
                                      EGLConfig config,
                                      EGLNativeWindowType win,
                                      const EGLint* attrib_list) const = 0;
  virtual void eglCreatePbufferSurface(EGLSurface return_value,
                                       EGLDisplay dpy,
                                       EGLConfig config,
                                       const EGLint* attrib_list) const = 0;
  virtual void eglCreatePixmapSurface(EGLSurface return_value,
                                      EGLDisplay dpy,
                                      EGLConfig config,
                                      EGLNativePixmapType pixmap,
                                      const EGLint* attrib_list) const = 0;
  virtual void eglDestroySurface(EGLBoolean return_value,
                                 EGLDisplay dpy,
                                 EGLSurface surface) const = 0;
  virtual void eglQuerySurface(EGLBoolean return_value,
                               EGLDisplay dpy,
                               EGLSurface surface,
                               EGLint attribute,
                               EGLint* value) const = 0;
  virtual void eglBindAPI(EGLBoolean return_value, EGLenum api) const = 0;
  virtual void eglQueryAPI(EGLenum return_value) const = 0;
  virtual void eglWaitClient(EGLBoolean return_value) const = 0;
  virtual void eglReleaseThread(EGLBoolean return_value) const = 0;
  virtual void eglCreatePbufferFromClientBuffer(EGLSurface return_value,
                                                EGLDisplay dpy,
                                                EGLenum buftype,
                                                EGLClientBuffer buffer,
                                                EGLConfig config,
                                                const EGLint* attrib_list) const = 0;
  virtual void eglSurfaceAttrib(EGLBoolean return_value,
                                EGLDisplay dpy,
                                EGLSurface surface,
                                EGLint attribute,
                                EGLint value) const = 0;
  virtual void eglBindTexImage(EGLBoolean return_value,
                               EGLDisplay dpy,
                               EGLSurface surface,
                               EGLint buffer) const = 0;
  virtual void eglReleaseTexImage(EGLBoolean return_value,
                                  EGLDisplay dpy,
                                  EGLSurface surface,
                                  EGLint buffer) const = 0;
  virtual void eglSwapInterval(EGLBoolean return_value, EGLDisplay dpy, EGLint interval) const = 0;
  virtual void eglCreateContext(EGLContext return_value,
                                EGLDisplay dpy,
                                EGLConfig config,
                                EGLContext share_context,
                                const EGLint* attrib_list) const = 0;
  virtual void eglDestroyContext(EGLBoolean return_value, EGLDisplay dpy, EGLContext ctx) const = 0;
  virtual void eglMakeCurrent(EGLBoolean return_value,
                              EGLDisplay dpy,
                              EGLSurface draw,
                              EGLSurface read,
                              EGLContext ctx) const = 0;
  virtual void eglGetCurrentContext(EGLContext return_value) const = 0;
  virtual void eglGetCurrentSurface(EGLSurface return_value, EGLint readdraw) const = 0;
  virtual void eglGetCurrentDisplay(EGLDisplay return_value) const = 0;
  virtual void eglQueryContext(EGLBoolean return_value,
                               EGLDisplay dpy,
                               EGLContext ctx,
                               EGLint attribute,
                               EGLint* value) const = 0;
  virtual void eglWaitGL(EGLBoolean return_value) const = 0;
  virtual void eglWaitNative(EGLBoolean return_value, EGLint engine) const = 0;
  virtual void eglSwapBuffers(EGLBoolean return_value,
                              EGLDisplay dpy,
                              EGLSurface surface) const = 0;
  virtual void eglCopyBuffers(EGLBoolean return_value,
                              EGLDisplay dpy,
                              EGLSurface surface,
                              EGLNativePixmapType target) const = 0;
  virtual void eglGetProcAddress(void* return_value, const char* procname) const = 0;
  virtual void eglClientWaitSyncKHR(EGLDisplay dpy,
                                    EGLSyncKHR sync,
                                    EGLint flags,
                                    EGLTimeKHR timeout) const = 0;
  virtual void eglClientWaitSyncNV(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout) const = 0;
  virtual void eglCreateDRMImageMESA(EGLDisplay dpy, const EGLint* attrib_list) const = 0;
  virtual void eglCreateFenceSyncNV(EGLDisplay dpy,
                                    EGLenum condition,
                                    const EGLint* attrib_list) const = 0;
  virtual void eglCreateImageKHR(EGLImageKHR return_value,
                                 EGLDisplay dpy,
                                 EGLContext ctx,
                                 EGLenum target,
                                 EGLClientBuffer buffer,
                                 const EGLint* attrib_list) const = 0;
  virtual void eglCreatePixmapSurfaceHI(EGLDisplay dpy, EGLConfig config, void* pixmap) const = 0;
  virtual void eglCreateStreamFromFileDescriptorKHR(
      EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor) const = 0;
  virtual void eglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list) const = 0;
  virtual void eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy,
                                                 EGLConfig config,
                                                 EGLStreamKHR stream,
                                                 const EGLint* attrib_list) const = 0;
  virtual void eglCreateSyncKHR(EGLSyncKHR return_value,
                                EGLDisplay dpy,
                                EGLenum type,
                                const EGLint* attrib_list) const = 0;
  virtual void eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) const = 0;
  virtual void eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream) const = 0;
  virtual void eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) const = 0;
  virtual void eglDestroySyncNV(EGLSyncNV sync) const = 0;
  virtual void eglExportDRMImageMESA(
      EGLDisplay dpy, EGLImageKHR image, EGLint* name, EGLint* handle, EGLint* stride) const = 0;
  virtual void eglFenceNV(EGLSyncNV sync) const = 0;
  virtual void eglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream) const = 0;
  virtual void eglGetSyncAttribKHR(EGLDisplay dpy,
                                   EGLSyncKHR sync,
                                   EGLint attribute,
                                   EGLint* value) const = 0;
  virtual void eglGetSyncAttribNV(EGLSyncNV sync, EGLint attribute, EGLint* value) const = 0;
  virtual void eglGetSystemTimeFrequencyNV() const = 0;
  virtual void eglGetSystemTimeNV() const = 0;
  virtual void eglLockSurfaceKHR(EGLDisplay display,
                                 EGLSurface surface,
                                 const EGLint* attrib_list) const = 0;
  virtual void eglPostSubBufferNV(EGLDisplay dpy,
                                  EGLSurface surface,
                                  EGLint x,
                                  EGLint y,
                                  EGLint width,
                                  EGLint height) const = 0;
  virtual void eglQueryStreamKHR(EGLDisplay dpy,
                                 EGLStreamKHR stream,
                                 EGLenum attribute,
                                 EGLint* value) const = 0;
  virtual void eglQueryStreamTimeKHR(EGLDisplay dpy,
                                     EGLStreamKHR stream,
                                     EGLenum attribute,
                                     EGLTimeKHR* value) const = 0;
  virtual void eglQueryStreamu64KHR(EGLDisplay dpy,
                                    EGLStreamKHR stream,
                                    EGLenum attribute,
                                    EGLuint64KHR* value) const = 0;
  virtual void eglQuerySurfacePointerANGLE(EGLDisplay dpy,
                                           EGLSurface surface,
                                           EGLint attribute,
                                           void** value) const = 0;
  virtual void eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode) const = 0;
  virtual void eglSignalSyncNV(EGLSyncNV sync, EGLenum mode) const = 0;
  virtual void eglStreamAttribKHR(EGLDisplay dpy,
                                  EGLStreamKHR stream,
                                  EGLenum attribute,
                                  EGLint value) const = 0;
  virtual void eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream) const = 0;
  virtual void eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy, EGLStreamKHR stream) const = 0;
  virtual void eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream) const = 0;
  virtual void eglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface) const = 0;
  virtual void eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) const = 0;
  virtual void eglSetBlobCacheFuncsANDROID(EGLDisplay dpy,
                                           EGLSetBlobFuncANDROID set,
                                           EGLGetBlobFuncANDROID get) const = 0;
  virtual void eglGetPlatformDisplayEXT(EGLDisplay return_value,
                                        EGLenum platform,
                                        Display* native_display,
                                        const EGLint* attrib_list) const = 0;
  virtual void eglCreatePlatformWindowSurfaceEXT(EGLSurface return_value,
                                                 EGLDisplay dpy,
                                                 EGLConfig config,
                                                 EGLNativeWindowType native_window,
                                                 const EGLint* attrib_list) const = 0;
};

} // namespace OpenGL

} // namespace gits

typedef gits::OpenGL::IRecorderWrapper*(STDCALL* FGITSRecoderOpenGL)();

extern "C" {
gits::OpenGL::IRecorderWrapper* STDCALL GITSRecorderOpenGL() VISIBLE;
}
