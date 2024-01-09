// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptblLibrary.h
*
*/

#pragma once
#include "tools.h"
#include "windowing.h"
#include "openglDrivers.h"

namespace gits {
namespace OpenGL {
//*********************** Native API ****************************
struct PtblNativeAPI {
  enum Type { UNKNOWN = 0, WGL, GLX, EGL };
  static Type _api;
  static Type _streamApi;

public:
  //Methods below are partially implemented - full implementation comes with native api compatibility feature
  static Type GetAPI() {
    return (_api != UNKNOWN) ? _api : throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  static void SetAPI(Type apitype) { //This option may be set only before setting stream API
    if (apitype == UNKNOWN) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    if (_api == UNKNOWN) {
      _api = apitype;
#if defined GITS_PLATFORM_WINDOWS
      if (_api != EGL && _api != WGL) {
#elif defined GITS_PLATFORM_X11
      if (_api != EGL && _api != GLX) {
#endif
        Log(ERR) << "Preselected API type not supported on current platform";
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  static Type GetStreamAPI() {
    return (_streamApi != UNKNOWN) ? _streamApi : throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  static void SetStreamAPI(Type apitype) {
    if (apitype == UNKNOWN) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    if (_streamApi != UNKNOWN) {
      return;
    }
    _streamApi = apitype;
    if (_api != UNKNOWN) {
      return;
    }
    //Intelligent API selection
#if defined GITS_PLATFORM_WINDOWS
    if (_streamApi == EGL) {
      _api = EGL;
    } else {
      _api = WGL;
    }
#elif defined GITS_PLATFORM_X11
    if (_streamApi == EGL) {
      _api = EGL;
    } else {
      _api = GLX;
    }
#else
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
};

inline PtblNativeAPI::Type PtblNtvApi() {
  return PtblNativeAPI::GetAPI();
}
inline void PtblNtvApi(PtblNativeAPI::Type api) {
  return PtblNativeAPI::SetAPI(api);
}
inline PtblNativeAPI::Type PtblNtvStreamApi() {
  return PtblNativeAPI::GetStreamAPI();
}
inline void PtblNtvStreamApi(PtblNativeAPI::Type api) {
  return PtblNativeAPI::SetStreamAPI(api);
}

//*********************** GL Params ****************************
enum PtblGLProfile { Compat = 300, Core, ES };
enum PtblCtxParam { VerMinor = 400, VerMajor, Profile };
typedef std::map<PtblCtxParam, int> PtblCtxParams;

//******************* PF Params ************************************

enum PtblPFAttrib {
  ptblPFDoubleBuffer = 100,
  ptblPFAccelerated,
  ptblPFPBuffer,
  ptblPFAuxBuffers,
  ptblPFRedSize,
  ptblPFGreenSize,
  ptblPFBlueSize,
  ptblPFAlphaSize,
  ptblPFDepthSize,
  ptblPFStencilSize,
  ptblPFAccumSize,
  ptblPFSamples,
  ptblPFSampleBuffers
};
typedef std::map<PtblPFAttrib, unsigned> PtblPFAttribs;

//********************** Portable Handle ****************************
uint64_t LastFake();
uint64_t GenFake();

class PtblHandle {
  uint64_t _fake;
  static std::map<uint64_t, uint64_t>& FakeToTrue();

  explicit PtblHandle(uint64_t fake) : _fake(fake) {
    FakeToTrue()[0] = 0;
  }

public:
  PtblHandle() : _fake(0) {}
  PtblHandle(const PtblHandle& other) : _fake(other._fake) {}
  ~PtblHandle() = default;
  PtblHandle& operator=(const PtblHandle& other) {
    if (this != &other) {
      this->_fake = other._fake;
    }
    return *this;
  }
  bool operator<(const PtblHandle cmp) const {
    return _fake < cmp._fake;
  }
  bool operator==(const PtblHandle cmp) const {
    return _fake == cmp._fake;
  }
  bool operator!=(const PtblHandle cmp) const {
    return _fake != cmp._fake;
  }

  //Get and Create methods adds an initial validation of passed fake values
  template <class T>
  static PtblHandle Get(T fake) {
    return ((uint64_t)fake <= LastFake()) ? PtblHandle((uint64_t)fake)
                                          : throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  static PtblHandle Create(uint64_t fake) {
    return (fake == LastFake()) ? PtblHandle(fake) : throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  class ProxyPtr {
    uint64_t _ptr;

  public:
    ProxyPtr(uint64_t ptr) : _ptr(ptr) {}
    template <class T>
    operator T() {
      return (T)_ptr;
    }
    bool operator==(const ProxyPtr cmp) const {
      return _ptr == cmp._ptr;
    }
    bool operator!=(const ProxyPtr cmp) const {
      return _ptr != cmp._ptr;
    }
  };

  bool HasTrueVal() const {
    return (FakeToTrue().find(_fake) != FakeToTrue().end());
  }
  ProxyPtr Fake() {
    return ProxyPtr(_fake);
  }
  ProxyPtr True() const {
    return (HasTrueVal()) ? ProxyPtr(FakeToTrue()[_fake])
                          : throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  template <class T>
  void True(T trueptr) {
    FakeToTrue()[_fake] = (uint64_t)(trueptr);
  }
};

//******************** Partable State tracking class ****************************
struct PortableState {
  struct PFData {
    PtblPFAttribs attribs;
  };

  struct SurfData {
    bool ispbuff;
    int x;
    int y;
    int width;
    int height;
    bool vis;
    std::string title;

    PtblHandle pf;
    SurfData() : ispbuff(false), x(0), y(0), width(0), height(0), vis(false) {}
  };

  struct CtxData {
    PtblHandle share;
    PtblHandle pf;
    PtblHandle surf;
    PtblCtxParams params;
  };

  struct Helper {
    std::map<PtblHandle, PtblHandle>
        mapHdcSurface; //WGL - There may be multiple HDC for single surface - this map is used to find the surface by HDC

    std::map<PtblHandle, PtblHandle>
        mapEglSurfWin; //EGL - EGLSurface to window handle (HWND for ie)
    std::map<PtblHandle, EGLenum>
        mapEglSurfToBuffType; //EGL - maps egl surface (Window only) to EGL_RENDER_BUFFER param value
    EGLenum currApiEgl;       //EGL - api bound using elgBindAPI

    PtblHandle currCtx; //WGL

    Helper();
  };

  std::map<PtblHandle, PFData> pfs;
  std::map<PtblHandle, SurfData> surfs;
  std::map<PtblHandle, CtxData> ctxs;
  Helper helper;

  static PortableState& Instance() {
    static PortableState ptblState;
    return ptblState;
  }
};

//**************** Execution Functions Dispatcher ****************************
void dispatchSetContext(PtblHandle ctx);
void dispatchDelContext(PtblHandle ctx);
void dispatchMakeBufferSwap(PtblHandle ctx);
void dispatchHelperCreatePBuffer(PtblHandle surf, PtblHandle pf);

//************** Drivers Initialization interface ****************************
void ptblInitialize(CGlDriver::TApiType api);

} // namespace OpenGL

//***************** Windowing Functions Interface ****************************
win_ptr_t ptblCreateWin(GLint width, GLint height, GLint x, GLint y, bool show);
win_ptr_t ptblCreateWin(
    GLXFBConfig pf, XVisualInfo* xinfo, GLint width, GLint height, GLint x, GLint y, bool show);
win_ptr_t ptblCreateWin(EGLConfig pf, GLint width, GLint height, GLint x, GLint y, bool show);
void ptblRemoveWin(win_ptr_t handle);
void ptblResizeWin(win_ptr_t handle, int width, int height);
void ptblMoveWin(win_ptr_t handle, int x, int y);
void ptblWinVisibility(win_ptr_t handle, bool show);
void ptblWinTitle(win_ptr_t handle, const std::string& title);
win_handle_t ptblGetWinHandle(win_ptr_t win);
} // namespace gits
