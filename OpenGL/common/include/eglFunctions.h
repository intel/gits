// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   eglFunctions.h
*
*/

#pragma once

#include "openglFunction.h"
#include "eglArguments.h"
#include "mapping.h"

namespace gits {
namespace OpenGL {
EGLConfig FindConfigEGL(EGLDisplay dpy, const std::vector<EGLint>& attribs);

/**
    * @brief OpenGL eglGetError() function call wrapper.
    *
    * OpenGL eglGetError() function call wrapper.
    * Returns: EGLint
    */
class CeglGetError : public CFunction {
  CEGLint _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetError();
  CeglGetError(EGLint return_value);
  virtual unsigned Id() const {
    return ID_EGL_GET_ERROR;
  }
  virtual const char* Name() const {
    return "eglGetError";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetDisplay() function call wrapper.
    *
    * OpenGL eglGetDisplay() function call wrapper.
    * Returns: EGLDisplay
    */
class CeglGetDisplay : public CFunction {
  CEGLDisplay _return_value;
  CEGLNativeDisplayType _display_id;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetDisplay();
  CeglGetDisplay(EGLDisplay return_value, EGLNativeDisplayType display_id);
  virtual unsigned Id() const {
    return ID_EGL_GET_DISPLAY;
  }
  virtual const char* Name() const {
    return "eglGetDisplay";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglInitialize() function call wrapper.
    *
    * OpenGL eglInitialize() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglInitialize : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLint _major;
  CEGLint _minor;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglInitialize();
  CeglInitialize(EGLBoolean return_value, EGLDisplay dpy, EGLint* major, EGLint* minor);
  virtual unsigned Id() const {
    return ID_EGL_INITIALIZE;
  }
  virtual const char* Name() const {
    return "eglInitialize";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglTerminate() function call wrapper.
    *
    * OpenGL eglTerminate() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglTerminate : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglTerminate();
  CeglTerminate(EGLBoolean return_value, EGLDisplay dpy);
  virtual unsigned Id() const {
    return ID_EGL_TERMINATE;
  }
  virtual const char* Name() const {
    return "eglTerminate";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglQueryString() function call wrapper.
    *
    * OpenGL eglQueryString() function call wrapper.
    * Returns: const char *
    */
class CeglQueryString : public CFunction {
  CGLchar::CSArray _return_value;
  CEGLDisplay _dpy;
  CEGLint _name;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglQueryString();
  CeglQueryString(const char* return_value, EGLDisplay dpy, EGLint name);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_STRING;
  }
  virtual const char* Name() const {
    return "eglQueryString";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetConfigs() function call wrapper.
    *
    * OpenGL eglGetConfigs() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglGetConfigs : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLConfig::CSArray _configs;
  CEGLint _config_size;
  CEGLint _num_config;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetConfigs();
  CeglGetConfigs(EGLBoolean return_value,
                 EGLDisplay dpy,
                 EGLConfig* configs,
                 EGLint config_size,
                 EGLint* num_config);
  virtual unsigned Id() const {
    return ID_EGL_GET_CONFIGS;
  }
  virtual const char* Name() const {
    return "eglGetConfigs";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglChooseConfig() function call wrapper.
    *
    * OpenGL eglChooseConfig() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglChooseConfig : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLint::CSArray _attrib_list;
  CEGLConfig::CSArray _configs;
  CEGLint _config_size;
  CEGLint _num_config;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglChooseConfig();
  CeglChooseConfig(EGLBoolean return_value,
                   EGLDisplay dpy,
                   const EGLint* attrib_list,
                   EGLConfig* configs,
                   EGLint config_size,
                   EGLint* num_config);
  virtual unsigned Id() const {
    return ID_EGL_CHOOSE_CONFIG;
  }
  virtual const char* Name() const {
    return "eglChooseConfig";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetConfigAttrib() function call wrapper.
    *
    * OpenGL eglGetConfigAttrib() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglGetConfigAttrib : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CEGLint _attribute;
  COutArgument<0> _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetConfigAttrib();
  CeglGetConfigAttrib(
      EGLBoolean return_value, EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value);
  virtual unsigned Id() const {
    return ID_EGL_GET_CONFIG_ATTRIB;
  }
  virtual const char* Name() const {
    return "eglGetConfigAttrib";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglCreateWindowSurface() function call wrapper.
    *
    * OpenGL eglCreateWindowSurface() function call wrapper.
    * Returns: EGLSurface
    */
class CeglCreateWindowSurface : public CFunction {
  CEGLSurface _return_value;
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CEGLNativeWindowType _win;
  CEGLint::CSArray _attrib_list;
  CEGLint::CSArray _config_attribs;
  CEGLint::CSArray _surface_attribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreateWindowSurface();
  CeglCreateWindowSurface(EGLSurface return_value,
                          EGLDisplay dpy,
                          EGLConfig config,
                          EGLNativeWindowType win,
                          const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_WINDOW_SURFACE;
  }
  virtual const char* Name() const {
    return "eglCreateWindowSurface";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglCreatePbufferSurface() function call wrapper.
    *
    * OpenGL eglCreatePbufferSurface() function call wrapper.
    * Returns: EGLSurface
    */
class CeglCreatePbufferSurface : public CFunction {
  CEGLSurface _return_value;
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CEGLint::CSArray _attrib_list;
  CEGLint::CSArray _config_attribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreatePbufferSurface();
  CeglCreatePbufferSurface(EGLSurface return_value,
                           EGLDisplay dpy,
                           EGLConfig config,
                           const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_PBUFFER_SURFACE;
  }
  virtual const char* Name() const {
    return "eglCreatePbufferSurface";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglCreatePixmapSurface() function call wrapper.
    *
    * OpenGL eglCreatePixmapSurface() function call wrapper.
    * Returns: EGLSurface
    */
class CeglCreatePixmapSurface : public CFunction {
  CEGLSurface _return_value;
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CEGLNativePixmapType _pixmap;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreatePixmapSurface();
  CeglCreatePixmapSurface(EGLSurface return_value,
                          EGLDisplay dpy,
                          EGLConfig config,
                          EGLNativePixmapType pixmap,
                          const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_PIXMAP_SURFACE;
  }
  virtual const char* Name() const {
    return "eglCreatePixmapSurface";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglDestroySurface() function call wrapper.
    *
    * OpenGL eglDestroySurface() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglDestroySurface : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglDestroySurface();
  CeglDestroySurface(EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface);
  virtual unsigned Id() const {
    return ID_EGL_DESTROY_SURFACE;
  }
  virtual const char* Name() const {
    return "eglDestroySurface";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglQuerySurface() function call wrapper.
    *
    * OpenGL eglQuerySurface() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglQuerySurface : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLint _attribute;
  CEGLint _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglQuerySurface();
  CeglQuerySurface(
      EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_SURFACE;
  }
  virtual const char* Name() const {
    return "eglQuerySurface";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglBindAPI() function call wrapper.
    *
    * OpenGL eglBindAPI() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglBindAPI : public CFunction {
  CEGLBoolean _return_value;
  CEGLenum _api;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglBindAPI();
  CeglBindAPI(EGLBoolean return_value, EGLenum api);
  virtual unsigned Id() const {
    return ID_EGL_BIND_API;
  }
  virtual const char* Name() const {
    return "eglBindAPI";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglQueryAPI() function call wrapper.
    *
    * OpenGL eglQueryAPI() function call wrapper.
    * Returns: EGLenum
    */
class CeglQueryAPI : public CFunction {
  CEGLenum _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglQueryAPI();
  CeglQueryAPI(EGLenum return_value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_API;
  }
  virtual const char* Name() const {
    return "eglQueryAPI";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglWaitClient() function call wrapper.
    *
    * OpenGL eglWaitClient() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglWaitClient : public CFunction {
  CEGLBoolean _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglWaitClient();
  CeglWaitClient(EGLBoolean return_value);
  virtual unsigned Id() const {
    return ID_EGL_WAIT_CLIENT;
  }
  virtual const char* Name() const {
    return "eglWaitClient";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglReleaseThread() function call wrapper.
    *
    * OpenGL eglReleaseThread() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglReleaseThread : public CFunction {
  CEGLBoolean _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglReleaseThread();
  CeglReleaseThread(EGLBoolean return_value);
  virtual unsigned Id() const {
    return ID_EGL_RELEASE_THREAD;
  }
  virtual const char* Name() const {
    return "eglReleaseThread";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglCreatePbufferFromClientBuffer() function call wrapper.
    *
    * OpenGL eglCreatePbufferFromClientBuffer() function call wrapper.
    * Returns: EGLSurface
    */
class CeglCreatePbufferFromClientBuffer : public CFunction {
  CEGLSurface _return_value;
  CEGLDisplay _dpy;
  CEGLenum _buftype;
  CEGLClientBuffer _buffer;
  CEGLConfig _config;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreatePbufferFromClientBuffer();
  CeglCreatePbufferFromClientBuffer(EGLSurface return_value,
                                    EGLDisplay dpy,
                                    EGLenum buftype,
                                    EGLClientBuffer buffer,
                                    EGLConfig config,
                                    const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_PBUFFER_FROM_CLIENT_BUFFER;
  }
  virtual const char* Name() const {
    return "eglCreatePbufferFromClientBuffer";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglSurfaceAttrib() function call wrapper.
    *
    * OpenGL eglSurfaceAttrib() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglSurfaceAttrib : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLint _attribute;
  CEGLint _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglSurfaceAttrib();
  CeglSurfaceAttrib(
      EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
  virtual unsigned Id() const {
    return ID_EGL_SURFACE_ATTRIB;
  }
  virtual const char* Name() const {
    return "eglSurfaceAttrib";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglBindTexImage() function call wrapper.
    *
    * OpenGL eglBindTexImage() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglBindTexImage : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLint _buffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglBindTexImage();
  CeglBindTexImage(EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface, EGLint buffer);
  virtual unsigned Id() const {
    return ID_EGL_BIND_TEX_IMAGE;
  }
  virtual const char* Name() const {
    return "eglBindTexImage";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglReleaseTexImage() function call wrapper.
    *
    * OpenGL eglReleaseTexImage() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglReleaseTexImage : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLint _buffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglReleaseTexImage();
  CeglReleaseTexImage(EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface, EGLint buffer);
  virtual unsigned Id() const {
    return ID_EGL_RELEASE_TEX_IMAGE;
  }
  virtual const char* Name() const {
    return "eglReleaseTexImage";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglSwapInterval() function call wrapper.
    *
    * OpenGL eglSwapInterval() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglSwapInterval : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLint _interval;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglSwapInterval();
  CeglSwapInterval(EGLBoolean return_value, EGLDisplay dpy, EGLint interval);
  virtual unsigned Id() const {
    return ID_EGL_SWAP_INTERVAL;
  }
  virtual const char* Name() const {
    return "eglSwapInterval";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglCreateContext() function call wrapper.
    *
    * OpenGL eglCreateContext() function call wrapper.
    * Returns: EGLContext
    */
class CeglCreateContext : public CFunction {
  CEGLContext _return_value;
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CEGLContext _share_context;
  CEGLint::CSArray _attrib_list;
  CEGLint::CSArray _config_attribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreateContext();
  CeglCreateContext(EGLContext return_value,
                    EGLDisplay dpy,
                    EGLConfig config,
                    EGLContext share_context,
                    const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_CONTEXT;
  }
  virtual const char* Name() const {
    return "eglCreateContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglDestroyContext() function call wrapper.
    *
    * OpenGL eglDestroyContext() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglDestroyContext : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLContext _ctx;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglDestroyContext();
  CeglDestroyContext(EGLBoolean return_value, EGLDisplay dpy, EGLContext ctx);
  virtual unsigned Id() const {
    return ID_EGL_DESTROY_CONTEXT;
  }
  virtual const char* Name() const {
    return "eglDestroyContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglMakeCurrent() function call wrapper.
    *
    * OpenGL eglMakeCurrent() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglMakeCurrent : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _draw;
  CEGLSurface _read;
  CEGLContext _ctx;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglMakeCurrent();
  CeglMakeCurrent(
      EGLBoolean return_value, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
  virtual unsigned Id() const {
    return ID_EGL_MAKE_CURRENT;
  }
  virtual const char* Name() const {
    return "eglMakeCurrent";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetCurrentContext() function call wrapper.
    *
    * OpenGL eglGetCurrentContext() function call wrapper.
    * Returns: EGLContext
    */
class CeglGetCurrentContext : public CFunction {
  CEGLContext _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetCurrentContext();
  CeglGetCurrentContext(EGLContext return_value);
  virtual unsigned Id() const {
    return ID_EGL_GET_CURRENT_CONTEXT;
  }
  virtual const char* Name() const {
    return "eglGetCurrentContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetCurrentSurface() function call wrapper.
    *
    * OpenGL eglGetCurrentSurface() function call wrapper.
    * Returns: EGLSurface
    */
class CeglGetCurrentSurface : public CFunction {
  CEGLSurface _return_value;
  CEGLint _readdraw;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetCurrentSurface();
  CeglGetCurrentSurface(EGLSurface return_value, EGLint readdraw);
  virtual unsigned Id() const {
    return ID_EGL_GET_CURRENT_SURFACE;
  }
  virtual const char* Name() const {
    return "eglGetCurrentSurface";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetCurrentDisplay() function call wrapper.
    *
    * OpenGL eglGetCurrentDisplay() function call wrapper.
    * Returns: EGLDisplay
    */
class CeglGetCurrentDisplay : public CFunction {
  CEGLDisplay _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetCurrentDisplay();
  CeglGetCurrentDisplay(EGLDisplay return_value);
  virtual unsigned Id() const {
    return ID_EGL_GET_CURRENT_DISPLAY;
  }
  virtual const char* Name() const {
    return "eglGetCurrentDisplay";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglQueryContext() function call wrapper.
    *
    * OpenGL eglQueryContext() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglQueryContext : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLContext _ctx;
  CEGLint _attribute;
  CEGLint _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglQueryContext();
  CeglQueryContext(
      EGLBoolean return_value, EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_CONTEXT;
  }
  virtual const char* Name() const {
    return "eglQueryContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglWaitGL() function call wrapper.
    *
    * OpenGL eglWaitGL() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglWaitGL : public CFunction {
  CEGLBoolean _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglWaitGL();
  CeglWaitGL(EGLBoolean return_value);
  virtual unsigned Id() const {
    return ID_EGL_WAIT_GL;
  }
  virtual const char* Name() const {
    return "eglWaitGL";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglWaitNative() function call wrapper.
    *
    * OpenGL eglWaitNative() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglWaitNative : public CFunction {
  CEGLBoolean _return_value;
  CEGLint _engine;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglWaitNative();
  CeglWaitNative(EGLBoolean return_value, EGLint engine);
  virtual unsigned Id() const {
    return ID_EGL_WAIT_NATIVE;
  }
  virtual const char* Name() const {
    return "eglWaitNative";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglSwapBuffers() function call wrapper.
    *
    * OpenGL eglSwapBuffers() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglSwapBuffers : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglSwapBuffers();
  CeglSwapBuffers(EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface);
  virtual unsigned Id() const {
    return ID_EGL_SWAP_BUFFERS;
  }
  virtual const char* Name() const {
    return "eglSwapBuffers";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglCopyBuffers() function call wrapper.
    *
    * OpenGL eglCopyBuffers() function call wrapper.
    * Returns: EGLBoolean
    */
class CeglCopyBuffers : public CFunction {
  CEGLBoolean _return_value;
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLNativePixmapType _target;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCopyBuffers();
  CeglCopyBuffers(EGLBoolean return_value,
                  EGLDisplay dpy,
                  EGLSurface surface,
                  EGLNativePixmapType target);
  virtual unsigned Id() const {
    return ID_EGL_COPY_BUFFERS;
  }
  virtual const char* Name() const {
    return "eglCopyBuffers";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL eglGetProcAddress() function call wrapper.
    *
    * OpenGL eglGetProcAddress() function call wrapper.
    * Returns: __eglMustCastToProperFunctionPointerType
    */
class CeglGetProcAddress : public CFunction {
  CGLsizeiptr _return_value;
  CGLchar::CSArray _procname;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetProcAddress();
  CeglGetProcAddress(GLsizeiptr return_value, const char* procname);
  virtual unsigned Id() const {
    return ID_EGL_GET_PROC_ADDRESS;
  }
  virtual const char* Name() const {
    return "eglGetProcAddress";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglClientWaitSyncKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLSyncKHR _sync;
  CEGLint _flags;
  CGLuint64 _timeout;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglClientWaitSyncKHR();
  CeglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
  virtual unsigned Id() const {
    return ID_EGL_CLIENT_WAIT_SYNC_KHR;
  }
  virtual const char* Name() const {
    return "eglClientWaitSyncKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglClientWaitSyncNV : public CFunction {
  CEGLSyncKHR _sync;
  CEGLint _flags;
  CGLuint64 _timeout;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglClientWaitSyncNV();
  CeglClientWaitSyncNV(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout);
  virtual unsigned Id() const {
    return ID_EGL_CLIENT_WAIT_SYNC_NV;
  }
  virtual const char* Name() const {
    return "eglClientWaitSyncNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateDRMImageMESA : public CFunction {
  CEGLDisplay _dpy;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglCreateDRMImageMESA();
  CeglCreateDRMImageMESA(EGLDisplay dpy, const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_DRMIMAGE_MESA;
  }
  virtual const char* Name() const {
    return "eglCreateDRMImageMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateFenceSyncNV : public CFunction {
  CEGLDisplay _dpy;
  CEGLenum _condition;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglCreateFenceSyncNV();
  CeglCreateFenceSyncNV(EGLDisplay dpy, EGLenum condition, const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_FENCE_SYNC_NV;
  }
  virtual const char* Name() const {
    return "eglCreateFenceSyncNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateImageKHR : public CFunction {
  CEGLImageKHR _return_value;
  CEGLDisplay _dpy;
  CEGLContext _ctx;
  CEGLenum _target;
  CEGLClientBuffer _buffer;
  CEGLint::CSArray _attrib_list;

  CBinaryResource _anb_resource;
  CEGLint _anb_width;
  CEGLint _anb_height;
  CEGLint _anb_format;
  CEGLint _anb_usage;
  CEGLint _anb_stride;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const;
  virtual const CArgument* Return() const {
    if (stream_older_than(GITS_EGL_IMAGE_MAPPING)) {
      return nullptr;
    } else {
      return &_return_value;
    }
  }

public:
  CeglCreateImageKHR();
  CeglCreateImageKHR(EGLImageKHR return_value,
                     EGLDisplay dpy,
                     EGLContext ctx,
                     EGLenum target,
                     EGLClientBuffer buffer,
                     const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_IMAGE_KHR;
  }
  virtual const char* Name() const {
    return "eglCreateImageKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreatePixmapSurfaceHI : public CFunction {
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CBadArg _pixmap;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglCreatePixmapSurfaceHI();
  CeglCreatePixmapSurfaceHI(EGLDisplay dpy, EGLConfig config, void* pixmap);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_PIXMAP_SURFACE_HI;
  }
  virtual const char* Name() const {
    return "eglCreatePixmapSurfaceHI";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateStreamFromFileDescriptorKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _file_descriptor;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglCreateStreamFromFileDescriptorKHR();
  CeglCreateStreamFromFileDescriptorKHR(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_STREAM_FROM_FILE_DESCRIPTOR_KHR;
  }
  virtual const char* Name() const {
    return "eglCreateStreamFromFileDescriptorKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateStreamKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglCreateStreamKHR();
  CeglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_STREAM_KHR;
  }
  virtual const char* Name() const {
    return "eglCreateStreamKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateStreamProducerSurfaceKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CBadArg _stream;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglCreateStreamProducerSurfaceKHR();
  CeglCreateStreamProducerSurfaceKHR(EGLDisplay dpy,
                                     EGLConfig config,
                                     EGLStreamKHR stream,
                                     const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_STREAM_PRODUCER_SURFACE_KHR;
  }
  virtual const char* Name() const {
    return "eglCreateStreamProducerSurfaceKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreateSyncKHR : public CFunction {
  CEGLSyncKHR _return_value;
  CEGLDisplay _dpy;
  CEGLenum _type;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreateSyncKHR();
  CeglCreateSyncKHR(EGLSyncKHR return_value,
                    EGLDisplay dpy,
                    EGLenum type,
                    const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_SYNC_KHR;
  }
  virtual const char* Name() const {
    return "eglCreateSyncKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglDestroyImageKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLImageKHR _image;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglDestroyImageKHR();
  CeglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image);
  virtual unsigned Id() const {
    return ID_EGL_DESTROY_IMAGE_KHR;
  }
  virtual const char* Name() const {
    return "eglDestroyImageKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglDestroyStreamKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglDestroyStreamKHR();
  CeglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream);
  virtual unsigned Id() const {
    return ID_EGL_DESTROY_STREAM_KHR;
  }
  virtual const char* Name() const {
    return "eglDestroyStreamKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglDestroySyncKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLSyncKHR _sync;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglDestroySyncKHR();
  CeglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync);
  virtual unsigned Id() const {
    return ID_EGL_DESTROY_SYNC_KHR;
  }
  virtual const char* Name() const {
    return "eglDestroySyncKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglDestroySyncNV : public CFunction {
  CEGLSyncKHR _sync;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CeglDestroySyncNV();
  CeglDestroySyncNV(EGLSyncNV sync);
  virtual unsigned Id() const {
    return ID_EGL_DESTROY_SYNC_NV;
  }
  virtual const char* Name() const {
    return "eglDestroySyncNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglExportDRMImageMESA : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _image;
  CBadArg _name;
  CBadArg _handle;
  CBadArg _stride;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }

public:
  CeglExportDRMImageMESA();
  CeglExportDRMImageMESA(
      EGLDisplay dpy, EGLImageKHR image, EGLint* name, EGLint* handle, EGLint* stride);
  virtual unsigned Id() const {
    return ID_EGL_EXPORT_DRMIMAGE_MESA;
  }
  virtual const char* Name() const {
    return "eglExportDRMImageMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglFenceNV : public CFunction {
  CEGLSyncKHR _sync;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CeglFenceNV();
  CeglFenceNV(EGLSyncNV sync);
  virtual unsigned Id() const {
    return ID_EGL_FENCE_NV;
  }
  virtual const char* Name() const {
    return "eglFenceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglGetStreamFileDescriptorKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglGetStreamFileDescriptorKHR();
  CeglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream);
  virtual unsigned Id() const {
    return ID_EGL_GET_STREAM_FILE_DESCRIPTOR_KHR;
  }
  virtual const char* Name() const {
    return "eglGetStreamFileDescriptorKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglGetSyncAttribKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLSyncKHR _sync;
  CEGLint _attribute;
  CEGLint _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglGetSyncAttribKHR();
  CeglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint* value);
  virtual unsigned Id() const {
    return ID_EGL_GET_SYNC_ATTRIB_KHR;
  }
  virtual const char* Name() const {
    return "eglGetSyncAttribKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglGetSyncAttribNV : public CFunction {
  CEGLSyncKHR _sync;
  CEGLint _attribute;
  CBadArg _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglGetSyncAttribNV();
  CeglGetSyncAttribNV(EGLSyncNV sync, EGLint attribute, EGLint* value);
  virtual unsigned Id() const {
    return ID_EGL_GET_SYNC_ATTRIB_NV;
  }
  virtual const char* Name() const {
    return "eglGetSyncAttribNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglGetSystemTimeFrequencyNV : public CFunction {

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CeglGetSystemTimeFrequencyNV();
  virtual unsigned Id() const {
    return ID_EGL_GET_SYSTEM_TIME_FREQUENCY_NV;
  }
  virtual const char* Name() const {
    return "eglGetSystemTimeFrequencyNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglGetSystemTimeNV : public CFunction {

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CeglGetSystemTimeNV();
  virtual unsigned Id() const {
    return ID_EGL_GET_SYSTEM_TIME_NV;
  }
  virtual const char* Name() const {
    return "eglGetSystemTimeNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglLockSurfaceKHR : public CFunction {
  CEGLDisplay _display;
  CEGLSurface _surface;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglLockSurfaceKHR();
  CeglLockSurfaceKHR(EGLDisplay display, EGLSurface surface, const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_LOCK_SURFACE_KHR;
  }
  virtual const char* Name() const {
    return "eglLockSurfaceKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglPostSubBufferNV : public CFunction {
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLint _x;
  CEGLint _y;
  CEGLint _width;
  CEGLint _height;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }

public:
  CeglPostSubBufferNV();
  CeglPostSubBufferNV(
      EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height);
  virtual unsigned Id() const {
    return ID_EGL_POST_SUB_BUFFER_NV;
  }
  virtual const char* Name() const {
    return "eglPostSubBufferNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglQueryStreamKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;
  CEGLenum _attribute;
  CBadArg _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglQueryStreamKHR();
  CeglQueryStreamKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint* value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_STREAM_KHR;
  }
  virtual const char* Name() const {
    return "eglQueryStreamKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglQueryStreamTimeKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;
  CEGLenum _attribute;
  CBadArg _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglQueryStreamTimeKHR();
  CeglQueryStreamTimeKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR* value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_STREAM_TIME_KHR;
  }
  virtual const char* Name() const {
    return "eglQueryStreamTimeKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglQueryStreamu64KHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;
  CEGLenum _attribute;
  CBadArg _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglQueryStreamu64KHR();
  CeglQueryStreamu64KHR(EGLDisplay dpy,
                        EGLStreamKHR stream,
                        EGLenum attribute,
                        EGLuint64KHR* value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_STREAMU64KHR;
  }
  virtual const char* Name() const {
    return "eglQueryStreamu64KHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglQuerySurfacePointerANGLE : public CFunction {
  CEGLDisplay _dpy;
  CEGLSurface _surface;
  CEGLint _attribute;
  CBadArg _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglQuerySurfacePointerANGLE();
  CeglQuerySurfacePointerANGLE(EGLDisplay dpy, EGLSurface surface, EGLint attribute, void** value);
  virtual unsigned Id() const {
    return ID_EGL_QUERY_SURFACE_POINTER_ANGLE;
  }
  virtual const char* Name() const {
    return "eglQuerySurfacePointerANGLE";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglSignalSyncKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLSyncKHR _sync;
  CEGLenum _mode;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglSignalSyncKHR();
  CeglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode);
  virtual unsigned Id() const {
    return ID_EGL_SIGNAL_SYNC_KHR;
  }
  virtual const char* Name() const {
    return "eglSignalSyncKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglSignalSyncNV : public CFunction {
  CEGLSyncKHR _sync;
  CEGLenum _mode;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglSignalSyncNV();
  CeglSignalSyncNV(EGLSyncNV sync, EGLenum mode);
  virtual unsigned Id() const {
    return ID_EGL_SIGNAL_SYNC_NV;
  }
  virtual const char* Name() const {
    return "eglSignalSyncNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglStreamAttribKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;
  CEGLenum _attribute;
  CEGLint _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CeglStreamAttribKHR();
  CeglStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
  virtual unsigned Id() const {
    return ID_EGL_STREAM_ATTRIB_KHR;
  }
  virtual const char* Name() const {
    return "eglStreamAttribKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglStreamConsumerAcquireKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglStreamConsumerAcquireKHR();
  CeglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream);
  virtual unsigned Id() const {
    return ID_EGL_STREAM_CONSUMER_ACQUIRE_KHR;
  }
  virtual const char* Name() const {
    return "eglStreamConsumerAcquireKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglStreamConsumerGLTextureExternalKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglStreamConsumerGLTextureExternalKHR();
  CeglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy, EGLStreamKHR stream);
  virtual unsigned Id() const {
    return ID_EGL_STREAM_CONSUMER_GLTEXTURE_EXTERNAL_KHR;
  }
  virtual const char* Name() const {
    return "eglStreamConsumerGLTextureExternalKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglStreamConsumerReleaseKHR : public CFunction {
  CEGLDisplay _dpy;
  CBadArg _stream;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglStreamConsumerReleaseKHR();
  CeglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream);
  virtual unsigned Id() const {
    return ID_EGL_STREAM_CONSUMER_RELEASE_KHR;
  }
  virtual const char* Name() const {
    return "eglStreamConsumerReleaseKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglUnlockSurfaceKHR : public CFunction {
  CEGLDisplay _display;
  CEGLSurface _surface;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CeglUnlockSurfaceKHR();
  CeglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface);
  virtual unsigned Id() const {
    return ID_EGL_UNLOCK_SURFACE_KHR;
  }
  virtual const char* Name() const {
    return "eglUnlockSurfaceKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglWaitSyncKHR : public CFunction {
  CEGLDisplay _dpy;
  CEGLSyncKHR _sync;
  CEGLint _flags;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CeglWaitSyncKHR();
  CeglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags);
  virtual unsigned Id() const {
    return ID_EGL_WAIT_SYNC_KHR;
  }
  virtual const char* Name() const {
    return "eglWaitSyncKHR";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglSetBlobCacheFuncsANDROID : public CFunction {
  CEGLDisplay _dpy;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CeglSetBlobCacheFuncsANDROID();
  CeglSetBlobCacheFuncsANDROID(EGLDisplay dpy,
                               EGLSetBlobFuncANDROID set,
                               EGLGetBlobFuncANDROID get);
  virtual unsigned Id() const {
    return ID_EGL_SET_BLOB_CACHE_FUNCS_ANDROID;
  }
  virtual const char* Name() const {
    return "eglSetBlobCacheFuncsANDROID";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglGetPlatformDisplayEXT : public CFunction {
  CEGLDisplay _return_value;
  CEGLenum _platform;
  CDisplayPtr _native_display;
  CEGLint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglGetPlatformDisplayEXT();
  CeglGetPlatformDisplayEXT(EGLDisplay return_value,
                            EGLenum platform,
                            Display* native_display,
                            const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_GET_PLATFORM_DISPLAY_EXT;
  }
  virtual const char* Name() const {
    return "eglGetPlatformDisplayEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

class CeglCreatePlatformWindowSurfaceEXT : public CFunction {
  CEGLSurface _return_value;
  CEGLDisplay _dpy;
  CEGLConfig _config;
  CEGLNativeWindowType _native_window;
  CEGLint::CSArray _attrib_list;
  CEGLint::CSArray _config_attribs;
  CEGLint::CSArray _surface_attribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CeglCreatePlatformWindowSurfaceEXT();
  CeglCreatePlatformWindowSurfaceEXT(EGLSurface return_value,
                                     EGLDisplay dpy,
                                     EGLConfig config,
                                     EGLNativeWindowType native_window,
                                     const EGLint* attrib_list);
  virtual unsigned Id() const {
    return ID_EGL_CREATE_PLATFORM_WINDOW_SURFACE_EXT;
  }
  virtual const char* Name() const {
    return "eglCreatePlatformWindowSurfaceEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};
} // namespace OpenGL
} // namespace gits
