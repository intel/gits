// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglFunction.h
 *
 * @brief Declaration of OpenGL specific function wrapper
 *
 */

#pragma once

#include "openglCommon.h"
#include "function.h"
#include "platform.h"

namespace gits {
namespace OpenGL {
/**
     * @brief OpenGL library specific function wrapper
     *
     * gits::OpenGL::CFunction is an OpenGL library specific function
     * call wrapper.
     */
class CFunction : public gits::CFunction {
public:
  enum TVersion {
    VERSION_UNKNOWN,
    VERSION_1_0,
    VERSION_1_1,
    VERSION_1_2,
    VERSION_1_3,
    VERSION_1_4,
    VERSION_1_5,
    VERSION_2_0,
    VERSION_2_1,
    VERSION_3_0,
    VERSION_3_1,
    VERSION_3_2,
    VERSION_3_3,
    VERSION_4_0,
    VERSION_4_1,
    VERSION_4_2,
  };

  enum TApiType {
    GITS_GL_BIND_APITYPE = GITS_BIT_AT_IDX(0),
    GITS_GL_CREATE_APITYPE = GITS_BIT_AT_IDX(1),
    GITS_GL_DELETE_APITYPE = GITS_BIT_AT_IDX(2),
    GITS_GL_FILL_APITYPE = GITS_BIT_AT_IDX(3),
    GITS_GL_GEN_APITYPE = GITS_BIT_AT_IDX(4),
    GITS_GL_GET_APITYPE = GITS_BIT_AT_IDX(5),
    GITS_GL_MAP_APITYPE = GITS_BIT_AT_IDX(6),
    GITS_GL_PARAM_APITYPE = GITS_BIT_AT_IDX(7),
    GITS_GL_RENDER_APITYPE = GITS_BIT_AT_IDX(8),
    GITS_GL_RESOURCE_APITYPE = GITS_BIT_AT_IDX(9),
    GITS_GL_QUERY_APITYPE = GITS_BIT_AT_IDX(10),
    GITS_GL_COPY_APITYPE = GITS_BIT_AT_IDX(11),
    GITS_GL_EXEC_APITYPE = GITS_BIT_AT_IDX(12),
    GITS_GL_GET_ESSENTIAL_APITYPE = GITS_BIT_AT_IDX(13),
  };

  enum TId {
    BEGIN_GL = CToken::ID_OPENGL,
#include "glIDs.h"

    //GL HELPER
    BEGIN_GL_HELPER = CToken::ID_GL_HELPER_TOKENS,
    ID_UPDATE_MAPPED_TEXTURE,
    ID_HELPER_WGL_UPDATE_WINDOW,
    ID_GITS_CLIENT_ARRAYS_UPDATE,
    ID_GITS_CLIENT_INDIRECT_ARRAYS_UPDATE,
    ID_GITS_LINK_PROGRAM_ATTRIBS_SETTINGS,
    ID_GITS_RENDERBUFFER_STORAGE,
    ID_GITS_UNMAP_BUFFER,
    ID_GITS_VIEWPORT_SETTINGS,
    ID_RESTORE_DEFAULT_GL_FRAMEBUFFER,
    ID_GITS_GL_FLUSH_MAPPED_BUFFER_RANGE,
    ID_GITS_COHERENT_BUFFER_MAPPING,
    ID_HELPER_WGL_UPDATE_WINDOW_THREAD_AWARE, // Buggy token; we're keeping the ID for back-compat.
    ID_GITS_LINK_PROGRAM_BUFFERS_SETTINGS,

    //WGL
    BEGIN_WGL = CToken::ID_WGL,
    ID_WGL_GET_DEFAULT_PROC_ADDRESS,
    ID_WGL_GET_LAYER_PALETTE_ENTRIES,
    ID_WGL_CHOOSE_PIXEL_FORMAT,
    ID_WGL_SET_PIXEL_FORMAT,
    ID_WGL_CREATE_CONTEXT,
    ID_WGL_GET_PIXEL_FORMAT,
    ID_WGL_DESCRIBE_PIXEL_FORMAT,
    ID_WGL_MAKE_CURRENT,
    ID_WGL_DELETE_CONTEXT,
    ID_WGL_SWAP_BUFFERS,
    ID_WGL_USE_FONT_BITMAPS_A,
    ID_WGL_CREATE_PBUFFER_ARB,
    ID_WGL_GET_PBUFFER_DC_ARB,
    ID_WGL_RELEASE_PBUFFER_DC_ARB,
    ID_WGL_DESTROY_PBUFFER_ARB,
    ID_WGL_SHARE_LISTS,
    ID_WGL_SWAP_LAYER_BUFFERS,
    ID_WGL_USE_FONT_BITMAPS_W,
    ID_WGL_USE_FONT_OUTLINES_A,
    ID_WGL_USE_FONT_OUTLINES_W,
    ID_WGL_CREATE_CONTEXT_ATTRIBS_ARB,
    ID_WGL_GET_CURRENT_DC,
    ID_WGL_GET_CURRENT_CONTEXT,
    ID_WGL_CREATE_LAYER_CONTEXT,
    ID_WGL_COPY_CONTEXT,
    ID_WGL_GET_PROC_ADDRESS,
    ID_WGL_CREATE_BUFFER_REGION_ARB,
    ID_WGL_DELETE_BUFFER_REGION_ARB,
    ID_WGL_SAVE_BUFFER_REGION_ARB,
    ID_WGL_RESTORE_BUFFER_REGION_ARB,
    ID_WGL_GET_EXTENSIONS_STRING_ARB,
    ID_WGL_GET_PIXEL_FORMAT_ATTRIBIV_ARB,
    ID_WGL_GET_PIXEL_FORMAT_ATTRIBFV_ARB,
    ID_WGL_CHOOSE_PIXEL_FORMAT_ARB,
    ID_WGL_MAKE_CONTEXT_CURRENT_ARB,
    ID_WGL_GET_CURRENT_READ_DC_ARB,
    ID_WGL_QUERY_PBUFFER_ARB,
    ID_WGL_BIND_TEX_IMAGE_ARB,
    ID_WGL_RELEASE_TEX_IMAGE_ARB,
    ID_WGL_SET_PBUFFER_ATTRIB_ARB,
    ID_WGL_CREATE_DISPLAY_COLOR_TABLE_EXT,
    ID_WGL_LOAD_DISPLAY_COLOR_TABLE_EXT,
    ID_WGL_BIND_DISPLAY_COLOR_TABLE_EXT,
    ID_WGL_DESTROY_DISPLAY_COLOR_TABLE_EXT,
    ID_WGL_GET_EXTENSIONS_STRING_EXT,
    ID_WGL_MAKE_CONTEXT_CURRENT_EXT,
    ID_WGL_GET_CURRENT_READ_DC_EXT,
    ID_WGL_CREATE_PBUFFER_EXT,
    ID_WGL_GET_PBUFFER_DC_EXT,
    ID_WGL_RELEASE_PBUFFER_DC_EXT,
    ID_WGL_DESTROY_PBUFFER_EXT,
    ID_WGL_QUERY_PBUFFER_EXT,
    ID_WGL_GET_PIXEL_FORMAT_ATTRIBIV_EXT,
    ID_WGL_GET_PIXEL_FORMAT_ATTRIBFV_EXT,
    ID_WGL_CHOOSE_PIXEL_FORMAT_EXT,
    ID_WGL_SWAP_INTERVAL_EXT,
    ID_WGL_GET_SWAP_INTERVAL_EXT,
    ID_WGL_ALLOCATE_MEMORY_NV,
    ID_WGL_FREE_MEMORY_NV,
    ID_WGL_GET_SYNC_VALUES_OML,
    ID_WGL_GET_MSC_RATE_OML,
    ID_WGL_SWAP_BUFFERS_MSC_OML,
    ID_WGL_SWAP_LAYER_BUFFERS_MSC_OML,
    ID_WGL_WAIT_FOR_MSC_OML,
    ID_WGL_WAIT_FOR_SBC_OML,
    ID_WGL_GET_DIGITAL_VIDEO_PARAMETERS_I3D,
    ID_WGL_SET_DIGITAL_VIDEO_PARAMETERS_I3D,
    ID_WGL_GET_GAMMA_TABLE_PARAMETERS_I3D,
    ID_WGL_SET_GAMMA_TABLE_PARAMETERS_I3D,
    ID_WGL_GET_GAMMA_TABLE_I3D,
    ID_WGL_SET_GAMMA_TABLE_I3D,
    ID_WGL_ENABLE_GENLOCK_I3D,
    ID_WGL_DISABLE_GENLOCK_I3D,
    ID_WGL_IS_ENABLED_GENLOCK_I3D,
    ID_WGL_GENLOCK_SOURCE_I3D,
    ID_WGL_GET_GENLOCK_SOURCE_I3D,
    ID_WGL_GENLOCK_SOURCE_EDGE_I3D,
    ID_WGL_GET_GENLOCK_SOURCE_EDGE_I3D,
    ID_WGL_GENLOCK_SAMPLE_RATE_I3D,
    ID_WGL_GET_GENLOCK_SAMPLE_RATE_I3D,
    ID_WGL_GENLOCK_SOURCE_DELAY_I3D,
    ID_WGL_GET_GENLOCK_SOURCE_DELAY_I3D,
    ID_WGL_QUERY_GENLOCK_MAX_SOURCE_DELAY_I3D,
    ID_WGL_CREATE_IMAGE_BUFFER_I3D,
    ID_WGL_DESTROY_IMAGE_BUFFER_I3D,
    ID_WGL_ASSOCIATE_IMAGE_BUFFER_EVENTS_I3D,
    ID_WGL_RELEASE_IMAGE_BUFFER_EVENTS_I3D,
    ID_WGL_ENABLE_FRAME_LOCK_I3D,
    ID_WGL_DISABLE_FRAME_LOCK_I3D,
    ID_WGL_IS_ENABLED_FRAME_LOCK_I3D,
    ID_WGL_QUERY_FRAME_LOCK_MASTER_I3D,
    ID_WGL_GET_FRAME_USAGE_I3D,
    ID_WGL_BEGIN_FRAME_TRACKING_I3D,
    ID_WGL_END_FRAME_TRACKING_I3D,
    ID_WGL_QUERY_FRAME_TRACKING_I3D,
    ID_WGL_SET_STEREO_EMITTER_STATE3DL,
    ID_WGL_ENUMERATE_VIDEO_DEVICES_NV,
    ID_WGL_BIND_VIDEO_DEVICE_NV,
    ID_WGL_QUERY_CURRENT_CONTEXT_NV,
    ID_WGL_GET_VIDEO_DEVICE_NV,
    ID_WGL_RELEASE_VIDEO_DEVICE_NV,
    ID_WGL_BIND_VIDEO_IMAGE_NV,
    ID_WGL_RELEASE_VIDEO_IMAGE_NV,
    ID_WGL_SEND_PBUFFER_TO_VIDEO_NV,
    ID_WGL_GET_VIDEO_INFO_NV,
    ID_WGL_JOIN_SWAP_GROUP_NV,
    ID_WGL_BIND_SWAP_BARRIER_NV,
    ID_WGL_QUERY_SWAP_GROUP_NV,
    ID_WGL_QUERY_MAX_SWAP_GROUPS_NV,
    ID_WGL_QUERY_FRAME_COUNT_NV,
    ID_WGL_RESET_FRAME_COUNT_NV,
    ID_WGL_ENUM_GPUS_NV,
    ID_WGL_ENUM_GPU_DEVICES_NV,
    ID_WGL_CREATE_AFFINITY_DCNV,
    ID_WGL_ENUM_GPUS_FROM_AFFINITY_DCNV,
    ID_WGL_DELETE_DCNV,
    ID_WGL_GET_GPUIDS_AMD,
    ID_WGL_GET_GPUINFO_AMD,
    ID_WGL_GET_CONTEXT_GPUIDAMD,
    ID_WGL_CREATE_ASSOCIATED_CONTEXT_AMD,
    ID_WGL_CREATE_ASSOCIATED_CONTEXT_ATTRIBS_AMD,
    ID_WGL_DELETE_ASSOCIATED_CONTEXT_AMD,
    ID_WGL_MAKE_ASSOCIATED_CONTEXT_CURRENT_AMD,
    ID_WGL_GET_CURRENT_ASSOCIATED_CONTEXT_AMD,
    ID_WGL_BLIT_CONTEXT_FRAMEBUFFER_AMD,
    ID_WGL_BIND_VIDEO_CAPTURE_DEVICE_NV,
    ID_WGL_ENUMERATE_VIDEO_CAPTURE_DEVICES_NV,
    ID_WGL_LOCK_VIDEO_CAPTURE_DEVICE_NV,
    ID_WGL_QUERY_VIDEO_CAPTURE_DEVICE_NV,
    ID_WGL_RELEASE_VIDEO_CAPTURE_DEVICE_NV,
    ID_WGL_COPY_IMAGE_SUB_DATA_NV,
    ID_WGL_SWAP_MULTIPLE_BUFFERS,

    //GLX
    BEGIN_GLX = CToken::ID_GLX,
    ID_GLX_CHOOSE_VISUAL,
    ID_GLX_CREATE_CONTEXT,
    ID_GLX_DESTROY_CONTEXT,
    ID_GLX_GET_PROC_ADDRESS_ARB,
    ID_GLX_MAKE_CURRENT,
    ID_GLX_SWAP_BUFFERS,
    ID_GLX_CREATE_CONTEXT_ATTRIBS_ARB,
    ID_GLX_CHOOSE_FB_CONFIG,
    ID_GLX_GET_VISUAL_FROM_FB_CONFIG,
    ID_GLX_QUERY_EXTENSIONS_STRING,
    ID_GLX_MAKE_CONTEXT_CURRENT,
    ID_GLX_COPY_CONTEXT,
    ID_GLX_CREATE_GLXPIXMAP,
    ID_GLX_DESTROY_GLXPIXMAP,
    ID_GLX_QUERY_EXTENSION,
    ID_GLX_QUERY_VERSION,
    ID_GLX_IS_DIRECT,
    ID_GLX_GET_CONFIG,
    ID_GLX_GET_CURRENT_CONTEXT,
    ID_GLX_GET_CURRENT_DRAWABLE,
    ID_GLX_WAIT_GL,
    ID_GLX_WAIT_X,
    ID_GLX_USE_XFONT,
    ID_GLX_QUERY_SERVER_STRING,
    ID_GLX_GET_CLIENT_STRING,
    ID_GLX_GET_CURRENT_DISPLAY,
    ID_GLX_GET_FBCONFIG_ATTRIB,
    ID_GLX_GET_FBCONFIGS,
    ID_GLX_CREATE_WINDOW,
    ID_GLX_DESTROY_WINDOW,
    ID_GLX_CREATE_PIXMAP,
    ID_GLX_DESTROY_PIXMAP,
    ID_GLX_CREATE_PBUFFER,
    ID_GLX_DESTROY_PBUFFER,
    ID_GLX_QUERY_DRAWABLE,
    ID_GLX_CREATE_NEW_CONTEXT,
    ID_GLX_GET_CURRENT_READ_DRAWABLE,
    ID_GLX_QUERY_CONTEXT,
    ID_GLX_SELECT_EVENT,
    ID_GLX_GET_SELECTED_EVENT,
    ID_GLX_ALLOCATE_MEMORY_NV,
    ID_GLX_FREE_MEMORY_NV,
    ID_GLX_BIND_TEX_IMAGE_ARB,
    ID_GLX_RELEASE_TEX_IMAGE_ARB,
    ID_GLX_DRAWABLE_ATTRIB_ARB,
    ID_GLX_GET_FRAME_USAGE_MESA,
    ID_GLX_BEGIN_FRAME_TRACKING_MESA,
    ID_GLX_END_FRAME_TRACKING_MESA,
    ID_GLX_QUERY_FRAME_TRACKING_MESA,
    ID_GLX_SWAP_INTERVAL_MESA,
    ID_GLX_GET_SWAP_INTERVAL_MESA,
    ID_GLX_BIND_TEX_IMAGE_EXT,
    ID_GLX_RELEASE_TEX_IMAGE_EXT,
    ID_GLX_GET_PROC_ADDRESS,
    ID_GLX_SWAP_INTERVAL_SGI,

    //EGL
    BEGIN_EGL = CToken::ID_EGL,
    ID_EGL_BIND_API,
    ID_EGL_BIND_TEX_IMAGE,
    ID_EGL_CHOOSE_CONFIG,
    ID_EGL_COPY_BUFFERS,
    ID_EGL_CREATE_CONTEXT,
    ID_EGL_CREATE_PBUFFER_FROM_CLIENT_BUFFER,
    ID_EGL_CREATE_PBUFFER_SURFACE,
    ID_EGL_CREATE_PIXMAP_SURFACE,
    ID_EGL_CREATE_WINDOW_SURFACE,
    ID_EGL_DESTROY_CONTEXT,
    ID_EGL_DESTROY_SURFACE,
    ID_EGL_GET_CONFIG_ATTRIB,
    ID_EGL_GET_CONFIGS,
    ID_EGL_GET_CURRENT_CONTEXT,
    ID_EGL_GET_CURRENT_DISPLAY,
    ID_EGL_GET_CURRENT_SURFACE,
    ID_EGL_GET_DISPLAY,
    ID_EGL_GET_ERROR,
    ID_EGL_GET_PROC_ADDRESS,
    ID_EGL_INITIALIZE,
    ID_EGL_MAKE_CURRENT,
    ID_EGL_QUERY_API,
    ID_EGL_QUERY_CONTEXT,
    ID_EGL_QUERY_STRING,
    ID_EGL_QUERY_SURFACE,
    ID_EGL_RELEASE_TEX_IMAGE,
    ID_EGL_RELEASE_THREAD,
    ID_EGL_SURFACE_ATTRIB,
    ID_EGL_SWAP_BUFFERS,
    ID_EGL_SWAP_INTERVAL,
    ID_EGL_TERMINATE,
    ID_EGL_WAIT_CLIENT,
    ID_EGL_WAIT_GL,
    ID_EGL_WAIT_NATIVE,
    ID_EGL_CLIENT_WAIT_SYNC_KHR,
    ID_EGL_CLIENT_WAIT_SYNC_NV,
    ID_EGL_CREATE_DRMIMAGE_MESA,
    ID_EGL_CREATE_FENCE_SYNC_NV,
    ID_EGL_CREATE_IMAGE_KHR,
    ID_EGL_CREATE_PIXMAP_SURFACE_HI,
    ID_EGL_CREATE_STREAM_FROM_FILE_DESCRIPTOR_KHR,
    ID_EGL_CREATE_STREAM_KHR,
    ID_EGL_CREATE_STREAM_PRODUCER_SURFACE_KHR,
    ID_EGL_CREATE_SYNC_KHR,
    ID_EGL_DESTROY_IMAGE_KHR,
    ID_EGL_DESTROY_STREAM_KHR,
    ID_EGL_DESTROY_SYNC_KHR,
    ID_EGL_DESTROY_SYNC_NV,
    ID_EGL_EXPORT_DRMIMAGE_MESA,
    ID_EGL_FENCE_NV,
    ID_EGL_GET_STREAM_FILE_DESCRIPTOR_KHR,
    ID_EGL_GET_SYNC_ATTRIB_KHR,
    ID_EGL_GET_SYNC_ATTRIB_NV,
    ID_EGL_GET_SYSTEM_TIME_FREQUENCY_NV,
    ID_EGL_GET_SYSTEM_TIME_NV,
    ID_EGL_LOCK_SURFACE_KHR,
    ID_EGL_POST_SUB_BUFFER_NV,
    ID_EGL_QUERY_STREAM_KHR,
    ID_EGL_QUERY_STREAM_TIME_KHR,
    ID_EGL_QUERY_STREAMU64KHR,
    ID_EGL_QUERY_SURFACE_POINTER_ANGLE,
    ID_EGL_SIGNAL_SYNC_KHR,
    ID_EGL_SIGNAL_SYNC_NV,
    ID_EGL_STREAM_ATTRIB_KHR,
    ID_EGL_STREAM_CONSUMER_ACQUIRE_KHR,
    ID_EGL_STREAM_CONSUMER_GLTEXTURE_EXTERNAL_KHR,
    ID_EGL_STREAM_CONSUMER_RELEASE_KHR,
    ID_EGL_UNLOCK_SURFACE_KHR,
    ID_EGL_WAIT_SYNC_KHR,
    ID_EGL_SET_BLOB_CACHE_FUNCS_ANDROID,
    ID_EGL_GET_PLATFORM_DISPLAY_EXT,
    ID_EGL_CREATE_PLATFORM_WINDOW_SURFACE_EXT,

    //end of enums marker
    ID_FUNCTION_END
  };

  static CFunction* Create(unsigned id);
  virtual const CArgument* Return() const {
    return nullptr;
  }
  virtual unsigned ResultCount() const {
    return 0;
  }
  virtual CArgument& Result(unsigned idx);

  virtual CLibrary::TId LibraryId() const {
    return CLibrary::ID_OPENGL;
  }
};

class CDrawFunction : public CFunction {
public:
  CDrawFunction();
  // Common 'Run' for all drawcalls. Do not override.
  virtual void Run();
  virtual void RunImpl() = 0;
  virtual void Write(CCodeOStream& stream) const;

private:
  unsigned _drawNum;
  unsigned _drawInFrameNum;
  unsigned _frameNum;
  void DrawFunctionRun();
};
} // namespace OpenGL

} // namespace gits
