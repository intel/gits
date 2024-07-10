// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptbl_wglLibrary.cpp
*
*/

#include "openglDrivers.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#include "tools_windows.h"
#endif

#include "ptbl_wglLibrary.h"
#include "ptblLibrary.h"
#include "openglTypes.h"
#include "openglEnums.h"

//******************************* Helper Functions *********************************
void gits::OpenGL::PtblToWGLPFDAttribs(PIXELFORMATDESCRIPTOR* pfd,
                                       const PtblPFAttribs& ptblattribs) {
  //Choose convert direction
  for (auto& ptblattr : ptblattribs) {
    switch (ptblattr.first) {
    case ptblPFDoubleBuffer:
      if (ptblattr.second > 0) {
        pfd->dwFlags = (BYTE)pfd->dwFlags | PFD_DOUBLEBUFFER;
      }
      break;
    case ptblPFAccelerated:
      if (ptblattr.second > 0) {
        pfd->dwFlags = (BYTE)pfd->dwFlags | PFD_GENERIC_ACCELERATED;
      }
      break;
    case ptblPFRedSize:
      pfd->cRedBits = (BYTE)ptblattr.second;
      break;
    case ptblPFGreenSize:
      pfd->cGreenBits = (BYTE)ptblattr.second;
      break;
    case ptblPFBlueSize:
      pfd->cBlueBits = (BYTE)ptblattr.second;
      break;
    case ptblPFAlphaSize:
      pfd->cAlphaBits = (BYTE)ptblattr.second;
      break;
    case ptblPFDepthSize:
      pfd->cDepthBits = (BYTE)ptblattr.second;
      break;
    case ptblPFStencilSize:
      pfd->cStencilBits = (BYTE)ptblattr.second;
      break;
    default:
      throw gits::ENotImplemented(EXCEPTION_MESSAGE);
    };
  }
};

void gits::OpenGL::WGLPFDToPtblAttribs(const PIXELFORMATDESCRIPTOR* pfd,
                                       PtblPFAttribs& ptblattribs) {
  ptblattribs.clear();
  //Choose convert direction
  if (pfd->dwFlags & PFD_DOUBLEBUFFER) {
    ptblattribs[ptblPFDoubleBuffer] = 1;
  }
  if (pfd->dwFlags & PFD_GENERIC_ACCELERATED) {
    ptblattribs[ptblPFAccelerated] = 1;
  }
  ptblattribs[ptblPFRedSize] = pfd->cRedBits;
  ptblattribs[ptblPFGreenSize] = pfd->cGreenBits;
  ptblattribs[ptblPFBlueSize] = pfd->cBlueBits;
  ptblattribs[ptblPFDepthSize] = pfd->cDepthBits;
  ptblattribs[ptblPFStencilSize] = pfd->cStencilBits;
}

gits::OpenGL::WGLARBPFAttribs gits::OpenGL::PtblToWGLARBAttribs(const PtblPFAttribs& ptblattribs) {
  WGLARBPFAttribs wglattribs;
  for (auto& ptblattr : ptblattribs) {
    switch (ptblattr.first) {
    case ptblPFDoubleBuffer:
      if (ptblattr.second > 0) {
        wglattribs.push_back(WGL_DOUBLE_BUFFER_ARB);
        wglattribs.push_back(1);
      }
      break;
    case ptblPFAccelerated:
      if (ptblattr.second > 0) {
        wglattribs.push_back(WGL_ACCELERATION_ARB);
        wglattribs.push_back(WGL_FULL_ACCELERATION_ARB);
      }
      break;
    case ptblPFPBuffer:
      if (ptblattr.second > 0) {
        wglattribs.push_back(WGL_DRAW_TO_PBUFFER_ARB);
        wglattribs.push_back(1);
      }
      break;
    case ptblPFRedSize:
      wglattribs.push_back(WGL_RED_BITS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFGreenSize:
      wglattribs.push_back(WGL_GREEN_BITS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFBlueSize:
      wglattribs.push_back(WGL_BLUE_BITS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFAlphaSize:
      wglattribs.push_back(WGL_ALPHA_BITS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFDepthSize:
      wglattribs.push_back(WGL_DEPTH_BITS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFStencilSize:
      wglattribs.push_back(WGL_STENCIL_BITS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFSamples:
      wglattribs.push_back(WGL_SAMPLES_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    case ptblPFSampleBuffers:
      wglattribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
      wglattribs.push_back(ptblattr.second);
      break;
    default:
      throw gits::ENotImplemented(EXCEPTION_MESSAGE);
      break;
    };
  }
  wglattribs.push_back(0);
  return wglattribs;
};

gits::OpenGL::PtblPFAttribs gits::OpenGL::WGLARBToPtblAttribs(const int* wglattribs) {
  PtblPFAttribs ptblattribs;
  auto ptr = wglattribs;
  while (*ptr != 0) {
    switch (*ptr) {
    case WGL_DOUBLE_BUFFER_ARB:
      ptr++;
      ptblattribs[ptblPFDoubleBuffer] = *ptr;
      break;
    case WGL_ACCELERATION_ARB:
      ptr++;
      ptblattribs[ptblPFAccelerated] = *ptr;
      break;
    case WGL_DRAW_TO_PBUFFER_ARB:
      ptr++;
      ptblattribs[ptblPFPBuffer] = *ptr;
      break;
    case WGL_RED_BITS_ARB:
      ptr++;
      ptblattribs[ptblPFRedSize] = *ptr;
      break;
    case WGL_GREEN_BITS_ARB:
      ptr++;
      ptblattribs[ptblPFGreenSize] = *ptr;
      break;
    case WGL_BLUE_BITS_ARB:
      ptr++;
      ptblattribs[ptblPFBlueSize] = *ptr;
      break;
    case WGL_ALPHA_BITS_ARB:
      ptr++;
      ptblattribs[ptblPFAlphaSize] = *ptr;
      break;
    case WGL_DEPTH_BITS_ARB:
      ptr++;
      ptblattribs[ptblPFDepthSize] = *ptr;
      break;
    case WGL_STENCIL_BITS_ARB:
      ptr++;
      ptblattribs[ptblPFStencilSize] = *ptr;
      break;
    case WGL_SAMPLES_ARB:
      ptr++;
      ptblattribs[ptblPFSamples] = *ptr;
      break;
    case WGL_SAMPLE_BUFFERS_ARB:
      ptr++;
      ptblattribs[ptblPFSampleBuffers] = *ptr;
      break;
    default:
      ptr++;
    };
    ptr++;
  }
  return ptblattribs;
};

gits::OpenGL::WGLCtxParams gits::OpenGL::GetUpdatedWGLCtxParams(const int* params) {
  bool forceNone =
      Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED;
  bool forceCompat = Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::COMPAT;
  bool forceCore = Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::CORE;
  bool forceES = Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::ES;
  bool forceVer = !Config::Get().opengl.shared.forceGLVersion.empty();

  WGLCtxParams newparams;
  WGLCtxParams noparams = {0}; // Used if params pointer is null.

  // Get not overwritten parameters.
  const int* ptr = params;
  if (params == nullptr) {
    // Nullptr means no parameters, which is the equivalent of a
    // null-terminated but otherwise empty vector. Thus, we create such a
    // vector and work on it instead of on params pointer.
    ptr = noparams.data();
  }
  while (*ptr != 0) {
    switch (*ptr) {
    case WGL_CONTEXT_PROFILE_MASK_ARB:
      if (forceNone) {
        newparams.push_back(*ptr);
        ++ptr;
        newparams.push_back(*ptr);
      } else {
        ptr++;
      }
      break;
    case WGL_CONTEXT_MINOR_VERSION_ARB:
    case WGL_CONTEXT_MAJOR_VERSION_ARB:
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

  // Get overwritten parameters.
  if (!forceNone) {
    newparams.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
    if (forceCompat) {
      newparams.push_back(WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
    } else if (forceCore) {
      newparams.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
    } else if (forceES) {
      newparams.push_back(WGL_CONTEXT_ES2_PROFILE_BIT_EXT);
    }
  }

  if (forceVer) {
    newparams.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
    newparams.push_back(Config::Get().opengl.shared.forceGLVersionMinor);
    newparams.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
    newparams.push_back(Config::Get().opengl.shared.forceGLVersionMajor);
  }

  newparams.push_back(0);
  return newparams;
}

gits::OpenGL::PtblCtxParams gits::OpenGL::WGLToPtblCtxParams(const int* wglparams) {
  PtblCtxParams ptblparams;
  auto ptr = wglparams;
  if (!ptr) {
    return ptblparams;
  }
  while (*ptr != 0) {
    switch (*ptr) {
    case WGL_CONTEXT_PROFILE_MASK_ARB:
      ptr++;
      if (*ptr & WGL_CONTEXT_CORE_PROFILE_BIT_ARB) {
        ptblparams[Profile] = Core;
      } else if (*ptr & WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB) {
        ptblparams[Profile] = Compat;
      } else if (*ptr & WGL_CONTEXT_ES2_PROFILE_BIT_EXT) {
        ptblparams[Profile] = ES;
      }
      break;
    case WGL_CONTEXT_MAJOR_VERSION_ARB:
      ptr++;
      ptblparams[VerMajor] = *ptr;
      break;
    case WGL_CONTEXT_MINOR_VERSION_ARB:
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

gits::OpenGL::WGLCtxParams gits::OpenGL::PtblToWGLCtxParams(const PtblCtxParams& ptblparams) {
  //Remove compatibility profile request as it is default and in some cases seems to not work
  auto ptblParamsMod = ptblparams;
  if (ptblParamsMod.find(Profile) != ptblParamsMod.end() && ptblParamsMod[Profile] == Compat) {
    ptblParamsMod.erase(Profile);
    ptblParamsMod.erase(VerMinor);
    ptblParamsMod.erase(VerMajor);
  }

  //Translate params to WGL
  WGLCtxParams wglparams;
  for (auto& param : ptblParamsMod) {
    switch (param.first) {
    case Profile:
      wglparams.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
      if (param.second == Core) {
        wglparams.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
      } else if (param.second == Compat) {
        wglparams.push_back(WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
      } else if (param.second == ES) {
        wglparams.push_back(WGL_CONTEXT_ES2_PROFILE_BIT_EXT);
      } else {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      break;
    case VerMajor:
      wglparams.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
      wglparams.push_back(param.second);
      break;
    case VerMinor:
      wglparams.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
      wglparams.push_back(param.second);
      break;
    }
  }
  wglparams.push_back(0);
  return wglparams;
}

namespace gits {
namespace OpenGL {
// Calls drv.wgl.wglMakeCurrent and logs a warning if it did not succeed.
BOOL wglMakeCurrent_checked(HDC hdc, HGLRC hglrc) {
#ifdef GITS_PLATFORM_WINDOWS
  BOOL retval;
  SetLastError(NO_ERROR);

  retval = gits::OpenGL::drv.wgl.wglMakeCurrent(hdc, hglrc);

  DWORD err = GetLastError();
  if (err != 0 || retval != TRUE) {
    Log(WARN) << "wglMakeCurrent failed. Error: " << Win32ErrorToString(err);
  }
  return retval;
#else
  throw ENotImplemented(std::string(EXCEPTION_MESSAGE) +
                        "wglMakeCurrent should only be called on Windows.");
#endif
}
} // namespace OpenGL
} // namespace gits

//****************** Portable interface *************************
int gits::OpenGL::ptbl_wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglChoosePixelFormat(hdc, ppfd);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle newpix = PtblHandle::Create(GenFake());
    PortableState::PFData& pfdata = PortableState::Instance().pfs[newpix];
    WGLPFDToPtblAttribs(ppfd, pfdata.attribs);
    return (int)newpix.Fake();
  }
}

BOOL gits::OpenGL::ptbl_wglChoosePixelFormatARB(HDC hdc,
                                                const int* piAttribIList,
                                                const FLOAT* pfAttribFList,
                                                UINT nMaxFormats,
                                                int* piFormats,
                                                UINT* nNumFormats) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglChoosePixelFormatARB(hdc, piAttribIList, pfAttribFList, nMaxFormats,
                                           piFormats, nNumFormats);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    *nNumFormats = 1;
    PtblHandle newpix = PtblHandle::Create(GenFake());
    *piFormats = (int)newpix.Fake();
    PortableState::PFData& pfdata = PortableState::Instance().pfs[newpix];
    pfdata.attribs = WGLARBToPtblAttribs(piAttribIList);
    return true;
  }
}

BOOL gits::OpenGL::ptbl_wglSetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* pfd) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return SetPixelFormat(hdc, format, pfd);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    auto& hwnd = pstate.helper.mapHdcSurface[PtblHandle::Get(hdc)];
    pstate.surfs[hwnd].pf = PtblHandle::Get((uint64_t)format);
    return true;
  }
}

HGLRC gits::OpenGL::ptbl_wglCreateContext(HDC hdc) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglCreateContext(hdc);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle newctx = PtblHandle::Create(GenFake());
    PortableState::Instance().ctxs[newctx] = PortableState::CtxData();
    return newctx.Fake();
  }
}

HGLRC gits::OpenGL::ptbl_wglCreateContextAttribsARB(HDC hDC,
                                                    HGLRC hShareContext,
                                                    const int* attribList) {
  //Update context params if configured
  const int* attribsListMod;
  std::vector<int> attribsListModVec;
  if (Config::Get().opengl.player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED &&
      Config::Get().opengl.shared.forceGLVersion.empty()) {
    attribsListMod = attribList;
  } else {
    attribsListModVec = GetUpdatedWGLCtxParams(attribList);
    attribsListMod = &attribsListModVec[0];
  }

  //Execute API
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglCreateContextAttribsARB(hDC, hShareContext, attribsListMod);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle newctx = PtblHandle::Create(GenFake());
    auto& pstate = PortableState::Instance();
    pstate.ctxs[newctx].share = PtblHandle::Get(hShareContext);

    pstate.ctxs[newctx].params = WGLToPtblCtxParams(attribsListMod);
    return newctx.Fake();
  }
}

BOOL gits::OpenGL::ptbl_wglMakeCurrent(HDC hdc, HGLRC hglrc) {

  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return wglMakeCurrent_checked(hdc, hglrc);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    auto& hwnd = pstate.helper.mapHdcSurface[PtblHandle::Get(hdc)];
    pstate.ctxs[PtblHandle::Get(hglrc)].surf = hwnd;
    pstate.ctxs[PtblHandle::Get(hglrc)].pf = pstate.surfs[hwnd].pf;
    pstate.helper.currCtx = PtblHandle::Get(hglrc);
    dispatchSetContext(PtblHandle::Get(hglrc));
    return true;
  }
}

BOOL gits::OpenGL::ptbl_wglSwapBuffers(HDC hdc) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglSwapBuffers(hdc);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    auto& hwnd = pstate.helper.mapHdcSurface[PtblHandle::Get(hdc)];
    PtblHandle ctx;
    for (auto& ctxdata : pstate.ctxs) {
      if (ctxdata.second.surf == hwnd) {
        ctx = ctxdata.first;
      }
    }
    dispatchMakeBufferSwap(ctx);
    return true;
  }
}

BOOL gits::OpenGL::ptbl_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglShareLists(hglrc1, hglrc2);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

HPBUFFERARB gits::OpenGL::ptbl_wglCreatePbufferARB(
    HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglCreatePbufferARB(hDC, iPixelFormat, iWidth, iHeight, piAttribList);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
  PtblHandle newpbuffer = PtblHandle::Create(GenFake());

  auto& pstate = PortableState::Instance();
  pstate.surfs[newpbuffer].ispbuff = true;
  pstate.surfs[newpbuffer].width = iWidth;
  pstate.surfs[newpbuffer].height = iHeight;
  pstate.surfs[newpbuffer].x = 0;
  pstate.surfs[newpbuffer].y = 0;
  pstate.surfs[newpbuffer].pf = PtblHandle::Get((uint64_t)iPixelFormat);

  return newpbuffer.Fake();
}
HDC gits::OpenGL::ptbl_wglGetPbufferDCARB(HPBUFFERARB hPbuffer) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglGetPbufferDCARB(hPbuffer);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
  PtblHandle ptblHdc = PtblHandle::Create(GenFake());
  PortableState::Instance().helper.mapHdcSurface[ptblHdc] = PtblHandle::Get(hPbuffer);
  return ptblHdc.Fake();
}

BOOL gits::OpenGL::ptbl_wglDeleteContext(HGLRC hglrc) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglDeleteContext(hglrc);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
  auto ptblHglrc = PtblHandle::Get(hglrc);
  auto& pstate = PortableState::Instance();
  pstate.ctxs.erase(ptblHglrc);
  if (ptblHglrc.HasTrueVal()) {
    dispatchDelContext(ptblHglrc);
  }
  return 1;
}

HGLRC gits::OpenGL::ptbl_wglGetCurrentContext() {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return drv.wgl.wglGetCurrentContext();
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
  return PortableState::Instance().helper.currCtx.Fake();
}

HDC gits::OpenGL::ptblGetDC(HWND hwnd) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return GetDC(hwnd);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    PtblHandle hdc = PtblHandle::Create(GenFake());
    PortableState::Instance().helper.mapHdcSurface[hdc] = PtblHandle::Get(hwnd);
    return hdc.Fake();
  }
}

HWND gits::OpenGL::ptblWindowFromDC(HDC dc) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return WindowFromDC(dc);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& pstate = PortableState::Instance();
    if (pstate.helper.mapHdcSurface.find(PtblHandle::Get(dc)) ==
        pstate.helper.mapHdcSurface.end()) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    return pstate.helper.mapHdcSurface[PtblHandle::Get(dc)].Fake();
  }
}

int gits::OpenGL::ptblReleaseDC(HWND hwnd, HDC hdc) {
  if (PtblNtvApi() == PtblNativeAPI::WGL) {
#ifdef GITS_PLATFORM_WINDOWS
    return ReleaseDC(hwnd, hdc);
#else
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  } else {
    auto& surfs = PortableState::Instance().surfs;
    auto& mapHdcHwnd = PortableState::Instance().helper.mapHdcSurface;
    if (surfs.find(PtblHandle::Get(hdc)) == surfs.end()) {
      return 0;
    } else {
      mapHdcHwnd.erase(PtblHandle::Get(hdc));
      surfs.erase(PtblHandle::Get(hdc));
      return 1;
    }
  }
}

//****************** Portable execution *************************
#ifdef GITS_PLATFORM_WINDOWS
namespace gits {
namespace OpenGL {
void InitWinDrv() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  static win_ptr_t win = CreateWin(10, 10, 0, 0, 0);
  PIXELFORMATDESCRIPTOR tmppfd = {sizeof(PIXELFORMATDESCRIPTOR)};
  SetPixelFormat(GetDC(win), ChoosePixelFormat(GetDC(win), &tmppfd), &tmppfd);
  HGLRC tmphglrc = drv.wgl.wglCreateContext(GetDC(win));
  if (!drv.wgl.wglMakeCurrent(GetDC(win), tmphglrc)) {
    Log(ERR) << "Can't set temporary, legacy context";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  initialized = true;
}

void helperChoosePFWGL(PtblHandle hdc, PtblHandle pformat) {
  if (pformat.HasTrueVal()) {
    return;
  }
  auto& pstate = PortableState::Instance();
  int pf = 0;
  unsigned num = 0;
  WGLARBPFAttribs pfattribs;
  pfattribs = PtblToWGLARBAttribs(pstate.pfs[pformat].attribs);
  SetLastError(NO_ERROR);
  drv.wgl.wglChoosePixelFormatARB(hdc.True(), &pfattribs[0], NULL, 1, &pf, &num);
  DWORD err = GetLastError();
  if (num == 0 || pf == 0) {
    Log(ERR) << "No pixel format chosen for passed attribs. Error: " << Win32ErrorToString(err);
    Log(ERR) << "Consider using the --minimalConfig option. See the user guide for more info.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  pformat.True((void*)pf);
}

void helperSetPFWGL(PtblHandle hdc, PtblHandle pformat) {
  if (GetPixelFormat(hdc.True()) != 0) {
    return;
  }
  //Set pixel format
  PIXELFORMATDESCRIPTOR ppfd;
  SetLastError(NO_ERROR);
  SetPixelFormat(hdc.True(), pformat.True(), &ppfd);
  DWORD err = GetLastError();
  if (err != 0) {
    Log(ERR) << "SetPixelFormat failed for pf=" << (int)pformat.True()
             << ". Error: " << Win32ErrorToString(err);
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
}

void helperPrepareSurfaceWGL(PtblHandle surf, PtblHandle ptblhdc) {
  InitWinDrv();
  auto& pstate = PortableState::Instance();
  PtblHandle ptblPFormat = pstate.surfs[surf].pf;

  //Create surface
  if (!surf.HasTrueVal()) {
    PortableState::SurfData& surfdata = pstate.surfs[surf];

    //PBuffer case
    if (surfdata.ispbuff) {
      execHelperCreatePBufferWGL(surf, ptblPFormat);
    }
    //Window case
    else {
      //Create Win
      win_ptr_t win =
          CreateWin(surfdata.width, surfdata.height, surfdata.x, surfdata.y, surfdata.vis);
      if (win == 0) {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      surf.True(win);

      //Get DC
      ptblhdc.True(GetDC(win));
    }
  }

  //Create HDC
  if (!ptblhdc.HasTrueVal()) {
    ptblhdc.True(GetDC(surf.True()));
  }

  //Create pixel format
  helperChoosePFWGL(ptblhdc, ptblPFormat);

  //Set pixel format
  helperSetPFWGL(ptblhdc, ptblPFormat);
}

void helperPrepareContextWGL(PtblHandle hdc, PtblHandle ctx) {
  if (ctx.HasTrueVal()) {
    return;
  }

  auto& pstate = PortableState::Instance();
  //Get shared ctx
  PtblHandle share = pstate.ctxs[ctx].share;
  HGLRC shareCtx = 0;
  if ((int)share.Fake() != 0 && share.HasTrueVal()) {
    shareCtx = share.True();
  } else if ((int)share.Fake() != 0 && !share.HasTrueVal()) {
    helperPrepareContextWGL(hdc, share); //Create shared context
    shareCtx = share.True();
  }

  //Get attribs
  WGLCtxParams params;
  params = PtblToWGLCtxParams(pstate.ctxs[ctx].params);

  SetLastError(NO_ERROR);
  HGLRC truectx = drv.wgl.wglCreateContextAttribsARB(hdc.True(), shareCtx, &params[0]);
  DWORD err = GetLastError();
  if (err != 0) {
    Log(ERR) << "wglCreateContextAttribsARB failed. Error: " << Win32ErrorToString(err);
    if (truectx == 0) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  ctx.True(truectx);
}
} // namespace OpenGL
} // namespace gits
#endif

void gits::OpenGL::execHelperCreatePBufferWGL(PtblHandle surf, PtblHandle format) {
#ifdef GITS_PLATFORM_WINDOWS
  InitWinDrv();
  auto& pstate = PortableState::Instance();
  PortableState::SurfData& surfdata = pstate.surfs[surf];
  PtblHandle ptblHdc;

  //Try to find HDC for current window
  for (auto elem : pstate.helper.mapHdcSurface) {
    if (elem.second == surf) {
      ptblHdc = elem.first;
    }
  }

  //Create Fake HDC if not exist
  if ((int)ptblHdc.Fake() == 0) {
    ptblHdc = PtblHandle::Create(GenFake());
    pstate.helper.mapHdcSurface[ptblHdc] = surf;
  }

  const int attribs[] = {0};

  //Get base DC
  HDC baseHDC = GetDC(0);
  PtblHandle ptblBaseHDC = PtblHandle::Create(GenFake());
  ptblBaseHDC.True(baseHDC);

  //Choose Pixel Format
  helperChoosePFWGL(ptblBaseHDC, format);

  //Create PBuffer
  SetLastError(NO_ERROR);
  HPBUFFERARB truePbuff = 0;
  truePbuff =
      drv.wgl.wglCreatePbufferARB(baseHDC, format.True(), surfdata.width, surfdata.height, attribs);
  DWORD err = GetLastError();
  if (err != 0) {
    Log(ERR) << "wglCreatePbufferARB failed. Error: " << Win32ErrorToString(err);
    if (truePbuff == 0) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  ReleaseDC(0, ptblBaseHDC.True());

  //Get DC
  HDC trueHdc = drv.wgl.wglGetPbufferDCARB(truePbuff);
  surf.True(truePbuff);
  ptblHdc.True(trueHdc);
#endif
}

void gits::OpenGL::execSetContextWGL(PtblHandle ctx) {
#ifdef GITS_PLATFORM_WINDOWS
  if ((void*)ctx.Fake() == 0) {
    drv.wgl.wglMakeCurrent(0, 0);
    return;
  }

  //Read hwnd and hdc from state
  auto& pstate = PortableState::Instance();
  if (pstate.ctxs.find(ctx) == pstate.ctxs.end()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  auto& surf = pstate.ctxs[ctx].surf;

  //Try to find HDC for current window
  PtblHandle hdc;
  for (auto elem : pstate.helper.mapHdcSurface) {
    if (elem.second == surf) {
      hdc = elem.first;
    }
  }

  //Create Fake HDC if not exist
  if ((int)hdc.Fake() == 0) {
    hdc = PtblHandle::Create(GenFake());
    pstate.helper.mapHdcSurface[hdc] = surf;
  }

  //Prepare surface
  helperPrepareSurfaceWGL(surf, hdc);

  //Prepare context
  helperPrepareContextWGL(hdc, ctx);

  //MakeCurrent
  wglMakeCurrent_checked(hdc.True(), ctx.True());
#endif
}

void gits::OpenGL::execDelContextWGL(PtblHandle ctx) {
#ifdef GITS_PLATFORM_WINDOWS
  drv.wgl.wglDeleteContext(ctx.True());
#endif
}

void gits::OpenGL::execBufferSwapWGL(PtblHandle ctx) {
#ifdef GITS_PLATFORM_WINDOWS
  //Read hwnd and hdc from state
  auto& pstate = PortableState::Instance();
  auto& hwnd = pstate.ctxs[ctx].surf;
  PtblHandle hdc;
  for (auto elem : pstate.helper.mapHdcSurface) {
    if (elem.second == hwnd) {
      hdc = elem.first;
    }
  }
  if ((int)hdc.Fake() == 0) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  //SwapBuffers
  drv.wgl.wglSwapBuffers(hdc.True());
#endif
}
