// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   windowContextState.cpp
 *
 * @brief Definition of OpenGL common part library implementation.
 *
 */

#include "platform.h"
#include "windowContextState.h"
#include "openglDrivers.h"
#include "ptbl_glxLibrary.h"
#include "ptblLibrary.h"
#include "openglEnums.h"

using namespace gits;
using namespace OpenGL;

//----------------------GLX WINDOW CONTEXT MANAGER----------------------
GlxWinCtxMngr::GlxWinCtxMngr(native_disp_t glxDpy, GLXContext glxCtx)
    : _dpy(glxDpy), _glxCtx(glxCtx) {}

void GlxWinCtxMngr::Add(GLXContext glxCtx, GlxWinCtxMngr* obj) {
  ctxMngrMap()[glxCtx] = obj;
}

GlxWinCtxMngr* GlxWinCtxMngr::Get(GLXContext glxCtx) {
  typeCtxMngrMap::const_iterator iter = ctxMngrMap().find(glxCtx);
  if (iter != ctxMngrMap().end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

void GlxWinCtxMngr::Remove(GLXContext glxCtx) {
  delete ctxMngrMap()[glxCtx];
  ctxMngrMap().erase(glxCtx);
}

void GlxWinCtxMngr::RemoveAll() {
  for (auto& elem : ctxMngrMap()) {
    Remove(elem.first);
  }
}

//----------------------GLX PLAYER MANAGER----------------------
GlxPlayerMngr::GlxPlayerMngr(Display* glxDpy, GLXContext glxCtx, GLXFBConfig fbconf)
    : GlxWinCtxMngr(glxDpy, glxCtx), _fbConfig(fbconf), _window(nullptr) {}

void GlxPlayerMngr::UpdateWindow(std::vector<int>& winparams) {
  if (!winparams.empty()) {
    if (!Configurator::Get().common.player.forceWindowSize.enabled) {
      _window->set_size(winparams[2], winparams[3]);
    }
    if (!Configurator::Get().common.player.forceWindowPos.enabled) {
      _window->set_position(winparams[0], winparams[1]);
    }
  }
}

void GlxPlayerMngr::UpdateWindowList(std::vector<win_handle_t>& windelete) {
  if (!windelete.empty()) {
    for (auto& winptr : windowsList()) {
      ReleaseWindow(winptr->handle());
    }
  }
}

win_handle_t GlxPlayerMngr::GetWindowHandle() const {
  if (_window != nullptr) {
    return _window->handle();
  } else {
    return 0;
  }
}

void GlxPlayerMngr::SetupWindow(std::vector<int>& winparams) {
  if (_fbConfig == nullptr) {
    SetupWindowWithoutId(winparams);
  } else {
    SetupWindowWithId(winparams);
  }
}

void GlxPlayerMngr::SetupWindowWithoutId(std::vector<int>& winparams) {
  int xpos = winparams[0];
  int ypos = winparams[1];
  int xsize = winparams[2];
  int ysize = winparams[3];
  if (Configurator::Get().common.player.forceWindowPos.enabled) {
    xpos = Configurator::Get().common.player.forceWindowPos.x;
    ypos = Configurator::Get().common.player.forceWindowPos.y;
  }

  if (Configurator::Get().common.player.forceWindowSize.enabled) {
    xsize = Configurator::Get().common.player.forceWindowSize.width;
    ysize = Configurator::Get().common.player.forceWindowSize.height;
  }

  int native_attribs[] = {GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, 0};
  int formats = 0;
  GLXFBConfig* configs = ptbl_glXChooseFBConfig((Display*)GetNativeDisplay(),
                                                ptbl_DefaultScreen((Display*)GetNativeDisplay()),
                                                native_attribs, &formats);
  if (formats == 0) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  XVisualInfo* vi =
      (XVisualInfo*)ptbl_glXGetVisualFromFBConfig((Display*)GetNativeDisplay(), configs[0]);
  _window = new Window_(configs[0], (XVisualInfo*)vi, xsize, ysize, xpos, ypos, (bool)winparams[4]);
  _window->set_title("gitsPlayer");
}

void GlxPlayerMngr::SetupWindowWithId(std::vector<int>& winparams) {
  int xpos = winparams[0];
  int ypos = winparams[1];
  int xsize = winparams[2];
  int ysize = winparams[3];

  if (Configurator::Get().common.player.forceWindowPos.enabled) {
    xpos = Configurator::Get().common.player.forceWindowPos.x;
    ypos = Configurator::Get().common.player.forceWindowPos.y;
  }

  if (Configurator::Get().common.player.forceWindowSize.enabled) {
    xsize = Configurator::Get().common.player.forceWindowSize.width;
    ysize = Configurator::Get().common.player.forceWindowSize.height;
  }

  XVisualInfo* vi =
      (XVisualInfo*)ptbl_glXGetVisualFromFBConfig((Display*)GetNativeDisplay(), _fbConfig);
  _window = new Window_(_fbConfig, (XVisualInfo*)vi, xsize, ysize, xpos, ypos, (bool)winparams[4]);
  _window->set_title("gitsPlayer");
}

void GlxPlayerMngr::SetContext() {}

void GlxPlayerMngr::ReleaseWindow(win_handle_t winHandle) {}
#if defined GITS_PLATFORM_X11
//----------------------GLX RECORDER MANAGER----------------------

GlxRecorderMngr::GlxRecorderMngr(native_disp_t glxDpy, GLXContext glxCtx)
    : GlxWinCtxMngr(glxDpy, glxCtx) {
  UpdateWindowHandle();
}

GlxRecorderMngr::~GlxRecorderMngr() {
  windowsList().erase(_windowHandle);
}

void GlxRecorderMngr::UpdateWindow(std::vector<int>& winparams) {
  std::vector<int> winparamsCurrent;
  ReadWindowParams(winparamsCurrent);

  if (winparamsCurrent != _winparamsLast) {
    winparams = winparamsCurrent;                 // Copy contents.
    _winparamsLast = std::move(winparamsCurrent); // Move contents.
  }
}

void GlxRecorderMngr::UpdateWindowList(std::vector<win_handle_t>& windelete) {
  // Fill a list of windows to be deleted.
  for (auto& windowPtr : windowsList()) {
    if (!IsWindow(*windowPtr)) {
      windelete.push_back(windowPtr->handle());
      windowsList().erase(windowPtr);
    }
  }
}

win_handle_t GlxRecorderMngr::GetWindowHandle() const {
  if (_windowHandle) {
    return _windowHandle->handle();
  } else {
    return 0;
  }
}

void GlxRecorderMngr::ReadWindowParams(std::vector<int>& params) const {
  params.resize(5, 0);
  if (_windowHandle) {
    params[0] = _windowHandle->get_pos().first;
    params[1] = _windowHandle->get_pos().second;
    params[2] = _windowHandle->get_size().first;
    params[3] = _windowHandle->get_size().second;
    params[4] = (int)_windowHandle->is_visible();
  }
}

std::shared_ptr<window_handle> GlxRecorderMngr::FindWindow(win_handle_t handle) const {
  for (auto& windowPtr : windowsList()) {
    if (windowPtr->handle() == handle) {
      return windowPtr;
    }
  }
  return {};
}

void GlxRecorderMngr::UpdateWindowHandle() {
  //Updates current window for context
  win_handle_t currentWindow = gits::OpenGL::drv.glx.glXGetCurrentDrawable();

  //Check if it's GLXWindow and get corresponding Window
  if (GlxRecorderMngr::GetSurface(currentWindow) != 0) {
    currentWindow = GlxRecorderMngr::GetSurface(currentWindow);
  }

  if (_windowHandle) {
    if (_windowHandle->handle() == currentWindow) {
      return;
    }
  }

  if (currentWindow != 0) {
    std::shared_ptr<window_handle> foundWindow = FindWindow(currentWindow);

    if (foundWindow) {
      _windowHandle = std::move(foundWindow);
    } else {
      _windowHandle = std::make_shared<window_handle>(currentWindow);
    }
  }
}

bool GlxRecorderMngr::IsWindow(const window_handle& windowhandle) const {
  char* windowName;
  if (XFetchName((Display*)_dpy, windowhandle.handle(), &windowName) == BadWindow) {
    return false;
  } else {
    XFree(windowName);
    return true;
  }
}

void GlxRecorderMngr::AddSurface(GLXWindow glxWindow, win_handle_t window) {
  _glxWindowToXWindowMap[glxWindow] = window;
}

void GlxRecorderMngr::RemoveSurface(GLXWindow glxWindow) {
  _glxWindowToXWindowMap.erase(glxWindow);
}

win_handle_t GlxRecorderMngr::GetSurface(GLXWindow glxWindow) {
  auto iter = _glxWindowToXWindowMap.find(glxWindow);
  if (iter != _glxWindowToXWindowMap.end()) {
    return iter->second;
  } else {
    return 0;
  }
}

std::map<GLXWindow, win_handle_t> GlxRecorderMngr::_glxWindowToXWindowMap;
#endif
