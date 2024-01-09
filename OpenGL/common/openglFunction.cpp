// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglFunction.cpp
 *
 * @brief Definition of OpenGL specific function wrapper
 *
 */

#include "openglFunction.h"
#include "openglTools.h"
#include "gitsFunctions.h"

#include "eglFunctions.h"
#include "wglFunctions.h"
#include "glxFunctions.h"
#include "glFunctions.h"
#include "exception.h"
#include "streams.h"
#include "log.h"
#include "config.h"
#include "pragmas.h"

#include <iostream>

namespace gits {
namespace OpenGL {

/**
 * @brief Creates OpenGL function call warapper
 *
 * Method creates OpenGL function call wrappers based on unique
 * identifier.
 *
 * @param id Unique OpenGL function identifer.
 *
 * @exception EOperationFailed Unknown OpenGL function identifier
 *
 * @return OpenGL function call wrapper.
 */
CFunction* CFunction::Create(unsigned id) {
  if (id < CFunction::ID_OPENGL || id >= CFunction::ID_FUNCTION_END) {
    return nullptr;
  }

  switch (id) {
  //EGL calls
  case ID_EGL_GET_ERROR:
    return new CeglGetError;
  case ID_EGL_GET_DISPLAY:
    return new CeglGetDisplay;
  case ID_EGL_INITIALIZE:
    return new CeglInitialize;
  case ID_EGL_TERMINATE:
    return new CeglTerminate;
  case ID_EGL_QUERY_STRING:
    return new CeglQueryString;
  case ID_EGL_GET_CONFIGS:
    return new CeglGetConfigs;
  case ID_EGL_CHOOSE_CONFIG:
    return new CeglChooseConfig;
  case ID_EGL_GET_CONFIG_ATTRIB:
    return new CeglGetConfigAttrib;
  case ID_EGL_CREATE_WINDOW_SURFACE:
    return new CeglCreateWindowSurface;
  case ID_EGL_CREATE_PBUFFER_SURFACE:
    return new CeglCreatePbufferSurface;
  case ID_EGL_CREATE_PIXMAP_SURFACE:
    return new CeglCreatePixmapSurface;
  case ID_EGL_DESTROY_SURFACE:
    return new CeglDestroySurface;
  case ID_EGL_QUERY_SURFACE:
    return new CeglQuerySurface;
  case ID_EGL_BIND_API:
    return new CeglBindAPI;
  case ID_EGL_QUERY_API:
    return new CeglQueryAPI;
  case ID_EGL_WAIT_CLIENT:
    return new CeglWaitClient;
  case ID_EGL_RELEASE_THREAD:
    return new CeglReleaseThread;
  case ID_EGL_CREATE_PBUFFER_FROM_CLIENT_BUFFER:
    return new CeglCreatePbufferFromClientBuffer;
  case ID_EGL_SURFACE_ATTRIB:
    return new CeglSurfaceAttrib;
  case ID_EGL_BIND_TEX_IMAGE:
    return new CeglBindTexImage;
  case ID_EGL_RELEASE_TEX_IMAGE:
    return new CeglReleaseTexImage;
  case ID_EGL_SWAP_INTERVAL:
    return new CeglSwapInterval;
  case ID_EGL_CREATE_CONTEXT:
    return new CeglCreateContext;
  case ID_EGL_DESTROY_CONTEXT:
    return new CeglDestroyContext;
  case ID_EGL_MAKE_CURRENT:
    return new CeglMakeCurrent;
  case ID_EGL_GET_CURRENT_CONTEXT:
    return new CeglGetCurrentContext;
  case ID_EGL_GET_CURRENT_SURFACE:
    return new CeglGetCurrentSurface;
  case ID_EGL_GET_CURRENT_DISPLAY:
    return new CeglGetCurrentDisplay;
  case ID_EGL_QUERY_CONTEXT:
    return new CeglQueryContext;
  case ID_EGL_WAIT_GL:
    return new CeglWaitGL;
  case ID_EGL_WAIT_NATIVE:
    return new CeglWaitNative;
  case ID_EGL_SWAP_BUFFERS:
    return new CeglSwapBuffers;
  case ID_EGL_COPY_BUFFERS:
    return new CeglCopyBuffers;
  case ID_EGL_GET_PROC_ADDRESS:
    return new CeglGetProcAddress;
  case ID_EGL_GET_PLATFORM_DISPLAY_EXT:
    return new CeglGetPlatformDisplayEXT;
  case ID_EGL_CREATE_PLATFORM_WINDOW_SURFACE_EXT:
    return new CeglCreatePlatformWindowSurfaceEXT;
  case ID_EGL_CLIENT_WAIT_SYNC_KHR:
    return new CeglClientWaitSyncKHR;
  case ID_EGL_CLIENT_WAIT_SYNC_NV:
    return new CeglClientWaitSyncNV;
  case ID_EGL_CREATE_DRMIMAGE_MESA:
    return new CeglCreateDRMImageMESA;
  case ID_EGL_CREATE_FENCE_SYNC_NV:
    return new CeglCreateFenceSyncNV;
  case ID_EGL_CREATE_IMAGE_KHR:
    return new CeglCreateImageKHR;
  case ID_EGL_CREATE_PIXMAP_SURFACE_HI:
    return new CeglCreatePixmapSurfaceHI;
  case ID_EGL_CREATE_STREAM_FROM_FILE_DESCRIPTOR_KHR:
    return new CeglCreateStreamFromFileDescriptorKHR;
  case ID_EGL_CREATE_STREAM_KHR:
    return new CeglCreateStreamKHR;
  case ID_EGL_CREATE_STREAM_PRODUCER_SURFACE_KHR:
    return new CeglCreateStreamProducerSurfaceKHR;
  case ID_EGL_CREATE_SYNC_KHR:
    return new CeglCreateSyncKHR;
  case ID_EGL_DESTROY_IMAGE_KHR:
    return new CeglDestroyImageKHR;
  case ID_EGL_DESTROY_STREAM_KHR:
    return new CeglDestroyStreamKHR;
  case ID_EGL_DESTROY_SYNC_KHR:
    return new CeglDestroySyncKHR;
  case ID_EGL_DESTROY_SYNC_NV:
    return new CeglDestroySyncNV;
  case ID_EGL_EXPORT_DRMIMAGE_MESA:
    return new CeglExportDRMImageMESA;
  case ID_EGL_FENCE_NV:
    return new CeglFenceNV;
  case ID_EGL_GET_STREAM_FILE_DESCRIPTOR_KHR:
    return new CeglGetStreamFileDescriptorKHR;
  case ID_EGL_GET_SYNC_ATTRIB_KHR:
    return new CeglGetSyncAttribKHR;
  case ID_EGL_GET_SYNC_ATTRIB_NV:
    return new CeglGetSyncAttribNV;
  case ID_EGL_GET_SYSTEM_TIME_FREQUENCY_NV:
    return new CeglGetSystemTimeFrequencyNV;
  case ID_EGL_GET_SYSTEM_TIME_NV:
    return new CeglGetSystemTimeNV;
  case ID_EGL_LOCK_SURFACE_KHR:
    return new CeglLockSurfaceKHR;
  case ID_EGL_POST_SUB_BUFFER_NV:
    return new CeglPostSubBufferNV;
  case ID_EGL_QUERY_STREAM_KHR:
    return new CeglQueryStreamKHR;
  case ID_EGL_QUERY_STREAM_TIME_KHR:
    return new CeglQueryStreamTimeKHR;
  case ID_EGL_QUERY_STREAMU64KHR:
    return new CeglQueryStreamu64KHR;
  case ID_EGL_QUERY_SURFACE_POINTER_ANGLE:
    return new CeglQuerySurfacePointerANGLE;
  case ID_EGL_SIGNAL_SYNC_KHR:
    return new CeglSignalSyncKHR;
  case ID_EGL_SIGNAL_SYNC_NV:
    return new CeglSignalSyncNV;
  case ID_EGL_STREAM_ATTRIB_KHR:
    return new CeglStreamAttribKHR;
  case ID_EGL_STREAM_CONSUMER_ACQUIRE_KHR:
    return new CeglStreamConsumerAcquireKHR;
  case ID_EGL_STREAM_CONSUMER_GLTEXTURE_EXTERNAL_KHR:
    return new CeglStreamConsumerGLTextureExternalKHR;
  case ID_EGL_STREAM_CONSUMER_RELEASE_KHR:
    return new CeglStreamConsumerReleaseKHR;
  case ID_EGL_UNLOCK_SURFACE_KHR:
    return new CeglUnlockSurfaceKHR;
  case ID_EGL_WAIT_SYNC_KHR:
    return new CeglWaitSyncKHR;

  //WGL Calls
  case ID_WGL_CHOOSE_PIXEL_FORMAT:
    return new CwglChoosePixelFormat;
  case ID_WGL_SET_PIXEL_FORMAT:
    return new CwglSetPixelFormat;
  case ID_WGL_GET_PIXEL_FORMAT:
    return new CwglGetPixelFormat;
  case ID_WGL_DESCRIBE_PIXEL_FORMAT:
    return new CwglDescribePixelFormat;
  case ID_WGL_COPY_CONTEXT:
    return new CwglCopyContext;
  case ID_WGL_CREATE_CONTEXT:
    return new CwglCreateContext;
  case ID_WGL_CREATE_LAYER_CONTEXT:
    return new CwglCreateLayerContext;
  case ID_WGL_DELETE_CONTEXT:
    return new CwglDeleteContext;
  case ID_WGL_GET_CURRENT_CONTEXT:
    return new CwglGetCurrentContext;
  case ID_WGL_GET_CURRENT_DC:
    return new CwglGetCurrentDC;
  case ID_WGL_GET_PROC_ADDRESS:
    return new CwglGetProcAddress;
  case ID_WGL_GET_DEFAULT_PROC_ADDRESS:
    return new CwglGetDefaultProcAddress;
  case ID_WGL_GET_LAYER_PALETTE_ENTRIES:
    return new CwglGetLayerPaletteEntries;
  case ID_WGL_MAKE_CURRENT:
    return new CwglMakeCurrent;
  case ID_WGL_SWAP_BUFFERS:
    return new CwglSwapBuffers;
  case ID_WGL_SWAP_MULTIPLE_BUFFERS:
    return new CwglSwapMultipleBuffers;
  case ID_WGL_SHARE_LISTS:
    return new CwglShareLists;
  case ID_WGL_SWAP_LAYER_BUFFERS:
    return new CwglSwapLayerBuffers;
  case ID_WGL_USE_FONT_BITMAPS_A:
    return new CwglUseFontBitmapsA;
  case ID_WGL_USE_FONT_BITMAPS_W:
    return new CwglUseFontBitmapsW;
  case ID_WGL_USE_FONT_OUTLINES_A:
    return new CwglUseFontOutlinesA;
  case ID_WGL_USE_FONT_OUTLINES_W:
    return new CwglUseFontOutlinesW;
  case ID_WGL_CREATE_BUFFER_REGION_ARB:
    return new CwglCreateBufferRegionARB;
  case ID_WGL_DELETE_BUFFER_REGION_ARB:
    return new CwglDeleteBufferRegionARB;
  case ID_WGL_SAVE_BUFFER_REGION_ARB:
    return new CwglSaveBufferRegionARB;
  case ID_WGL_RESTORE_BUFFER_REGION_ARB:
    return new CwglRestoreBufferRegionARB;
  case ID_WGL_GET_EXTENSIONS_STRING_ARB:
    return new CwglGetExtensionsStringARB;
  case ID_WGL_GET_PIXEL_FORMAT_ATTRIBIV_ARB:
    return new CwglGetPixelFormatAttribivARB;
  case ID_WGL_GET_PIXEL_FORMAT_ATTRIBFV_ARB:
    return new CwglGetPixelFormatAttribfvARB;
  case ID_WGL_CHOOSE_PIXEL_FORMAT_ARB:
    return new CwglChoosePixelFormatARB;
  case ID_WGL_MAKE_CONTEXT_CURRENT_ARB:
    return new CwglMakeContextCurrentARB;
  case ID_WGL_GET_CURRENT_READ_DC_ARB:
    return new CwglGetCurrentReadDCARB;
  case ID_WGL_CREATE_PBUFFER_ARB:
    return new CwglCreatePbufferARB;
  case ID_WGL_GET_PBUFFER_DC_ARB:
    return new CwglGetPbufferDCARB;
  case ID_WGL_RELEASE_PBUFFER_DC_ARB:
    return new CwglReleasePbufferDCARB;
  case ID_WGL_DESTROY_PBUFFER_ARB:
    return new CwglDestroyPbufferARB;
  case ID_WGL_QUERY_PBUFFER_ARB:
    return new CwglQueryPbufferARB;
  case ID_WGL_BIND_TEX_IMAGE_ARB:
    return new CwglBindTexImageARB;
  case ID_WGL_RELEASE_TEX_IMAGE_ARB:
    return new CwglReleaseTexImageARB;
  case ID_WGL_SET_PBUFFER_ATTRIB_ARB:
    return new CwglSetPbufferAttribARB;
  case ID_WGL_CREATE_CONTEXT_ATTRIBS_ARB:
    return new CwglCreateContextAttribsARB;
  case ID_WGL_CREATE_DISPLAY_COLOR_TABLE_EXT:
    return new CwglCreateDisplayColorTableEXT;
  case ID_WGL_LOAD_DISPLAY_COLOR_TABLE_EXT:
    return new CwglLoadDisplayColorTableEXT;
  case ID_WGL_BIND_DISPLAY_COLOR_TABLE_EXT:
    return new CwglBindDisplayColorTableEXT;
  case ID_WGL_DESTROY_DISPLAY_COLOR_TABLE_EXT:
    return new CwglDestroyDisplayColorTableEXT;
  case ID_WGL_GET_EXTENSIONS_STRING_EXT:
    return new CwglGetExtensionsStringEXT;
  case ID_WGL_MAKE_CONTEXT_CURRENT_EXT:
    return new CwglMakeContextCurrentEXT;
  case ID_WGL_GET_CURRENT_READ_DC_EXT:
    return new CwglGetCurrentReadDCEXT;
  case ID_WGL_CREATE_PBUFFER_EXT:
    return new CwglCreatePbufferEXT;
  case ID_WGL_GET_PBUFFER_DC_EXT:
    return new CwglGetPbufferDCEXT;
  case ID_WGL_RELEASE_PBUFFER_DC_EXT:
    return new CwglReleasePbufferDCEXT;
  case ID_WGL_DESTROY_PBUFFER_EXT:
    return new CwglDestroyPbufferEXT;
  case ID_WGL_QUERY_PBUFFER_EXT:
    return new CwglQueryPbufferEXT;
  case ID_WGL_GET_PIXEL_FORMAT_ATTRIBIV_EXT:
    return new CwglGetPixelFormatAttribivEXT;
  case ID_WGL_GET_PIXEL_FORMAT_ATTRIBFV_EXT:
    return new CwglGetPixelFormatAttribfvEXT;
  case ID_WGL_CHOOSE_PIXEL_FORMAT_EXT:
    return new CwglChoosePixelFormatEXT;
  case ID_WGL_SWAP_INTERVAL_EXT:
    return new CwglSwapIntervalEXT;
  case ID_WGL_GET_SWAP_INTERVAL_EXT:
    return new CwglGetSwapIntervalEXT;
  case ID_WGL_ALLOCATE_MEMORY_NV:
    return new CwglAllocateMemoryNV;
  case ID_WGL_FREE_MEMORY_NV:
    return new CwglFreeMemoryNV;
  case ID_WGL_GET_SYNC_VALUES_OML:
    return new CwglGetSyncValuesOML;
  case ID_WGL_GET_MSC_RATE_OML:
    return new CwglGetMscRateOML;
  case ID_WGL_SWAP_BUFFERS_MSC_OML:
    return new CwglSwapBuffersMscOML;
  case ID_WGL_SWAP_LAYER_BUFFERS_MSC_OML:
    return new CwglSwapLayerBuffersMscOML;
  case ID_WGL_WAIT_FOR_MSC_OML:
    return new CwglWaitForMscOML;
  case ID_WGL_WAIT_FOR_SBC_OML:
    return new CwglWaitForSbcOML;
  case ID_WGL_GET_DIGITAL_VIDEO_PARAMETERS_I3D:
    return new CwglGetDigitalVideoParametersI3D;
  case ID_WGL_SET_DIGITAL_VIDEO_PARAMETERS_I3D:
    return new CwglSetDigitalVideoParametersI3D;
  case ID_WGL_GET_GAMMA_TABLE_PARAMETERS_I3D:
    return new CwglGetGammaTableParametersI3D;
  case ID_WGL_SET_GAMMA_TABLE_PARAMETERS_I3D:
    return new CwglSetGammaTableParametersI3D;
  case ID_WGL_GET_GAMMA_TABLE_I3D:
    return new CwglGetGammaTableI3D;
  case ID_WGL_SET_GAMMA_TABLE_I3D:
    return new CwglSetGammaTableI3D;
  case ID_WGL_ENABLE_GENLOCK_I3D:
    return new CwglEnableGenlockI3D;
  case ID_WGL_DISABLE_GENLOCK_I3D:
    return new CwglDisableGenlockI3D;
  case ID_WGL_IS_ENABLED_GENLOCK_I3D:
    return new CwglIsEnabledGenlockI3D;
  case ID_WGL_GENLOCK_SOURCE_I3D:
    return new CwglGenlockSourceI3D;
  case ID_WGL_GET_GENLOCK_SOURCE_I3D:
    return new CwglGetGenlockSourceI3D;
  case ID_WGL_GENLOCK_SOURCE_EDGE_I3D:
    return new CwglGenlockSourceEdgeI3D;
  case ID_WGL_GET_GENLOCK_SOURCE_EDGE_I3D:
    return new CwglGetGenlockSourceEdgeI3D;
  case ID_WGL_GENLOCK_SAMPLE_RATE_I3D:
    return new CwglGenlockSampleRateI3D;
  case ID_WGL_GET_GENLOCK_SAMPLE_RATE_I3D:
    return new CwglGetGenlockSampleRateI3D;
  case ID_WGL_GENLOCK_SOURCE_DELAY_I3D:
    return new CwglGenlockSourceDelayI3D;
  case ID_WGL_GET_GENLOCK_SOURCE_DELAY_I3D:
    return new CwglGetGenlockSourceDelayI3D;
  case ID_WGL_QUERY_GENLOCK_MAX_SOURCE_DELAY_I3D:
    return new CwglQueryGenlockMaxSourceDelayI3D;
  case ID_WGL_CREATE_IMAGE_BUFFER_I3D:
    return new CwglCreateImageBufferI3D;
  case ID_WGL_DESTROY_IMAGE_BUFFER_I3D:
    return new CwglDestroyImageBufferI3D;
  case ID_WGL_ASSOCIATE_IMAGE_BUFFER_EVENTS_I3D:
    return new CwglAssociateImageBufferEventsI3D;
  case ID_WGL_RELEASE_IMAGE_BUFFER_EVENTS_I3D:
    return new CwglReleaseImageBufferEventsI3D;
  case ID_WGL_ENABLE_FRAME_LOCK_I3D:
    return new CwglEnableFrameLockI3D;
  case ID_WGL_DISABLE_FRAME_LOCK_I3D:
    return new CwglDisableFrameLockI3D;
  case ID_WGL_IS_ENABLED_FRAME_LOCK_I3D:
    return new CwglIsEnabledFrameLockI3D;
  case ID_WGL_QUERY_FRAME_LOCK_MASTER_I3D:
    return new CwglQueryFrameLockMasterI3D;
  case ID_WGL_GET_FRAME_USAGE_I3D:
    return new CwglGetFrameUsageI3D;
  case ID_WGL_BEGIN_FRAME_TRACKING_I3D:
    return new CwglBeginFrameTrackingI3D;
  case ID_WGL_END_FRAME_TRACKING_I3D:
    return new CwglEndFrameTrackingI3D;
  case ID_WGL_QUERY_FRAME_TRACKING_I3D:
    return new CwglQueryFrameTrackingI3D;
  case ID_WGL_SET_STEREO_EMITTER_STATE3DL:
    return new CwglSetStereoEmitterState3DL;
  case ID_WGL_ENUMERATE_VIDEO_DEVICES_NV:
    return new CwglEnumerateVideoDevicesNV;
  case ID_WGL_BIND_VIDEO_DEVICE_NV:
    return new CwglBindVideoDeviceNV;
  case ID_WGL_QUERY_CURRENT_CONTEXT_NV:
    return new CwglQueryCurrentContextNV;
  case ID_WGL_GET_VIDEO_DEVICE_NV:
    return new CwglGetVideoDeviceNV;
  case ID_WGL_RELEASE_VIDEO_DEVICE_NV:
    return new CwglReleaseVideoDeviceNV;
  case ID_WGL_BIND_VIDEO_IMAGE_NV:
    return new CwglBindVideoImageNV;
  case ID_WGL_RELEASE_VIDEO_IMAGE_NV:
    return new CwglReleaseVideoImageNV;
  case ID_WGL_SEND_PBUFFER_TO_VIDEO_NV:
    return new CwglSendPbufferToVideoNV;
  case ID_WGL_GET_VIDEO_INFO_NV:
    return new CwglGetVideoInfoNV;
  case ID_WGL_JOIN_SWAP_GROUP_NV:
    return new CwglJoinSwapGroupNV;
  case ID_WGL_BIND_SWAP_BARRIER_NV:
    return new CwglBindSwapBarrierNV;
  case ID_WGL_QUERY_SWAP_GROUP_NV:
    return new CwglQuerySwapGroupNV;
  case ID_WGL_QUERY_MAX_SWAP_GROUPS_NV:
    return new CwglQueryMaxSwapGroupsNV;
  case ID_WGL_QUERY_FRAME_COUNT_NV:
    return new CwglQueryFrameCountNV;
  case ID_WGL_RESET_FRAME_COUNT_NV:
    return new CwglResetFrameCountNV;
  case ID_WGL_ENUM_GPUS_NV:
    return new CwglEnumGpusNV;
  case ID_WGL_ENUM_GPU_DEVICES_NV:
    return new CwglEnumGpuDevicesNV;
  case ID_WGL_CREATE_AFFINITY_DCNV:
    return new CwglCreateAffinityDCNV;
  case ID_WGL_ENUM_GPUS_FROM_AFFINITY_DCNV:
    return new CwglEnumGpusFromAffinityDCNV;
  case ID_WGL_DELETE_DCNV:
    return new CwglDeleteDCNV;
  case ID_WGL_GET_GPUIDS_AMD:
    return new CwglGetGPUIDsAMD;
  case ID_WGL_GET_GPUINFO_AMD:
    return new CwglGetGPUInfoAMD;
  case ID_WGL_GET_CONTEXT_GPUIDAMD:
    return new CwglGetContextGPUIDAMD;
  case ID_WGL_CREATE_ASSOCIATED_CONTEXT_AMD:
    return new CwglCreateAssociatedContextAMD;
  case ID_WGL_CREATE_ASSOCIATED_CONTEXT_ATTRIBS_AMD:
    return new CwglCreateAssociatedContextAttribsAMD;
  case ID_WGL_DELETE_ASSOCIATED_CONTEXT_AMD:
    return new CwglDeleteAssociatedContextAMD;
  case ID_WGL_MAKE_ASSOCIATED_CONTEXT_CURRENT_AMD:
    return new CwglMakeAssociatedContextCurrentAMD;
  case ID_WGL_GET_CURRENT_ASSOCIATED_CONTEXT_AMD:
    return new CwglGetCurrentAssociatedContextAMD;
  case ID_WGL_BLIT_CONTEXT_FRAMEBUFFER_AMD:
    return new CwglBlitContextFramebufferAMD;
  case ID_WGL_BIND_VIDEO_CAPTURE_DEVICE_NV:
    return new CwglBindVideoCaptureDeviceNV;
  case ID_WGL_ENUMERATE_VIDEO_CAPTURE_DEVICES_NV:
    return new CwglEnumerateVideoCaptureDevicesNV;
  case ID_WGL_LOCK_VIDEO_CAPTURE_DEVICE_NV:
    return new CwglLockVideoCaptureDeviceNV;
  case ID_WGL_QUERY_VIDEO_CAPTURE_DEVICE_NV:
    return new CwglQueryVideoCaptureDeviceNV;
  case ID_WGL_RELEASE_VIDEO_CAPTURE_DEVICE_NV:
    return new CwglReleaseVideoCaptureDeviceNV;
  case ID_WGL_COPY_IMAGE_SUB_DATA_NV:
    return new CwglCopyImageSubDataNV;
  case ID_HELPER_WGL_UPDATE_WINDOW:
    return new ChelperWglUpdateWindow;

  //LINUX GLX
  case ID_GLX_CHOOSE_VISUAL:
    return new CglXChooseVisual;
  case ID_GLX_CREATE_CONTEXT:
    return new CglXCreateContext;
  case ID_GLX_DESTROY_CONTEXT:
    return new CglXDestroyContext;
  case ID_GLX_MAKE_CURRENT:
    return new CglXMakeCurrent;
  case ID_GLX_COPY_CONTEXT:
    return new CglXCopyContext;
  case ID_GLX_SWAP_BUFFERS:
    return new CglXSwapBuffers;
  case ID_GLX_CREATE_GLXPIXMAP:
    return new CglXCreateGLXPixmap;
  case ID_GLX_DESTROY_GLXPIXMAP:
    return new CglXDestroyGLXPixmap;
  case ID_GLX_QUERY_EXTENSION:
    return new CglXQueryExtension;
  case ID_GLX_QUERY_VERSION:
    return new CglXQueryVersion;
  case ID_GLX_IS_DIRECT:
    return new CglXIsDirect;
  case ID_GLX_GET_CONFIG:
    return new CglXGetConfig;
  case ID_GLX_GET_CURRENT_CONTEXT:
    return new CglXGetCurrentContext;
  case ID_GLX_GET_CURRENT_DRAWABLE:
    return new CglXGetCurrentDrawable;
  case ID_GLX_WAIT_GL:
    return new CglXWaitGL;
  case ID_GLX_WAIT_X:
    return new CglXWaitX;
  case ID_GLX_USE_XFONT:
    return new CglXUseXFont;
  case ID_GLX_QUERY_EXTENSIONS_STRING:
    return new CglXQueryExtensionsString;
  case ID_GLX_QUERY_SERVER_STRING:
    return new CglXQueryServerString;
  case ID_GLX_GET_CLIENT_STRING:
    return new CglXGetClientString;
  case ID_GLX_GET_CURRENT_DISPLAY:
    return new CglXGetCurrentDisplay;
  case ID_GLX_CHOOSE_FB_CONFIG:
    return new CglXChooseFBConfig;
  case ID_GLX_GET_FBCONFIG_ATTRIB:
    return new CglXGetFBConfigAttrib;
  case ID_GLX_GET_FBCONFIGS:
    return new CglXGetFBConfigs;
  case ID_GLX_GET_VISUAL_FROM_FB_CONFIG:
    return new CglXGetVisualFromFBConfig;
  case ID_GLX_CREATE_WINDOW:
    return new CglXCreateWindow;
  case ID_GLX_DESTROY_WINDOW:
    return new CglXDestroyWindow;
  case ID_GLX_CREATE_PIXMAP:
    return new CglXCreatePixmap;
  case ID_GLX_DESTROY_PIXMAP:
    return new CglXDestroyPixmap;
  case ID_GLX_CREATE_PBUFFER:
    return new CglXCreatePbuffer;
  case ID_GLX_DESTROY_PBUFFER:
    return new CglXDestroyPbuffer;
  case ID_GLX_QUERY_DRAWABLE:
    return new CglXQueryDrawable;
  case ID_GLX_CREATE_NEW_CONTEXT:
    return new CglXCreateNewContext;
  case ID_GLX_MAKE_CONTEXT_CURRENT:
    return new CglXMakeContextCurrent;
  case ID_GLX_GET_CURRENT_READ_DRAWABLE:
    return new CglXGetCurrentReadDrawable;
  case ID_GLX_QUERY_CONTEXT:
    return new CglXQueryContext;
  case ID_GLX_SELECT_EVENT:
    return new CglXSelectEvent;
  case ID_GLX_GET_SELECTED_EVENT:
    return new CglXGetSelectedEvent;
  case ID_GLX_GET_PROC_ADDRESS_ARB:
    return new CglXGetProcAddressARB;
  case ID_GLX_GET_PROC_ADDRESS:
    return new CglXGetProcAddressARB;
  case ID_GLX_ALLOCATE_MEMORY_NV:
    return new CglXAllocateMemoryNV;
  case ID_GLX_FREE_MEMORY_NV:
    return new CglXFreeMemoryNV;
  case ID_GLX_BIND_TEX_IMAGE_ARB:
    return new CglXBindTexImageARB;
  case ID_GLX_RELEASE_TEX_IMAGE_ARB:
    return new CglXReleaseTexImageARB;
  case ID_GLX_DRAWABLE_ATTRIB_ARB:
    return new CglXDrawableAttribARB;
  case ID_GLX_GET_FRAME_USAGE_MESA:
    return new CglXGetFrameUsageMESA;
  case ID_GLX_BEGIN_FRAME_TRACKING_MESA:
    return new CglXBeginFrameTrackingMESA;
  case ID_GLX_END_FRAME_TRACKING_MESA:
    return new CglXEndFrameTrackingMESA;
  case ID_GLX_QUERY_FRAME_TRACKING_MESA:
    return new CglXQueryFrameTrackingMESA;
  case ID_GLX_SWAP_INTERVAL_MESA:
    return new CglXSwapIntervalMESA;
  case ID_GLX_GET_SWAP_INTERVAL_MESA:
    return new CglXGetSwapIntervalMESA;
  case ID_GLX_BIND_TEX_IMAGE_EXT:
    return new CglXBindTexImageEXT;
  case ID_GLX_RELEASE_TEX_IMAGE_EXT:
    return new CglXReleaseTexImageEXT;
  case ID_GLX_CREATE_CONTEXT_ATTRIBS_ARB:
    return new CglXCreateContextAttribsARB;
  case ID_GLX_SWAP_INTERVAL_SGI:
    return new CglXSwapIntervalSGI;

  //GL HELPER
  case ID_RESTORE_DEFAULT_GL_FRAMEBUFFER:
    return new CRestoreDefaultGLFramebuffer;
  case ID_EGL_SET_BLOB_CACHE_FUNCS_ANDROID:
    return new CeglSetBlobCacheFuncsANDROID;
  case ID_GITS_CLIENT_ARRAYS_UPDATE:
    return new CgitsClientArraysUpdate;
  case ID_GITS_CLIENT_INDIRECT_ARRAYS_UPDATE:
    return new CgitsClientIndirectArraysUpdate;
  case ID_GITS_LINK_PROGRAM_ATTRIBS_SETTINGS:
    return new CgitsLinkProgramAttribsSetting;
  case ID_GITS_RENDERBUFFER_STORAGE:
    return new CgitsRenderbufferStorage;
  case ID_GITS_UNMAP_BUFFER:
    return new CgitsUnmapBuffer;
  case ID_GITS_VIEWPORT_SETTINGS:
    return new CgitsViewportSettings;
  case ID_GITS_GL_FLUSH_MAPPED_BUFFER_RANGE:
    return new CgitsFlushMappedBufferRange;
  case ID_GITS_COHERENT_BUFFER_MAPPING:
    return new CgitsCoherentBufferMapping;
  case ID_GITS_LINK_PROGRAM_BUFFERS_SETTINGS:
    return new CgitsLinkProgramBuffersSetting;
  case ID_UPDATE_MAPPED_TEXTURE:
    return new CUpdateMappedTexture;

#include "glIDswitch.h"
  default:;
  }

  Log(ERR) << "Unknown OpenGL function with ID: " << id;
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

CArgument& CFunction::Result(unsigned idx) {
  Log(ERR) << "Results not supported in OpenGL!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

namespace {
bool IsClearOrBlitDrawcall(CFunction* cfunc) {
  return (cfunc->Id() == CFunction::ID_GL_CLEAR ||
          cfunc->Id() == CFunction::ID_GL_BLIT_FRAMEBUFFER ||
          cfunc->Id() == CFunction::ID_GL_BLIT_FRAMEBUFFER_EXT);
}
bool IsClear(CFunction* cfunc) {
  switch (cfunc->Id()) {
  case CFunction::ID_GL_CLEAR:
    //case CFunction::ID_GL_CLEAR_BUFFERFI:
    //case CFunction::ID_GL_CLEAR_BUFFERFV:
    //case CFunction::ID_GL_CLEAR_BUFFERIV:
    //case CFunction::ID_GL_CLEAR_BUFFERUIV:
    return true;
  default:
    return false;
  }
}
bool IsCopyResource(CFunction* cfunc) {
  switch (cfunc->Id()) {
  case CFunction::ID_GL_COPY_TEXTURE_IMAGE_1D_EXT:
  case CFunction::ID_GL_COPY_TEXTURE_IMAGE_2D_EXT:
  case CFunction::ID_GL_COPY_TEXTURE_SUB_IMAGE_1D_EXT:
  case CFunction::ID_GL_COPY_TEXTURE_SUB_IMAGE_2D_EXT:
  case CFunction::ID_GL_COPY_TEXTURE_SUB_IMAGE_3D_EXT:
  case CFunction::ID_GL_COPY_MULTI_TEX_IMAGE_1D_EXT:
  case CFunction::ID_GL_COPY_MULTI_TEX_IMAGE_2D_EXT:
  case CFunction::ID_GL_COPY_MULTI_TEX_SUB_IMAGE_1D_EXT:
  case CFunction::ID_GL_COPY_MULTI_TEX_SUB_IMAGE_2D_EXT:
  case CFunction::ID_GL_COPY_MULTI_TEX_SUB_IMAGE_3D_EXT:
  case CFunction::ID_GL_BLIT_FRAMEBUFFER:
  case CFunction::ID_GL_BLIT_FRAMEBUFFER_ANGLE:
  case CFunction::ID_GL_BLIT_FRAMEBUFFER_EXT:
  case CFunction::ID_GL_COPY_PIXELS:
  case CFunction::ID_GL_COPY_TEX_IMAGE_1D:
  case CFunction::ID_GL_COPY_TEX_IMAGE_1D_EXT:
  case CFunction::ID_GL_COPY_TEX_IMAGE_2D:
  case CFunction::ID_GL_COPY_TEX_IMAGE_2D_EXT:
  case CFunction::ID_GL_COPY_TEX_SUB_IMAGE_1D:
  case CFunction::ID_GL_COPY_TEX_SUB_IMAGE_1D_EXT:
  case CFunction::ID_GL_COPY_TEX_SUB_IMAGE_2D:
  case CFunction::ID_GL_COPY_TEX_SUB_IMAGE_2D_EXT:
  case CFunction::ID_GL_COPY_TEX_SUB_IMAGE_3D:
  case CFunction::ID_GL_COPY_TEX_SUB_IMAGE_3D_EXT:
  case CFunction::ID_GL_COPY_BUFFER_SUB_DATA:
    return true;
  default:
    return false;
  }
}
} // namespace

CDrawFunction::CDrawFunction()
    : _drawNum(CGits::Instance().CurrentDrawCount()),
      _drawInFrameNum(CGits::Instance().CurrentDrawInFrameCount()),
      _frameNum(CGits::Instance().CurrentFrame()) {}

void CDrawFunction::Run() {
  this->DrawFunctionRun();
}

NOINLINE std::unique_ptr<ScissorStateStash> HandleForceScissor(CDrawFunction* ptr) {
  std::unique_ptr<ScissorStateStash> scissorStatePtr;
  if (!IsClearOrBlitDrawcall(ptr)) {
    std::vector<int> viewport(4);
    drv.gl.glGetIntegerv(GL_VIEWPORT, &viewport[0]);
    const auto& player = Config::Get().player;
    if (!player.affectViewport ||
        (viewport[2] == player.affectedViewport[0] && viewport[3] == player.affectedViewport[1])) {
      scissorStatePtr.reset(new ScissorStateStash());
      const std::vector<int>& rect = player.scissorCoords;
      drv.gl.glEnable(GL_SCISSOR_TEST);
      drv.gl.glScissor(rect[0], rect[1], rect[2], rect[3]);
    }
  }
  return scissorStatePtr;
}

NOINLINE void HandleCaptureDrawsPre() {
  const auto currentDrawCount = CGits::Instance().CurrentDrawCount();
  const auto& player = Config::Get().player;

  if (player.captureDrawsPre && player.captureDraws[currentDrawCount]) {
    capture_drawbuffer(GetPathForImageDumping(),
                       "drawcall-" + std::to_string(currentDrawCount) + "-pre", false);
  }
}

NOINLINE void HandleCaptureDraws2DTexs() {
  const auto currentDrawCount = CGits::Instance().CurrentDrawCount();
  const auto& player = Config::Get().player;

  if (player.captureDraws2DTexs[currentDrawCount]) {
    GLint activeTexUnit = 0;
    drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexUnit);
    GLint maxTexUnits = 0;
    if (curctx::IsEs1()) {
      drv.gl.glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxTexUnits);
    } else {
      drv.gl.glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);
    }

    std::set<GLint> boundTexSet;
    std::set<GLint>::iterator itSet;
    for (int i = 0; i < maxTexUnits; i++) {
      drv.gl.glActiveTexture(GL_TEXTURE0 + i);
      GLint boundTex = BoundTexture(GL_TEXTURE_2D);

      if (boundTexSet.find(boundTex) == boundTexSet.end()) {
        boundTexSet.insert(boundTex);
        capture_bound_texture2D(GL_TEXTURE_2D, GetPathForImageDumping(),
                                "draw" + std::to_string(currentDrawCount) + "_unit" +
                                    std::to_string(i) + "_tex" + std::to_string(boundTex));
      }
    }
    drv.gl.glActiveTexture(activeTexUnit);
  }
}

NOINLINE void HandleTrace() {
  const auto currentDrawCount = CGits::Instance().CurrentDrawCount();

  if (ShouldLog(TRACEV)) {
    StatePrinter statePrinter;
    statePrinter.PrintToLog();
  }
  Log(TRACE, NO_NEWLINE) << "draw: " << currentDrawCount
                         << " frame: " << CGits::Instance().CurrentFrame()
                         << " drawInFrame: " << CGits::Instance().CurrentDrawInFrameCount() << "  ";
}

NOINLINE void HandleCaptureDraws() {
  const auto currentDrawCount = CGits::Instance().CurrentDrawCount();
  const auto& player = Config::Get().player;

  if (player.captureDraws[currentDrawCount]) {
    capture_drawbuffer(GetPathForImageDumping(),
                       "drawcall-" + std::to_string(currentDrawCount) + "-post", false);
  }
}

void CDrawFunction::DrawFunctionRun() {
  CGits::Instance().DrawCountUp();

  const auto isStateRestore = CGits::Instance().IsStateRestoration();
  const auto currentDrawCount = CGits::Instance().CurrentDrawCount();
  const auto& player = Config::Get().player;

  const bool keepDraw = isStateRestore || (player.keepDraws[currentDrawCount] &&
                                           player.keepFrames[CGits::Instance().CurrentFrame()]);
  if (!keepDraw) {
    return;
  }
  std::unique_ptr<ScissorStateStash> scissorStatePtr;
  if (player.forceScissor) {
    scissorStatePtr = HandleForceScissor(this);
  }

  if (player.captureDrawsPre) {
    HandleCaptureDrawsPre();
  }

  if (!player.captureDraws2DTexs.empty()) {
    HandleCaptureDraws2DTexs();
  }
  CGits::Instance().traceGLAPIBypass = false;
  HandleTrace();

  // forward to drawcall implementation
  this->RunImpl();
  CGits::Instance().traceGLAPIBypass = true;

  if (!player.captureDraws.empty()) {
    HandleCaptureDraws();
  }

  CGits::Instance().traceGLAPIBypass = false;

  if (scissorStatePtr && !IsClearOrBlitDrawcall(this)) {
    scissorStatePtr->Restore();
  }
}

void CDrawFunction::Write(CCodeOStream& stream) const {
  stream << "\n/*draw: " << std::dec << _drawNum << " frame: " << _frameNum
         << " drawInFrame: " << _drawInFrameNum << "*/";
  gits::CFunction::Write(stream);
}
} // namespace OpenGL
} // namespace gits
