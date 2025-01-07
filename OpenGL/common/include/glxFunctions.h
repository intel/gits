// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   glxFunctions.h
*
*/

#pragma once

#include "openglFunction.h"
#include "mapping.h"
#include "nonglArguments.h"
#include "glxArguments.h"

namespace gits {

namespace OpenGL {

/**
     * @brief OpenGL glXChooseVisual() function call wrapper.
     *
     * OpenGL glXChooseVisual() function call wrapper.
     * Returns: XVisualInfo*
     */
class CglXChooseVisual : public CFunction {
  CXVisualInfoPtr _return_value;
  CDisplayPtr _dpy;
  Cint _screen;
  Cint::CSArray _attribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXChooseVisual();
  CglXChooseVisual(XVisualInfo* return_value, Display* dpy, int screen, int* attribList);
  virtual unsigned Id() const {
    return ID_GLX_CHOOSE_VISUAL;
  }
  virtual const char* Name() const {
    return "glXChooseVisual";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreateContext() function call wrapper.
     *
     * OpenGL glXCreateContext() function call wrapper.
     * Returns: GLXContext
     */
class CglXCreateContext : public CFunction {
  CGLXContext _return_value;
  CDisplayPtr _dpy;
  CXVisualInfoPtr _vis;
  CGLXContext _shareList;
  CBool _direct;
  Cint::CSArray _attribs;
  Cint::CSArray _fbAttribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXCreateContext();
  CglXCreateContext(
      GLXContext return_value, Display* dpy, XVisualInfo* vis, GLXContext shareList, Bool direct);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_CONTEXT;
  }
  virtual const char* Name() const {
    return "glXCreateContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXDestroyContext() function call wrapper.
     *
     * OpenGL glXDestroyContext() function call wrapper.
     * Returns: void
     */
class CglXDestroyContext : public CFunction {
  CDisplayPtr _dpy;
  CGLXContext _ctx;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CglXDestroyContext();
  CglXDestroyContext(Display* dpy, GLXContext ctx);
  virtual unsigned Id() const {
    return ID_GLX_DESTROY_CONTEXT;
  }
  virtual const char* Name() const {
    return "glXDestroyContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXMakeCurrent() function call wrapper.
     *
     * OpenGL glXMakeCurrent() function call wrapper.
     * Returns: Bool
     */
class CglXMakeCurrent : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  CGLXContext _ctx;
  Cint::CSArray _winparams;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXMakeCurrent();
  CglXMakeCurrent(Bool return_value, Display* dpy, GLXDrawable drawable, GLXContext ctx);
  virtual unsigned Id() const {
    return ID_GLX_MAKE_CURRENT;
  }
  virtual const char* Name() const {
    return "glXMakeCurrent";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCopyContext() function call wrapper.
     *
     * OpenGL glXCopyContext() function call wrapper.
     * Returns: void
     */
class CglXCopyContext : public CFunction {
  CDisplayPtr _dpy;
  CGLXContext _src;
  CGLXContext _dst;
  Culong _mask;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CglXCopyContext();
  CglXCopyContext(Display* dpy, GLXContext src, GLXContext dst, unsigned long mask);
  virtual unsigned Id() const {
    return ID_GLX_COPY_CONTEXT;
  }
  virtual const char* Name() const {
    return "glXCopyContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXSwapBuffers() function call wrapper.
     *
     * OpenGL glXSwapBuffers() function call wrapper.
     * Returns: void
     */
class CglXSwapBuffers : public CFunction {
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CglXSwapBuffers();
  CglXSwapBuffers(Display* dpy, GLXDrawable drawable);
  virtual unsigned Id() const {
    return ID_GLX_SWAP_BUFFERS;
  }
  virtual const char* Name() const {
    return "glXSwapBuffers";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreateGLXPixmap() function call wrapper.
     *
     * OpenGL glXCreateGLXPixmap() function call wrapper.
     * Returns: GLXPixmap
     */
class CglXCreateGLXPixmap : public CFunction {
  CGLXPixmap _return_value;
  CDisplayPtr _dpy;
  CXVisualInfoPtr _visual;
  CPixmap _pixmap;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXCreateGLXPixmap();
  CglXCreateGLXPixmap(GLXPixmap return_value, Display* dpy, XVisualInfo* visual, Pixmap pixmap);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_GLXPIXMAP;
  }
  virtual const char* Name() const {
    return "glXCreateGLXPixmap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXDestroyGLXPixmap() function call wrapper.
     *
     * OpenGL glXDestroyGLXPixmap() function call wrapper.
     * Returns: void
     */
class CglXDestroyGLXPixmap : public CFunction {
  CDisplayPtr _dpy;
  CGLXPixmap _pixmap;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CglXDestroyGLXPixmap();
  CglXDestroyGLXPixmap(Display* dpy, GLXPixmap pixmap);
  virtual unsigned Id() const {
    return ID_GLX_DESTROY_GLXPIXMAP;
  }
  virtual const char* Name() const {
    return "glXDestroyGLXPixmap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryExtension() function call wrapper.
     *
     * OpenGL glXQueryExtension() function call wrapper.
     * Returns: Bool
     */
class CglXQueryExtension : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  Cint::CSArray _errorb;
  Cint::CSArray _event;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXQueryExtension();
  CglXQueryExtension(Bool return_value, Display* dpy, int* errorb, int* event);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_EXTENSION;
  }
  virtual const char* Name() const {
    return "glXQueryExtension";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryVersion() function call wrapper.
     *
     * OpenGL glXQueryVersion() function call wrapper.
     * Returns: Bool
     */
class CglXQueryVersion : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  Cint::CSArray _maj;
  Cint::CSArray _min;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXQueryVersion();
  CglXQueryVersion(Bool return_value, Display* dpy, int* maj, int* min);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_VERSION;
  }
  virtual const char* Name() const {
    return "glXQueryVersion";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXIsDirect() function call wrapper.
     *
     * OpenGL glXIsDirect() function call wrapper.
     * Returns: Bool
     */
class CglXIsDirect : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  CGLXContext _ctx;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXIsDirect();
  CglXIsDirect(Bool return_value, Display* dpy, GLXContext ctx);
  virtual unsigned Id() const {
    return ID_GLX_IS_DIRECT;
  }
  virtual const char* Name() const {
    return "glXIsDirect";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetConfig() function call wrapper.
     *
     * OpenGL glXGetConfig() function call wrapper.
     * Returns: int
     */
class CglXGetConfig : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CXVisualInfoPtr _visual;
  Cint _attrib;
  Cint::CSArray _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetConfig();
  CglXGetConfig(int return_value, Display* dpy, XVisualInfo* visual, int attrib, int* value);
  virtual unsigned Id() const {
    return ID_GLX_GET_CONFIG;
  }
  virtual const char* Name() const {
    return "glXGetConfig";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetCurrentContext() function call wrapper.
     *
     * OpenGL glXGetCurrentContext() function call wrapper.
     * Returns: GLXContext
     */
class CglXGetCurrentContext : public CFunction {
  CGLXContext _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetCurrentContext();
  CglXGetCurrentContext(GLXContext return_value);
  virtual unsigned Id() const {
    return ID_GLX_GET_CURRENT_CONTEXT;
  }
  virtual const char* Name() const {
    return "glXGetCurrentContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetCurrentDrawable() function call wrapper.
     *
     * OpenGL glXGetCurrentDrawable() function call wrapper.
     * Returns: GLXDrawable
     */
class CglXGetCurrentDrawable : public CFunction {
  CGLXDrawable _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetCurrentDrawable();
  CglXGetCurrentDrawable(GLXDrawable return_value);
  virtual unsigned Id() const {
    return ID_GLX_GET_CURRENT_DRAWABLE;
  }
  virtual const char* Name() const {
    return "glXGetCurrentDrawable";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXWaitGL() function call wrapper.
     *
     * OpenGL glXWaitGL() function call wrapper.
     * Returns: void
     */
class CglXWaitGL : public CFunction {

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CglXWaitGL();
  virtual unsigned Id() const {
    return ID_GLX_WAIT_GL;
  }
  virtual const char* Name() const {
    return "glXWaitGL";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXWaitX() function call wrapper.
     *
     * OpenGL glXWaitX() function call wrapper.
     * Returns: void
     */
class CglXWaitX : public CFunction {

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CglXWaitX();
  virtual unsigned Id() const {
    return ID_GLX_WAIT_X;
  }
  virtual const char* Name() const {
    return "glXWaitX";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXUseXFont() function call wrapper.
     *
     * OpenGL glXUseXFont() function call wrapper.
     * Returns: void
     */
class CglXUseXFont : public CFunction {
  CFont _font;
  Cint _first;
  Cint _count;
  Cint _list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CglXUseXFont();
  CglXUseXFont(Font font, int first, int count, int list);
  virtual unsigned Id() const {
    return ID_GLX_USE_XFONT;
  }
  virtual const char* Name() const {
    return "glXUseXFont";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryExtensionsString() function call wrapper.
     *
     * OpenGL glXQueryExtensionsString() function call wrapper.
     * Returns: const char*
     */
class CglXQueryExtensionsString : public CFunction {
  Cchar::CSArray _return_value;
  CDisplayPtr _dpy;
  Cint _screen;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXQueryExtensionsString();
  CglXQueryExtensionsString(const char* return_value, Display* dpy, int screen);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_EXTENSIONS_STRING;
  }
  virtual const char* Name() const {
    return "glXQueryExtensionsString";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryServerString() function call wrapper.
     *
     * OpenGL glXQueryServerString() function call wrapper.
     * Returns: const char*
     */
class CglXQueryServerString : public CFunction {
  Cchar::CSArray _return_value;
  CDisplayPtr _dpy;
  Cint _screen;
  Cint _name;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXQueryServerString();
  CglXQueryServerString(const char* return_value, Display* dpy, int screen, int name);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_SERVER_STRING;
  }
  virtual const char* Name() const {
    return "glXQueryServerString";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetClientString() function call wrapper.
     *
     * OpenGL glXGetClientString() function call wrapper.
     * Returns: const char*
     */
class CglXGetClientString : public CFunction {
  Cchar::CSArray _return_value;
  CDisplayPtr _dpy;
  Cint _name;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetClientString();
  CglXGetClientString(const char* return_value, Display* dpy, int name);
  virtual unsigned Id() const {
    return ID_GLX_GET_CLIENT_STRING;
  }
  virtual const char* Name() const {
    return "glXGetClientString";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetCurrentDisplay() function call wrapper.
     *
     * OpenGL glXGetCurrentDisplay() function call wrapper.
     * Returns: Display*
     */
class CglXGetCurrentDisplay : public CFunction {
  CDisplayPtr _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetCurrentDisplay();
  CglXGetCurrentDisplay(Display* return_value);
  virtual unsigned Id() const {
    return ID_GLX_GET_CURRENT_DISPLAY;
  }
  virtual const char* Name() const {
    return "glXGetCurrentDisplay";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXChooseFBConfig() function call wrapper.
     *
     * OpenGL glXChooseFBConfig() function call wrapper.
     * Returns: GLXFBConfig*
     */
class CglXChooseFBConfig : public CFunction {
  CGLXFBConfig::CSArray _return_value;
  CDisplayPtr _dpy;
  Cint _screen;
  Cint::CSArray _attribList;
  Cint::CSArray _nitems;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXChooseFBConfig();
  CglXChooseFBConfig(
      GLXFBConfig* return_value, Display* dpy, int screen, const int* attribList, int* nitems);
  virtual unsigned Id() const {
    return ID_GLX_CHOOSE_FB_CONFIG;
  }
  virtual const char* Name() const {
    return "glXChooseFBConfig";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetFBConfigAttrib() function call wrapper.
     *
     * OpenGL glXGetFBConfigAttrib() function call wrapper.
     * Returns: int
     */
class CglXGetFBConfigAttrib : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;
  Cint _attribute;
  Cint::CSArray _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetFBConfigAttrib();
  CglXGetFBConfigAttrib(
      int return_value, Display* dpy, GLXFBConfig config, int attribute, int* value);
  virtual unsigned Id() const {
    return ID_GLX_GET_FBCONFIG_ATTRIB;
  }
  virtual const char* Name() const {
    return "glXGetFBConfigAttrib";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetFBConfigs() function call wrapper.
     *
     * OpenGL glXGetFBConfigs() function call wrapper.
     * Returns: GLXFBConfig*
     */
class CglXGetFBConfigs : public CFunction {
  CGLXFBConfig::CSArray _return_value;
  CDisplayPtr _dpy;
  Cint _screen;
  Cint::CSArray _nelements;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetFBConfigs();
  CglXGetFBConfigs(GLXFBConfig* return_value, Display* dpy, int screen, int* nelements);
  virtual unsigned Id() const {
    return ID_GLX_GET_FBCONFIGS;
  }
  virtual const char* Name() const {
    return "glXGetFBConfigs";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetVisualFromFBConfig() function call wrapper.
     *
     * OpenGL glXGetVisualFromFBConfig() function call wrapper.
     * Returns: XVisualInfo*
     */
class CglXGetVisualFromFBConfig : public CFunction {
  CXVisualInfoPtr _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetVisualFromFBConfig();
  CglXGetVisualFromFBConfig(XVisualInfo* return_value, Display* dpy, GLXFBConfig config);
  virtual unsigned Id() const {
    return ID_GLX_GET_VISUAL_FROM_FB_CONFIG;
  }
  virtual const char* Name() const {
    return "glXGetVisualFromFBConfig";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreateWindow() function call wrapper.
     *
     * OpenGL glXCreateWindow() function call wrapper.
     * Returns: GLXWindow
     */
class CglXCreateWindow : public CFunction {
  CGLXWindow _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;
  CWindow _win;
  Cint::CSArray _attribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXCreateWindow();
  CglXCreateWindow(
      GLXWindow return_value, Display* dpy, GLXFBConfig config, Window win, const int* attribList);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_WINDOW;
  }
  virtual const char* Name() const {
    return "glXCreateWindow";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXDestroyWindow() function call wrapper.
     *
     * OpenGL glXDestroyWindow() function call wrapper.
     * Returns: void
     */
class CglXDestroyWindow : public CFunction {
  CDisplayPtr _dpy;
  CGLXWindow _window;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CglXDestroyWindow();
  CglXDestroyWindow(Display* dpy, GLXWindow window);
  virtual unsigned Id() const {
    return ID_GLX_DESTROY_WINDOW;
  }
  virtual const char* Name() const {
    return "glXDestroyWindow";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreatePixmap() function call wrapper.
     *
     * OpenGL glXCreatePixmap() function call wrapper.
     * Returns: GLXPixmap
     */
class CglXCreatePixmap : public CFunction {
  CGLXPixmap _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;
  CPixmap _pixmap;
  Cint::CSArray _attribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXCreatePixmap();
  CglXCreatePixmap(GLXPixmap return_value,
                   Display* dpy,
                   GLXFBConfig config,
                   Pixmap pixmap,
                   const int* attribList);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_PIXMAP;
  }
  virtual const char* Name() const {
    return "glXCreatePixmap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXDestroyPixmap() function call wrapper.
     *
     * OpenGL glXDestroyPixmap() function call wrapper.
     * Returns: void
     */
class CglXDestroyPixmap : public CFunction {
  CDisplayPtr _dpy;
  CGLXPixmap _pixmap;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CglXDestroyPixmap();
  CglXDestroyPixmap(Display* dpy, GLXPixmap pixmap);
  virtual unsigned Id() const {
    return ID_GLX_DESTROY_PIXMAP;
  }
  virtual const char* Name() const {
    return "glXDestroyPixmap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreatePbuffer() function call wrapper.
     *
     * OpenGL glXCreatePbuffer() function call wrapper.
     * Returns: GLXPbuffer
     */
class CglXCreatePbuffer : public CFunction {
  CGLXPbuffer _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;
  Cint::CSArray _attribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXCreatePbuffer();
  CglXCreatePbuffer(GLXPbuffer return_value,
                    Display* dpy,
                    GLXFBConfig config,
                    const int* attribList);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_PBUFFER;
  }
  virtual const char* Name() const {
    return "glXCreatePbuffer";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXDestroyPbuffer() function call wrapper.
     *
     * OpenGL glXDestroyPbuffer() function call wrapper.
     * Returns: void
     */
class CglXDestroyPbuffer : public CFunction {
  CDisplayPtr _dpy;
  CGLXPbuffer _pbuf;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CglXDestroyPbuffer();
  CglXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf);
  virtual unsigned Id() const {
    return ID_GLX_DESTROY_PBUFFER;
  }
  virtual const char* Name() const {
    return "glXDestroyPbuffer";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryDrawable() function call wrapper.
     *
     * OpenGL glXQueryDrawable() function call wrapper.
     * Returns: void
     */
class CglXQueryDrawable : public CFunction {
  CDisplayPtr _dpy;
  CGLXDrawable _draw;
  Cint _attribute;
  Cuint::CSArray _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CglXQueryDrawable();
  CglXQueryDrawable(Display* dpy, GLXDrawable draw, int attribute, unsigned int* value);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_DRAWABLE;
  }
  virtual const char* Name() const {
    return "glXQueryDrawable";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreateNewContext() function call wrapper.
     *
     * OpenGL glXCreateNewContext() function call wrapper.
     * Returns: GLXContext
     */
class CglXCreateNewContext : public CFunction {
  CGLXContext _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;
  Cint _renderType;
  CGLXContext _shareList;
  CBool _direct;
  Cint::CSArray _fbconfig_attribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

  void AddFbConfigAttrib(Display* dpy, GLXFBConfig config, int attribute);
  void FinishFbConfigAttribList();

public:
  CglXCreateNewContext();
  CglXCreateNewContext(GLXContext return_value,
                       Display* dpy,
                       GLXFBConfig config,
                       int renderType,
                       GLXContext shareList,
                       Bool direct);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_NEW_CONTEXT;
  }
  virtual const char* Name() const {
    return "glXCreateNewContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXMakeContextCurrent() function call wrapper.
     *
     * OpenGL glXMakeContextCurrent() function call wrapper.
     * Returns: Bool
     */
class CglXMakeContextCurrent : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _draw;
  CGLXDrawable _read;
  CGLXContext _ctx;
  Cint::CSArray _winparams;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXMakeContextCurrent();
  CglXMakeContextCurrent(
      Bool return_value, Display* dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
  virtual unsigned Id() const {
    return ID_GLX_MAKE_CONTEXT_CURRENT;
  }
  virtual const char* Name() const {
    return "glXMakeContextCurrent";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetCurrentReadDrawable() function call wrapper.
     *
     * OpenGL glXGetCurrentReadDrawable() function call wrapper.
     * Returns: GLXDrawable
     */
class CglXGetCurrentReadDrawable : public CFunction {
  CGLXDrawable _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetCurrentReadDrawable();
  CglXGetCurrentReadDrawable(GLXDrawable return_value);
  virtual unsigned Id() const {
    return ID_GLX_GET_CURRENT_READ_DRAWABLE;
  }
  virtual const char* Name() const {
    return "glXGetCurrentReadDrawable";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryContext() function call wrapper.
     *
     * OpenGL glXQueryContext() function call wrapper.
     * Returns: int
     */
class CglXQueryContext : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CGLXContext _ctx;
  Cint _attribute;
  Cint::CSArray _value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXQueryContext();
  CglXQueryContext(int return_value, Display* dpy, GLXContext ctx, int attribute, int* value);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_CONTEXT;
  }
  virtual const char* Name() const {
    return "glXQueryContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXSelectEvent() function call wrapper.
     *
     * OpenGL glXSelectEvent() function call wrapper.
     * Returns: void
     */
class CglXSelectEvent : public CFunction {
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  Culong _mask;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CglXSelectEvent();
  CglXSelectEvent(Display* dpy, GLXDrawable drawable, unsigned long mask);
  virtual unsigned Id() const {
    return ID_GLX_SELECT_EVENT;
  }
  virtual const char* Name() const {
    return "glXSelectEvent";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetSelectedEvent() function call wrapper.
     *
     * OpenGL glXGetSelectedEvent() function call wrapper.
     * Returns: void
     */
class CglXGetSelectedEvent : public CFunction {
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  Culong::CSArray _mask;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CglXGetSelectedEvent();
  CglXGetSelectedEvent(Display* dpy, GLXDrawable drawable, unsigned long* mask);
  virtual unsigned Id() const {
    return ID_GLX_GET_SELECTED_EVENT;
  }
  virtual const char* Name() const {
    return "glXGetSelectedEvent";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetProcAddressARB() function call wrapper.
     *
     * OpenGL glXGetProcAddressARB() function call wrapper.
     * Returns: void*
     */
class CglXGetProcAddressARB : public CFunction {
  CGLvoidPtr _return_value;
  CGLubyte::CSArray _func_name;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetProcAddressARB();
  CglXGetProcAddressARB(void* return_value, const GLubyte* func_name);
  virtual unsigned Id() const {
    return ID_GLX_GET_PROC_ADDRESS_ARB;
  }
  virtual const char* Name() const {
    return "glXGetProcAddressARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetProcAddress() function call wrapper.
     *
     * OpenGL glXGetProcAddress() function call wrapper.
     * Returns: void*
     */
class CglXGetProcAddress : public CFunction {
  CGLvoidPtr _return_value;
  CGLubyte::CSArray _func_name;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetProcAddress();
  CglXGetProcAddress(void* return_value, const GLubyte* func_name);
  virtual unsigned Id() const {
    return ID_GLX_GET_PROC_ADDRESS;
  }
  virtual const char* Name() const {
    return "glXGetProcAddress";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXAllocateMemoryNV() function call wrapper.
     *
     * OpenGL glXAllocateMemoryNV() function call wrapper.
     * Returns: void*
     */
class CglXAllocateMemoryNV : public CFunction {
  CGLvoidPtr _return_value;
  CGLsizei _size;
  CGLfloat _readfreq;
  CGLfloat _writefreq;
  CGLfloat _priority;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXAllocateMemoryNV();
  CglXAllocateMemoryNV(
      void* return_value, GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
  virtual unsigned Id() const {
    return ID_GLX_ALLOCATE_MEMORY_NV;
  }
  virtual const char* Name() const {
    return "glXAllocateMemoryNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXFreeMemoryNV() function call wrapper.
     *
     * OpenGL glXFreeMemoryNV() function call wrapper.
     * Returns: void
     */
class CglXFreeMemoryNV : public CFunction {
  CGLvoidPtr _pointer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CglXFreeMemoryNV();
  CglXFreeMemoryNV(GLvoid* pointer);
  virtual unsigned Id() const {
    return ID_GLX_FREE_MEMORY_NV;
  }
  virtual const char* Name() const {
    return "glXFreeMemoryNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXBindTexImageARB() function call wrapper.
     *
     * OpenGL glXBindTexImageARB() function call wrapper.
     * Returns: Bool
     */
class CglXBindTexImageARB : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  CGLXPbuffer _pbuffer;
  Cint _buffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXBindTexImageARB();
  CglXBindTexImageARB(Bool return_value, Display* dpy, GLXPbuffer pbuffer, int buffer);
  virtual unsigned Id() const {
    return ID_GLX_BIND_TEX_IMAGE_ARB;
  }
  virtual const char* Name() const {
    return "glXBindTexImageARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXReleaseTexImageARB() function call wrapper.
     *
     * OpenGL glXReleaseTexImageARB() function call wrapper.
     * Returns: Bool
     */
class CglXReleaseTexImageARB : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  CGLXPbuffer _pbuffer;
  Cint _buffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXReleaseTexImageARB();
  CglXReleaseTexImageARB(Bool return_value, Display* dpy, GLXPbuffer pbuffer, int buffer);
  virtual unsigned Id() const {
    return ID_GLX_RELEASE_TEX_IMAGE_ARB;
  }
  virtual const char* Name() const {
    return "glXReleaseTexImageARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXDrawableAttribARB() function call wrapper.
     *
     * OpenGL glXDrawableAttribARB() function call wrapper.
     * Returns: Bool
     */
class CglXDrawableAttribARB : public CFunction {
  CBool _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _draw;
  Cint::CSArray _attribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXDrawableAttribARB();
  CglXDrawableAttribARB(Bool return_value, Display* dpy, GLXDrawable draw, const int* attribList);
  virtual unsigned Id() const {
    return ID_GLX_DRAWABLE_ATTRIB_ARB;
  }
  virtual const char* Name() const {
    return "glXDrawableAttribARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetFrameUsageMESA() function call wrapper.
     *
     * OpenGL glXGetFrameUsageMESA() function call wrapper.
     * Returns: int
     */
class CglXGetFrameUsageMESA : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  Cfloat::CSArray _usage;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetFrameUsageMESA();
  CglXGetFrameUsageMESA(int return_value, Display* dpy, GLXDrawable drawable, float* usage);
  virtual unsigned Id() const {
    return ID_GLX_GET_FRAME_USAGE_MESA;
  }
  virtual const char* Name() const {
    return "glXGetFrameUsageMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXBeginFrameTrackingMESA() function call wrapper.
     *
     * OpenGL glXBeginFrameTrackingMESA() function call wrapper.
     * Returns: int
     */
class CglXBeginFrameTrackingMESA : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXBeginFrameTrackingMESA();
  CglXBeginFrameTrackingMESA(int return_value, Display* dpy, GLXDrawable drawable);
  virtual unsigned Id() const {
    return ID_GLX_BEGIN_FRAME_TRACKING_MESA;
  }
  virtual const char* Name() const {
    return "glXBeginFrameTrackingMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXEndFrameTrackingMESA() function call wrapper.
     *
     * OpenGL glXEndFrameTrackingMESA() function call wrapper.
     * Returns: int
     */
class CglXEndFrameTrackingMESA : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXEndFrameTrackingMESA();
  CglXEndFrameTrackingMESA(int return_value, Display* dpy, GLXDrawable drawable);
  virtual unsigned Id() const {
    return ID_GLX_END_FRAME_TRACKING_MESA;
  }
  virtual const char* Name() const {
    return "glXEndFrameTrackingMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXQueryFrameTrackingMESA() function call wrapper.
     *
     * OpenGL glXQueryFrameTrackingMESA() function call wrapper.
     * Returns: int
     */
class CglXQueryFrameTrackingMESA : public CFunction {
  Cint _return_value;
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  Cint64_t::CSArray _swapCount;
  Cint64_t::CSArray _missedFrames;
  Cfloat::CSArray _lastMissedUsage;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXQueryFrameTrackingMESA();
  CglXQueryFrameTrackingMESA(int return_value,
                             Display* dpy,
                             GLXDrawable drawable,
                             int64_t* swapCount,
                             int64_t* missedFrames,
                             float* lastMissedUsage);
  virtual unsigned Id() const {
    return ID_GLX_QUERY_FRAME_TRACKING_MESA;
  }
  virtual const char* Name() const {
    return "glXQueryFrameTrackingMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXSwapIntervalMESA() function call wrapper.
     *
     * OpenGL glXSwapIntervalMESA() function call wrapper.
     * Returns: int
     */
class CglXSwapIntervalMESA : public CFunction {
  Cint _return_value;
  Cuint _interval;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXSwapIntervalMESA();
  CglXSwapIntervalMESA(int return_value, unsigned int interval);
  virtual unsigned Id() const {
    return ID_GLX_SWAP_INTERVAL_MESA;
  }
  virtual const char* Name() const {
    return "glXSwapIntervalMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXGetSwapIntervalMESA() function call wrapper.
     *
     * OpenGL glXGetSwapIntervalMESA() function call wrapper.
     * Returns: int
     */
class CglXGetSwapIntervalMESA : public CFunction {
  Cint _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXGetSwapIntervalMESA();
  CglXGetSwapIntervalMESA(int return_value);
  virtual unsigned Id() const {
    return ID_GLX_GET_SWAP_INTERVAL_MESA;
  }
  virtual const char* Name() const {
    return "glXGetSwapIntervalMESA";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXBindTexImageEXT() function call wrapper.
     *
     * OpenGL glXBindTexImageEXT() function call wrapper.
     * Returns: void
     */
class CglXBindTexImageEXT : public CFunction {
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  Cint _buffer;
  Cint::CSArray _attrib_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }

public:
  CglXBindTexImageEXT();
  CglXBindTexImageEXT(Display* dpy, GLXDrawable drawable, int buffer, const int* attrib_list);
  virtual unsigned Id() const {
    return ID_GLX_BIND_TEX_IMAGE_EXT;
  }
  virtual const char* Name() const {
    return "glXBindTexImageEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXReleaseTexImageEXT() function call wrapper.
     *
     * OpenGL glXReleaseTexImageEXT() function call wrapper.
     * Returns: void
     */
class CglXReleaseTexImageEXT : public CFunction {
  CDisplayPtr _dpy;
  CGLXDrawable _drawable;
  Cint _buffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CglXReleaseTexImageEXT();
  CglXReleaseTexImageEXT(Display* dpy, GLXDrawable drawable, int buffer);
  virtual unsigned Id() const {
    return ID_GLX_RELEASE_TEX_IMAGE_EXT;
  }
  virtual const char* Name() const {
    return "glXReleaseTexImageEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXCreateContextAttribsARB() function call wrapper.
     *
     * OpenGL glXCreateContextAttribsARB() function call wrapper.
     * Returns: GLXContext
     */
class CglXCreateContextAttribsARB : public CFunction {
  CGLXContext _return_value;
  CDisplayPtr _dpy;
  CGLXFBConfig _config;
  CGLXContext _share_context;
  CBool _direct;
  Cint::CSArray _attrib_list;
  Cint::CSArray _fbconfig_attribs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

  void AddFbConfigAttrib(Display* dpy, GLXFBConfig config, int attribute);
  void FinishFbConfigAttribList();

public:
  CglXCreateContextAttribsARB();
  CglXCreateContextAttribsARB(GLXContext return_value,
                              Display* dpy,
                              GLXFBConfig config,
                              GLXContext share_context,
                              Bool direct,
                              const int* attrib_list);
  virtual unsigned Id() const {
    return ID_GLX_CREATE_CONTEXT_ATTRIBS_ARB;
  }
  virtual const char* Name() const {
    return "glXCreateContextAttribsARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
     * @brief OpenGL glXSwapIntervalSGI() function call wrapper.
     *
     * OpenGL glXSwapIntervalSGI() function call wrapper.
     * Returns: int
     */
class CglXSwapIntervalSGI : public CFunction {
  Cint _return_value;
  Cint _interval;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual const CArgument* Return() const {
    return &_return_value;
  }

public:
  CglXSwapIntervalSGI();
  CglXSwapIntervalSGI(int return_value, int interval);
  virtual unsigned Id() const {
    return ID_GLX_SWAP_INTERVAL_SGI;
  }
  virtual const char* Name() const {
    return "glXSwapIntervalSGI";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

} // namespace OpenGL
} // namespace gits
