// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"
#include "windowing.h"
#include "platform.h"
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

//----------------------GLX WINDOW CONTEXT MANAGER----------------------
class GlxWinCtxMngr {
public:
  GlxWinCtxMngr(native_disp_t glxDpy, GLXContext glxCtx);
  virtual ~GlxWinCtxMngr() {}

  static void Add(GLXContext glxCtx, GlxWinCtxMngr* obj);
  static GlxWinCtxMngr* Get(GLXContext glxCtx);
  static void Remove(GLXContext glxCtx);
  static void RemoveAll();

  virtual void UpdateWindow(std::vector<int>& winparams) = 0;
  virtual void UpdateWindowList(std::vector<win_handle_t>& windelete) = 0;
  virtual GLXContext GetGlxContext() const = 0;
  virtual win_handle_t GetWindowHandle() const = 0;
  virtual void GetWindowParams(std::vector<int>& winparams) = 0;

protected:
  native_disp_t _dpy;
  GLXContext _glxCtx;

  void PlayerWindowUpdate();
  void RecorderWindowParamsUpdate();

  typedef std::map<GLXContext, GlxWinCtxMngr*> typeCtxMngrMap;
  static typeCtxMngrMap& ctxMngrMap() { //MAP: GLXContext/GlxWinCtxMngr
    INIT_NEW_STATIC_OBJ(map, typeCtxMngrMap)
    return map;
  }
};

//----------------------GLX PLAYER MANAGER----------------------
class GlxPlayerMngr : public GlxWinCtxMngr {
public:
  GlxPlayerMngr(Display* glxDpy, GLXContext glxCtx, GLXFBConfig fbconf = (GLXFBConfig)0);
  virtual ~GlxPlayerMngr() {}

  void SetContext();
  GLXContext GetGlxContext() const {
    return _glxCtx;
  }
  void SetupWindow(std::vector<int>& winparams);

  void UpdateWindow(std::vector<int>& winparams);
  void UpdateWindowList(std::vector<win_handle_t>& windelete);
  win_handle_t GetWindowHandle() const;
  void GetWindowParams(std::vector<int>& winparams) {
    throw std::runtime_error("GetWindowParams is not used in player");
  }

private:
  GLXFBConfig _fbConfig;
  Window_* _window;

  void ReleaseWindow(win_handle_t winHandle);
  void SetupWindowWithId(std::vector<int>& winparams);
  void SetupWindowWithoutId(std::vector<int>& winparams);

  static std::set<Window_*>& windowsList() {
    INIT_NEW_STATIC_OBJ(set, std::set<Window_*>)
    return set;
  }
};

#if defined GITS_PLATFORM_X11
//----------------------GLX RECORDER MANAGER----------------------
class GlxRecorderMngr : public GlxWinCtxMngr {
public:
  GlxRecorderMngr(native_disp_t glxDpy, GLXContext glxCtx);
  GlxRecorderMngr(const GlxRecorderMngr& other) = delete;
  GlxRecorderMngr& operator=(const GlxRecorderMngr& other) = delete;
  virtual ~GlxRecorderMngr();

  void UpdateWindowHandle();
  GLXContext GetGlxContext() const {
    return _glxCtx;
  }

  void UpdateWindow(std::vector<int>& winparams);
  void UpdateWindowList(std::vector<win_handle_t>& windelete);
  win_handle_t GetWindowHandle() const;
  void GetWindowParams(std::vector<int>& winparams) {
    winparams = _winparamsLast;
  }

  static void AddSurface(GLXWindow glxWindow, win_handle_t window);
  static void RemoveSurface(GLXWindow glxWindow);
  static win_handle_t GetSurface(GLXWindow glxWindow);

private:
  std::shared_ptr<window_handle> _windowHandle;
  std::vector<int> _winparamsLast;
  static std::map<GLXWindow, win_handle_t> _glxWindowToXWindowMap;

  void ReadWindowParams(std::vector<int>& params) const;
  std::shared_ptr<window_handle> FindWindow(win_handle_t handle) const;

  bool IsWindow(const window_handle& windowhandle) const;

  static std::set<std::shared_ptr<window_handle>>& windowsList() {
    INIT_NEW_STATIC_OBJ(set, std::set<std::shared_ptr<window_handle>>)
    return set;
  }
};

#endif
