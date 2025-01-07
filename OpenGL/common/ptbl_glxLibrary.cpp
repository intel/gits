// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptbl_glxLibrary.cpp
*
*/

#include "ptbl_glxLibrary.h"
#include "ptblLibrary.h"
#include "openglDrivers.h"
#include "openglTypes.h"
#include "openglEnums.h"
#if defined GITS_PLATFORM_X11
#define XVisualInfo XVisualInfo_
#include <GL/glx.h>
#undef True
#undef XVisualInfo
#endif

//****************** Helper functions ***************************
gits::OpenGL::GLXPFAttribs gits::OpenGL::PtblToGLXARBAttribs(const PtblPFAttribs& ptblattribs) {
  GLXPFAttribs glxattribs;
  for (auto& ptblattr : ptblattribs) {
    switch (ptblattr.first) {
    case ptblPFDoubleBuffer:
      if (ptblattr.second > 0) {
        glxattribs.push_back(GLX_DOUBLEBUFFER);
        glxattribs.push_back(1);
      }
      break;
    case ptblPFRedSize:
      glxattribs.push_back(GLX_RED_SIZE);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFGreenSize:
      glxattribs.push_back(GLX_GREEN_SIZE);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFBlueSize:
      glxattribs.push_back(GLX_BLUE_SIZE);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFAlphaSize:
      glxattribs.push_back(GLX_ALPHA_SIZE);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFDepthSize:
      glxattribs.push_back(GLX_DEPTH_SIZE);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFStencilSize:
      glxattribs.push_back(GLX_STENCIL_SIZE);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFSamples:
      glxattribs.push_back(GLX_SAMPLES_ARB);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFSampleBuffers:
      glxattribs.push_back(GLX_SAMPLE_BUFFERS_ARB);
      glxattribs.push_back(ptblattr.second);
      break;
    case ptblPFAccelerated:
    case ptblPFPBuffer:
      break;
    default:
      throw gits::ENotImplemented(EXCEPTION_MESSAGE);
      break;
    };
  }
  glxattribs.push_back(0);
  return glxattribs;
}

gits::OpenGL::PtblPFAttribs gits::OpenGL::GLXToPtblAttribs(const int* glxattribs) {
  PtblPFAttribs ptblattribs;
  auto ptr = glxattribs;

  ptblattribs[ptblPFAccelerated] = 1;
  while (*ptr != 0) {
    switch (*ptr) {
    case GLX_DOUBLEBUFFER:
      ptr++;
      ptblattribs[ptblPFDoubleBuffer] = *ptr;
      break;
    case GLX_RED_SIZE:
      ptr++;
      ptblattribs[ptblPFRedSize] = *ptr;
      break;
    case GLX_GREEN_SIZE:
      ptr++;
      ptblattribs[ptblPFGreenSize] = *ptr;
      break;
    case GLX_BLUE_SIZE:
      ptr++;
      ptblattribs[ptblPFBlueSize] = *ptr;
      break;
    case GLX_ALPHA_SIZE:
      ptr++;
      ptblattribs[ptblPFAlphaSize] = *ptr;
      break;
    case GLX_DEPTH_SIZE:
      ptr++;
      ptblattribs[ptblPFDepthSize] = *ptr;
      break;
    case GLX_STENCIL_SIZE:
      ptr++;
      ptblattribs[ptblPFStencilSize] = *ptr;
      break;
    case GLX_SAMPLES_ARB:
      ptr++;
      ptblattribs[ptblPFSamples] = *ptr;
      break;
    case GLX_SAMPLE_BUFFERS_ARB:
      ptr++;
      ptblattribs[ptblPFSampleBuffers] = *ptr;
      break;
    default:
      ptr++;
    };
    ptr++;
  }
  return ptblattribs;
}

gits::OpenGL::PtblPFAttribs gits::OpenGL::XVisualToPtblAttribs(const int* glxattribs) {
  PtblPFAttribs ptblattribs;
  auto ptr = glxattribs;

  ptblattribs[ptblPFAccelerated] = 1;
  while (*ptr != 0) {
    switch (*ptr) {
    case GLX_DOUBLEBUFFER:
      ptblattribs[ptblPFDoubleBuffer] = 1;
      break;
    case GLX_RED_SIZE:
      ptr++;
      ptblattribs[ptblPFRedSize] = *ptr;
      break;
    case GLX_GREEN_SIZE:
      ptr++;
      ptblattribs[ptblPFGreenSize] = *ptr;
      break;
    case GLX_BLUE_SIZE:
      ptr++;
      ptblattribs[ptblPFBlueSize] = *ptr;
      break;
    case GLX_ALPHA_SIZE:
      ptr++;
      ptblattribs[ptblPFAlphaSize] = *ptr;
      break;
    case GLX_DEPTH_SIZE:
      ptr++;
      ptblattribs[ptblPFDepthSize] = *ptr;
      break;
    case GLX_STENCIL_SIZE:
      ptr++;
      ptblattribs[ptblPFDepthSize] = *ptr;
      break;
    case GLX_SAMPLES_ARB:
      ptr++;
      ptblattribs[ptblPFSamples] = *ptr;
      break;
    case GLX_SAMPLE_BUFFERS_ARB:
      ptr++;
      ptblattribs[ptblPFSampleBuffers] = *ptr;
      break;
    case GLX_ACCUM_RED_SIZE:
    case GLX_AUX_BUFFERS:
    case GLX_ACCUM_GREEN_SIZE:
    case GLX_ACCUM_BLUE_SIZE:
    case GLX_ACCUM_ALPHA_SIZE:
    case GLX_BUFFER_SIZE:
    case GLX_LEVEL:
      ptr++;
      break;
    case GLX_USE_GL:
    case GLX_RGBA:
    case GLX_STEREO:
      break;
    default:
      throw ENotImplemented(EXCEPTION_MESSAGE);
    };
    ptr++;
  }
  return ptblattribs;
}

gits::OpenGL::GLXCtxParams gits::OpenGL::GetUpdatedGLXCtxParams(const int* ptblparams) {
  bool forceNone =
      Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED;
  bool forceCompat = Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::COMPAT;
  bool forceCore = Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::CORE;
  bool forceES = Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::ES;
  bool forceVer = !Config::Get().opengl.shared.forceGLVersion.empty();

  GLXCtxParams newparams;

  // Get not overwritten params
  const int* ptr = ptblparams;
  while (*ptr != 0) {
    switch (*ptr) {
    case GLX_CONTEXT_PROFILE_MASK_ARB:
      if (forceNone) {
        newparams.push_back(*ptr);
        ++ptr;
        newparams.push_back(*ptr);
      } else {
        ptr++;
      }
      break;
    case GLX_CONTEXT_MINOR_VERSION_ARB:
    case GLX_CONTEXT_MAJOR_VERSION_ARB:
      if (!forceVer) {
        newparams.push_back(*ptr);
        ++ptr;
        newparams.push_back(*ptr);
      } else {
        ptr++;
      }
      break;
    default:
      ptr++;
    }
    ptr++;
  }

  // Get overwritten params
  if (!forceNone) {
    newparams.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
    if (forceCompat) {
      newparams.push_back(GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
    } else if (forceCore) {
      newparams.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
    } else if (forceES) {
      newparams.push_back(GLX_CONTEXT_ES2_PROFILE_BIT_EXT);
    }
  }

  if (forceVer) {
    newparams.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
    newparams.push_back(Config::Get().opengl.shared.forceGLVersionMinor);
    newparams.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
    newparams.push_back(Config::Get().opengl.shared.forceGLVersionMajor);
  }

  newparams.push_back(0);
  return newparams;
}

gits::OpenGL::PtblCtxParams gits::OpenGL::GLXToPtblCtxParams(const int* glxparams) {
  PtblCtxParams ptblparams;
  auto ptr = glxparams;
  while (*ptr != 0) {
    switch (*ptr) {
    case GLX_CONTEXT_PROFILE_MASK_ARB:
      ptr++;
      if (*ptr & GLX_CONTEXT_CORE_PROFILE_BIT_ARB) {
        ptblparams[Profile] = Core;
      } else if (*ptr & GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB) {
        ptblparams[Profile] = Compat;
      } else if (*ptr & GLX_CONTEXT_ES2_PROFILE_BIT_EXT) {
        ptblparams[Profile] = ES;
      }
      break;
    case GLX_CONTEXT_MAJOR_VERSION_ARB:
      ptr++;
      ptblparams[VerMajor] = *ptr;
      break;
    case GLX_CONTEXT_MINOR_VERSION_ARB:
      ptr++;
      ptblparams[VerMinor] = *ptr;
      break;
    default:
      ptr++;
    }
    ptr++;
  }
  return ptblparams;
}

gits::OpenGL::GLXCtxParams gits::OpenGL::PtblToGLXCtxParams(const PtblCtxParams& ptblparams) {
  // Remove compatibility profile request as it is default and in some cases
  // seems to not work
  auto ptblParamsMod = ptblparams;
  if (ptblParamsMod.find(Profile) != ptblParamsMod.end() && ptblParamsMod[Profile] == Compat) {
    ptblParamsMod.erase(Profile);
    ptblParamsMod.erase(VerMinor);
    ptblParamsMod.erase(VerMajor);
  }

  // Translate params to GLX
  GLXCtxParams glxparams;
  for (auto& param : ptblParamsMod) {
    switch (param.first) {
    case Profile:
      glxparams.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
      if (param.second == Core) {
        glxparams.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
      } else if (param.second == Compat) {
        glxparams.push_back(GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
      } else if (param.second == ES) {
        glxparams.push_back(GLX_CONTEXT_ES2_PROFILE_BIT_EXT);
      } else {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      break;
    case VerMajor:
      glxparams.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
      glxparams.push_back(param.second);
      break;
    case VerMinor:
      glxparams.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
      glxparams.push_back(param.second);
      break;
    }
  }
  glxparams.push_back(0);
  return glxparams;
}

//****************** Portable interface *************************
XVisualInfo* gits::OpenGL::ptbl_glXChooseVisual(Display* dpy, int screen, int* attribList) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXChooseVisual(dpy, screen, attribList);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle ptblPf = PtblHandle::Create(GenFake());
    // XvisualInfo is kept in same map as FBConfig
    PortableState::Instance().pfs[ptblPf].attribs = XVisualToPtblAttribs(attribList);

    return ptblPf.Fake();
  }
}

GLXFBConfig* gits::OpenGL::ptbl_glXChooseFBConfig(Display* dpy,
                                                  int screen,
                                                  const int* attribList,
                                                  int* nitems) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXChooseFBConfig(dpy, screen, attribList, nitems);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    *nitems = 1;
    PtblHandle ptblPf = PtblHandle::Create(GenFake());
    PortableState::Instance().pfs[ptblPf].attribs = GLXToPtblAttribs(attribList);

    static void* persistStore = nullptr;
    persistStore = ptblPf.Fake();
    return (GLXFBConfig*)&persistStore;
  }
}

XVisualInfo* gits::OpenGL::ptbl_glXGetVisualFromFBConfig(Display* dpy, GLXFBConfig config) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXGetVisualFromFBConfig(dpy, config);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    // We keep same attribs for XVisualInfo and FBConfig
    PtblHandle ptblXVis = PtblHandle::Create(GenFake());
    PtblHandle ptblFbConf = PtblHandle::Get(config);
    auto& pstate = PortableState::Instance();
    pstate.pfs[ptblXVis].attribs = pstate.pfs[ptblFbConf].attribs;

    return ptblXVis.Fake();
  }
}

GLXContext gits::OpenGL::ptbl_glXCreateContext(Display* dpy,
                                               XVisualInfo* vis,
                                               GLXContext shareList,
                                               Bool direct) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXCreateContext(dpy, vis, shareList, direct);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle ptblCtx = PtblHandle::Create(GenFake());
    auto& pstate = PortableState::Instance();
    pstate.ctxs[ptblCtx].share = PtblHandle::Get(shareList);
    pstate.ctxs[ptblCtx].pf = PtblHandle::Get(vis);
    return ptblCtx.Fake();
  }
}

GLXContext gits::OpenGL::ptbl_glXCreateNewContext(
    Display* dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXCreateNewContext(dpy, config, renderType, shareList, direct);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle ptblCtx = PtblHandle::Create(GenFake());
    auto& pstate = PortableState::Instance();
    pstate.ctxs[ptblCtx].share = PtblHandle::Get(shareList);
    pstate.ctxs[ptblCtx].pf = PtblHandle::Get(config);
    return ptblCtx.Fake();
  }
}

GLXContext gits::OpenGL::ptbl_glXCreateContextAttribsARB(Display* dpy,
                                                         GLXFBConfig config,
                                                         GLXContext share_context,
                                                         Bool direct,
                                                         const int* attrib_list) {
  // Update cokntext params if configured
  const int* attribsListMod;
  std::vector<int> attribsListModVec;
  if (Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED &&
      Config::Get().opengl.shared.forceGLVersion.empty()) {
    attribsListMod = attrib_list;
  } else {
    attribsListModVec = GetUpdatedGLXCtxParams(attrib_list);
    attribsListMod = &attribsListModVec[0];
  }

  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXCreateContextAttribsARB(dpy, config, share_context, direct, attribsListMod);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle ptblCtx = PtblHandle::Create(GenFake());
    auto& pstate = PortableState::Instance();
    pstate.ctxs[ptblCtx].share = PtblHandle::Get(share_context);
    pstate.ctxs[ptblCtx].pf = PtblHandle::Get(config);
    pstate.ctxs[ptblCtx].params = GLXToPtblCtxParams(attribsListMod);
    return ptblCtx.Fake();
  }
}

void gits::OpenGL::ptbl_glXDestroyContext(Display* dpy, GLXContext ctx) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    drv.glx.glXDestroyContext(dpy, ctx);
    return;
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PortableState::Instance().ctxs.erase(PtblHandle::Get(ctx));
    dispatchDelContext(PtblHandle::Get(ctx));
  }
}

Bool gits::OpenGL::ptbl_glXMakeCurrent(Display* dpy, GLXDrawable drawable, GLXContext ctx) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXMakeCurrent(dpy, drawable, ctx);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    auto ptblSurf = PtblHandle::Get((void*)drawable);
    auto ptblCtx = PtblHandle::Get(ctx);
    pstate.ctxs[ptblCtx].surf = ptblSurf;
    pstate.surfs[ptblSurf].pf = pstate.ctxs[ptblCtx].pf;
    dispatchSetContext(ptblCtx);
    return true;
  }
}

Bool gits::OpenGL::ptbl_glXMakeContextCurrent(Display* dpy,
                                              GLXDrawable draw,
                                              GLXDrawable read,
                                              GLXContext ctx) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXMakeContextCurrent(dpy, draw, read, ctx);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    auto ptblSurf = PtblHandle::Get((void*)draw);
    auto ptblCtx = PtblHandle::Get((void*)ctx);
    pstate.ctxs[ptblCtx].surf = ptblSurf;
    pstate.surfs[ptblSurf].pf = pstate.ctxs[ptblCtx].pf;
    dispatchSetContext(ptblCtx);
    return true;
  }
}

void gits::OpenGL::ptbl_glXSwapBuffers(Display* dpy, GLXDrawable drawable) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    drv.glx.glXSwapBuffers(dpy, drawable);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    auto ptblSurf = PtblHandle::Get((void*)drawable);
    PtblHandle ptblCtx;
    for (auto& ctxdata : pstate.ctxs) {
      if (ctxdata.second.surf == ptblSurf) {
        ptblCtx = ctxdata.first;
      }
    }
    dispatchMakeBufferSwap(ptblCtx);
  }
}

GLXPbuffer gits::OpenGL::ptbl_glXCreatePbuffer(Display* dpy,
                                               GLXFBConfig config,
                                               const int* attribList) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return drv.glx.glXCreatePbuffer(dpy, config, attribList);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    // Create fake
    PtblHandle ptblSurf = PtblHandle::Create(GenFake());

    // Fetch attribs
    auto& pstate = PortableState::Instance();
    const int* ptr = attribList;
    int pbuffWidth = 0;
    int pbuffHeight = 0;
    while (*ptr != 0) {
      if (*ptr == GLX_PBUFFER_WIDTH) {
        ptr++;
        pbuffWidth = *ptr;
      } else if (*ptr == GLX_PBUFFER_HEIGHT) {
        ptr++;
        pbuffHeight = *ptr;
      } else {
        ptr++;
      }
      ptr++;
    }
    pstate.surfs[ptblSurf].height = pbuffHeight;
    pstate.surfs[ptblSurf].width = pbuffWidth;

    return ptblSurf.Fake();
  }
}

void gits::OpenGL::ptbl_XFree(GLXFBConfig* fbconf) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    XFree(fbconf);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
}

int gits::OpenGL::ptbl_DefaultScreen(Display* display) {
  if (PtblNtvApi() == PtblNativeAPI::GLX) {
#ifdef GITS_PLATFORM_X11
    return DefaultScreen(display);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle ptblDefScreen = PtblHandle::Create(GenFake());
    return ptblDefScreen.Fake();
  }
}

//****************** Portable execution *************************
#ifdef GITS_PLATFORM_X11
int xerrorhandler(Display* dsp, XErrorEvent* error) {
  char errorstring[128];
  XGetErrorText(dsp, error->error_code, errorstring, 128);

  Log(ERR) << "X error--" << errorstring << std::endl;
  return 1;
}
#endif

void gits::OpenGL::execSetContextGLX(PtblHandle ctx) {
#ifdef GITS_PLATFORM_X11
  // Initialize
  static bool initialized = false;
  if (!initialized) {
    XSetErrorHandler(xerrorhandler);
    initialized = true;
  }

  if ((void*)ctx.Fake() == nullptr) {
    drv.glx.glXMakeCurrent((Display*)GetNativeDisplay(), 0, nullptr);
    return;
  }

  PortableState& pstate = PortableState::Instance();
  PortableState::CtxData& ctxState = pstate.ctxs[ctx];
  PtblHandle ptblPF = ctxState.pf;
  PtblHandle ptblSurf = ctxState.surf;
  PortableState::PFData& pfState = pstate.pfs[ptblPF];
  // Choose Pixel Format
  if (!ptblPF.HasTrueVal()) {
    int fbConfigCount = 0;
    GLXFBConfig* fbConfigs = drv.glx.glXChooseFBConfig(
        (Display*)GetNativeDisplay(), DefaultScreen((Display*)GetNativeDisplay()),
        &PtblToGLXARBAttribs(pfState.attribs)[0], &fbConfigCount);
    if (fbConfigs == nullptr) {
      Log(ERR) << "glXChooseFBConfig failed - no config matches were found";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    ptblPF.True(*fbConfigs);
  }

  // Create Surface
  if (!ptblSurf.HasTrueVal()) {
    PortableState::SurfData& surfData = pstate.surfs[ptblSurf];
    if (surfData.ispbuff) {
      int attribs[] = {GLX_PBUFFER_WIDTH, surfData.width, GLX_PBUFFER_HEIGHT, surfData.height, 0};
      GLXPbuffer pbuff =
          drv.glx.glXCreatePbuffer((Display*)GetNativeDisplay(), ptblPF.True(), attribs);
      if (pbuff == (GLXPbuffer)0) {
        Log(ERR) << "glXCreatePbuffer failed";
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      ptblSurf.True(pbuff);
    } else {
      XVisualInfo* vi = (XVisualInfo*)drv.glx.glXGetVisualFromFBConfig((Display*)GetNativeDisplay(),
                                                                       ptblPF.True());
      win_ptr_t win = CreateWin((GLXFBConfig)ptblPF.True(), vi, surfData.width, surfData.height,
                                surfData.x, surfData.y, surfData.vis);
      ptblSurf.True(win);
    }
  }

  // Create Context
  if (!ctx.HasTrueVal()) {
    auto glxCtxParams = PtblToGLXCtxParams(ctxState.params);
    GLXContext glxCtx = drv.glx.glXCreateContextAttribsARB(
        (Display*)GetNativeDisplay(), ptblPF.True(), nullptr, true, &glxCtxParams[0]);
    if (glxCtx == nullptr) {
      Log(ERR) << "glXCreateContextAttribsARB failed";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    ctx.True(glxCtx);
  }

  // Make Current
  drv.glx.glXMakeCurrent((Display*)GetNativeDisplay(), ptblSurf.True(), ctx.True());
#endif
}

void gits::OpenGL::execDelContextGLX(PtblHandle ctx) {
#ifdef GITS_PLATFORM_X11
  drv.glx.glXDestroyContext((Display*)GetNativeDisplay(), ctx.True());
#endif
}

void gits::OpenGL::execBufferSwapGLX(PtblHandle ctx) {
#ifdef GITS_PLATFORM_X11
  PortableState pstate = PortableState::Instance();
  drv.glx.glXSwapBuffers((Display*)GetNativeDisplay(), pstate.ctxs[ctx].surf.True());
#endif
}
