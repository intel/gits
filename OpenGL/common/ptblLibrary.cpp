// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptblLibrary.cpp
*
*/

#include "ptblLibrary.h"
#include "ptbl_wglLibrary.h"
#include "ptbl_glxLibrary.h"
#include "ptbl_eglLibrary.h"
#include "config.h"
#include "tools_lite.h"
#include "openglEnums.h"

//*********************** Native API ****************************
gits::OpenGL::PtblNativeAPI::Type gits::OpenGL::PtblNativeAPI::_api = UNKNOWN;
gits::OpenGL::PtblNativeAPI::Type gits::OpenGL::PtblNativeAPI::_streamApi = UNKNOWN;

//********************** Portable Handle ****************************
std::map<uint64_t, uint64_t>& gits::OpenGL::PtblHandle::FakeToTrue() {
  typedef std::map<uint64_t, uint64_t> maptype;
  INIT_NEW_STATIC_OBJ(map, maptype)
  return map;
}

static unsigned fakeHandle = 0;
uint64_t gits::OpenGL::LastFake() {
  return fakeHandle;
}
uint64_t gits::OpenGL::GenFake() {
  return ++fakeHandle;
}

//******************** Portable State *********************************
gits::OpenGL::PortableState::Helper::Helper() {
  currApiEgl = EGL_OPENGL_ES_API;
  auto forcedApi = Configurator::Get().opengl.player.forceGLProfile;
  if (forcedApi == TForcedGLProfile::CORE || forcedApi == TForcedGLProfile::COMPAT) {
    currApiEgl = EGL_OPENGL_API;
  }
}

//**************** Execution Functions Dispatcher ****************************
void gits::OpenGL::dispatchSetContext(PtblHandle ctx) {
  switch (PtblNtvApi()) {
  case PtblNativeAPI::WGL:
    execSetContextWGL(ctx);
    break;
  case PtblNativeAPI::GLX:
    execSetContextGLX(ctx);
    break;
  case PtblNativeAPI::EGL:
    execSetContextEGL(ctx);
    break;
  default:
    std::runtime_error(EXCEPTION_MESSAGE);
  }
}

void gits::OpenGL::dispatchDelContext(PtblHandle ctx) {
  switch (PtblNtvApi()) {
  case PtblNativeAPI::WGL:
    execDelContextWGL(ctx);
    break;
  case PtblNativeAPI::GLX:
    execDelContextGLX(ctx);
    break;
  case PtblNativeAPI::EGL:
    execDelContextEGL(ctx);
    break;
  default:
    std::runtime_error(EXCEPTION_MESSAGE);
  }
}

void gits::OpenGL::dispatchMakeBufferSwap(PtblHandle ctx) {
  switch (PtblNtvApi()) {
  case PtblNativeAPI::WGL:
    execBufferSwapWGL(ctx);
    break;
  case PtblNativeAPI::GLX:
    execBufferSwapGLX(ctx);
    break;
  case PtblNativeAPI::EGL:
    execBufferSwapEGL(ctx);
    break;
  default:
    std::runtime_error(EXCEPTION_MESSAGE);
  }
}

void gits::OpenGL::dispatchHelperCreatePBuffer(PtblHandle surf, PtblHandle pf) {
  switch (PtblNtvApi()) {
  case PtblNativeAPI::WGL:
    execHelperCreatePBufferWGL(surf, pf);
    break;
  case PtblNativeAPI::GLX:
    ENotImplemented(EXCEPTION_MESSAGE);
    break;
  case PtblNativeAPI::EGL:
    ENotImplemented(EXCEPTION_MESSAGE);
    break;
  default:
    std::runtime_error(EXCEPTION_MESSAGE);
  }
}

//************** Drivers Initialization interface ****************************
void gits::OpenGL::ptblInitialize(CGlDriver::TApiType api) {
  if (Configurator::Get().opengl.shared.forceGLVersion.empty() &&
      Configurator::Get().opengl.player.forceGLProfile == TForcedGLProfile::NO_PROFILE_FORCED) {
    drv.gl.Initialize(api);
    return;
  }
  Log(INFO) << "Initializing modified GL Version";

  if (Configurator::Get().opengl.player.forceGLProfile == TForcedGLProfile::CORE ||
      Configurator::Get().opengl.player.forceGLProfile == TForcedGLProfile::COMPAT) {
    drv.gl.Initialize(CGlDriver::API_GL);
  } else if (Configurator::Get().opengl.player.forceGLProfile == TForcedGLProfile::ES) {
    if (!Configurator::Get().opengl.shared.forceGLVersion.empty()) {
      if (Configurator::Get().opengl.shared.forceGLVersionMajor >= 2) {
        drv.gl.Initialize(CGlDriver::API_GLES2);
      } else {
        drv.gl.Initialize(CGlDriver::API_GLES1);
      }
    } else {
      if (api == CGlDriver::API_GLES1) {
        drv.gl.Initialize(CGlDriver::API_GLES1);
      } else if (api == CGlDriver::API_GLES2) {
        drv.gl.Initialize(CGlDriver::API_GLES2);
      } else {
        drv.gl.Initialize(CGlDriver::API_GLES2);
      }
    }
  } else {
    drv.gl.Initialize(api);
  }
}

//***************** Windowing Functions Interface ****************************
namespace {
win_ptr_t PostCreateWinAction(
    win_ptr_t win, GLint width, GLint height, GLint x, GLint y, bool show) {
  using namespace gits;
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    //Regular mode (no portability needed)
    if (win == 0) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    return win;
  } else {
    auto& pstate = PortableState::Instance();
    auto ptblWin = PtblHandle::Create(GenFake());

    //Store portable state
    pstate.surfs[ptblWin].x = x;
    pstate.surfs[ptblWin].y = y;
    pstate.surfs[ptblWin].width = width;
    pstate.surfs[ptblWin].height = height;
    pstate.surfs[ptblWin].vis = show;
    //In portable mode we always return fake handle but if window has been created we also save a true pointer value.
    if (win != (win_ptr_t)0) {
      ptblWin.True(win);
    }
    return ptblWin.Fake();
  }
}

gits::OpenGL::PortableState::SurfData* GetSurfaceData(gits::OpenGL::PtblHandle win) {
  return &gits::OpenGL::PortableState::Instance().surfs[win];
}
} // namespace

win_ptr_t gits::ptblCreateWin(GLint width, GLint height, GLint x, GLint y, bool show) {
  win_ptr_t win = 0;
#if defined GITS_PLATFORM_WINDOWS
  win = CreateWin(width, height, x, y, show);
#endif
  return PostCreateWinAction(win, width, height, x, y, show);
}

win_ptr_t gits::ptblCreateWin(
    GLXFBConfig pf, XVisualInfo* xinfo, GLint width, GLint height, GLint x, GLint y, bool show) {
  win_ptr_t win = 0;
#if defined GITS_PLATFORM_WINDOWS || defined GITS_PLATFORM_X11
  win = CreateWin(pf, xinfo, width, height, x, y, show);
#endif
  return PostCreateWinAction(win, width, height, x, y, show);
}

win_ptr_t gits::ptblCreateWin(
    EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show) {
  win_ptr_t win = 0;
  win = CreateWin(pf, width, height, x, y, show);
  return PostCreateWinAction(win, width, height, x, y, show);
}

void gits::ptblRemoveWin(win_ptr_t handle) {
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    RemoveWin(handle);
  } else {
    PtblHandle win = PtblHandle::Get(handle); //it may be PtblNtvWin or PtblSurface

    //Remove window if created
    if (win.HasTrueVal()) {
      RemoveWin(win.True());
    }

    //Remove window state
    PortableState::Instance().surfs.erase(win);
  }
}

void gits::ptblResizeWin(win_ptr_t handle, int width, int height) {
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    ResizeWin(handle, width, height);
  } else {
    PtblHandle win = PtblHandle::Get(handle);
    if (win.HasTrueVal()) {
      ResizeWin(win.True(), width, height);
    }

    auto surfDataPtr = GetSurfaceData(win);
    surfDataPtr->width = width;
    surfDataPtr->height = height;
  }
}
void gits::ptblMoveWin(win_ptr_t handle, int x, int y) {
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    MoveWin(handle, x, y);
  } else {
    PtblHandle win = PtblHandle::Get(handle);
    if (win.HasTrueVal()) {
      MoveWin(win.True(), x, y);
    }

    auto surfDataPtr = GetSurfaceData(win);
    surfDataPtr->x = x;
    surfDataPtr->y = y;
  }
}
void gits::ptblWinVisibility(win_ptr_t handle, bool show) {
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    WinVisibility(handle, show);
  } else {
    PtblHandle win = PtblHandle::Get(handle);
    if (win.HasTrueVal()) {
      WinVisibility(win.True(), show);
    }

    auto surfDataPtr = GetSurfaceData(win);
    surfDataPtr->vis = show;
  }
}
void gits::ptblWinTitle(win_ptr_t handle, const std::string& title) {
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    WinTitle(handle, title);
  } else {
    PtblHandle win = PtblHandle::Get(handle);
    if (win.HasTrueVal()) {
      WinTitle(win.True(), title);
    }

    auto surfDataPtr = GetSurfaceData(win);
    surfDataPtr->title = title;
  }
}
win_handle_t gits::ptblGetWinHandle(win_ptr_t win) {
  using namespace OpenGL;
  if (PtblNtvApi() == PtblNtvStreamApi()) {
    return GetWinHandle(win);
  } else {
    return (win_handle_t)win;
  }
}

//********************** Portable Helper ****************************
