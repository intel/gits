// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   glxFunctions.cpp
 *
 */

#include "glxFunctions.h"
#include "platform.h"
#include "openglLibrary.h"
#include "openglDrivers.h"
#include "gits.h"
#include "exception.h"
#include "library.h"
#include "log.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include <map>
#include "pfattribs.h"
#include "stateDynamic.h"
#include "windowContextState.h"
#include "ptblLibrary.h"
#include "ptbl_glxLibrary.h"
#include "opengl_apis_iface.h"

#include <cerrno>

#if defined GITS_PLATFORM_X11
#include <X11/Xlib.h>
#endif
#include "openglEnums.h"

/* ***************************** ID_GLX_CHOOSE_VISUAL *************************** */

gits::OpenGL::CglXChooseVisual::CglXChooseVisual() {}

gits::OpenGL::CglXChooseVisual::CglXChooseVisual(XVisualInfo* return_value,
                                                 Display* dpy,
                                                 int screen,
                                                 int* attribList)
    : _return_value(return_value), _dpy(dpy), _screen(screen), _attribList(attribList, 0, 1) {
  PtblNtvStreamApi(PtblNativeAPI::Type::GLX);
#ifdef GITS_PLATFORM_X11
  ptblInitialize(CGlDriver::API_GL);
#endif
}

gits::CArgument& gits::OpenGL::CglXChooseVisual::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _screen, _attribList);
}

void gits::OpenGL::CglXChooseVisual::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::GLX);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  ptblInitialize(CGlDriver::API_GL);
  // drv.glx.glXChooseVisual(*_dpy, *_screen, *_attribList);
#endif
}

/* ***************************** ID_GLX_CREATE_CONTEXT *************************** */

gits::OpenGL::CglXCreateContext::CglXCreateContext() {}

gits::OpenGL::CglXCreateContext::CglXCreateContext(
    GLXContext return_value, Display* dpy, XVisualInfo* vis, GLXContext shareList, Bool direct)
    : _return_value(return_value), _dpy(dpy), _vis(vis), _shareList(shareList), _direct(direct) {
#ifdef GITS_PLATFORM_X11
  // Get visual attributes
  int value = 0;

  _fbAttribs.Vector().push_back(GLX_RENDER_TYPE);
  drv.glx.glXGetConfig(dpy, vis, GLX_RGBA, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_RGBA);
    _fbAttribs.Vector().push_back(GLX_RGBA_BIT);
  } else {
    _fbAttribs.Vector().push_back(GLX_COLOR_INDEX_BIT);
  }

  _fbAttribs.Vector().push_back(GLX_DOUBLEBUFFER);
  drv.glx.glXGetConfig(dpy, vis, GLX_DOUBLEBUFFER, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_DOUBLEBUFFER);
    _fbAttribs.Vector().push_back(GL_TRUE);
  } else {
    _fbAttribs.Vector().push_back(GL_FALSE);
  }

  _fbAttribs.Vector().push_back(GLX_STEREO);
  drv.glx.glXGetConfig(dpy, vis, GLX_STEREO, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_STEREO);
    _fbAttribs.Vector().push_back(GL_TRUE);
  } else {
    _fbAttribs.Vector().push_back(GL_FALSE);
  }

  drv.glx.glXGetConfig(dpy, vis, GLX_ALPHA_SIZE, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_ALPHA_SIZE);
    _attribs.Vector().push_back(value);
  }
  _fbAttribs.Vector().push_back(GLX_ALPHA_SIZE);
  _fbAttribs.Vector().push_back(value);

  drv.glx.glXGetConfig(dpy, vis, GLX_DEPTH_SIZE, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_DEPTH_SIZE);
    _attribs.Vector().push_back(value);
  }
  _fbAttribs.Vector().push_back(GLX_DEPTH_SIZE);
  _fbAttribs.Vector().push_back(value);

  drv.glx.glXGetConfig(dpy, vis, GLX_STENCIL_SIZE, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_STENCIL_SIZE);
    _attribs.Vector().push_back(value);
  }
  _fbAttribs.Vector().push_back(GLX_STENCIL_SIZE);
  _fbAttribs.Vector().push_back(value);

  drv.glx.glXGetConfig(dpy, vis, GLX_ACCUM_RED_SIZE, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_ACCUM_RED_SIZE);
    _attribs.Vector().push_back(value);
  }
  _fbAttribs.Vector().push_back(GLX_ACCUM_RED_SIZE);
  _fbAttribs.Vector().push_back(value);

  drv.glx.glXGetConfig(dpy, vis, GLX_BUFFER_SIZE, &value);
  if (value) {
    _attribs.Vector().push_back(GLX_BUFFER_SIZE);
    _attribs.Vector().push_back(value);
  }
  _fbAttribs.Vector().push_back(GLX_BUFFER_SIZE);
  _fbAttribs.Vector().push_back(value);

  _fbAttribs.Vector().push_back(GLX_X_RENDERABLE);
  _fbAttribs.Vector().push_back(GL_TRUE);

  _fbAttribs.Vector().push_back(GLX_DRAWABLE_TYPE);
  _fbAttribs.Vector().push_back(GL_TRUE);

  _attribs.Vector().push_back(None);
  _fbAttribs.Vector().push_back(None);

  SD().AddContext(return_value, shareList);
#endif
}

gits::CArgument& gits::OpenGL::CglXCreateContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _vis, _shareList, _direct, _attribs, _fbAttribs);
}

void gits::OpenGL::CglXCreateContext::Run() {
  ptblInitialize(CGlDriver::API_GL);
  // Init
  Display* dpy = (Display*)GetNativeDisplay();
  _dpy.AddMapping(dpy);

  // Choose visual
  XVisualInfo* vis = ptbl_glXChooseVisual(dpy, ptbl_DefaultScreen(dpy), *_attribs);
  if (vis == nullptr) {
    LOG_ERROR << "glXChooseVisual failed: undefined GLX attribute encountered in attribList";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  _vis.AddMapping(vis);

  // Create context
  GLXContext ctx = ptbl_glXCreateContext(dpy, vis, nullptr, GL_TRUE);
  if (ctx == nullptr) {
    LOG_ERROR << "glXCreateContext failed on the client side";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  _return_value.AddMapping(ctx);

  // Choose FBConfig
  int fbConfigCount;
  GLXFBConfig* fbConfigs =
      ptbl_glXChooseFBConfig(dpy, ptbl_DefaultScreen(dpy), *_fbAttribs, &fbConfigCount);
  if (fbConfigs == nullptr) {
    LOG_ERROR << "glXChooseFBConfig failed: no config matches were found";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  GlxPlayerMngr* glxPlayerMngr = new GlxPlayerMngr(dpy, ctx, fbConfigs[0]);
  GlxWinCtxMngr::Add(ctx, glxPlayerMngr);

  SD().AddContext(ctx, *_shareList);
}

/* ***************************** ID_GLX_DESTROY_CONTEXT *************************** */

gits::OpenGL::CglXDestroyContext::CglXDestroyContext() {}

gits::OpenGL::CglXDestroyContext::CglXDestroyContext(Display* dpy, GLXContext ctx)
    : _dpy(dpy), _ctx(ctx) {
#ifdef GITS_PLATFORM_X11
  GlxWinCtxMngr::Remove(ctx);
  CStateDynamicNative::Get().UnMapglXCtxToDispSurfByCtx((void*)ctx);
#endif
}

gits::CArgument& gits::OpenGL::CglXDestroyContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _ctx);
}

void gits::OpenGL::CglXDestroyContext::Run() {
  GlxWinCtxMngr::Remove(*_ctx);
  ptbl_glXDestroyContext(*_dpy, *_ctx);
  SD().RemoveContext(*_ctx);
  _ctx.RemoveMapping();
}

/* ***************************** ID_GLX_MAKE_CURRENT *************************** */

gits::OpenGL::CglXMakeCurrent::CglXMakeCurrent() {}

gits::OpenGL::CglXMakeCurrent::CglXMakeCurrent(Bool return_value,
                                               Display* dpy,
                                               GLXDrawable drawable,
                                               GLXContext ctx)
    : _return_value(return_value), _dpy(dpy), _drawable(drawable), _ctx(ctx) {
#ifdef GITS_PLATFORM_X11
  GlxWinCtxMngr* ctxManager = GlxWinCtxMngr::Get(ctx);
  if (ctxManager == nullptr) {
    GlxRecorderMngr* glxRecorderMngr = new GlxRecorderMngr(dpy, ctx);
    GlxWinCtxMngr::Add(ctx, glxRecorderMngr);

    // Redundant check, so Klocwork doesn't complain.
    ctxManager = GlxWinCtxMngr::Get(ctx);
    if (ctxManager == nullptr) {
      LOG_ERROR << "Value for context " << ctx
                << " not found in map, even though it was just added there.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }

  // Get Window data.
  GlxRecorderMngr* glxRecorderMngr = dynamic_cast<GlxRecorderMngr*>(ctxManager);
  if (glxRecorderMngr == nullptr) {
    LOG_ERROR << "GlxRecorderMngr is not properly set.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  glxRecorderMngr->UpdateWindowHandle();
  glxRecorderMngr->UpdateWindow(_winparams.Vector());
  glxRecorderMngr->GetWindowParams(_winparams.Vector());

  SD().SetCurrentContext((void*)ctx);
  CStateDynamicNative::Get().MapglXCtxToSurf((void*)ctx, drawable, drawable);
#endif
}

gits::CArgument& gits::OpenGL::CglXMakeCurrent::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _ctx, _winparams);
}

void gits::OpenGL::CglXMakeCurrent::Run() {
  if (*_ctx == 0) {
    ptbl_glXMakeCurrent(*_dpy, 0, nullptr);
    return;
  }

  GlxPlayerMngr* glxPlayerMngr = dynamic_cast<GlxPlayerMngr*>(GlxWinCtxMngr::Get(*_ctx));

  if (glxPlayerMngr == nullptr) {
    LOG_ERROR << "CglXMakeCurrent called on unmapped context";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  if ((_winparams.Vector().size() != 0) &&
      ((!_drawable.CheckMapping()) || ((uint64_t)glxPlayerMngr->GetWindowHandle() != *_drawable) ||
       (glxPlayerMngr->GetWindowHandle() == 0))) {
    glxPlayerMngr->SetupWindow(_winparams.Vector());
  }

  _drawable.AddMapping((uint64_t)glxPlayerMngr->GetWindowHandle());
  // TODO: check why without this line glXMakeCurrent fails (throws BadAlloc)
  ptbl_glXMakeCurrent(*_dpy, 0, nullptr);
  ptbl_glXMakeCurrent(*_dpy, (GLXDrawable)*_drawable, *_ctx);

  SD().SetCurrentContext((void*)*_ctx);
}

/* ***************************** ID_GLX_COPY_CONTEXT *************************** */

gits::OpenGL::CglXCopyContext::CglXCopyContext() {}

gits::OpenGL::CglXCopyContext::CglXCopyContext(Display* dpy,
                                               GLXContext src,
                                               GLXContext dst,
                                               unsigned long mask)
    : _dpy(dpy), _src(src), _dst(dst), _mask(mask) {}

gits::CArgument& gits::OpenGL::CglXCopyContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _src, _dst, _mask);
}

void gits::OpenGL::CglXCopyContext::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXCopyContext(*_dpy, *_src, *_dst, *_mask);
#endif
}

/* ***************************** ID_GLX_SWAP_BUFFERS *************************** */

gits::OpenGL::CglXSwapBuffers::CglXSwapBuffers() {}

gits::OpenGL::CglXSwapBuffers::CglXSwapBuffers(Display* dpy, GLXDrawable drawable)
    : _dpy(dpy), _drawable(drawable) {}

gits::CArgument& gits::OpenGL::CglXSwapBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable);
}

void gits::OpenGL::CglXSwapBuffers::Run() {
  TODO("Reconsider keeping exact recorder side frame numbers, do we need this???")
  PreSwap();
  ptbl_glXSwapBuffers(*_dpy, (GLXDrawable)*_drawable);
}

/* ***************************** ID_GLX_CREATE_GLXPIXMAP *************************** */

gits::OpenGL::CglXCreateGLXPixmap::CglXCreateGLXPixmap() {}

gits::OpenGL::CglXCreateGLXPixmap::CglXCreateGLXPixmap(GLXPixmap return_value,
                                                       Display* dpy,
                                                       XVisualInfo* visual,
                                                       Pixmap pixmap)
    : _return_value(return_value), _dpy(dpy), _visual(visual), _pixmap(pixmap) {}

gits::CArgument& gits::OpenGL::CglXCreateGLXPixmap::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _visual, _pixmap);
}

void gits::OpenGL::CglXCreateGLXPixmap::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXCreateGLXPixmap(*_dpy, *_visual, *_pixmap);
#endif
}

/* ***************************** ID_GLX_DESTROY_GLXPIXMAP *************************** */

gits::OpenGL::CglXDestroyGLXPixmap::CglXDestroyGLXPixmap() {}

gits::OpenGL::CglXDestroyGLXPixmap::CglXDestroyGLXPixmap(Display* dpy, GLXPixmap pixmap)
    : _dpy(dpy), _pixmap(pixmap) {}

gits::CArgument& gits::OpenGL::CglXDestroyGLXPixmap::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _pixmap);
}

void gits::OpenGL::CglXDestroyGLXPixmap::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXDestroyGLXPixmap(*_dpy, *_pixmap);
#endif
}

/* ***************************** ID_GLX_QUERY_EXTENSION *************************** */

gits::OpenGL::CglXQueryExtension::CglXQueryExtension() {}

gits::OpenGL::CglXQueryExtension::CglXQueryExtension(Bool return_value,
                                                     Display* dpy,
                                                     int* errorb,
                                                     int* event)
    : _return_value(return_value), _dpy(dpy), _errorb(1, errorb), _event(1, event) {}

gits::CArgument& gits::OpenGL::CglXQueryExtension::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _errorb, _event);
}

void gits::OpenGL::CglXQueryExtension::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXQueryExtension(*_dpy, *_errorb, *_event);
#endif
}

/* ***************************** ID_GLX_QUERY_VERSION *************************** */

gits::OpenGL::CglXQueryVersion::CglXQueryVersion() {}

gits::OpenGL::CglXQueryVersion::CglXQueryVersion(Bool return_value,
                                                 Display* dpy,
                                                 int* maj,
                                                 int* min)
    : _return_value(return_value), _dpy(dpy), _maj(1, maj), _min(1, min) {
#ifdef GITS_PLATFORM_X11
  drv.gl.Initialize(CGlDriver::API_GL);
#endif
}

gits::CArgument& gits::OpenGL::CglXQueryVersion::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _maj, _min);
}

void gits::OpenGL::CglXQueryVersion::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::GLX);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  ptblInitialize(CGlDriver::API_GL);
  // drv.glx.glXQueryVersion(*_dpy, *_maj, *_min);
#endif
}

/* ***************************** ID_GLX_IS_DIRECT *************************** */

gits::OpenGL::CglXIsDirect::CglXIsDirect() {}

gits::OpenGL::CglXIsDirect::CglXIsDirect(Bool return_value, Display* dpy, GLXContext ctx)
    : _return_value(return_value), _dpy(dpy), _ctx(ctx) {}

gits::CArgument& gits::OpenGL::CglXIsDirect::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _ctx);
}

void gits::OpenGL::CglXIsDirect::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXIsDirect(*_dpy, *_ctx);
#endif
}

/* ***************************** ID_GLX_GET_CONFIG *************************** */

gits::OpenGL::CglXGetConfig::CglXGetConfig() {}

gits::OpenGL::CglXGetConfig::CglXGetConfig(
    int return_value, Display* dpy, XVisualInfo* visual, int attrib, int* value)
    : _return_value(return_value), _dpy(dpy), _visual(visual), _attrib(attrib), _value(1, value) {}

gits::CArgument& gits::OpenGL::CglXGetConfig::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _visual, _attrib, _value);
}

void gits::OpenGL::CglXGetConfig::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetConfig(*_dpy, *_visual, *_attrib, *_value);
#endif
}

/* ***************************** ID_GLX_GET_CURRENT_CONTEXT *************************** */

gits::OpenGL::CglXGetCurrentContext::CglXGetCurrentContext() {}

gits::OpenGL::CglXGetCurrentContext::CglXGetCurrentContext(GLXContext return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CglXGetCurrentContext::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXGetCurrentContext::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetCurrentContext();
#endif
}

/* ***************************** ID_GLX_GET_CURRENT_DRAWABLE *************************** */

gits::OpenGL::CglXGetCurrentDrawable::CglXGetCurrentDrawable() {}

gits::OpenGL::CglXGetCurrentDrawable::CglXGetCurrentDrawable(GLXDrawable return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CglXGetCurrentDrawable::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXGetCurrentDrawable::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetCurrentDrawable();
#endif
}

/* ***************************** ID_GLX_WAIT_GL *************************** */

gits::OpenGL::CglXWaitGL::CglXWaitGL() {}

gits::CArgument& gits::OpenGL::CglXWaitGL::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXWaitGL::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXWaitGL();
#endif
}

/* ***************************** ID_GLX_WAIT_X *************************** */

gits::OpenGL::CglXWaitX::CglXWaitX() {}

gits::CArgument& gits::OpenGL::CglXWaitX::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXWaitX::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXWaitX();
#endif
}

/* ***************************** ID_GLX_USE_XFONT *************************** */

gits::OpenGL::CglXUseXFont::CglXUseXFont() {}

gits::OpenGL::CglXUseXFont::CglXUseXFont(Font font, int first, int count, int list)
    : _font(font), _first(first), _count(count), _list(list) {}

gits::CArgument& gits::OpenGL::CglXUseXFont::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _font, _first, _count, _list);
}

void gits::OpenGL::CglXUseXFont::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXUseXFont(*_font, *_first, *_count, *_list);
#endif
}

/* ***************************** ID_GLX_QUERY_EXTENSIONS_STRING *************************** */

gits::OpenGL::CglXQueryExtensionsString::CglXQueryExtensionsString() {}

gits::OpenGL::CglXQueryExtensionsString::CglXQueryExtensionsString(const char* return_value,
                                                                   Display* dpy,
                                                                   int screen)
    : _return_value(return_value, '\0', 1), _dpy(dpy), _screen(screen) {
#ifdef GITS_PLATFORM_X11
  drv.gl.Initialize(CGlDriver::API_GL);
#endif
}

gits::CArgument& gits::OpenGL::CglXQueryExtensionsString::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _screen);
}

void gits::OpenGL::CglXQueryExtensionsString::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::GLX);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  ptblInitialize(CGlDriver::API_GL);
  // drv.glx.glXQueryExtensionsString(*_dpy, *_screen);
#endif
}

/* ***************************** ID_GLX_QUERY_SERVER_STRING *************************** */

gits::OpenGL::CglXQueryServerString::CglXQueryServerString() {}

gits::OpenGL::CglXQueryServerString::CglXQueryServerString(const char* return_value,
                                                           Display* dpy,
                                                           int screen,
                                                           int name)
    : _return_value(return_value, '\0', 1), _dpy(dpy), _screen(screen), _name(name) {}

gits::CArgument& gits::OpenGL::CglXQueryServerString::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _screen, _name);
}

void gits::OpenGL::CglXQueryServerString::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXQueryServerString(*_dpy, *_screen, *_name);
#endif
}

/* ***************************** ID_GLX_GET_CLIENT_STRING *************************** */

gits::OpenGL::CglXGetClientString::CglXGetClientString() {}

gits::OpenGL::CglXGetClientString::CglXGetClientString(const char* return_value,
                                                       Display* dpy,
                                                       int name)
    : _return_value(return_value, '\0', 1), _dpy(dpy), _name(name) {}

gits::CArgument& gits::OpenGL::CglXGetClientString::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _name);
}

void gits::OpenGL::CglXGetClientString::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetClientString(*_dpy, *_name);
#endif
}

/* ***************************** ID_GLX_GET_CURRENT_DISPLAY *************************** */

gits::OpenGL::CglXGetCurrentDisplay::CglXGetCurrentDisplay() {}

gits::OpenGL::CglXGetCurrentDisplay::CglXGetCurrentDisplay(Display* return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CglXGetCurrentDisplay::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXGetCurrentDisplay::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetCurrentDisplay();
#endif
}

/* ***************************** ID_GLX_CHOOSE_FBCONFIG *************************** */

gits::OpenGL::CglXChooseFBConfig::CglXChooseFBConfig() {}

gits::OpenGL::CglXChooseFBConfig::CglXChooseFBConfig(
    GLXFBConfig* return_value, Display* dpy, int screen, const int* attribList, int* nitems)
    : _return_value(1, return_value),
      _dpy(dpy),
      _screen(screen),
      _attribList(attribList, 0, 2),
      _nitems(1, nitems) {
  PtblNtvStreamApi(PtblNativeAPI::Type::GLX);
#ifdef GITS_PLATFORM_X11
  drv.gl.Initialize(CGlDriver::API_GL);
#endif
}

gits::CArgument& gits::OpenGL::CglXChooseFBConfig::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _screen, _attribList, _nitems);
}

void gits::OpenGL::CglXChooseFBConfig::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::GLX);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  ptblInitialize(CGlDriver::API_GL);
  // drv.glx.glXChooseFBConfig(*_dpy, *_screen, *_attribList, *_nitems);
#endif
}

/* ***************************** ID_GLX_GET_FBCONFIG_ATTRIB *************************** */

gits::OpenGL::CglXGetFBConfigAttrib::CglXGetFBConfigAttrib() {}

gits::OpenGL::CglXGetFBConfigAttrib::CglXGetFBConfigAttrib(
    int return_value, Display* dpy, GLXFBConfig config, int attribute, int* value)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _attribute(attribute),
      _value(1, value) {}

gits::CArgument& gits::OpenGL::CglXGetFBConfigAttrib::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _attribute, _value);
}

void gits::OpenGL::CglXGetFBConfigAttrib::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetFBConfigAttrib(*_dpy, *_config, *_attribute, *_value);
#endif
}

/* ***************************** ID_GLX_GET_FBCONFIGS *************************** */

gits::OpenGL::CglXGetFBConfigs::CglXGetFBConfigs() {}

gits::OpenGL::CglXGetFBConfigs::CglXGetFBConfigs(GLXFBConfig* return_value,
                                                 Display* dpy,
                                                 int screen,
                                                 int* nelements)
    : _return_value(1, return_value), _dpy(dpy), _screen(screen), _nelements(1, nelements) {}

gits::CArgument& gits::OpenGL::CglXGetFBConfigs::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _screen, _nelements);
}

void gits::OpenGL::CglXGetFBConfigs::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetFBConfigs(*_dpy, *_screen, *_nelements);
#endif
}

/* ***************************** ID_GLX_GET_VISUAL_FROM_FBCONFIG *************************** */

gits::OpenGL::CglXGetVisualFromFBConfig::CglXGetVisualFromFBConfig() {}

gits::OpenGL::CglXGetVisualFromFBConfig::CglXGetVisualFromFBConfig(XVisualInfo* return_value,
                                                                   Display* dpy,
                                                                   GLXFBConfig config)
    : _return_value(return_value), _dpy(dpy), _config(config) {}

gits::CArgument& gits::OpenGL::CglXGetVisualFromFBConfig::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config);
}

void gits::OpenGL::CglXGetVisualFromFBConfig::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetVisualFromFBConfig(*_dpy, *_config);
#endif
}

/* ***************************** ID_GLX_CREATE_WINDOW *************************** */

gits::OpenGL::CglXCreateWindow::CglXCreateWindow() {}

gits::OpenGL::CglXCreateWindow::CglXCreateWindow(
    GLXWindow return_value, Display* dpy, GLXFBConfig config, Window win, const int* attribList)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _win(win),
      _attribList(attribList, 0, 2) {
#ifdef GITS_PLATFORM_X11
  GlxRecorderMngr::AddSurface(return_value, win);
#endif
}

gits::CArgument& gits::OpenGL::CglXCreateWindow::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _win, _attribList);
}

void gits::OpenGL::CglXCreateWindow::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXCreateWindow(*_dpy, *_config, *_win, *_attribList);
#endif
}

/* ***************************** ID_GLX_DESTROY_WINDOW *************************** */

gits::OpenGL::CglXDestroyWindow::CglXDestroyWindow() {}

gits::OpenGL::CglXDestroyWindow::CglXDestroyWindow(Display* dpy, GLXWindow window)
    : _dpy(dpy), _window(window) {
#ifdef GITS_PLATFORM_X11
  GlxRecorderMngr::RemoveSurface(window);
#endif
}

gits::CArgument& gits::OpenGL::CglXDestroyWindow::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _window);
}

void gits::OpenGL::CglXDestroyWindow::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXDestroyWindow(*_dpy, *_window);
#endif
}

/* ***************************** ID_GLX_CREATE_PIXMAP *************************** */

gits::OpenGL::CglXCreatePixmap::CglXCreatePixmap() {}

gits::OpenGL::CglXCreatePixmap::CglXCreatePixmap(
    GLXPixmap return_value, Display* dpy, GLXFBConfig config, Pixmap pixmap, const int* attribList)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _pixmap(pixmap),
      _attribList(attribList, 0, 2) {}

gits::CArgument& gits::OpenGL::CglXCreatePixmap::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _pixmap, _attribList);
}

void gits::OpenGL::CglXCreatePixmap::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXCreatePixmap(*_dpy, *_config, *_pixmap, *_attribList);
#endif
}

/* ***************************** ID_GLX_DESTROY_PIXMAP *************************** */

gits::OpenGL::CglXDestroyPixmap::CglXDestroyPixmap() {}

gits::OpenGL::CglXDestroyPixmap::CglXDestroyPixmap(Display* dpy, GLXPixmap pixmap)
    : _dpy(dpy), _pixmap(pixmap) {}

gits::CArgument& gits::OpenGL::CglXDestroyPixmap::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _pixmap);
}

void gits::OpenGL::CglXDestroyPixmap::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXDestroyPixmap(*_dpy, *_pixmap);
#endif
}

/* ***************************** ID_GLX_CREATE_PBUFFER *************************** */

gits::OpenGL::CglXCreatePbuffer::CglXCreatePbuffer() {}

gits::OpenGL::CglXCreatePbuffer::CglXCreatePbuffer(GLXPbuffer return_value,
                                                   Display* dpy,
                                                   GLXFBConfig config,
                                                   const int* attribList)
    : _return_value(return_value), _dpy(dpy), _config(config), _attribList(attribList, 0, 2) {}

gits::CArgument& gits::OpenGL::CglXCreatePbuffer::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _attribList);
}

void gits::OpenGL::CglXCreatePbuffer::Run() {
  ptbl_glXCreatePbuffer(*_dpy, *_config, *_attribList);
}

/* ***************************** ID_GLX_DESTROY_PBUFFER *************************** */

gits::OpenGL::CglXDestroyPbuffer::CglXDestroyPbuffer() {}

gits::OpenGL::CglXDestroyPbuffer::CglXDestroyPbuffer(Display* dpy, GLXPbuffer pbuf)
    : _dpy(dpy), _pbuf(pbuf) {}

gits::CArgument& gits::OpenGL::CglXDestroyPbuffer::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _pbuf);
}

void gits::OpenGL::CglXDestroyPbuffer::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXDestroyPbuffer(*_dpy, *_pbuf);
#endif
}

/* ***************************** ID_GLX_QUERY_DRAWABLE *************************** */

gits::OpenGL::CglXQueryDrawable::CglXQueryDrawable() {}

gits::OpenGL::CglXQueryDrawable::CglXQueryDrawable(Display* dpy,
                                                   GLXDrawable draw,
                                                   int attribute,
                                                   unsigned int* value)
    : _dpy(dpy), _draw(draw), _attribute(attribute), _value(1, value) {}

gits::CArgument& gits::OpenGL::CglXQueryDrawable::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _draw, _attribute, _value);
}

void gits::OpenGL::CglXQueryDrawable::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXQueryDrawable(*_dpy, *_draw, *_attribute, *_value);
#endif
}

/* ***************************** ID_GLX_CREATE_NEW_CONTEXT *************************** */

gits::OpenGL::CglXCreateNewContext::CglXCreateNewContext() {}

gits::OpenGL::CglXCreateNewContext::CglXCreateNewContext(GLXContext return_value,
                                                         Display* dpy,
                                                         GLXFBConfig config,
                                                         int renderType,
                                                         GLXContext shareList,
                                                         Bool direct)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _renderType(renderType),
      _shareList(shareList),
      _direct(direct) {
#ifdef GITS_PLATFORM_X11
  AddFbConfigAttrib(dpy, config, GLX_BUFFER_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_DEPTH_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_STENCIL_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_X_RENDERABLE);
  AddFbConfigAttrib(dpy, config, GLX_DRAWABLE_TYPE);
  AddFbConfigAttrib(dpy, config, GLX_RENDER_TYPE);
  AddFbConfigAttrib(dpy, config, GLX_DOUBLEBUFFER);
  AddFbConfigAttrib(dpy, config, GLX_STEREO);
  AddFbConfigAttrib(dpy, config, GLX_ALPHA_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_ACCUM_RED_SIZE);
  FinishFbConfigAttribList();

  SD().AddContext(return_value, shareList);
#endif
}

gits::CArgument& gits::OpenGL::CglXCreateNewContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _renderType, _shareList, _direct,
                       _fbconfig_attribs);
}

void gits::OpenGL::CglXCreateNewContext::AddFbConfigAttrib(Display* dpy,
                                                           GLXFBConfig config,
                                                           int attribute) {
#ifdef GITS_PLATFORM_X11
  int value;
  if (drv.glx.glXGetFBConfigAttrib(dpy, config, attribute, &value) == GLX_BAD_ATTRIBUTE) {
    LOG_ERROR << "Invalid attribute in glXGetFBConfigAttrib(...): " << attribute;
    throw ENotFound(EXCEPTION_MESSAGE);
  }
  _fbconfig_attribs.Vector().push_back(attribute);
  _fbconfig_attribs.Vector().push_back(value);
#endif
}

void gits::OpenGL::CglXCreateNewContext::FinishFbConfigAttribList() {
  _fbconfig_attribs.Vector().push_back(0);
}

void gits::OpenGL::CglXCreateNewContext::Run() {
  ptblInitialize(CGlDriver::API_GL);
  // Init
  _dpy.AddMapping((Display*)GetNativeDisplay());

  // Choose FBConfig
  int fbConfigCount;
  GLXFBConfig* fbConfigs = ptbl_glXChooseFBConfig((Display*)GetNativeDisplay(),
                                                  ptbl_DefaultScreen((Display*)GetNativeDisplay()),
                                                  *_fbconfig_attribs, &fbConfigCount);
  if (fbConfigs == nullptr) {
    LOG_ERROR << "glXChooseFBConfig failed: no config matches were found";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  _config.AddMapping(fbConfigs[0]);

  // Create context
  GLXContext ctx = ptbl_glXCreateNewContext((Display*)GetNativeDisplay(), fbConfigs[0],
                                            *_renderType, nullptr, *_direct);
  if (ctx == nullptr) {
    LOG_ERROR << "glXCreateNewContext failed on the client side";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  _return_value.AddMapping(ctx);

  GlxPlayerMngr* glxPlayerMngr = new GlxPlayerMngr((Display*)GetNativeDisplay(), ctx, *_config);
  GlxWinCtxMngr::Add(ctx, glxPlayerMngr);

  ptbl_XFree(fbConfigs);

  SD().AddContext(ctx, *_shareList);
}

/* ***************************** ID_GLX_MAKE_CONTEXT_CURRENT *************************** */

gits::OpenGL::CglXMakeContextCurrent::CglXMakeContextCurrent() {}

gits::OpenGL::CglXMakeContextCurrent::CglXMakeContextCurrent(
    Bool return_value, Display* dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
    : _return_value(return_value), _dpy(dpy), _draw(draw), _read(read), _ctx(ctx) {
#ifdef GITS_PLATFORM_X11
  GlxWinCtxMngr* ctxManager = GlxWinCtxMngr::Get(ctx);
  if (ctxManager == nullptr) {
    GlxRecorderMngr* glxRecorderMngr = new GlxRecorderMngr(dpy, ctx);
    GlxWinCtxMngr::Add(ctx, glxRecorderMngr);

    // Redundant check, so Klocwork doesn't complain.
    ctxManager = GlxWinCtxMngr::Get(ctx);
    if (ctxManager == nullptr) {
      LOG_ERROR << "Value for context " << ctx
                << " not found in map, even though it was just added there.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  // Get Window data.
  GlxRecorderMngr* glxRecorderMngr = dynamic_cast<GlxRecorderMngr*>(ctxManager);
  if (glxRecorderMngr == nullptr) {
    LOG_ERROR << "GlxRecorderMngr is not properly set.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  glxRecorderMngr->UpdateWindowHandle();
  glxRecorderMngr->UpdateWindow(_winparams.Vector());
  glxRecorderMngr->GetWindowParams(_winparams.Vector());

  SD().SetCurrentContext((void*)ctx);
  CStateDynamicNative::Get().MapglXCtxToSurf((void*)ctx, draw, read);
#endif
}

gits::CArgument& gits::OpenGL::CglXMakeContextCurrent::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _draw, _read, _ctx, _winparams);
}

void gits::OpenGL::CglXMakeContextCurrent::Run() {
  if (*_ctx == 0) {
    ptbl_glXMakeContextCurrent(*_dpy, 0, 0, nullptr);
    return;
  }

  GlxPlayerMngr* glxPlayerMngr = dynamic_cast<GlxPlayerMngr*>(GlxWinCtxMngr::Get(*_ctx));

  if (glxPlayerMngr == nullptr) {
    LOG_ERROR << "CglXMakeContextCurrent called on unmapped context";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  if ((_winparams.Vector().size() != 0) &&
      ((!_draw.CheckMapping()) || ((uint64_t)glxPlayerMngr->GetWindowHandle() != *_draw) ||
       (glxPlayerMngr->GetWindowHandle() == 0))) {
    glxPlayerMngr->SetupWindow(_winparams.Vector());
  }

  _draw.AddMapping((uint64_t)glxPlayerMngr->GetWindowHandle());
  ptbl_glXMakeContextCurrent(*_dpy, (GLXDrawable)*_draw, (GLXDrawable)*_draw, *_ctx);

  SD().SetCurrentContext((void*)*_ctx);
}

/* ***************************** ID_GLX_GET_CURRENT_READ_DRAWABLE *************************** */

gits::OpenGL::CglXGetCurrentReadDrawable::CglXGetCurrentReadDrawable() {}

gits::OpenGL::CglXGetCurrentReadDrawable::CglXGetCurrentReadDrawable(GLXDrawable return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CglXGetCurrentReadDrawable::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXGetCurrentReadDrawable::Run(){
#ifdef GITS_PLATFORM_X11
// drv.glx.glXGetCurrentReadDrawable();
#endif
}

/* ***************************** ID_GLX_QUERY_CONTEXT *************************** */

gits::OpenGL::CglXQueryContext::CglXQueryContext() {
}

gits::OpenGL::CglXQueryContext::CglXQueryContext(
    int return_value, Display* dpy, GLXContext ctx, int attribute, int* value)
    : _return_value(return_value), _dpy(dpy), _ctx(ctx), _attribute(attribute), _value(1, value) {}

gits::CArgument& gits::OpenGL::CglXQueryContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _ctx, _attribute, _value);
}

void gits::OpenGL::CglXQueryContext::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXQueryContext(*_dpy, *_ctx, *_attribute, *_value);
#endif
}

/* ***************************** ID_GLX_SELECT_EVENT *************************** */

gits::OpenGL::CglXSelectEvent::CglXSelectEvent() {}

gits::OpenGL::CglXSelectEvent::CglXSelectEvent(Display* dpy,
                                               GLXDrawable drawable,
                                               unsigned long mask)
    : _dpy(dpy), _drawable(drawable), _mask(mask) {}

gits::CArgument& gits::OpenGL::CglXSelectEvent::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _mask);
}

void gits::OpenGL::CglXSelectEvent::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXSelectEvent(*_dpy, *_drawable, *_mask);
#endif
}

/* ***************************** ID_GLX_GET_SELECTED_EVENT *************************** */

gits::OpenGL::CglXGetSelectedEvent::CglXGetSelectedEvent() {}

gits::OpenGL::CglXGetSelectedEvent::CglXGetSelectedEvent(Display* dpy,
                                                         GLXDrawable drawable,
                                                         unsigned long* mask)
    : _dpy(dpy), _drawable(drawable), _mask(1, mask) {}

gits::CArgument& gits::OpenGL::CglXGetSelectedEvent::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _mask);
}

void gits::OpenGL::CglXGetSelectedEvent::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXGetSelectedEvent(*_dpy, *_drawable, *_mask);
#endif
}

/* ***************************** ID_GLX_GET_PROC_ADDRESS_ARB *************************** */

gits::OpenGL::CglXGetProcAddressARB::CglXGetProcAddressARB() {}

gits::OpenGL::CglXGetProcAddressARB::CglXGetProcAddressARB(void* return_value,
                                                           const GLubyte* func_name)
    : _return_value(return_value), _func_name(func_name, '\0', 1) {}

gits::CArgument& gits::OpenGL::CglXGetProcAddressARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _func_name);
}

void gits::OpenGL::CglXGetProcAddressARB::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetProcAddressARB(*_func_name);
#endif
}

/* ***************************** ID_GLX_GET_PROC_ADDRESS *************************** */

gits::OpenGL::CglXGetProcAddress::CglXGetProcAddress() {}

gits::OpenGL::CglXGetProcAddress::CglXGetProcAddress(void* return_value, const GLubyte* func_name)
    : _return_value(return_value), _func_name(func_name, '\0', 1) {}

gits::CArgument& gits::OpenGL::CglXGetProcAddress::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _func_name);
}

void gits::OpenGL::CglXGetProcAddress::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXGetProcAddress(*_func_name);
#endif
}

/* ***************************** ID_GLX_ALLOCATE_MEMORY_NV *************************** */

gits::OpenGL::CglXAllocateMemoryNV::CglXAllocateMemoryNV() {}

gits::OpenGL::CglXAllocateMemoryNV::CglXAllocateMemoryNV(
    void* return_value, GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority)
    : _return_value(return_value),
      _size(size),
      _readfreq(readfreq),
      _writefreq(writefreq),
      _priority(priority) {}

gits::CArgument& gits::OpenGL::CglXAllocateMemoryNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _size, _readfreq, _writefreq, _priority);
}

void gits::OpenGL::CglXAllocateMemoryNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXAllocateMemoryNV(*_size, *_readfreq, *_writefreq, *_priority);
#endif
}

/* ***************************** ID_GLX_FREE_MEMORY_NV *************************** */

gits::OpenGL::CglXFreeMemoryNV::CglXFreeMemoryNV() {}

gits::OpenGL::CglXFreeMemoryNV::CglXFreeMemoryNV(GLvoid* pointer) : _pointer(pointer) {}

gits::CArgument& gits::OpenGL::CglXFreeMemoryNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _pointer);
}

void gits::OpenGL::CglXFreeMemoryNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXFreeMemoryNV(*_pointer);
#endif
}

/* ***************************** ID_GLX_BIND_TEX_IMAGE_ARB *************************** */

gits::OpenGL::CglXBindTexImageARB::CglXBindTexImageARB() {}

gits::OpenGL::CglXBindTexImageARB::CglXBindTexImageARB(Bool return_value,
                                                       Display* dpy,
                                                       GLXPbuffer pbuffer,
                                                       int buffer)
    : _return_value(return_value), _dpy(dpy), _pbuffer(pbuffer), _buffer(buffer) {}

gits::CArgument& gits::OpenGL::CglXBindTexImageARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _pbuffer, _buffer);
}

void gits::OpenGL::CglXBindTexImageARB::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXBindTexImageARB(*_dpy, *_pbuffer, *_buffer);
#endif
}

/* ***************************** ID_GLX_RELEASE_TEX_IMAGE_ARB *************************** */

gits::OpenGL::CglXReleaseTexImageARB::CglXReleaseTexImageARB() {}

gits::OpenGL::CglXReleaseTexImageARB::CglXReleaseTexImageARB(Bool return_value,
                                                             Display* dpy,
                                                             GLXPbuffer pbuffer,
                                                             int buffer)
    : _return_value(return_value), _dpy(dpy), _pbuffer(pbuffer), _buffer(buffer) {}

gits::CArgument& gits::OpenGL::CglXReleaseTexImageARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _pbuffer, _buffer);
}

void gits::OpenGL::CglXReleaseTexImageARB::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXReleaseTexImageARB(*_dpy, *_pbuffer, *_buffer);
#endif
}

/* ***************************** ID_GLX_DRAWABLE_ATTRIB_ARB *************************** */

gits::OpenGL::CglXDrawableAttribARB::CglXDrawableAttribARB() {}

gits::OpenGL::CglXDrawableAttribARB::CglXDrawableAttribARB(Bool return_value,
                                                           Display* dpy,
                                                           GLXDrawable draw,
                                                           const int* attribList)
    : _return_value(return_value), _dpy(dpy), _draw(draw), _attribList(attribList, 0, 2) {}

gits::CArgument& gits::OpenGL::CglXDrawableAttribARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _draw, _attribList);
}

void gits::OpenGL::CglXDrawableAttribARB::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXDrawableAttribARB(*_dpy, *_draw, *_attribList);
#endif
}

/* ***************************** ID_GLX_GET_FRAME_USAGE_MESA *************************** */

gits::OpenGL::CglXGetFrameUsageMESA::CglXGetFrameUsageMESA() {}

gits::OpenGL::CglXGetFrameUsageMESA::CglXGetFrameUsageMESA(int return_value,
                                                           Display* dpy,
                                                           GLXDrawable drawable,
                                                           float* usage)
    : _return_value(return_value), _dpy(dpy), _drawable(drawable), _usage(1, usage) {}

gits::CArgument& gits::OpenGL::CglXGetFrameUsageMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _usage);
}

void gits::OpenGL::CglXGetFrameUsageMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXGetFrameUsageMESA(*_dpy, *_drawable, *_usage);
#endif
}

/* ***************************** ID_GLX_BEGIN_FRAME_TRACKING_MESA *************************** */

gits::OpenGL::CglXBeginFrameTrackingMESA::CglXBeginFrameTrackingMESA() {}

gits::OpenGL::CglXBeginFrameTrackingMESA::CglXBeginFrameTrackingMESA(int return_value,
                                                                     Display* dpy,
                                                                     GLXDrawable drawable)
    : _return_value(return_value), _dpy(dpy), _drawable(drawable) {}

gits::CArgument& gits::OpenGL::CglXBeginFrameTrackingMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable);
}

void gits::OpenGL::CglXBeginFrameTrackingMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXBeginFrameTrackingMESA(*_dpy, *_drawable);
#endif
}

/* ***************************** ID_GLX_END_FRAME_TRACKING_MESA *************************** */

gits::OpenGL::CglXEndFrameTrackingMESA::CglXEndFrameTrackingMESA() {}

gits::OpenGL::CglXEndFrameTrackingMESA::CglXEndFrameTrackingMESA(int return_value,
                                                                 Display* dpy,
                                                                 GLXDrawable drawable)
    : _return_value(return_value), _dpy(dpy), _drawable(drawable) {}

gits::CArgument& gits::OpenGL::CglXEndFrameTrackingMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable);
}

void gits::OpenGL::CglXEndFrameTrackingMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXEndFrameTrackingMESA(*_dpy, *_drawable);
#endif
}

/* ***************************** ID_GLX_QUERY_FRAME_TRACKING_MESA *************************** */

gits::OpenGL::CglXQueryFrameTrackingMESA::CglXQueryFrameTrackingMESA() {}

gits::OpenGL::CglXQueryFrameTrackingMESA::CglXQueryFrameTrackingMESA(int return_value,
                                                                     Display* dpy,
                                                                     GLXDrawable drawable,
                                                                     int64_t* swapCount,
                                                                     int64_t* missedFrames,
                                                                     float* lastMissedUsage)
    : _return_value(return_value),
      _dpy(dpy),
      _drawable(drawable),
      _swapCount(1, swapCount),
      _missedFrames(1, missedFrames),
      _lastMissedUsage(1, lastMissedUsage) {}

gits::CArgument& gits::OpenGL::CglXQueryFrameTrackingMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _swapCount, _missedFrames,
                       _lastMissedUsage);
}

void gits::OpenGL::CglXQueryFrameTrackingMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  // drv.glx.glXQueryFrameTrackingMESA(*_dpy, *_drawable, *_swapCount,
  // *_missedFrames, *_lastMissedUsage);
#endif
}

/* ***************************** ID_GLX_SWAP_INTERVAL_MESA *************************** */

gits::OpenGL::CglXSwapIntervalMESA::CglXSwapIntervalMESA() {}

gits::OpenGL::CglXSwapIntervalMESA::CglXSwapIntervalMESA(int return_value, unsigned int interval)
    : _return_value(return_value), _interval(interval) {}

gits::CArgument& gits::OpenGL::CglXSwapIntervalMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _interval);
}

void gits::OpenGL::CglXSwapIntervalMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXSwapIntervalMESA(*_interval);
#endif
}

/* ***************************** ID_GLX_GET_SWAP_INTERVAL_MESA *************************** */

gits::OpenGL::CglXGetSwapIntervalMESA::CglXGetSwapIntervalMESA() {}

gits::OpenGL::CglXGetSwapIntervalMESA::CglXGetSwapIntervalMESA(int return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CglXGetSwapIntervalMESA::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CglXGetSwapIntervalMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXGetSwapIntervalMESA();
#endif
}

/* ***************************** ID_GLX_BIND_TEX_IMAGE_EXT *************************** */

gits::OpenGL::CglXBindTexImageEXT::CglXBindTexImageEXT() {}

gits::OpenGL::CglXBindTexImageEXT::CglXBindTexImageEXT(Display* dpy,
                                                       GLXDrawable drawable,
                                                       int buffer,
                                                       const int* attrib_list)
    : _dpy(dpy), _drawable(drawable), _buffer(buffer), _attrib_list(attrib_list, 0, 2) {}

gits::CArgument& gits::OpenGL::CglXBindTexImageEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _buffer, _attrib_list);
}

void gits::OpenGL::CglXBindTexImageEXT::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXBindTexImageEXT(*_dpy, *_drawable, *_buffer, *_attrib_list);
#endif
}

/* ***************************** ID_GLX_RELEASE_TEX_IMAGE_EXT *************************** */

gits::OpenGL::CglXReleaseTexImageEXT::CglXReleaseTexImageEXT() {}

gits::OpenGL::CglXReleaseTexImageEXT::CglXReleaseTexImageEXT(Display* dpy,
                                                             GLXDrawable drawable,
                                                             int buffer)
    : _dpy(dpy), _drawable(drawable), _buffer(buffer) {}

gits::CArgument& gits::OpenGL::CglXReleaseTexImageEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _drawable, _buffer);
}

void gits::OpenGL::CglXReleaseTexImageEXT::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXReleaseTexImageEXT(*_dpy, *_drawable, *_buffer);
#endif
}

/* ***************************** ID_GLX_CREATE_CONTEXT_ATTRIBS_ARB *************************** */

gits::OpenGL::CglXCreateContextAttribsARB::CglXCreateContextAttribsARB() {}

gits::OpenGL::CglXCreateContextAttribsARB::CglXCreateContextAttribsARB(GLXContext return_value,
                                                                       Display* dpy,
                                                                       GLXFBConfig config,
                                                                       GLXContext share_context,
                                                                       Bool direct,
                                                                       const int* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _share_context(share_context),
      _direct(direct),
      _attrib_list(attrib_list, 0, 2) {
#ifdef GITS_PLATFORM_X11
  AddFbConfigAttrib(dpy, config, GLX_BUFFER_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_DEPTH_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_STENCIL_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_X_RENDERABLE);
  AddFbConfigAttrib(dpy, config, GLX_DRAWABLE_TYPE);
  AddFbConfigAttrib(dpy, config, GLX_RENDER_TYPE);
  AddFbConfigAttrib(dpy, config, GLX_DOUBLEBUFFER);
  AddFbConfigAttrib(dpy, config, GLX_STEREO);
  AddFbConfigAttrib(dpy, config, GLX_ALPHA_SIZE);
  AddFbConfigAttrib(dpy, config, GLX_ACCUM_RED_SIZE);
  FinishFbConfigAttribList();

  SD().AddContext(return_value, share_context);
#endif
}

gits::CArgument& gits::OpenGL::CglXCreateContextAttribsARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _share_context, _direct, _attrib_list,
                       _fbconfig_attribs);
}

void gits::OpenGL::CglXCreateContextAttribsARB::AddFbConfigAttrib(Display* dpy,
                                                                  GLXFBConfig config,
                                                                  int attribute) {
#ifdef GITS_PLATFORM_X11
  int value;
  if (drv.glx.glXGetFBConfigAttrib(dpy, config, attribute, &value) == GLX_BAD_ATTRIBUTE) {
    LOG_ERROR << "Invalid attribute in glXGetFBConfigAttrib(...): " << attribute;
    throw ENotFound(EXCEPTION_MESSAGE);
  }
  _fbconfig_attribs.Vector().push_back(attribute);
  _fbconfig_attribs.Vector().push_back(value);
#endif
}

void gits::OpenGL::CglXCreateContextAttribsARB::FinishFbConfigAttribList() {
  _fbconfig_attribs.Vector().push_back(0);
}

void gits::OpenGL::CglXCreateContextAttribsARB::Run() {
  ptblInitialize(CGlDriver::API_GL);

  // Init
  _dpy.AddMapping((Display*)GetNativeDisplay());

  // Choose FBConfig
  int fbConfigCount;
  GLXFBConfig* fbConfigs = ptbl_glXChooseFBConfig((Display*)GetNativeDisplay(),
                                                  ptbl_DefaultScreen((Display*)GetNativeDisplay()),
                                                  *_fbconfig_attribs, &fbConfigCount);
  if (fbConfigs == nullptr) {
    LOG_ERROR << "glXChooseFBConfig failed: no config matches were found";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  _config.AddMapping(fbConfigs[0]);

  // Create context
  GLXContext ctx = ptbl_glXCreateContextAttribsARB((Display*)GetNativeDisplay(), fbConfigs[0],
                                                   nullptr, *_direct, *_attrib_list);
  if (ctx == nullptr) {
    LOG_ERROR << "glXCreateContextAttribsARB failed on the client side";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  _return_value.AddMapping(ctx);

  GlxPlayerMngr* glxPlayerMngr = new GlxPlayerMngr((Display*)GetNativeDisplay(), ctx, *_config);
  GlxWinCtxMngr::Add(ctx, glxPlayerMngr);

  ptbl_XFree(fbConfigs);

  SD().AddContext(ctx, *_share_context);
}

/* ***************************** ID_GLX_SWAP_INTERVAL_SGI *************************** */

gits::OpenGL::CglXSwapIntervalSGI::CglXSwapIntervalSGI() {}

gits::OpenGL::CglXSwapIntervalSGI::CglXSwapIntervalSGI(int return_value, int interval)
    : _return_value(return_value), _interval(interval) {}

gits::CArgument& gits::OpenGL::CglXSwapIntervalSGI::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _interval);
}

void gits::OpenGL::CglXSwapIntervalSGI::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
#ifdef GITS_PLATFORM_X11
  drv.glx.glXSwapIntervalSGI(*_interval);
#endif
}
