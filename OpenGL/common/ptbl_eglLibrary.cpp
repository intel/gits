// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptbl_eglLibrary.cpp
*
*/

#include "ptbl_eglLibrary.h"
#include "openglDrivers.h"
#include "openglEnums.h"
#include "windowing.h"
#include "eglFunctions.h"

gits::OpenGL::EGLPFAttribs gits::OpenGL::PtblToEGLAttribs(const PtblPFAttribs& ptblattribs) {
  EGLPFAttribs eglattribs;
  for (auto& ptblattr : ptblattribs) {
    switch (ptblattr.first) {
    case ptblPFAccelerated:
      if (ptblattr.second == 0) {
        eglattribs.push_back(EGL_CONFIG_CAVEAT);
        eglattribs.push_back(EGL_SLOW_CONFIG);
      }
      break;
    case ptblPFPBuffer:
    case ptblPFDoubleBuffer:
      break;
    case ptblPFRedSize:
      eglattribs.push_back(EGL_RED_SIZE);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFGreenSize:
      eglattribs.push_back(EGL_GREEN_SIZE);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFBlueSize:
      eglattribs.push_back(EGL_BLUE_SIZE);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFAlphaSize:
      eglattribs.push_back(EGL_ALPHA_SIZE);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFDepthSize:
      eglattribs.push_back(EGL_DEPTH_SIZE);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFStencilSize:
      eglattribs.push_back(EGL_STENCIL_SIZE);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFSamples:
      eglattribs.push_back(EGL_SAMPLES);
      eglattribs.push_back(ptblattr.second);
      break;
    case ptblPFSampleBuffers:
      eglattribs.push_back(EGL_SAMPLE_BUFFERS);
      eglattribs.push_back(ptblattr.second);
      break;
    default:
      throw gits::ENotImplemented(EXCEPTION_MESSAGE);
      break;
    };
  }

  eglattribs.push_back(EGL_RENDERABLE_TYPE);
  if (Config::Get().player.forceGLProfile == TForcedGLProfile::COMPAT ||
      Config::Get().player.forceGLProfile == TForcedGLProfile::CORE) {
    eglattribs.push_back(EGL_OPENGL_BIT);
  } else {
    eglattribs.push_back(EGL_OPENGL_ES_BIT);
  }

  eglattribs.push_back(EGL_NONE);
  return eglattribs;
}

gits::OpenGL::PtblPFAttribs gits::OpenGL::EGLToPtblAttribs(const EGLint* eglattribs) {
  //Choose convert direction
  PtblPFAttribs ptblattribs;
  const EGLint* eglPtr = eglattribs;
  if (eglPtr != nullptr) {
    while (*eglPtr != EGL_NONE) {
      switch (*eglPtr) {
      case EGL_CONFIG_CAVEAT:
        eglPtr++;
        if (*eglPtr != EGL_SLOW_CONFIG) {
          ptblattribs[ptblPFAccelerated] = 0;
        } else {
          ptblattribs[ptblPFAccelerated] = 1;
        }
        break;
      case EGL_RED_SIZE:
        eglPtr++;
        ptblattribs[ptblPFRedSize] = *eglPtr;
        break;
      case EGL_GREEN_SIZE:
        eglPtr++;
        ptblattribs[ptblPFGreenSize] = *eglPtr;
        break;
      case EGL_BLUE_SIZE:
        eglPtr++;
        ptblattribs[ptblPFBlueSize] = *eglPtr;
        break;
      case EGL_ALPHA_SIZE:
        eglPtr++;
        ptblattribs[ptblPFAlphaSize] = *eglPtr;
        break;
      case EGL_DEPTH_SIZE:
        eglPtr++;
        ptblattribs[ptblPFDepthSize] = *eglPtr;
        break;
      case EGL_STENCIL_SIZE:
        eglPtr++;
        ptblattribs[ptblPFStencilSize] = *eglPtr;
        break;
      case EGL_SAMPLES:
        eglPtr++;
        ptblattribs[ptblPFSamples] = *eglPtr;
        break;
      case EGL_SAMPLE_BUFFERS:
        eglPtr++;
        ptblattribs[ptblPFSampleBuffers] = *eglPtr;
        break;
      default:
        eglPtr++;
      };
      eglPtr++;
    }
  }
  return ptblattribs;
}

std::vector<int> gits::OpenGL::GetUpdatedEGLCtxParams(const int* params) {
  bool forceNone = Config::Get().player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED;
  bool forceCompat = Config::Get().player.forceGLProfile == TForcedGLProfile::COMPAT;
  bool forceCore = Config::Get().player.forceGLProfile == TForcedGLProfile::CORE;
  bool forceES = Config::Get().player.forceGLProfile == TForcedGLProfile::ES;
  bool forceVer = Config::Get().player.forceGLVersion;
  std::vector<int> eglCtxParams;

  const int* ptr = params;
  while (*ptr != EGL_NONE) {
    switch (*ptr) {
    case EGL_CONTEXT_CLIENT_VERSION:
      if (forceNone && !forceVer) {
        eglCtxParams.push_back(*ptr);
        ++ptr;
        eglCtxParams.push_back(*ptr);
      } else {
        ptr++;
      }
      break;
    default:
      ptr++;
    }
    ptr++;
  }

  if (forceCompat || forceCore) {
    eglCtxParams.push_back(EGL_NONE);
    return eglCtxParams;
  }

  if ((forceES || forceNone) && forceVer) {
    int majorVer = Config::Get().player.forceGLVersionMajor;
    int minorVer = Config::Get().player.forceGLVersionMinor;

    eglCtxParams.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
    eglCtxParams.push_back(majorVer);
    eglCtxParams.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
    eglCtxParams.push_back(minorVer);
  }

  eglCtxParams.push_back(EGL_NONE);
  return eglCtxParams;
}

gits::OpenGL::PtblCtxParams gits::OpenGL::EGLToPtblCtxParams(const EGLint* eglparams) {
  bool forceCompat = Config::Get().player.forceGLProfile == TForcedGLProfile::COMPAT;
  bool forceCore = Config::Get().player.forceGLProfile == TForcedGLProfile::CORE;
  bool forceES = Config::Get().player.forceGLProfile == TForcedGLProfile::ES;
  bool forceVer = Config::Get().player.forceGLVersion;

  PtblCtxParams ptblParams;
  const EGLint* eglParamPtr = eglparams;
  if (eglParamPtr != nullptr) {
    while (*eglParamPtr != EGL_NONE) {
      switch (*eglParamPtr) {
      case EGL_CONTEXT_CLIENT_VERSION:
        eglParamPtr++;
        ptblParams[Profile] = ES;
        ptblParams[VerMajor] = *eglParamPtr;
        ptblParams[VerMinor] = 0;
        break;
      default:
        eglParamPtr++;
      }
      eglParamPtr++;
    }
  }

  //Get global API settings
  if (PortableState::Instance().helper.currApiEgl == EGL_OPENGL_ES_API) {
    ptblParams[Profile] = ES;
  } else if (PortableState::Instance().helper.currApiEgl == EGL_OPENGL_API) {
    ptblParams[Profile] = Compat;
  }

  //Update params with forced values
  if (forceCompat) {
    ptblParams[Profile] = Compat;
  } else if (forceCore) {
    ptblParams[Profile] = Core;
  } else if (forceES) {
    ptblParams[Profile] = ES;
  }
  if (forceVer) {
    ptblParams[VerMajor] = Config::Get().player.forceGLVersionMajor;
    ptblParams[VerMinor] = Config::Get().player.forceGLVersionMinor;
  }
  return ptblParams;
}

gits::OpenGL::EGLCtxAttribs gits::OpenGL::PtblToEGLCtxParams(const PtblCtxParams& ptblparams) {
  EGLCtxAttribs eglparams;
  if (ptblparams.find(VerMajor) != ptblparams.end()) {
    eglparams.push_back(EGL_CONTEXT_CLIENT_VERSION);
    eglparams.push_back(ptblparams.at(VerMajor));
  }
  eglparams.push_back(EGL_NONE);
  return eglparams;
}

EGLSurface gits::OpenGL::ptbl_eglCreateWindowSurface(EGLDisplay dpy,
                                                     EGLConfig config,
                                                     EGLNativeWindowType win,
                                                     const EGLint* attrib_list) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglCreateWindowSurface(dpy, config, win, attrib_list);
  } else {
    PtblHandle ptblEglSurf = PtblHandle::Create(GenFake());
    PtblHandle ptblWin = PtblHandle::Get(win);
    auto& pstate = PortableState::Instance();
    pstate.helper.mapEglSurfWin[ptblEglSurf] = ptblWin;

    //Map buffer type to egl surface (will update pixel format params in buffer swap)
    if (attrib_list != nullptr) {
      auto ptr = attrib_list;
      while (*ptr != EGL_NONE) {
        if (*ptr == EGL_RENDER_BUFFER) {
          ptr++;
          pstate.helper.mapEglSurfToBuffType[ptblEglSurf] = *ptr;
          ptr++;
        } else {
          ptr++;
          ptr++;
        }
      }
    }

    return ptblEglSurf.Fake();
  }
}

EGLContext gits::OpenGL::ptbl_eglCreateContext(EGLDisplay dpy,
                                               EGLConfig config,
                                               EGLContext share_context,
                                               const EGLint* attrib_list) {
  //Update context params if configured
  const int* attribsListMod;
  std::vector<int> attribsListModVec;
  if (Config::Get().player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED &&
      Config::Get().player.forceGLVersion == false) {
    attribsListMod = attrib_list;
  } else {
    attribsListModVec = GetUpdatedEGLCtxParams(const_cast<int*>(attrib_list));
    attribsListMod = &attribsListModVec[0];
  }

  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglCreateContext(dpy, config, share_context, attribsListMod);
  } else {
    PtblHandle ptblCtx = PtblHandle::Create(GenFake());
    PtblHandle ptblSharedCtx = PtblHandle::Get(share_context);
    PtblHandle ptblConfig = PtblHandle::Get(config);
    PortableState::Instance().ctxs[ptblCtx].params = EGLToPtblCtxParams(attrib_list);
    PortableState::Instance().ctxs[ptblCtx].pf = ptblConfig;
    PortableState::Instance().ctxs[ptblCtx].share = ptblSharedCtx;
    return ptblCtx.Fake();
  }
}

EGLBoolean gits::OpenGL::ptbl_eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglDestroyContext(dpy, ctx);
  } else {
    auto ptblCtx = PtblHandle::Get(ctx);
    auto& pstate = PortableState::Instance();
    pstate.ctxs.erase(ptblCtx);
    if (ptblCtx.HasTrueVal()) {
      dispatchDelContext(ptblCtx);
    }
    return 1;
  }
}

EGLBoolean gits::OpenGL::ptbl_eglMakeCurrent(EGLDisplay dpy,
                                             EGLSurface draw,
                                             EGLSurface read,
                                             EGLContext ctx) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglMakeCurrent(dpy, draw, read, ctx);
  } else {
    auto& pstate = PortableState::Instance();
    PtblHandle ptblCtx = PtblHandle::Get(ctx);
    PtblHandle ptblEglSurf = PtblHandle::Get(draw);
    PtblHandle ptblSurf = pstate.helper.mapEglSurfWin[ptblEglSurf];

    pstate.ctxs[ptblCtx].surf = ptblSurf;
    pstate.surfs[ptblSurf].pf = pstate.ctxs[ptblCtx].pf;

    //Update buffer swap info in pixel format
    if (pstate.helper.mapEglSurfToBuffType[ptblEglSurf] == EGL_SINGLE_BUFFER) {
      pstate.pfs[pstate.ctxs[ptblCtx].pf].attribs[ptblPFDoubleBuffer] = 0;
    } else {
      pstate.pfs[pstate.ctxs[ptblCtx].pf].attribs[ptblPFDoubleBuffer] = 1;
    }

    dispatchSetContext(ptblCtx);
    return 1;
  }
}

EGLBoolean gits::OpenGL::ptbl_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglSwapBuffers(dpy, surface);
  } else {
    auto& pstate = PortableState::Instance();
    PtblHandle ptblEglSurf = PtblHandle::Get(surface);
    PtblHandle ptblSurf = pstate.helper.mapEglSurfWin.at(ptblEglSurf);
    PtblHandle ctx;
    for (auto& ctxdata : pstate.ctxs) {
      if (ctxdata.second.surf == ptblSurf) {
        ctx = ctxdata.first;
      }
    }

    dispatchMakeBufferSwap(ctx);
    return 1;
  }
}

EGLSurface gits::OpenGL::ptbl_eglCreatePbufferSurface(EGLDisplay dpy,
                                                      EGLConfig config,
                                                      const EGLint* attrib_list) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglCreatePbufferSurface(dpy, config, attrib_list);
  } else {
    PtblHandle ptblPbuffer = PtblHandle::Create(GenFake());

    //Read attribs
    EGLint pbufferWidth = 0;
    EGLint pbufferHeight = 0;
    auto attribPtr = attrib_list;
    while (*attribPtr != 0) {
      switch (*attribPtr) {
      case EGL_HEIGHT:
        attribPtr++;
        pbufferHeight = *attribPtr;
        break;
      case EGL_WIDTH:
        attribPtr++;
        pbufferWidth = *attribPtr;
        break;
      default:
        attribPtr++;
        break;
      }
      attribPtr++;
    }

    auto& pstate = PortableState::Instance();
    pstate.surfs[ptblPbuffer].ispbuff = true;
    pstate.surfs[ptblPbuffer].x = 0;
    pstate.surfs[ptblPbuffer].y = 0;
    pstate.surfs[ptblPbuffer].width = pbufferWidth;
    pstate.surfs[ptblPbuffer].height = pbufferHeight;

    //In case of PBuffer Win handle is same as EGLSurface
    pstate.helper.mapEglSurfWin[ptblPbuffer] = ptblPbuffer;

    return ptblPbuffer.Fake();
  }
}

EGLBoolean gits::OpenGL::ptbl_eglChooseConfig(EGLDisplay dpy,
                                              const EGLint* attrib_list,
                                              EGLConfig* configs,
                                              EGLint config_size,
                                              EGLint* num_config) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
  } else {
    PtblHandle ptblConfig = PtblHandle::Create(GenFake());
    *configs = ptblConfig.Fake();
    *num_config = 1;

    PortableState::PFData& pfdata = PortableState::Instance().pfs[ptblConfig];
    pfdata.attribs = EGLToPtblAttribs(attrib_list);
    return 1;
  }
}

EGLBoolean gits::OpenGL::ptbl_eglBindAPI(EGLenum api) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglBindAPI(api);
  } else {
    PortableState::Instance().helper.currApiEgl = api;
    return 1;
  }
}

EGLenum gits::OpenGL::ptbl_eglQueryAPI() {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return drv.egl.eglQueryAPI();
  } else {
    return PortableState::Instance().helper.currApiEgl;
  }
}

EGLConfig gits::OpenGL::ptblFindConfigEGL(EGLDisplay dpy, const std::vector<EGLint>& attribs) {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return FindConfigEGL(dpy, attribs);
  } else {
    PtblHandle ptblConfig = PtblHandle::Create(GenFake());
    PortableState::PFData& pfdata = PortableState::Instance().pfs[ptblConfig];

    //Add terminator
    std::vector<EGLint> nonConstAttribs = attribs;
    nonConstAttribs.push_back(EGL_NONE);

    pfdata.attribs = EGLToPtblAttribs(&nonConstAttribs[0]);
    return ptblConfig.Fake();
  }
}

EGLDisplay gits::OpenGL::ptbl_GetEGLDisplay() {
  if (PtblNtvApi() == PtblNativeAPI::EGL) {
    return GetEGLDisplay();
  } else {
    return nullptr;
  }
}

void gits::OpenGL::execSetContextEGL(PtblHandle ctx) {
  auto& pstate = PortableState::Instance();
  if ((void*)ctx.Fake() == nullptr) {
    drv.egl.eglMakeCurrent(GetEGLDisplay(), nullptr, nullptr, nullptr);
    return;
  }

  PtblHandle ptblPf = pstate.ctxs[ctx].pf;
  PtblHandle ptblSurf = pstate.ctxs[ctx].surf;

  //Initialize
  static bool eglReady = false;
  if (!eglReady) {
    EGLint glType = EGL_OPENGL_ES_API;
    auto forcedProfile = Config::Get().player.forceGLProfile;
    if (forcedProfile == TForcedGLProfile::COMPAT || forcedProfile == TForcedGLProfile::CORE) {
      glType = EGL_OPENGL_API;
    }

    drv.egl.eglInitialize(GetEGLDisplay(), nullptr, nullptr);
    drv.egl.eglBindAPI(glType);
    eglReady = true;
  }

  //Choose Pixel Format
  if (!ptblPf.HasTrueVal()) {
    EGLConfig pf;
    EGLint num;
    drv.egl.eglGetError();
    EGLBoolean result = drv.egl.eglChooseConfig(
        GetEGLDisplay(), &PtblToEGLAttribs(pstate.pfs[ptblPf].attribs)[0], &pf, 1, &num);
    if (result == 0) {
      Log(ERR) << "eglChooseConfig failed. EGL Error: " << drv.egl.eglGetError();
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    ptblPf.True(pf);
  }

  //Create Context
  if (!ctx.HasTrueVal()) {
    EGLContext newctx =
        drv.egl.eglCreateContext(GetEGLDisplay(), ptblPf.True(), pstate.ctxs[ctx].share.True(),
                                 &PtblToEGLCtxParams(pstate.ctxs[ctx].params)[0]);
    ctx.True(newctx);
    EGLint err = drv.egl.eglGetError();
    if (err != EGL_SUCCESS) {
      Log(ERR) << "eglCreateContext failed. EGL Error: " << err;
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }

  //Create Window
  auto& surfData = pstate.surfs[ptblSurf];
  if (!ptblSurf.HasTrueVal() && !surfData.ispbuff) {
    win_ptr_t win = CreateWin(surfData.pf.True(), surfData.width, surfData.height, surfData.x,
                              surfData.y, surfData.vis);
    ptblSurf.True(win);
  }

  //Create fake EGLSurface
  PtblHandle ptblEglSurf;
  for (auto& elem : pstate.helper.mapEglSurfWin) {
    if (elem.second == ptblSurf) {
      ptblEglSurf = elem.first;
    }
  }
  if ((int)ptblEglSurf.Fake() == 0) { //EGLSurface not found in map
    ptblEglSurf = PtblHandle::Create(GenFake());
    pstate.helper.mapEglSurfWin[ptblEglSurf] = ptblSurf;
  }

  //Create EGLSurface
  if (!ptblEglSurf.HasTrueVal()) {
    if (surfData.ispbuff) {
      //Create PBuffer EGL Surface
      std::vector<EGLint> attribs;
      attribs.push_back(EGL_WIDTH);
      attribs.push_back(surfData.width);
      attribs.push_back(EGL_HEIGHT);
      attribs.push_back(surfData.height);
      attribs.push_back(EGL_NONE);

      drv.egl.eglGetError();
      EGLSurface surf =
          drv.egl.eglCreatePbufferSurface(GetEGLDisplay(), ptblPf.True(), &attribs[0]);
      EGLint err = drv.egl.eglGetError();
      if (err != EGL_SUCCESS) {
        Log(ERR) << "eglCreatePbufferSurface failed. EGL Error: " << err;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      ptblEglSurf.True(surf);
    } else {
      //Create Window EGL Surface
      drv.egl.eglGetError();
      EGLSurface surf =
          drv.egl.eglCreateWindowSurface(GetEGLDisplay(), ptblPf.True(), ptblSurf.True(), nullptr);
      EGLint err = drv.egl.eglGetError();
      if (err != EGL_SUCCESS) {
        Log(ERR) << "eglCreateWindowSurface failed. EGL Error: " << err;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      ptblEglSurf.True(surf);
    }
  }

  drv.egl.eglMakeCurrent(GetEGLDisplay(), ptblEglSurf.True(), ptblEglSurf.True(), ctx.True());
}

void gits::OpenGL::execDelContextEGL(PtblHandle ctx) {
  drv.egl.eglDestroyContext(GetEGLDisplay(), ctx.True());
}

void gits::OpenGL::execBufferSwapEGL(PtblHandle ctx) {
  auto& pstate = PortableState::Instance();
  PtblHandle ptblSurf = pstate.ctxs[ctx].surf;
  PtblHandle ptblEglSurf;
  for (auto& elem : pstate.helper.mapEglSurfWin) {
    if (elem.second == ptblSurf) {
      ptblEglSurf = elem.first;
    }
  }
  if ((int)ptblEglSurf.Fake() == 0) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  drv.egl.eglSwapBuffers(GetEGLDisplay(), ptblEglSurf.True());
}
