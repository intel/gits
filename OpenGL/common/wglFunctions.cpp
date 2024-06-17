// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   wglFunctions.cpp
*
*/

#include "openglDrivers.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "wglFunctions.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "gits.h"
#include "exception.h"
#include "library.h"
#include "log.h"
#include "scheduler.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include <map>
#include <algorithm>
#include "config.h"
#include "pfattribs.h"
#include "windowing.h"
#include "config.h"
#include "platform.h"
#include "ptblLibrary.h"
#include "ptbl_wglLibrary.h"
#include "opengl_apis_iface.h"

/* ****************************** UPDATE HDC *************************************** */

namespace {
using gits::OpenGL::CHDC;
using gits::OpenGL::CHWND;
using gits::OpenGL::CStateDynamicNative;
HDC UpdateHDC(CHDC& hdc, gits::OpenGL::CHWND& hwnd, bool zeroHwndToUse = false) {
  using namespace gits;
  using namespace OpenGL;
  if ((CHDC::CheckMapping(hdc.Original())) && (ptblWindowFromDC((HDC)*hdc))) {
    return CHDC::GetMapping(hdc.Original());
  } else {
    if (zeroHwndToUse) {
      CHDC::AddMapping(hdc.Original(), ptblGetDC(nullptr));
      return CHDC::GetMapping(hdc.Original());
    } else {
      if (CHWND::CheckMapping(hwnd.Original())) {
        HDC hdcNew = ptblGetDC(*hwnd);

        CStateDynamicNative& state = CStateDynamicNative::Get();
        HDC hdcOld = (HDC)state.MapGetHDCs(*hwnd);

        if (hdcOld != hdcNew) {
          state.ReMapHdc(hdcOld, hdcNew);
          state.MapAddHDC((void*)*hwnd, (void*)hdcNew);
          CHDC::RemoveMapping(hdcOld);
          ptblReleaseDC(*hwnd, hdcOld);
          CHDC::AddMapping(hdc.Original(), hdcNew);
        }

        return hdcNew;
      }
      CHDC::AddMapping(hdc.Original(), ptblGetDC(nullptr));
      return CHDC::GetMapping(hdc.Original());
    }
  }
}
} // namespace

/* ****************************** GET PIXEL FORMAT PARAMETERS *************************************** */
#if defined GITS_PLATFORM_WINDOWS

namespace {
using gits::Cint;
using gits::OpenGL::drv;
void GetPixelFormatParams(HDC hdc, int format, Cint::CSArray& attribs, Cint::CSArray& values) {
  int pfAttribsQueryList[] = {
      WGL_DRAW_TO_WINDOW_ARB, WGL_DRAW_TO_BITMAP_ARB, WGL_ACCELERATION_ARB, WGL_SWAP_METHOD_ARB,
      WGL_SUPPORT_OPENGL_ARB, WGL_DOUBLE_BUFFER_ARB,  WGL_PIXEL_TYPE_ARB,   WGL_RED_BITS_ARB,
      WGL_RED_SHIFT_ARB,      WGL_GREEN_BITS_ARB,     WGL_BLUE_BITS_ARB,    WGL_ALPHA_BITS_ARB,
      WGL_ACCUM_BITS_ARB,     WGL_DEPTH_BITS_ARB,     WGL_STENCIL_BITS_ARB, WGL_SAMPLE_BUFFERS_ARB,
      WGL_SAMPLES_ARB,        WGL_DRAW_TO_PBUFFER_ARB};
  int nAttribs = sizeof(pfAttribsQueryList) / sizeof(pfAttribsQueryList[0]);

  //Save pixel format query attribs
  attribs.Vector().assign(pfAttribsQueryList, pfAttribsQueryList + nAttribs);

  //Save pixel format attribs values
  values.Vector().resize(nAttribs);

  // We need to know if using extension will succeed beforehand here.
  if (drv.wgl.wglGetProcAddress("wglGetPixelFormatAttribivARB")) {
    int _return_value = drv.wgl.wglGetPixelFormatAttribivARB(
        hdc, format, 0, nAttribs, &attribs.Vector()[0], &values.Vector()[0]);

    if (_return_value == 0) {
      Log(WARN)
          << "Choosing pixel format using ARB Extension function failed. Using WIN GDI function.";
      PIXELFORMATDESCRIPTOR pfd_current;
      drv.wgl.wglDescribePixelFormat(hdc, format, sizeof(PIXELFORMATDESCRIPTOR), &pfd_current);
      gits::GetAttribsFromPFD(&pfd_current, nAttribs, &attribs.Vector()[0], &values.Vector()[0]);
    }

  } else {
    PIXELFORMATDESCRIPTOR pfd_current;
    drv.wgl.wglDescribePixelFormat(hdc, format, sizeof(PIXELFORMATDESCRIPTOR), &pfd_current);
    gits::GetAttribsFromPFD(&pfd_current, nAttribs, &attribs.Vector()[0], &values.Vector()[0]);
  }
}
} // namespace

#endif

/* ************************** CHOOSE PIXEL FORMAT ******************************* */
namespace {
using gits::Cint;
int SelectPixelFormat(HDC dc, Cint::CSArray& attribs, Cint::CSArray& values) {
  //CHOOSE PIXEL FORMAT
  using namespace gits;
  using namespace OpenGL;
  int piFormat = -1;
  UINT numFormats;

  if (PtblNtvApi() != PtblNtvStreamApi()
#ifdef GITS_PLATFORM_WINDOWS
      || drv.wgl.wglGetProcAddress("wglChoosePixelFormatARB")
#endif
  ) {
    //CHOOSE PIXEL FORMAT USING ARB EXTENSION FUNCTION
    auto iter_attribs = attribs.Vector().begin();
    auto iter_values = values.Vector().begin();
    auto iter_attribs_end = attribs.Vector().end();
    auto iter_values_end = values.Vector().end();
    std::vector<int> attribs_values;
    // Merge attributes and values, skipping some if necessary.
    // TODO: Use std::views::zip when available.
    for (; iter_attribs != iter_attribs_end && iter_values != iter_values_end;
         ++iter_attribs, ++iter_values) {
      if (Config::Get().opengl.player.forceNoMSAA &&
          (*iter_attribs == WGL_SAMPLES_ARB || *iter_attribs == WGL_SAMPLE_BUFFERS_ARB)) {
        continue;
      }
      if (!Config::Get().opengl.player.minimalConfig ||
          (Config::Get().opengl.player.minimalConfig &&
           (*iter_attribs == WGL_SAMPLES_ARB || *iter_attribs == WGL_SAMPLE_BUFFERS_ARB ||
            *iter_attribs == WGL_ACCELERATION_ARB || *iter_attribs == WGL_DOUBLE_BUFFER_ARB ||
            *iter_attribs == WGL_COLOR_BITS_ARB || *iter_attribs == WGL_RED_BITS_ARB ||
            *iter_attribs == WGL_GREEN_BITS_ARB || *iter_attribs == WGL_BLUE_BITS_ARB ||
            *iter_attribs == WGL_ALPHA_BITS_ARB || *iter_attribs == WGL_DEPTH_BITS_ARB ||
            *iter_attribs == WGL_STENCIL_BITS_ARB))) {
        attribs_values.push_back(*iter_attribs);
        attribs_values.push_back(*iter_values);
      }
    }
    attribs_values.push_back(0);

    int _return_value =
        ptbl_wglChoosePixelFormatARB(dc, &attribs_values[0], nullptr, 1, &piFormat, &numFormats);
    if (_return_value == 0) {
      Log(WARN)
          << "Choosing pixel format using ARB Extension function failed. Using WIN GDI function.";

      //CHOOSE PIXEL FORMAT USING WIN GDI FUNCTION
      PIXELFORMATDESCRIPTOR pfd_current;
      GetPFDFromAttribs((int)attribs.Vector().size(), &attribs.Vector()[0], &values.Vector()[0],
                        &pfd_current);
      piFormat = ptbl_wglChoosePixelFormat(dc, &pfd_current);
    }
  } else {
    //CHOOSE PIXEL FORMAT USING WIN GDI FUNCTION
    PIXELFORMATDESCRIPTOR pfd_current;
    GetPFDFromAttribs((int)attribs.Vector().size(), &attribs.Vector()[0], &values.Vector()[0],
                      &pfd_current);
    piFormat = ptbl_wglChoosePixelFormat(dc, &pfd_current);
  }

  return piFormat;
}
} // namespace

/* ****************************GET WINDOW PARAMS********************************* */
namespace {
void GetWindowParams(window_handle& window, std::vector<int>& params) {
  params.resize(4);
  window.get_dimensions(params[0], params[1], params[2], params[3]);
  params.push_back((int)window.is_visible());
}
} // namespace

/* *************************UPDATE WINDOWS PARAMS*********************************** */

namespace gits {
namespace OpenGL {
using gits::Cint;
using gits::OpenGL::CHWND;
using gits::OpenGL::CLibrary;
#if defined GITS_PLATFORM_WINDOWS
void UpdateWindowsRec(HWND hwnd, Cint::CSArray& winparams, CHWND::CSArray& hwnd_del_list) {
  CStateDynamicNative& state = CStateDynamicNative::Get();

  //Get current window parameters
  window_handle currentWindow(hwnd);
  std::vector<int> winparams_current;
  GetWindowParams(currentWindow, winparams_current);

  //Compare last winparams vs current
  std::vector<int> winparams_last;
  if (state.MapFindWindowRecorder((void*)hwnd, winparams_last)) {
    if (winparams_current != winparams_last) {
      winparams.Vector().assign(&winparams_current[0],
                                &winparams_current[0] + (int)winparams_current.size());
      state.MapUpdateWindowRecorder((void*)hwnd, winparams_current);
    }
  } else {
    Log(WARN) << "Ignoring update parameters of an unknown window";
    //throw gits::EOperationFailed(EXCEPTION_MESSAGE); - Maya 2012 workaround
  }
  if (Config::Get().opengl.recorder.doNotRemoveWin) {
    return;
  }

  //Create list of windows to delete
  std::vector<void*> hwndList;
  std::vector<HWND>& hwndDeleteList = hwnd_del_list.Vector();
  state.MapGetWinHandlesListRecorder(hwndList);
  for (auto hwndObj : hwndList) {
    if (!IsWindow((HWND)hwndObj)) {
      hwndDeleteList.push_back((HWND)hwndObj);
      state.MapDeleteWindowRecorder(hwndObj);
      state.UnMapHglrcHdcByHwnd(hwndObj);
    }
  }
}
#endif
/* *************************UPDATE WINDOWS*********************************** */

void UpdateWindows(const CHWND& hwnd,
                   const Cint::CSArray& winparams,
                   const CHWND::CSArray& hwnd_del_list) {
  CStateDynamicNative& state = CStateDynamicNative::Get();

  //Current window resize
  if (!winparams.Vector().empty()) {
    if (*hwnd == 0) {
      return;
    }
    Window_* window = state.MapFindWindowPlayer((win_handle_t)*hwnd);
    if (!Config::Get().common.player.forceWindowSize.enabled) {
      window->set_size(winparams.Vector()[2], winparams.Vector()[3]);
    }
    if (!Config::Get().common.player.forceWindowPos.enabled) {
      window->set_position(winparams.Vector()[0], winparams.Vector()[1]);
    }
    if (!Config::Get().common.player.showWindowsWA) {
      window->set_visibility((bool)winparams.Vector()[4]);
    }
  }

  //Winapis operating on HWNDs and HDCs are thread affine so apis like CloseWindow can't be called if there is no certainty
  //that window was created in the same thread
  /// @todo Fix workaround to not close windows in multithreaded application
  if (!CGits::Instance().MultithreadedApp() || !Config::Get().common.player.faithfulThreading) {
    //Delete windows
    if (!hwnd_del_list.Vector().empty()) {
      for (auto hwndIter : hwnd_del_list.Vector()) {
        ptblReleaseDC((HWND)CHWND::GetMapping(hwndIter),
                      (HDC)state.MapGetHDCs(CHWND::GetMapping(hwndIter)));
        state.MapDelHWND(CHWND::GetMapping(hwndIter));
        state.MapDeleteWindowPlayer((win_handle_t)CHWND::GetMapping(hwndIter));
        state.UnMapHglrcHdcByHwnd((void*)CHWND::GetMapping(hwndIter));
      }
    }
  }
}
} // namespace OpenGL
} // namespace gits

/* ************************* FAKE WindowFromDC FNC FOR LINUX *********************** */

// We need this function for cross-platform compatibility - Linux doesn't use
// HDC and this function will never be used, but we need it for compilation
// because it's widely used in below code for win32. It seems to be a better
// solution than thousands of "ifdefs".
#ifndef GITS_PLATFORM_WINDOWS
namespace {
HWND WindowFromDC(HDC hDC) {
  return nullptr;
}
} // namespace
#endif

/* ***************************** WGL_CHOOSE_PIXEL_FORMAT *************************** */

gits::OpenGL::CwglChoosePixelFormat::CwglChoosePixelFormat() {}

gits::OpenGL::CwglChoosePixelFormat::CwglChoosePixelFormat(int return_value,
                                                           HDC hdc,
                                                           const PIXELFORMATDESCRIPTOR* ppfd)
    : _hdc(hdc), _hwnd(WindowFromDC(hdc)), _pfd(*(const PIXELFORMATDESCRIPTOR_*)ppfd) {
  PtblNtvStreamApi(PtblNativeAPI::Type::WGL);
}

gits::CArgument& gits::OpenGL::CwglChoosePixelFormat::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hwnd, _pfd);
}

void gits::OpenGL::CwglChoosePixelFormat::Run() {
#if defined GITS_PLATFORM_WINDOWS
  PtblNtvStreamApi(PtblNativeAPI::Type::WGL);
  //If wglChoosePixelFormat uses unknown HDC add one to map
  if (!CHDC::CheckMapping(_hdc.Original())) {
    HDC curr_hdc = ptblGetDC(0);
    CHDC::AddMapping(_hdc.Original(), curr_hdc);
  }

  drv.wgl.wglChoosePixelFormat(UpdateHDC(_hdc, _hwnd), (const PIXELFORMATDESCRIPTOR*)&*_pfd);
#endif
}

/* ***************************** WGL_SET_PIXEL_FORMAT *************************** */

gits::OpenGL::CwglSetPixelFormat::CwglSetPixelFormat() {}

gits::OpenGL::CwglSetPixelFormat::CwglSetPixelFormat(BOOL return_value,
                                                     HDC hdc,
                                                     int format,
                                                     const PIXELFORMATDESCRIPTOR*)
#if defined GITS_PLATFORM_WINDOWS
    : _return_value(return_value), _hdc(hdc), _format(format), _hwnd(WindowFromDC(hdc)) {
  PtblNtvStreamApi(PtblNativeAPI::Type::WGL);

  //GET PIXEL FORMAT
  GetPixelFormatParams(hdc, format, _attribs, _values);

  //GET WINDOW INFO
  auto hwnd = _hwnd.Original();
  CStateDynamicNative& state = CStateDynamicNative::Get();
  window_handle currentWindow(hwnd);

  GetWindowParams(currentWindow, _winparams.Vector());

  //Add window to map
  state.MapAddWindowRecorder((void*)hwnd, _winparams.Vector());
  Log(INFO) << "Application setting pixelformat: " << format;
}
#else
{
}
#endif

gits::CArgument& gits::OpenGL::CwglSetPixelFormat::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hwnd, _format, _attribs, _values, _winparams);
}

void gits::OpenGL::CwglSetPixelFormat::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::WGL);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));

  //Set info that stream was recorded in native mode
  CStateDynamicNative& state = CStateDynamicNative::Get();

  //check whether the window that was bound with this hdc in the recorded app already exist,
  //and if so, link this hdc with that window and skip creation of a new one
  if (_hwnd.CheckMapping()) {
    _hdc.AddMapping(ptblGetDC(_hwnd.Value()));
    return;
  }

  //CREATE WINDOW
  int xpos = _winparams.Vector()[0];
  int ypos = _winparams.Vector()[1];
  if (Config::Get().common.player.forceWindowPos.enabled) {
    xpos = Config::Get().common.player.forceWindowPos.x;
    ypos = Config::Get().common.player.forceWindowPos.y;
  }

  int xsize = _winparams.Vector()[2];
  int ysize = _winparams.Vector()[3];
  if (Config::Get().common.player.forceWindowSize.enabled) {
    xsize = Config::Get().common.player.forceWindowSize.width;
    ysize = Config::Get().common.player.forceWindowSize.height;
  }

  bool isWindowVisible = (bool)_winparams.Vector()[4];
  if (gits::Config::Get().common.player.showWindowsWA) {
    isWindowVisible = true;
  }

  Window_* window = new Window_(xsize, ysize, xpos, ypos, isWindowVisible);
  window->set_title("gitsPlayer");
  HDC dc = ptblGetDC(static_cast<HWND>((void*)window->handle()));

  //save window object in state dynamic map
  state.MapAddWindowPlayer(window->handle(), window);
  state.MapAddHDC((void*)window->handle(), (void*)dc);

  _hwnd.AddMapping((HWND)window->handle());
  _hdc.AddMapping(dc);

  //RUN WGLSETPIXELFORMAT

  PIXELFORMATDESCRIPTOR pfd;
  int pformat = SelectPixelFormat(dc, _attribs, _values);
  ptbl_wglSetPixelFormat(dc, pformat, &pfd);
}

/* ***************************** WGL_GET_PIXEL_FORMAT *************************** */

gits::OpenGL::CwglGetPixelFormat::CwglGetPixelFormat() {}

gits::OpenGL::CwglGetPixelFormat::CwglGetPixelFormat(int return_value, HDC hdc)
    : _return_value(return_value), _hdc(hdc), _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetPixelFormat::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hwnd);
}

void gits::OpenGL::CwglGetPixelFormat::Run() {
#if defined GITS_PLATFORM_WINDOWS
  if (CHDC::CheckMapping(_hdc.Original())) {
    drv.wgl.wglGetPixelFormat(UpdateHDC(_hdc, _hwnd, true));
  }
#endif
}

/* ***************************** WGL_DESCRIBE_PIXEL_FORMAT *************************** */

gits::OpenGL::CwglDescribePixelFormat::CwglDescribePixelFormat() {}

gits::OpenGL::CwglDescribePixelFormat::CwglDescribePixelFormat(
    int return_value, HDC hdc, int format, unsigned nBytes, PIXELFORMATDESCRIPTOR* ppfd)
    : _return_value(return_value),
      _hdc(hdc),
      _format(format),
      _nBytes(nBytes),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglDescribePixelFormat::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _format, _nBytes, _ppfd, _hwnd);
}

void gits::OpenGL::CwglDescribePixelFormat::Run() {
#if defined GITS_PLATFORM_WINDOWS
  //This call is not played because it has no influence on stream
  //drv.wgl.wglDescribePixelFormat(*_hdc, *_format, *_nBytes, (void*)*_ppfd);
#endif
}

/* ***************************** WGL_COPY_CONTEXT *************************** */

gits::OpenGL::CwglCopyContext::CwglCopyContext() {}

gits::OpenGL::CwglCopyContext::CwglCopyContext(BOOL return_value,
                                               HGLRC hglrcSrc,
                                               HGLRC hglrcDst,
                                               UINT mask)
    : _return_value(return_value), _hglrcSrc(hglrcSrc), _hglrcDst(hglrcDst), _mask(mask) {}

gits::CArgument& gits::OpenGL::CwglCopyContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hglrcSrc, _hglrcDst, _mask);
}

void gits::OpenGL::CwglCopyContext::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglCopyContext(*_hglrcSrc, *_hglrcDst, *_mask);
#endif
}

/* ***************************** WGL_CREATE_CONTEXT *************************** */

gits::OpenGL::CwglCreateContext::CwglCreateContext() {}

gits::OpenGL::CwglCreateContext::CwglCreateContext(HGLRC return_value, HDC hdc)
    : _return_value(return_value), _hdc(hdc), _hwnd(WindowFromDC(hdc)) {
  SD().AddContext(return_value, nullptr);
}

gits::CArgument& gits::OpenGL::CwglCreateContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hwnd);
}

void gits::OpenGL::CwglCreateContext::Run() {
  HGLRC hglrc = nullptr;
  HDC hdc = (HDC)UpdateHDC(_hdc, _hwnd);

  if (hglrc == nullptr) {
    hglrc = ptbl_wglCreateContext(hdc);
  }

  CHGLRC::AddMapping(_return_value.Original(), hglrc);
  SD().AddContext(hglrc, nullptr);
}

/* ***************************** WGL_CREATE_LAYER_CONTEXT *************************** */

gits::OpenGL::CwglCreateLayerContext::CwglCreateLayerContext() {}

gits::OpenGL::CwglCreateLayerContext::CwglCreateLayerContext(HGLRC return_value,
                                                             HDC hdc,
                                                             int iLayerPlane)
    : _return_value(return_value), _hdc(hdc), _iLayerPlane(iLayerPlane), _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglCreateLayerContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _iLayerPlane, _hwnd);
}

void gits::OpenGL::CwglCreateLayerContext::Run() {
#if defined GITS_PLATFORM_WINDOWS
  void* hglrc = drv.wgl.wglCreateLayerContext(UpdateHDC(_hdc, _hwnd), *_iLayerPlane);
  CHGLRC::AddMapping(_return_value.Original(), (HGLRC)hglrc);
#endif
}

/* ***************************** WGL_DELETE_CONTEXT *************************** */

gits::OpenGL::CwglDeleteContext::CwglDeleteContext() {}

gits::OpenGL::CwglDeleteContext::CwglDeleteContext(BOOL return_value, HGLRC hglrc)
    : _return_value(return_value), _hglrc(hglrc) {
  CStateDynamicNative::Get().UnMapHglrcHdcByHglrc((void*)hglrc);
  SD().RemoveContext((void*)hglrc);
}

gits::CArgument& gits::OpenGL::CwglDeleteContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hglrc);
}

void gits::OpenGL::CwglDeleteContext::Run() {
  if (CHGLRC::CheckMapping(_hglrc.Original())) {
    ptbl_wglDeleteContext(*_hglrc);
    CStateDynamicNative::Get().UnMapHglrcHdcByHglrc(*_hglrc);
    SD().RemoveContext(*_hglrc);
    _hglrc.RemoveMapping();
  }
}

/* ***************************** WGL_GET_CURRENT_CONTEXT *************************** */

gits::OpenGL::CwglGetCurrentContext::CwglGetCurrentContext() {}

gits::OpenGL::CwglGetCurrentContext::CwglGetCurrentContext(HGLRC return_value) {}

gits::CArgument& gits::OpenGL::CwglGetCurrentContext::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetCurrentContext::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetCurrentContext();
#endif
}

/* ***************************** WGL_GET_CURRENT_DC *************************** */

gits::OpenGL::CwglGetCurrentDC::CwglGetCurrentDC() {}

gits::OpenGL::CwglGetCurrentDC::CwglGetCurrentDC(HDC return_value) {}

gits::CArgument& gits::OpenGL::CwglGetCurrentDC::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetCurrentDC::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetCurrentDC();
#endif
}

/* ***************************** WGL_GET_PROC_ADDRESS *************************** */

gits::OpenGL::CwglGetProcAddress::CwglGetProcAddress() {}

gits::OpenGL::CwglGetProcAddress::CwglGetProcAddress(LPCSTR lpszProc) : _lpszProc(lpszProc, 0, 1) {}

gits::CArgument& gits::OpenGL::CwglGetProcAddress::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _lpszProc);
}

void gits::OpenGL::CwglGetProcAddress::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetProcAddress(*_lpszProc);
#endif
}

/* ***************************** WGL_MAKE_CURRENT *************************** */

gits::OpenGL::CwglMakeCurrent::CwglMakeCurrent() {}

gits::OpenGL::CwglMakeCurrent::CwglMakeCurrent(BOOL return_value, HDC hdc, HGLRC hglrc)
    : _return_value(return_value), _hdc(hdc), _hglrc(hglrc), _hwnd(WindowFromDC(hdc)) {
#if defined GITS_PLATFORM_WINDOWS
  if (return_value && (hglrc != nullptr) && (SD().GetContextStateData(hglrc) == nullptr)) {
    // If the wglMakeCurrent function succeeds but the context data has not yet been tracked,
    // initiate tracking of the context data at this point.
    Log(WARN) << "wglMakeCurrent function succeeded, yet context data remains untracked. "
                 "Initiating tracking now.";
    SD().AddContext(hglrc, nullptr);
  }

  PtblNtvStreamApi(PtblNativeAPI::Type::WGL);
  if (hdc != 0 && _hwnd.Original() != NULL) {
    UpdateWindowsRec(_hwnd.Original(), _winparams, _hwnd_del_list);
  }

  CStateDynamicNative::Get().MapHglrcToHdc((void*)hglrc, (void*)hdc);
  SD().SetCurrentContext((void*)hglrc);
#endif
}

gits::CArgument& gits::OpenGL::CwglMakeCurrent::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hglrc, _hwnd, _winparams, _hwnd_del_list);
}

void gits::OpenGL::CwglMakeCurrent::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::WGL);
  HDC hdc = nullptr;
  if (_hdc.Original() != 0) {
    hdc = (HDC)UpdateHDC(_hdc, _hwnd);
  }

  if (!_hglrc.CheckMapping()) {
    // Ensure the context is created before calling wglMakeCurrent. If not, proceed to create it.
    Log(WARN) << "The context required for wglMakeCurrent has not been created. Proceeding to "
                 "create the context.";
    _hglrc.AddMapping(ptbl_wglCreateContext(hdc));
    SD().AddContext(*_hglrc, nullptr);
  }

  ptbl_wglMakeCurrent(hdc, *_hglrc);
  if (*_hglrc != 0 && _hwnd.Original() != NULL) {
    ptblInitialize(CGlDriver::API_GL);
  }

  if (_hwnd.Original() != NULL) {
    UpdateWindows(_hwnd, _winparams, _hwnd_del_list);
  }
  CStateDynamicNative::Get().MapHglrcToHdc((void*)*_hglrc, (void*)hdc);
  SD().SetCurrentContext((void*)*_hglrc);
}

/* ***************************** WGL_SHARE_LISTS *************************** */

gits::OpenGL::CwglShareLists::CwglShareLists() {}

gits::OpenGL::CwglShareLists::CwglShareLists(BOOL return_value, HGLRC hglrc1, HGLRC hglrc2)
    : _return_value(return_value), _hglrc1(hglrc1), _hglrc2(hglrc2) {
  SD().ShareContext((void*)hglrc1, (void*)hglrc2);
}

gits::CArgument& gits::OpenGL::CwglShareLists::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hglrc1, _hglrc2);
}

void gits::OpenGL::CwglShareLists::Run() {
  ptbl_wglShareLists(*_hglrc1, *_hglrc2);
  SD().ShareContext(*_hglrc1, *_hglrc2);
}

/* ***************************** WGL_USE_FONT_BITMAPS_A *************************** */

gits::OpenGL::CwglUseFontBitmapsA::CwglUseFontBitmapsA() {}

gits::OpenGL::CwglUseFontBitmapsA::CwglUseFontBitmapsA(BOOL return_value,
                                                       HDC hdc,
                                                       DWORD first,
                                                       DWORD count,
                                                       DWORD listBase,
                                                       std::string str,
                                                       std::vector<int> args)
    : _return_value(return_value),
      _hdc(hdc),
      _first(first),
      _count(count),
      _listBase(listBase),
      _hwnd(WindowFromDC(hdc)),
      _fontName(str.c_str(), '\0', 1),
      _fontArgs(args) {}

gits::CArgument& gits::OpenGL::CwglUseFontBitmapsA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _first, _count, _listBase, _hwnd, _fontName,
                       _fontArgs);
}

void gits::OpenGL::CwglUseFontBitmapsA::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HDC hdc = UpdateHDC(_hdc, _hwnd, true);
  HFONT hfont = CreateFontA((*_fontArgs)[0], (*_fontArgs)[1], (*_fontArgs)[2], (*_fontArgs)[3],
                            (*_fontArgs)[4], (*_fontArgs)[5], (*_fontArgs)[6], (*_fontArgs)[7],
                            (*_fontArgs)[8], (*_fontArgs)[9], (*_fontArgs)[10], (*_fontArgs)[11],
                            (*_fontArgs)[12], *_fontName);
  HGDIOBJ last = SelectObject(hdc, hfont);

  drv.wgl.wglUseFontBitmapsA(hdc, *_first, *_count, *_listBase);

  SelectObject(hdc, last);
  DeleteObject(hfont);
#endif
}

/* ***************************** WGL_USE_FONT_BITMAPS_W *************************** */

gits::OpenGL::CwglUseFontBitmapsW::CwglUseFontBitmapsW() {}

gits::OpenGL::CwglUseFontBitmapsW::CwglUseFontBitmapsW(BOOL return_value,
                                                       HDC hdc,
                                                       DWORD first,
                                                       DWORD count,
                                                       DWORD listBase,
                                                       std::string str,
                                                       std::vector<int> args)
    : _return_value(return_value),
      _hdc(hdc),
      _first(first),
      _count(count),
      _listBase(listBase),
      _hwnd(WindowFromDC(hdc)),
      _fontName(str.c_str(), '\0', 1),
      _fontArgs(args) {}

gits::CArgument& gits::OpenGL::CwglUseFontBitmapsW::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _first, _count, _listBase, _hwnd, _fontName,
                       _fontArgs);
}

void gits::OpenGL::CwglUseFontBitmapsW::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HDC hdc = UpdateHDC(_hdc, _hwnd, true);
  HFONT hfont = CreateFontA((*_fontArgs)[0], (*_fontArgs)[1], (*_fontArgs)[2], (*_fontArgs)[3],
                            (*_fontArgs)[4], (*_fontArgs)[5], (*_fontArgs)[6], (*_fontArgs)[7],
                            (*_fontArgs)[8], (*_fontArgs)[9], (*_fontArgs)[10], (*_fontArgs)[11],
                            (*_fontArgs)[12], *_fontName);
  HGDIOBJ last = SelectObject(hdc, hfont);

  drv.wgl.wglUseFontBitmapsW(hdc, *_first, *_count, *_listBase);

  SelectObject(hdc, last);
  DeleteObject(hfont);
#endif
}

/* ***************************** WGL_SWAP_LAYER_BUFFERS *************************** */

gits::OpenGL::CwglSwapLayerBuffers::CwglSwapLayerBuffers() {}

gits::OpenGL::CwglSwapLayerBuffers::CwglSwapLayerBuffers(BOOL return_value, HDC hdc, UINT fuPlanes)
    : _return_value(return_value), _hdc(hdc), _fuPlanes(fuPlanes), _hwnd(WindowFromDC(hdc)) {
#if defined GITS_PLATFORM_WINDOWS
  if (hdc != 0 && _hwnd.Original() != NULL) {
    UpdateWindowsRec(_hwnd.Original(), _winparams, _hwnd_del_list);
  }
#endif
}

gits::CArgument& gits::OpenGL::CwglSwapLayerBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _fuPlanes, _hwnd, _winparams, _hwnd_del_list);
}

void gits::OpenGL::CwglSwapLayerBuffers::Run() {
#if defined GITS_PLATFORM_WINDOWS
  if (_hdc.Original() != 0 && _hwnd.Original() != NULL) {
    UpdateWindows(_hwnd, _winparams, _hwnd_del_list);

    PreSwap();

    drv.wgl.wglSwapLayerBuffers(UpdateHDC(_hdc, _hwnd), *_fuPlanes);
    CALL_ONCE[&] {
      SetForegroundWindow(*_hwnd);
    };
  } else {
    drv.wgl.wglSwapLayerBuffers(0, *_fuPlanes);
  }
#endif
}

/* ***************************** WGL_SWAP_BUFFERS *************************** */
namespace {
bool CompareHDCs(HDC hdc1, HDC hdc2) {
  if (hdc1 == hdc2) {
    return true;
  }
  HWND hwnd1 = gits::OpenGL::ptblWindowFromDC(hdc1);
  HWND hwnd2 = gits::OpenGL::ptblWindowFromDC(hdc2);
  if (hwnd1 == hwnd2) {
    return true;
  }
  return false;
}
} // namespace

gits::OpenGL::CwglSwapBuffers::CwglSwapBuffers() {}

gits::OpenGL::CwglSwapBuffers::CwglSwapBuffers(BOOL return_value, HDC hdc)
    : _return_value(return_value), _hdc(hdc), _hwnd(WindowFromDC(hdc)) {
#if defined GITS_PLATFORM_WINDOWS
  if (hdc != 0 && _hwnd.Original() != NULL) {
    UpdateWindowsRec(_hwnd.Original(), _winparams, _hwnd_del_list);
  }
#endif
}

gits::CArgument& gits::OpenGL::CwglSwapBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hwnd, _winparams, _hwnd_del_list);
}

void gits::OpenGL::CwglSwapBuffers::Run() {
#ifdef GITS_PLATFORM_WINDOWS
  if (Config::Get().common.player.captureScreenshot) {
    drv.wgl.wglSwapIntervalEXT(1);
  }
#endif
  if (_hdc.Original() != 0 && _hwnd.Original() != NULL) {
    UpdateWindows(_hwnd, _winparams, _hwnd_del_list);

    //Change context if swap hdc is different than current hdc
    HGLRC currentHGLRC = ptbl_wglGetCurrentContext();
    HDC currentHDC = (HDC)CStateDynamicNative::Get().GetHdcFromHglrc(currentHGLRC);
    bool sameDC = CompareHDCs(currentHDC, UpdateHDC(_hdc, _hwnd));
    HGLRC swapHGLRC = (HGLRC)CStateDynamicNative::Get().GetHglrcFromHdc(UpdateHDC(_hdc, _hwnd));
    if (!sameDC && swapHGLRC != nullptr) {
      ptbl_wglMakeCurrent(UpdateHDC(_hdc, _hwnd), swapHGLRC);
    }

    PreSwap();
#ifdef GITS_PLATFORM_WINDOWS
    CALL_ONCE[&] {
      SetForegroundWindow(*_hwnd);
    };
#endif
    if (!sameDC && swapHGLRC != nullptr) {
      ptbl_wglMakeCurrent(currentHDC, currentHGLRC);
    }

    ptbl_wglSwapBuffers(UpdateHDC(_hdc, _hwnd));
  } else {
    ptbl_wglSwapBuffers(nullptr);
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (Config::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()] &&
      Config::Get().common.player.captureScreenshot) {
    sleep_millisec(1000);
    ScreenshotSave(CGits::Instance().CurrentFrame(), *_hwnd);
  }
#endif
}

/* ***************************** WGL_SWAP_MULTIPLE_BUFFERS *************************** */

gits::OpenGL::CwglSwapMultipleBuffers::CwglSwapMultipleBuffers() {}

gits::OpenGL::CwglSwapMultipleBuffers::CwglSwapMultipleBuffers(BOOL return_value,
                                                               GLuint buffers,
                                                               HDC* hdc)
    : _return_value(return_value), _buffers(buffers), _hdc(), _hwnd(WindowFromDC(*hdc)) {
#if defined GITS_PLATFORM_WINDOWS

  if (buffers > 1) {
    Log(WARN) << "wglSwapMultipleBuffers with more then 1 buffer used. GITS updates window params "
                 "for first buffer only";
  }

  UpdateWindowsRec(_hwnd.Original(), _winparams, _hwnd_del_list);
  for (unsigned int idx = 0; idx < *_buffers; idx++) {
    CHDC hdcobj(hdc[idx]);
    _hdc.Vector().push_back(hdcobj.Original());
  }

#endif
}

gits::CArgument& gits::OpenGL::CwglSwapMultipleBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _buffers, _hdc, _hwnd, _winparams, _hwnd_del_list);
}

void gits::OpenGL::CwglSwapMultipleBuffers::Run() {
#if defined GITS_PLATFORM_WINDOWS

  if (*_buffers > 1) {
    Log(WARN) << "wglSwapMultipleBuffers with more then 1 buffer used. GITS updates window params "
                 "for first buffer only";
  }

  CHDC xhdc(**_hdc);
  HDC hdc = UpdateHDC(xhdc, _hwnd);
  drv.wgl.wglSwapMultipleBuffers(1, (WGLSWAP*)&hdc);
#endif
}

/* ***************************** WGL_USE_FONT_OUTLINES_A *************************** */

gits::OpenGL::CwglUseFontOutlinesA::CwglUseFontOutlinesA() {}

gits::OpenGL::CwglUseFontOutlinesA::CwglUseFontOutlinesA(BOOL return_value,
                                                         HDC hdc,
                                                         DWORD first,
                                                         DWORD count,
                                                         DWORD listBase,
                                                         FLOAT deviation,
                                                         FLOAT extrusion,
                                                         int format,
                                                         LPGLYPHMETRICSFLOAT lpgmf,
                                                         std::string str,
                                                         std::vector<int> args)
    : _return_value(return_value),
      _hdc(hdc),
      _first(first),
      _count(count),
      _listBase(listBase),
      _deviation(deviation),
      _extrusion(extrusion),
      _format(format),
      _hwnd(WindowFromDC(hdc)),
      _fontName(str.c_str(), '\0', 1),
      _fontArgs(args) {}

gits::CArgument& gits::OpenGL::CwglUseFontOutlinesA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _first, _count, _listBase, _deviation, _extrusion,
                       _format, _hwnd, _fontName, _fontArgs);
}

void gits::OpenGL::CwglUseFontOutlinesA::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
  HFONT hfont = CreateFontA((*_fontArgs)[0], (*_fontArgs)[1], (*_fontArgs)[2], (*_fontArgs)[3],
                            (*_fontArgs)[4], (*_fontArgs)[5], (*_fontArgs)[6], (*_fontArgs)[7],
                            (*_fontArgs)[8], (*_fontArgs)[9], (*_fontArgs)[10], (*_fontArgs)[11],
                            (*_fontArgs)[12], *_fontName);

  HGDIOBJ last = SelectObject(hdc, hfont);
  drv.wgl.wglUseFontOutlinesA(UpdateHDC(_hdc, _hwnd, true), *_first, *_count, *_listBase,
                              *_deviation, *_extrusion, *_format, 0);
  SelectObject(hdc, last);
  DeleteObject(hfont);
#endif
}

/* ***************************** WGL_USE_FONT_OUTLINES_W *************************** */

gits::OpenGL::CwglUseFontOutlinesW::CwglUseFontOutlinesW() {}

gits::OpenGL::CwglUseFontOutlinesW::CwglUseFontOutlinesW(BOOL return_value,
                                                         HDC hdc,
                                                         DWORD first,
                                                         DWORD count,
                                                         DWORD listBase,
                                                         FLOAT deviation,
                                                         FLOAT extrusion,
                                                         int format,
                                                         LPGLYPHMETRICSFLOAT lpgmf,
                                                         std::string str,
                                                         std::vector<int> args)
    : _return_value(return_value),
      _hdc(hdc),
      _first(first),
      _count(count),
      _listBase(listBase),
      _deviation(deviation),
      _extrusion(extrusion),
      _format(format),
      _hwnd(WindowFromDC(hdc)),
      _fontName(str.c_str(), '\0', 1),
      _fontArgs(args) {}

gits::CArgument& gits::OpenGL::CwglUseFontOutlinesW::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _first, _count, _listBase, _deviation, _extrusion,
                       _format, _hwnd, _fontName, _fontArgs);
}

void gits::OpenGL::CwglUseFontOutlinesW::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
  HFONT hfont = CreateFontA((*_fontArgs)[0], (*_fontArgs)[1], (*_fontArgs)[2], (*_fontArgs)[3],
                            (*_fontArgs)[4], (*_fontArgs)[5], (*_fontArgs)[6], (*_fontArgs)[7],
                            (*_fontArgs)[8], (*_fontArgs)[9], (*_fontArgs)[10], (*_fontArgs)[11],
                            (*_fontArgs)[12], *_fontName);

  HGDIOBJ last = SelectObject(hdc, hfont);
  drv.wgl.wglUseFontOutlinesW(UpdateHDC(_hdc, _hwnd, true), *_first, *_count, *_listBase,
                              *_deviation, *_extrusion, *_format, 0);
  SelectObject(hdc, last);
  DeleteObject(hfont);
#endif
}

/* ***************************** WGL_CREATE_BUFFER_REGION_ARB *************************** */

gits::OpenGL::CwglCreateBufferRegionARB::CwglCreateBufferRegionARB() {}

gits::OpenGL::CwglCreateBufferRegionARB::CwglCreateBufferRegionARB(HANDLE return_value,
                                                                   HDC hDC,
                                                                   int iLayerPlane,
                                                                   UINT uType)
    : _return_value(return_value),
      _hDC(hDC),
      _iLayerPlane(iLayerPlane),
      _uType(uType),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglCreateBufferRegionARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iLayerPlane, _uType, _hwnd);
}

void gits::OpenGL::CwglCreateBufferRegionARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HANDLE hRegion =
      drv.wgl.wglCreateBufferRegionARB((HDC)UpdateHDC(_hDC, _hwnd), *_iLayerPlane, *_uType);
  CHANDLE::AddMapping(_return_value.Original(), hRegion);
#endif
}

/* ***************************** WGL_DELETE_BUFFER_REGION_ARB *************************** */

gits::OpenGL::CwglDeleteBufferRegionARB::CwglDeleteBufferRegionARB() {}

gits::OpenGL::CwglDeleteBufferRegionARB::CwglDeleteBufferRegionARB(HANDLE hRegion)
    : _hRegion(hRegion) {}

gits::CArgument& gits::OpenGL::CwglDeleteBufferRegionARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hRegion);
}

void gits::OpenGL::CwglDeleteBufferRegionARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDeleteBufferRegionARB(*_hRegion);
#endif
}

/* ***************************** WGL_SAVE_BUFFER_REGION_ARB *************************** */

gits::OpenGL::CwglSaveBufferRegionARB::CwglSaveBufferRegionARB() {}

gits::OpenGL::CwglSaveBufferRegionARB::CwglSaveBufferRegionARB(
    BOOL return_value, HANDLE hRegion, int x, int y, int width, int height)
    : _return_value(return_value),
      _hRegion(hRegion),
      _x(x),
      _y(y),
      _width(width),
      _height(height) {}

gits::CArgument& gits::OpenGL::CwglSaveBufferRegionARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hRegion, _x, _y, _width, _height);
}

void gits::OpenGL::CwglSaveBufferRegionARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSaveBufferRegionARB(*_hRegion, *_x, *_y, *_width, *_height);
#endif
}

/* ***************************** WGL_RESTORE_BUFFER_REGION_ARB *************************** */

gits::OpenGL::CwglRestoreBufferRegionARB::CwglRestoreBufferRegionARB() {}

gits::OpenGL::CwglRestoreBufferRegionARB::CwglRestoreBufferRegionARB(
    BOOL return_value, HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc)
    : _return_value(return_value),
      _hRegion(hRegion),
      _x(x),
      _y(y),
      _width(width),
      _height(height),
      _xSrc(xSrc),
      _ySrc(ySrc) {}

gits::CArgument& gits::OpenGL::CwglRestoreBufferRegionARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hRegion, _x, _y, _width, _height, _xSrc, _ySrc);
}

void gits::OpenGL::CwglRestoreBufferRegionARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglRestoreBufferRegionARB(*_hRegion, *_x, *_y, *_width, *_height, *_xSrc, *_ySrc);
#endif
}

/* ***************************** WGL_GET_EXTENSIONS_STRING_ARB *************************** */

gits::OpenGL::CwglGetExtensionsStringARB::CwglGetExtensionsStringARB() {}

gits::OpenGL::CwglGetExtensionsStringARB::CwglGetExtensionsStringARB(const char* return_value,
                                                                     HDC hdc)
    : _hdc(hdc), _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetExtensionsStringARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _hwnd);
}

void gits::OpenGL::CwglGetExtensionsStringARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetExtensionsStringARB((HDC)UpdateHDC(_hdc, _hwnd, true));
#endif
}

/* ***************************** WGL_GET_PIXEL_FORMAT_ATTRIBIV_ARB *************************** */

gits::OpenGL::CwglGetPixelFormatAttribivARB::CwglGetPixelFormatAttribivARB() {}

gits::OpenGL::CwglGetPixelFormatAttribivARB::CwglGetPixelFormatAttribivARB(BOOL return_value,
                                                                           HDC hdc,
                                                                           int iPixelFormat,
                                                                           int iLayerPlane,
                                                                           UINT nAttributes,
                                                                           const int* piAttributes,
                                                                           int* piValues)
    : _hdc(hdc),
      _iPixelFormat(iPixelFormat),
      _iLayerPlane(iLayerPlane),
      _nAttributes(nAttributes),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetPixelFormatAttribivARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _iPixelFormat, _iLayerPlane, _nAttributes,
                       _piAttributes, _piValues, _hwnd);
}

void gits::OpenGL::CwglGetPixelFormatAttribivARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  static std::vector<int> piAttributes;
  static std::vector<int> piValues;

  piAttributes.resize(std::max<size_t>(piAttributes.size(), *_nAttributes));
  piValues.resize(std::max<size_t>(piValues.size(), *_nAttributes));

  drv.wgl.wglGetPixelFormatAttribivARB(UpdateHDC(_hdc, _hwnd, true), *_iPixelFormat, *_iLayerPlane,
                                       *_nAttributes, *_piAttributes, *_piValues);
#endif
}

/* ***************************** WGL_GET_PIXEL_FORMAT_ATTRIBFV_ARB *************************** */

gits::OpenGL::CwglGetPixelFormatAttribfvARB::CwglGetPixelFormatAttribfvARB() {}

gits::OpenGL::CwglGetPixelFormatAttribfvARB::CwglGetPixelFormatAttribfvARB(BOOL return_value,
                                                                           HDC hdc,
                                                                           int iPixelFormat,
                                                                           int iLayerPlane,
                                                                           UINT nAttributes,
                                                                           const int* piAttributes,
                                                                           FLOAT* pfValues)
    : _hdc(hdc),
      _iPixelFormat(iPixelFormat),
      _iLayerPlane(iLayerPlane),
      _nAttributes(nAttributes),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetPixelFormatAttribfvARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _iPixelFormat, _iLayerPlane, _nAttributes,
                       _piAttributes, _pfValues, _hwnd);
}

void gits::OpenGL::CwglGetPixelFormatAttribfvARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  static std::vector<int> piAttributes;
  static std::vector<float> pfValues;

  piAttributes.resize(std::max<size_t>(piAttributes.size(), *_nAttributes));
  pfValues.resize(std::max<size_t>(pfValues.size(), *_nAttributes));

  drv.wgl.wglGetPixelFormatAttribfvARB(UpdateHDC(_hdc, _hwnd, true), *_iPixelFormat, *_iLayerPlane,
                                       *_nAttributes, *_piAttributes, *_pfValues);
#endif
}

/* ***************************** WGL_CHOOSE_PIXEL_FORMAT_ARB *************************** */

gits::OpenGL::CwglChoosePixelFormatARB::CwglChoosePixelFormatARB() {}

gits::OpenGL::CwglChoosePixelFormatARB::CwglChoosePixelFormatARB(BOOL return_value,
                                                                 HDC hdc,
                                                                 const int* piAttribIList,
                                                                 const FLOAT* pfAttribFList,
                                                                 UINT nMaxFormats,
                                                                 int* piFormats,
                                                                 UINT* nNumFormats)
    : _return_value(return_value),
      _hdc(hdc),
      _piAttribIList(piAttribIList, 0, 2),
      _pfAttribFList(pfAttribFList, 0, 2),
      _nMaxFormats(nMaxFormats),
      _piFormats(*nNumFormats, piFormats),
      _nNumFormats(1, nNumFormats),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglChoosePixelFormatARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _piAttribIList, _pfAttribFList, _nMaxFormats,
                       _piFormats, _nNumFormats, _hwnd);
}

void gits::OpenGL::CwglChoosePixelFormatARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  //This call is not played because it has no influence on stream
  //drv.wgl.wglChoosePixelFormatARB((HDC)*_hdc, *_piAttribIList, *_pfAttribFList, *_nMaxFormats, *_piFormats, *_nNumFormats);
#endif
}

/* ***************************** WGL_MAKE_CONTEXT_CURRENT_ARB *************************** */

gits::OpenGL::CwglMakeContextCurrentARB::CwglMakeContextCurrentARB() {}

gits::OpenGL::CwglMakeContextCurrentARB::CwglMakeContextCurrentARB(BOOL return_value,
                                                                   HDC hDrawDC,
                                                                   HDC hReadDC,
                                                                   HGLRC hglrc)
    : _return_value(return_value),
      _hDrawDC(hDrawDC),
      _hReadDC(hReadDC),
      _hglrc(hglrc),
      _hwnd(WindowFromDC(hDrawDC)) {}

gits::CArgument& gits::OpenGL::CwglMakeContextCurrentARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDrawDC, _hReadDC, _hglrc, _hwnd);
}

void gits::OpenGL::CwglMakeContextCurrentARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglMakeContextCurrentARB((HDC)UpdateHDC(_hDrawDC, _hwnd), (HDC)*_hReadDC, (HGLRC)*_hglrc);
#endif
}

/* ***************************** WGL_GET_CURRENT_READ_DCARB *************************** */

gits::OpenGL::CwglGetCurrentReadDCARB::CwglGetCurrentReadDCARB() {}

gits::OpenGL::CwglGetCurrentReadDCARB::CwglGetCurrentReadDCARB(HDC return_value) {}

gits::CArgument& gits::OpenGL::CwglGetCurrentReadDCARB::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetCurrentReadDCARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetCurrentReadDCARB();
#endif
}

/* ***************************** WGL_CREATE_PBUFFER_ARB *************************** */

gits::OpenGL::CwglCreatePbufferARB::CwglCreatePbufferARB() {}

gits::OpenGL::CwglCreatePbufferARB::CwglCreatePbufferARB(HPBUFFERARB return_value,
                                                         HDC hDC,
                                                         int iPixelFormat,
                                                         int iWidth,
                                                         int iHeight,
                                                         const int* piAttribList)
    : _return_value(return_value),
      _hDC(hDC),
      _iPixelFormat(iPixelFormat),
      _iWidth(iWidth),
      _iHeight(iHeight),
      _piAttribList(piAttribList, 0, 2) {
#if defined GITS_PLATFORM_WINDOWS
  GetPixelFormatParams(hDC, iPixelFormat, _attribs, _values);
#endif
}

gits::CArgument& gits::OpenGL::CwglCreatePbufferARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iPixelFormat, _iWidth, _iHeight, _piAttribList,
                       _attribs, _values);
}

void gits::OpenGL::CwglCreatePbufferARB::Run() {
  // If wglCreatePbufferARB uses unknown HDC, add one to map.
  if (!CHDC::CheckMapping(_hDC.Original())) {
    HDC curr_hdc = ptblGetDC(nullptr);
    CHDC::AddMapping(_hDC.Original(), curr_hdc);
  }
  HPBUFFERARB hpbuffer =
      ptbl_wglCreatePbufferARB((HDC)*_hDC, SelectPixelFormat((HDC)*_hDC, _attribs, _values),
                               *_iWidth, *_iHeight, *_piAttribList);
  CHPBUFFERARB::AddMapping(_return_value.Original(), hpbuffer);
}

/* ***************************** WGL_GET_PBUFFER_DCARB *************************** */

gits::OpenGL::CwglGetPbufferDCARB::CwglGetPbufferDCARB() {}

gits::OpenGL::CwglGetPbufferDCARB::CwglGetPbufferDCARB(HDC return_value, HPBUFFERARB hPbuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer) {}

gits::CArgument& gits::OpenGL::CwglGetPbufferDCARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer);
}

void gits::OpenGL::CwglGetPbufferDCARB::Run() {
  HDC hdc = ptbl_wglGetPbufferDCARB((HPBUFFERARB)*_hPbuffer);
  CHDC::AddMapping(_return_value.Original(), hdc);
}

/* ***************************** WGL_RELEASE_PBUFFER_DCARB *************************** */

gits::OpenGL::CwglReleasePbufferDCARB::CwglReleasePbufferDCARB() {}

gits::OpenGL::CwglReleasePbufferDCARB::CwglReleasePbufferDCARB(int return_value,
                                                               HPBUFFERARB hPbuffer,
                                                               HDC hDC)
    : _return_value(return_value), _hPbuffer(hPbuffer), _hDC(hDC) {}

gits::CArgument& gits::OpenGL::CwglReleasePbufferDCARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _hDC);
}

void gits::OpenGL::CwglReleasePbufferDCARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  if (CHDC::CheckMapping(_hDC.Original())) {
    drv.wgl.wglReleasePbufferDCARB((HPBUFFERARB)*_hPbuffer, (HDC)*_hDC);
  }
#endif
}

/* ***************************** WGL_DESTROY_PBUFFER_ARB *************************** */

gits::OpenGL::CwglDestroyPbufferARB::CwglDestroyPbufferARB() {}

gits::OpenGL::CwglDestroyPbufferARB::CwglDestroyPbufferARB(BOOL return_value, HPBUFFERARB hPbuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer) {}

gits::CArgument& gits::OpenGL::CwglDestroyPbufferARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer);
}

void gits::OpenGL::CwglDestroyPbufferARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDestroyPbufferARB((HPBUFFERARB)*_hPbuffer);
#endif
}

/* ***************************** WGL_QUERY_PBUFFER_ARB *************************** */

gits::OpenGL::CwglQueryPbufferARB::CwglQueryPbufferARB() {}

gits::OpenGL::CwglQueryPbufferARB::CwglQueryPbufferARB(BOOL return_value,
                                                       HPBUFFERARB hPbuffer,
                                                       int iAttribute,
                                                       int* piValue)
    : _hPbuffer(hPbuffer), _iAttribute(iAttribute) {}

gits::CArgument& gits::OpenGL::CwglQueryPbufferARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _iAttribute, _piValue);
}

void gits::OpenGL::CwglQueryPbufferARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryPbufferARB(*_hPbuffer, *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_BIND_TEX_IMAGE_ARB *************************** */

gits::OpenGL::CwglBindTexImageARB::CwglBindTexImageARB() {}

gits::OpenGL::CwglBindTexImageARB::CwglBindTexImageARB(BOOL return_value,
                                                       HPBUFFERARB hPbuffer,
                                                       int iBuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer), _iBuffer(iBuffer) {}

gits::CArgument& gits::OpenGL::CwglBindTexImageARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _iBuffer);
}

void gits::OpenGL::CwglBindTexImageARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBindTexImageARB((HPBUFFERARB)*_hPbuffer, *_iBuffer);
#endif
}

/* ***************************** WGL_RELEASE_TEX_IMAGE_ARB *************************** */

gits::OpenGL::CwglReleaseTexImageARB::CwglReleaseTexImageARB() {}

gits::OpenGL::CwglReleaseTexImageARB::CwglReleaseTexImageARB(BOOL return_value,
                                                             HPBUFFERARB hPbuffer,
                                                             int iBuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer), _iBuffer(iBuffer) {}

gits::CArgument& gits::OpenGL::CwglReleaseTexImageARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _iBuffer);
}

void gits::OpenGL::CwglReleaseTexImageARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglReleaseTexImageARB((HPBUFFERARB)*_hPbuffer, *_iBuffer);
#endif
}

/* ***************************** WGL_SET_PBUFFER_ATTRIB_ARB *************************** */

gits::OpenGL::CwglSetPbufferAttribARB::CwglSetPbufferAttribARB() {}

gits::OpenGL::CwglSetPbufferAttribARB::CwglSetPbufferAttribARB(BOOL return_value,
                                                               HPBUFFERARB hPbuffer,
                                                               const int* piAttribList)
    : _return_value(return_value), _hPbuffer(hPbuffer), _piAttribList(piAttribList, 0, 2) {}

gits::CArgument& gits::OpenGL::CwglSetPbufferAttribARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _piAttribList);
}

void gits::OpenGL::CwglSetPbufferAttribARB::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSetPbufferAttribARB((HPBUFFERARB)*_hPbuffer, *_piAttribList);
#endif
}

/* ***************************** WGL_CREATE_CONTEXT_ATTRIBS_ARB *************************** */

gits::OpenGL::CwglCreateContextAttribsARB::CwglCreateContextAttribsARB() {}

gits::OpenGL::CwglCreateContextAttribsARB::CwglCreateContextAttribsARB(HGLRC return_value,
                                                                       HDC hDC,
                                                                       HGLRC hShareContext,
                                                                       const int* attribList)
    : _return_value(return_value),
      _hDC(hDC),
      _hShareContext(hShareContext),
      _attribList(attribList, 0, 2),
      _hwnd(WindowFromDC(hDC)) {
  SD().AddContext(return_value, hShareContext);
}

gits::CArgument& gits::OpenGL::CwglCreateContextAttribsARB::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _hShareContext, _attribList, _hwnd);
}

void gits::OpenGL::CwglCreateContextAttribsARB::Run() {
  HGLRC hglrc =
      ptbl_wglCreateContextAttribsARB((HDC)UpdateHDC(_hDC, _hwnd), *_hShareContext, *_attribList);

  CHGLRC::AddMapping(_return_value.Original(), hglrc);
  SD().AddContext(hglrc, *_hShareContext);
}

/* ***************************** WGL_CREATE_DISPLAY_COLOR_TABLE_EXT *************************** */

gits::OpenGL::CwglCreateDisplayColorTableEXT::CwglCreateDisplayColorTableEXT() {}

gits::OpenGL::CwglCreateDisplayColorTableEXT::CwglCreateDisplayColorTableEXT(GLboolean return_value,
                                                                             GLushort id)
    : _return_value(return_value), _id(id) {}

gits::CArgument& gits::OpenGL::CwglCreateDisplayColorTableEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _id);
}

void gits::OpenGL::CwglCreateDisplayColorTableEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglCreateDisplayColorTableEXT(*_id);
#endif
}

/* ***************************** WGL_LOAD_DISPLAY_COLOR_TABLE_EXT *************************** */

gits::OpenGL::CwglLoadDisplayColorTableEXT::CwglLoadDisplayColorTableEXT() {}

gits::OpenGL::CwglLoadDisplayColorTableEXT::CwglLoadDisplayColorTableEXT(GLboolean return_value,
                                                                         const GLushort* table,
                                                                         GLuint length)
    : _return_value(return_value), _table(length * 3, table), _length(length) {}

gits::CArgument& gits::OpenGL::CwglLoadDisplayColorTableEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _table, _length);
}

void gits::OpenGL::CwglLoadDisplayColorTableEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglLoadDisplayColorTableEXT(*_table, *_length);
#endif
}

/* ***************************** WGL_BIND_DISPLAY_COLOR_TABLE_EXT *************************** */

gits::OpenGL::CwglBindDisplayColorTableEXT::CwglBindDisplayColorTableEXT() {}

gits::OpenGL::CwglBindDisplayColorTableEXT::CwglBindDisplayColorTableEXT(GLboolean return_value,
                                                                         GLushort id)
    : _return_value(return_value), _id(id) {}

gits::CArgument& gits::OpenGL::CwglBindDisplayColorTableEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _id);
}

void gits::OpenGL::CwglBindDisplayColorTableEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBindDisplayColorTableEXT(*_id);
#endif
}

/* ***************************** WGL_DESTROY_DISPLAY_COLOR_TABLE_EXT *************************** */

gits::OpenGL::CwglDestroyDisplayColorTableEXT::CwglDestroyDisplayColorTableEXT() {}

gits::OpenGL::CwglDestroyDisplayColorTableEXT::CwglDestroyDisplayColorTableEXT(GLushort id)
    : _id(id) {}

gits::CArgument& gits::OpenGL::CwglDestroyDisplayColorTableEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _id);
}

void gits::OpenGL::CwglDestroyDisplayColorTableEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDestroyDisplayColorTableEXT(*_id);
#endif
}

/* ***************************** WGL_GET_EXTENSIONS_STRING_EXT *************************** */

gits::OpenGL::CwglGetExtensionsStringEXT::CwglGetExtensionsStringEXT() {}

gits::OpenGL::CwglGetExtensionsStringEXT::CwglGetExtensionsStringEXT(const char* return_value)
    : _return_value(return_value, 0, 1) {}

gits::CArgument& gits::OpenGL::CwglGetExtensionsStringEXT::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetExtensionsStringEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetExtensionsStringEXT();
#endif
}

/* ***************************** WGL_MAKE_CONTEXT_CURRENT_EXT *************************** */

gits::OpenGL::CwglMakeContextCurrentEXT::CwglMakeContextCurrentEXT() {}

gits::OpenGL::CwglMakeContextCurrentEXT::CwglMakeContextCurrentEXT(BOOL return_value,
                                                                   HDC hDrawDC,
                                                                   HDC hReadDC,
                                                                   HGLRC hglrc)
    : _return_value(return_value),
      _hDrawDC(hDrawDC),
      _hReadDC(hReadDC),
      _hglrc(hglrc),
      _hwnd(WindowFromDC(hDrawDC)) {}

gits::CArgument& gits::OpenGL::CwglMakeContextCurrentEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDrawDC, _hReadDC, _hglrc, _hwnd);
}

void gits::OpenGL::CwglMakeContextCurrentEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglMakeContextCurrentEXT((HDC)UpdateHDC(_hDrawDC, _hwnd), (HDC)*_hReadDC, (HGLRC)*_hglrc);
#endif
}

/* ***************************** WGL_GET_CURRENT_READ_DCEXT *************************** */

gits::OpenGL::CwglGetCurrentReadDCEXT::CwglGetCurrentReadDCEXT() {}

gits::OpenGL::CwglGetCurrentReadDCEXT::CwglGetCurrentReadDCEXT(HDC return_value) {}

gits::CArgument& gits::OpenGL::CwglGetCurrentReadDCEXT::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetCurrentReadDCEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetCurrentReadDCEXT();
#endif
}

/* ***************************** WGL_CREATE_PBUFFER_EXT *************************** */

gits::OpenGL::CwglCreatePbufferEXT::CwglCreatePbufferEXT() {}

gits::OpenGL::CwglCreatePbufferEXT::CwglCreatePbufferEXT(HPBUFFEREXT return_value,
                                                         HDC hDC,
                                                         int iPixelFormat,
                                                         int iWidth,
                                                         int iHeight,
                                                         const int* piAttribList)
    : _return_value(return_value),
      _hDC(hDC),
      _iPixelFormat(iPixelFormat),
      _iWidth(iWidth),
      _iHeight(iHeight),
      _piAttribList(piAttribList, 0, 2) {}

gits::CArgument& gits::OpenGL::CwglCreatePbufferEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iPixelFormat, _iWidth, _iHeight, _piAttribList,
                       _attribs, _values);
}

void gits::OpenGL::CwglCreatePbufferEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS

  //If wglCreatePbufferEXT uses unknown HDC add one to map
  if (!CHDC::CheckMapping(_hDC.Original())) {
    HDC curr_hdc = ptblGetDC(0);
    CHDC::AddMapping(_hDC.Original(), curr_hdc);
  }

  HPBUFFEREXT hpbuffer =
      drv.wgl.wglCreatePbufferEXT((HDC)*_hDC, SelectPixelFormat((HDC)*_hDC, _attribs, _values),
                                  *_iWidth, *_iHeight, *_piAttribList);
  CHPBUFFEREXT::AddMapping(_return_value.Original(), hpbuffer);
#else
  Log(ERR) << "" << Name() << "portability not implemented.";
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

/* ***************************** WGL_GET_PBUFFER_DCEXT *************************** */

gits::OpenGL::CwglGetPbufferDCEXT::CwglGetPbufferDCEXT() {}

gits::OpenGL::CwglGetPbufferDCEXT::CwglGetPbufferDCEXT(HDC return_value, HPBUFFEREXT hPbuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer) {}

gits::CArgument& gits::OpenGL::CwglGetPbufferDCEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer);
}

void gits::OpenGL::CwglGetPbufferDCEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HDC hdc = drv.wgl.wglGetPbufferDCEXT((HPBUFFEREXT)*_hPbuffer);
  CHDC::AddMapping(_return_value.Original(), hdc);
#endif
}

/* ***************************** WGL_RELEASE_PBUFFER_DCEXT *************************** */

gits::OpenGL::CwglReleasePbufferDCEXT::CwglReleasePbufferDCEXT() {}

gits::OpenGL::CwglReleasePbufferDCEXT::CwglReleasePbufferDCEXT(int return_value,
                                                               HPBUFFEREXT hPbuffer,
                                                               HDC hDC)
    : _return_value(return_value), _hPbuffer(hPbuffer), _hDC(hDC) {}

gits::CArgument& gits::OpenGL::CwglReleasePbufferDCEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _hDC);
}

void gits::OpenGL::CwglReleasePbufferDCEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglReleasePbufferDCEXT((HPBUFFEREXT)*_hPbuffer, (HDC)*_hDC);
#endif
}

/* ***************************** WGL_DESTROY_PBUFFER_EXT *************************** */

gits::OpenGL::CwglDestroyPbufferEXT::CwglDestroyPbufferEXT() {}

gits::OpenGL::CwglDestroyPbufferEXT::CwglDestroyPbufferEXT(BOOL return_value, HPBUFFEREXT hPbuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer) {}

gits::CArgument& gits::OpenGL::CwglDestroyPbufferEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer);
}

void gits::OpenGL::CwglDestroyPbufferEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDestroyPbufferEXT((HPBUFFEREXT)*_hPbuffer);
#endif
}

/* ***************************** WGL_QUERY_PBUFFER_EXT *************************** */

gits::OpenGL::CwglQueryPbufferEXT::CwglQueryPbufferEXT() {}

gits::OpenGL::CwglQueryPbufferEXT::CwglQueryPbufferEXT(BOOL return_value,
                                                       HPBUFFEREXT hPbuffer,
                                                       int iAttribute,
                                                       int* piValue)
    : _hPbuffer(hPbuffer), _iAttribute(iAttribute) {}

gits::CArgument& gits::OpenGL::CwglQueryPbufferEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _iAttribute, _piValue);
}

void gits::OpenGL::CwglQueryPbufferEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryPbufferEXT(*_hPbuffer, *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_GET_PIXEL_FORMAT_ATTRIBIV_EXT *************************** */

gits::OpenGL::CwglGetPixelFormatAttribivEXT::CwglGetPixelFormatAttribivEXT() {}

gits::OpenGL::CwglGetPixelFormatAttribivEXT::CwglGetPixelFormatAttribivEXT(BOOL return_value,
                                                                           HDC hdc,
                                                                           int iPixelFormat,
                                                                           int iLayerPlane,
                                                                           UINT nAttributes,
                                                                           int* piAttributes,
                                                                           int* piValues)
    : _hdc(hdc),
      _iPixelFormat(iPixelFormat),
      _iLayerPlane(iLayerPlane),
      _nAttributes(nAttributes),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetPixelFormatAttribivEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _iPixelFormat, _iLayerPlane, _nAttributes,
                       _piAttributes, _piValues, _hwnd);
}

void gits::OpenGL::CwglGetPixelFormatAttribivEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetPixelFormatAttribivEXT(UpdateHDC(_hdc, _hwnd, true), *_iPixelFormat, *_iLayerPlane,
                                       *_nAttributes, *_piAttributes, *_piValues);
#endif
}

/* ***************************** WGL_GET_PIXEL_FORMAT_ATTRIBFV_EXT *************************** */

gits::OpenGL::CwglGetPixelFormatAttribfvEXT::CwglGetPixelFormatAttribfvEXT() {}

gits::OpenGL::CwglGetPixelFormatAttribfvEXT::CwglGetPixelFormatAttribfvEXT(BOOL return_value,
                                                                           HDC hdc,
                                                                           int iPixelFormat,
                                                                           int iLayerPlane,
                                                                           UINT nAttributes,
                                                                           int* piAttributes,
                                                                           FLOAT* pfValues)
    : _hdc(hdc),
      _iPixelFormat(iPixelFormat),
      _iLayerPlane(iLayerPlane),
      _nAttributes(nAttributes),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetPixelFormatAttribfvEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _iPixelFormat, _iLayerPlane, _nAttributes,
                       _piAttributes, _pfValues, _hwnd);
}

void gits::OpenGL::CwglGetPixelFormatAttribfvEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetPixelFormatAttribfvEXT((HDC)UpdateHDC(_hdc, _hwnd, true), *_iPixelFormat,
                                       *_iLayerPlane, *_nAttributes, *_piAttributes, *_pfValues);
#endif
}

/* ***************************** WGL_CHOOSE_PIXEL_FORMAT_EXT *************************** */

gits::OpenGL::CwglChoosePixelFormatEXT::CwglChoosePixelFormatEXT() {}

gits::OpenGL::CwglChoosePixelFormatEXT::CwglChoosePixelFormatEXT(BOOL return_value,
                                                                 HDC hdc,
                                                                 const int* piAttribIList,
                                                                 const FLOAT* pfAttribFList,
                                                                 UINT nMaxFormats,
                                                                 int* piFormats,
                                                                 UINT* nNumFormats)
    : _hdc(hdc),
      _piAttribIList(piAttribIList, 0, 2),
      _pfAttribFList(pfAttribFList, 0, 2),
      _nMaxFormats(nMaxFormats),
      _piFormats(*nNumFormats, piFormats),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglChoosePixelFormatEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _piAttribIList, _pfAttribFList, _nMaxFormats,
                       _piFormats, _hwnd);
}

void gits::OpenGL::CwglChoosePixelFormatEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  UINT nNumFormats = 0;
  //If wglChoosePixelFormatEXT uses unknown HDC add one to map
  if (!CHDC::CheckMapping(_hdc.Original())) {
    HDC curr_hdc = ptblGetDC(0);
    CHDC::AddMapping(_hdc.Original(), curr_hdc);
  }

  drv.wgl.wglChoosePixelFormatEXT((HDC)UpdateHDC(_hdc, _hwnd), *_piAttribIList, *_pfAttribFList,
                                  *_nMaxFormats, *_piFormats, &nNumFormats);
#else
  Log(ERR) << "" << Name() << "wglCreatePbufferEXT portability not implemented.";
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

/* ***************************** WGL_SWAP_INTERVAL_EXT *************************** */

gits::OpenGL::CwglSwapIntervalEXT::CwglSwapIntervalEXT() {}

gits::OpenGL::CwglSwapIntervalEXT::CwglSwapIntervalEXT(BOOL return_value, int interval)
    : _return_value(return_value), _interval(interval) {}

gits::CArgument& gits::OpenGL::CwglSwapIntervalEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _interval);
}

void gits::OpenGL::CwglSwapIntervalEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSwapIntervalEXT(*_interval);
#endif
}

/* ***************************** WGL_GET_SWAP_INTERVAL_EXT *************************** */

gits::OpenGL::CwglGetSwapIntervalEXT::CwglGetSwapIntervalEXT() {}

gits::OpenGL::CwglGetSwapIntervalEXT::CwglGetSwapIntervalEXT(int return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CwglGetSwapIntervalEXT::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetSwapIntervalEXT::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetSwapIntervalEXT();
#endif
}

/* ***************************** WGL_ALLOCATE_MEMORY_NV *************************** */

gits::OpenGL::CwglAllocateMemoryNV::CwglAllocateMemoryNV() {}

gits::OpenGL::CwglAllocateMemoryNV::CwglAllocateMemoryNV(
    void* return_value, GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority)
    : _return_value(nullptr),
      _size(size),
      _readfreq(readfreq),
      _writefreq(writefreq),
      _priority(priority) {}

gits::CArgument& gits::OpenGL::CwglAllocateMemoryNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _size, _readfreq, _writefreq, _priority);
}

void gits::OpenGL::CwglAllocateMemoryNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglAllocateMemoryNV(*_size, *_readfreq, *_writefreq, *_priority);
#endif
}

/* ***************************** WGL_FREE_MEMORY_NV *************************** */

gits::OpenGL::CwglFreeMemoryNV::CwglFreeMemoryNV() {}

gits::OpenGL::CwglFreeMemoryNV::CwglFreeMemoryNV(void* pointer) : _pointer(nullptr) {}

gits::CArgument& gits::OpenGL::CwglFreeMemoryNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _pointer);
}

void gits::OpenGL::CwglFreeMemoryNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglFreeMemoryNV(*_pointer);
#endif
}

/* ***************************** WGL_GET_SYNC_VALUES_OML *************************** */

gits::OpenGL::CwglGetSyncValuesOML::CwglGetSyncValuesOML() {}

gits::OpenGL::CwglGetSyncValuesOML::CwglGetSyncValuesOML(
    BOOL return_value, HDC hdc, INT64* ust, INT64* msc, INT64* sbc)
    : _return_value(return_value),
      _hdc(hdc),
      _ust(1, ust),
      _msc(1, msc),
      _sbc(1, sbc),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetSyncValuesOML::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _ust, _msc, _sbc, _hwnd);
}

void gits::OpenGL::CwglGetSyncValuesOML::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetSyncValuesOML((HDC)UpdateHDC(_hdc, _hwnd), *_ust, *_msc, *_sbc);
#endif
}

/* ***************************** WGL_GET_MSC_RATE_OML *************************** */

gits::OpenGL::CwglGetMscRateOML::CwglGetMscRateOML() {}

gits::OpenGL::CwglGetMscRateOML::CwglGetMscRateOML(BOOL return_value,
                                                   HDC hdc,
                                                   INT32* numerator,
                                                   INT32* denominator)
    : _return_value(return_value),
      _hdc(hdc),
      _numerator(1, numerator),
      _denominator(1, denominator),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglGetMscRateOML::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _numerator, _denominator, _hwnd);
}

void gits::OpenGL::CwglGetMscRateOML::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetMscRateOML((HDC)UpdateHDC(_hdc, _hwnd), *_numerator, *_denominator);
#endif
}

/* ***************************** WGL_SWAP_BUFFERS_MSC_OML *************************** */

gits::OpenGL::CwglSwapBuffersMscOML::CwglSwapBuffersMscOML() {}

gits::OpenGL::CwglSwapBuffersMscOML::CwglSwapBuffersMscOML(
    INT64 return_value, HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder)
    : _return_value(return_value),
      _hdc(hdc),
      _target_msc(target_msc),
      _divisor(divisor),
      _remainder(remainder),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglSwapBuffersMscOML::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _target_msc, _divisor, _remainder, _hwnd);
}

void gits::OpenGL::CwglSwapBuffersMscOML::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSwapBuffersMscOML((HDC)UpdateHDC(_hdc, _hwnd), *_target_msc, *_divisor, *_remainder);
#endif
}

/* ***************************** WGL_SWAP_LAYER_BUFFERS_MSC_OML *************************** */

gits::OpenGL::CwglSwapLayerBuffersMscOML::CwglSwapLayerBuffersMscOML() {}

gits::OpenGL::CwglSwapLayerBuffersMscOML::CwglSwapLayerBuffersMscOML(
    INT64 return_value, HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder)
    : _return_value(return_value),
      _hdc(hdc),
      _fuPlanes(fuPlanes),
      _target_msc(target_msc),
      _divisor(divisor),
      _remainder(remainder),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglSwapLayerBuffersMscOML::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _fuPlanes, _target_msc, _divisor, _remainder,
                       _hwnd);
}

void gits::OpenGL::CwglSwapLayerBuffersMscOML::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSwapLayerBuffersMscOML((HDC)UpdateHDC(_hdc, _hwnd), *_fuPlanes, *_target_msc,
                                    *_divisor, *_remainder);
#endif
}

/* ***************************** WGL_WAIT_FOR_MSC_OML *************************** */

gits::OpenGL::CwglWaitForMscOML::CwglWaitForMscOML() {}

gits::OpenGL::CwglWaitForMscOML::CwglWaitForMscOML(BOOL return_value,
                                                   HDC hdc,
                                                   INT64 target_msc,
                                                   INT64 divisor,
                                                   INT64 remainder,
                                                   INT64* ust,
                                                   INT64* msc,
                                                   INT64* sbc)
    : _return_value(return_value),
      _hdc(hdc),
      _target_msc(target_msc),
      _divisor(divisor),
      _remainder(remainder),
      _ust(1, ust),
      _msc(1, msc),
      _sbc(1, sbc),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglWaitForMscOML::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _target_msc, _divisor, _remainder, _ust, _msc, _sbc,
                       _hwnd);
}

void gits::OpenGL::CwglWaitForMscOML::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglWaitForMscOML((HDC)UpdateHDC(_hdc, _hwnd), *_target_msc, *_divisor, *_remainder, *_ust,
                           *_msc, *_sbc);
#endif
}

/* ***************************** WGL_WAIT_FOR_SBC_OML *************************** */

gits::OpenGL::CwglWaitForSbcOML::CwglWaitForSbcOML() {}

gits::OpenGL::CwglWaitForSbcOML::CwglWaitForSbcOML(
    BOOL return_value, HDC hdc, INT64 target_sbc, INT64* ust, INT64* msc, INT64* sbc)
    : _return_value(return_value),
      _hdc(hdc),
      _target_sbc(target_sbc),
      _ust(1, ust),
      _msc(1, msc),
      _sbc(1, sbc),
      _hwnd(WindowFromDC(hdc)) {}

gits::CArgument& gits::OpenGL::CwglWaitForSbcOML::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc, _target_sbc, _ust, _msc, _sbc, _hwnd);
}

void gits::OpenGL::CwglWaitForSbcOML::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglWaitForSbcOML((HDC)UpdateHDC(_hdc, _hwnd), *_target_sbc, *_ust, *_msc, *_sbc);
#endif
}

/* ***************************** WGL_GET_DIGITAL_VIDEO_PARAMETERS_I3D *************************** */

gits::OpenGL::CwglGetDigitalVideoParametersI3D::CwglGetDigitalVideoParametersI3D() {}

gits::OpenGL::CwglGetDigitalVideoParametersI3D::CwglGetDigitalVideoParametersI3D(BOOL return_value,
                                                                                 HDC hDC,
                                                                                 int iAttribute,
                                                                                 int* piValue)
    : _return_value(return_value),
      _hDC(hDC),
      _iAttribute(iAttribute),
      _piValue(1, piValue),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetDigitalVideoParametersI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iAttribute, _piValue, _hwnd);
}

void gits::OpenGL::CwglGetDigitalVideoParametersI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetDigitalVideoParametersI3D((HDC)UpdateHDC(_hDC, _hwnd), *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_SET_DIGITAL_VIDEO_PARAMETERS_I3D *************************** */

gits::OpenGL::CwglSetDigitalVideoParametersI3D::CwglSetDigitalVideoParametersI3D() {}

gits::OpenGL::CwglSetDigitalVideoParametersI3D::CwglSetDigitalVideoParametersI3D(BOOL return_value,
                                                                                 HDC hDC,
                                                                                 int iAttribute,
                                                                                 const int* piValue)
    : _return_value(return_value),
      _hDC(hDC),
      _iAttribute(iAttribute),
      _piValue(1, piValue),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglSetDigitalVideoParametersI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iAttribute, _piValue, _hwnd);
}

void gits::OpenGL::CwglSetDigitalVideoParametersI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSetDigitalVideoParametersI3D((HDC)UpdateHDC(_hDC, _hwnd), *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_GET_GAMMA_TABLE_PARAMETERS_I3D *************************** */

gits::OpenGL::CwglGetGammaTableParametersI3D::CwglGetGammaTableParametersI3D() {}

gits::OpenGL::CwglGetGammaTableParametersI3D::CwglGetGammaTableParametersI3D(BOOL return_value,
                                                                             HDC hDC,
                                                                             int iAttribute,
                                                                             int* piValue)
    : _return_value(return_value),
      _hDC(hDC),
      _iAttribute(iAttribute),
      _piValue(1, piValue),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetGammaTableParametersI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iAttribute, _piValue, _hwnd);
}

void gits::OpenGL::CwglGetGammaTableParametersI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGammaTableParametersI3D((HDC)UpdateHDC(_hDC, _hwnd), *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_SET_GAMMA_TABLE_PARAMETERS_I3D *************************** */

gits::OpenGL::CwglSetGammaTableParametersI3D::CwglSetGammaTableParametersI3D() {}

gits::OpenGL::CwglSetGammaTableParametersI3D::CwglSetGammaTableParametersI3D(BOOL return_value,
                                                                             HDC hDC,
                                                                             int iAttribute,
                                                                             const int* piValue)
    : _return_value(return_value),
      _hDC(hDC),
      _iAttribute(iAttribute),
      _piValue(1, piValue),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglSetGammaTableParametersI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iAttribute, _piValue, _hwnd);
}

void gits::OpenGL::CwglSetGammaTableParametersI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSetGammaTableParametersI3D((HDC)UpdateHDC(_hDC, _hwnd), *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_GET_GAMMA_TABLE_I3D *************************** */

gits::OpenGL::CwglGetGammaTableI3D::CwglGetGammaTableI3D() {}

gits::OpenGL::CwglGetGammaTableI3D::CwglGetGammaTableI3D(
    BOOL return_value, HDC hDC, int iEntries, USHORT* puRed, USHORT* puGreen, USHORT* puBlue)
    : _return_value(return_value),
      _hDC(hDC),
      _iEntries(iEntries),
      _puRed(iEntries, puRed),
      _puGreen(iEntries, puGreen),
      _puBlue(iEntries, puBlue),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetGammaTableI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iEntries, _puRed, _puGreen, _puBlue, _hwnd);
}

void gits::OpenGL::CwglGetGammaTableI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGammaTableI3D((HDC)UpdateHDC(_hDC, _hwnd), *_iEntries, *_puRed, *_puGreen,
                              *_puBlue);
#endif
}

/* ***************************** WGL_SET_GAMMA_TABLE_I3D *************************** */

gits::OpenGL::CwglSetGammaTableI3D::CwglSetGammaTableI3D() {}

gits::OpenGL::CwglSetGammaTableI3D::CwglSetGammaTableI3D(BOOL return_value,
                                                         HDC hDC,
                                                         int iEntries,
                                                         const USHORT* puRed,
                                                         const USHORT* puGreen,
                                                         const USHORT* puBlue)
    : _return_value(return_value),
      _hDC(hDC),
      _iEntries(iEntries),
      _puRed(iEntries, puRed),
      _puGreen(iEntries, puGreen),
      _puBlue(iEntries, puBlue),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglSetGammaTableI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _iEntries, _puRed, _puGreen, _puBlue, _hwnd);
}

void gits::OpenGL::CwglSetGammaTableI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSetGammaTableI3D((HDC)UpdateHDC(_hDC, _hwnd), *_iEntries, *_puRed, *_puGreen,
                              *_puBlue);
#endif
}

/* ***************************** WGL_ENABLE_GENLOCK_I3D *************************** */

gits::OpenGL::CwglEnableGenlockI3D::CwglEnableGenlockI3D() {}

gits::OpenGL::CwglEnableGenlockI3D::CwglEnableGenlockI3D(BOOL return_value, HDC hDC)
    : _return_value(return_value), _hDC(hDC), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglEnableGenlockI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _hwnd);
}

void gits::OpenGL::CwglEnableGenlockI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEnableGenlockI3D((HDC)UpdateHDC(_hDC, _hwnd));
#endif
}

/* ***************************** WGL_DISABLE_GENLOCK_I3D *************************** */

gits::OpenGL::CwglDisableGenlockI3D::CwglDisableGenlockI3D() {}

gits::OpenGL::CwglDisableGenlockI3D::CwglDisableGenlockI3D(BOOL return_value, HDC hDC)
    : _return_value(return_value), _hDC(hDC), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglDisableGenlockI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _hwnd);
}

void gits::OpenGL::CwglDisableGenlockI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDisableGenlockI3D((HDC)UpdateHDC(_hDC, _hwnd));
#endif
}

/* ***************************** WGL_IS_ENABLED_GENLOCK_I3D *************************** */

gits::OpenGL::CwglIsEnabledGenlockI3D::CwglIsEnabledGenlockI3D() {}

gits::OpenGL::CwglIsEnabledGenlockI3D::CwglIsEnabledGenlockI3D(BOOL return_value,
                                                               HDC hDC,
                                                               BOOL* pFlag)
    : _return_value(return_value), _hDC(hDC), _pFlag(1, pFlag), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglIsEnabledGenlockI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _pFlag, _hwnd);
}

void gits::OpenGL::CwglIsEnabledGenlockI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglIsEnabledGenlockI3D((HDC)UpdateHDC(_hDC, _hwnd), *_pFlag);
#endif
}

/* ***************************** WGL_GENLOCK_SOURCE_I3D *************************** */

gits::OpenGL::CwglGenlockSourceI3D::CwglGenlockSourceI3D() {}

gits::OpenGL::CwglGenlockSourceI3D::CwglGenlockSourceI3D(BOOL return_value, HDC hDC, UINT uSource)
    : _return_value(return_value), _hDC(hDC), _uSource(uSource), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGenlockSourceI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uSource, _hwnd);
}

void gits::OpenGL::CwglGenlockSourceI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGenlockSourceI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uSource);
#endif
}

/* ***************************** WGL_GET_GENLOCK_SOURCE_I3D *************************** */

gits::OpenGL::CwglGetGenlockSourceI3D::CwglGetGenlockSourceI3D() {}

gits::OpenGL::CwglGetGenlockSourceI3D::CwglGetGenlockSourceI3D(BOOL return_value,
                                                               HDC hDC,
                                                               UINT* uSource)
    : _return_value(return_value), _hDC(hDC), _uSource(1, uSource), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetGenlockSourceI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uSource, _hwnd);
}

void gits::OpenGL::CwglGetGenlockSourceI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGenlockSourceI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uSource);
#endif
}

/* ***************************** WGL_GENLOCK_SOURCE_EDGE_I3D *************************** */

gits::OpenGL::CwglGenlockSourceEdgeI3D::CwglGenlockSourceEdgeI3D() {}

gits::OpenGL::CwglGenlockSourceEdgeI3D::CwglGenlockSourceEdgeI3D(BOOL return_value,
                                                                 HDC hDC,
                                                                 UINT uEdge)
    : _return_value(return_value), _hDC(hDC), _uEdge(uEdge), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGenlockSourceEdgeI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uEdge, _hwnd);
}

void gits::OpenGL::CwglGenlockSourceEdgeI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGenlockSourceEdgeI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uEdge);
#endif
}

/* ***************************** WGL_GET_GENLOCK_SOURCE_EDGE_I3D *************************** */

gits::OpenGL::CwglGetGenlockSourceEdgeI3D::CwglGetGenlockSourceEdgeI3D() {}

gits::OpenGL::CwglGetGenlockSourceEdgeI3D::CwglGetGenlockSourceEdgeI3D(BOOL return_value,
                                                                       HDC hDC,
                                                                       UINT* uEdge)
    : _return_value(return_value), _hDC(hDC), _uEdge(1, uEdge), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetGenlockSourceEdgeI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uEdge, _hwnd);
}

void gits::OpenGL::CwglGetGenlockSourceEdgeI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGenlockSourceEdgeI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uEdge);
#endif
}

/* ***************************** WGL_GENLOCK_SAMPLE_RATE_I3D *************************** */

gits::OpenGL::CwglGenlockSampleRateI3D::CwglGenlockSampleRateI3D() {}

gits::OpenGL::CwglGenlockSampleRateI3D::CwglGenlockSampleRateI3D(BOOL return_value,
                                                                 HDC hDC,
                                                                 UINT uRate)
    : _return_value(return_value), _hDC(hDC), _uRate(uRate), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGenlockSampleRateI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uRate, _hwnd);
}

void gits::OpenGL::CwglGenlockSampleRateI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGenlockSampleRateI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uRate);
#endif
}

/* ***************************** WGL_GET_GENLOCK_SAMPLE_RATE_I3D *************************** */

gits::OpenGL::CwglGetGenlockSampleRateI3D::CwglGetGenlockSampleRateI3D() {}

gits::OpenGL::CwglGetGenlockSampleRateI3D::CwglGetGenlockSampleRateI3D(BOOL return_value,
                                                                       HDC hDC,
                                                                       UINT* uRate)
    : _return_value(return_value), _hDC(hDC), _uRate(1, uRate), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetGenlockSampleRateI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uRate, _hwnd);
}

void gits::OpenGL::CwglGetGenlockSampleRateI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGenlockSampleRateI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uRate);
#endif
}

/* ***************************** WGL_GENLOCK_SOURCE_DELAY_I3D *************************** */

gits::OpenGL::CwglGenlockSourceDelayI3D::CwglGenlockSourceDelayI3D() {}

gits::OpenGL::CwglGenlockSourceDelayI3D::CwglGenlockSourceDelayI3D(BOOL return_value,
                                                                   HDC hDC,
                                                                   UINT uDelay)
    : _return_value(return_value), _hDC(hDC), _uDelay(uDelay), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGenlockSourceDelayI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uDelay, _hwnd);
}

void gits::OpenGL::CwglGenlockSourceDelayI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGenlockSourceDelayI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uDelay);
#endif
}

/* ***************************** WGL_GET_GENLOCK_SOURCE_DELAY_I3D *************************** */

gits::OpenGL::CwglGetGenlockSourceDelayI3D::CwglGetGenlockSourceDelayI3D() {}

gits::OpenGL::CwglGetGenlockSourceDelayI3D::CwglGetGenlockSourceDelayI3D(BOOL return_value,
                                                                         HDC hDC,
                                                                         UINT* uDelay)
    : _return_value(return_value), _hDC(hDC), _uDelay(1, uDelay), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetGenlockSourceDelayI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uDelay, _hwnd);
}

void gits::OpenGL::CwglGetGenlockSourceDelayI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGenlockSourceDelayI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uDelay);
#endif
}

/* ***************************** WGL_QUERY_GENLOCK_MAX_SOURCE_DELAY_I3D *************************** */

gits::OpenGL::CwglQueryGenlockMaxSourceDelayI3D::CwglQueryGenlockMaxSourceDelayI3D() {}

gits::OpenGL::CwglQueryGenlockMaxSourceDelayI3D::CwglQueryGenlockMaxSourceDelayI3D(
    BOOL return_value, HDC hDC, UINT* uMaxLineDelay, UINT* uMaxPixelDelay)
    : _return_value(return_value),
      _hDC(hDC),
      _uMaxLineDelay(1, uMaxLineDelay),
      _uMaxPixelDelay(1, uMaxPixelDelay),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglQueryGenlockMaxSourceDelayI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uMaxLineDelay, _uMaxPixelDelay, _hwnd);
}

void gits::OpenGL::CwglQueryGenlockMaxSourceDelayI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryGenlockMaxSourceDelayI3D((HDC)UpdateHDC(_hDC, _hwnd), *_uMaxLineDelay,
                                           *_uMaxPixelDelay);
#endif
}

/* ***************************** WGL_CREATE_IMAGE_BUFFER_I3D *************************** */

gits::OpenGL::CwglCreateImageBufferI3D::CwglCreateImageBufferI3D() {}

gits::OpenGL::CwglCreateImageBufferI3D::CwglCreateImageBufferI3D(LPVOID return_value,
                                                                 HDC hDC,
                                                                 DWORD dwSize,
                                                                 UINT uFlags)
    : _return_value(return_value),
      _hDC(hDC),
      _dwSize(dwSize),
      _uFlags(uFlags),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglCreateImageBufferI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _dwSize, _uFlags, _hwnd);
}

void gits::OpenGL::CwglCreateImageBufferI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglCreateImageBufferI3D((HDC)UpdateHDC(_hDC, _hwnd), *_dwSize, *_uFlags);
#endif
}

/* ***************************** WGL_DESTROY_IMAGE_BUFFER_I3D *************************** */

gits::OpenGL::CwglDestroyImageBufferI3D::CwglDestroyImageBufferI3D() {}

gits::OpenGL::CwglDestroyImageBufferI3D::CwglDestroyImageBufferI3D(BOOL return_value,
                                                                   HDC hDC,
                                                                   LPVOID pAddress)
    : _return_value(return_value), _hDC(hDC), _pAddress(pAddress), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglDestroyImageBufferI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _pAddress, _hwnd);
}

void gits::OpenGL::CwglDestroyImageBufferI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDestroyImageBufferI3D((HDC)UpdateHDC(_hDC, _hwnd), (LPVOID)*_pAddress);
#endif
}

/* ***************************** WGL_ASSOCIATE_IMAGE_BUFFER_EVENTS_I3D *************************** */

gits::OpenGL::CwglAssociateImageBufferEventsI3D::CwglAssociateImageBufferEventsI3D() {}

gits::OpenGL::CwglAssociateImageBufferEventsI3D::CwglAssociateImageBufferEventsI3D(
    BOOL return_value,
    HDC hDC,
    const HANDLE* pEvent,
    const LPVOID* pAddress,
    const DWORD* pSize,
    UINT count)
    : _return_value(return_value),
      _hDC(hDC),
      _pEvent(),
      _pAddress(1, pAddress),
      _pSize(1, pSize),
      _count(count),
      _hwnd(WindowFromDC(hDC)) {
  for (unsigned int idx = 0; idx < *_count; idx++) {
    CHANDLE handleObj(pEvent[idx]);
    _pEvent.Vector().push_back(handleObj.Original());
  }
}

gits::CArgument& gits::OpenGL::CwglAssociateImageBufferEventsI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _pEvent, _pAddress, _pSize, _count, _hwnd);
}

void gits::OpenGL::CwglAssociateImageBufferEventsI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglAssociateImageBufferEventsI3D((HDC)UpdateHDC(_hDC, _hwnd), *_pEvent, *_pAddress,
                                           *_pSize, *_count);
#endif
}

/* ***************************** WGL_RELEASE_IMAGE_BUFFER_EVENTS_I3D *************************** */

gits::OpenGL::CwglReleaseImageBufferEventsI3D::CwglReleaseImageBufferEventsI3D() {}

gits::OpenGL::CwglReleaseImageBufferEventsI3D::CwglReleaseImageBufferEventsI3D(
    BOOL return_value, HDC hDC, const LPVOID* pAddress, UINT count)
    : _return_value(return_value),
      _hDC(hDC),
      _pAddress(1, pAddress),
      _count(count),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglReleaseImageBufferEventsI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _pAddress, _count, _hwnd);
}

void gits::OpenGL::CwglReleaseImageBufferEventsI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglReleaseImageBufferEventsI3D((HDC)UpdateHDC(_hDC, _hwnd), *_pAddress, *_count);
#endif
}

/* ***************************** WGL_ENABLE_FRAME_LOCK_I3D *************************** */

gits::OpenGL::CwglEnableFrameLockI3D::CwglEnableFrameLockI3D() {}

gits::OpenGL::CwglEnableFrameLockI3D::CwglEnableFrameLockI3D(BOOL return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CwglEnableFrameLockI3D::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglEnableFrameLockI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEnableFrameLockI3D();
#endif
}

/* ***************************** WGL_DISABLE_FRAME_LOCK_I3D *************************** */

gits::OpenGL::CwglDisableFrameLockI3D::CwglDisableFrameLockI3D() {}

gits::OpenGL::CwglDisableFrameLockI3D::CwglDisableFrameLockI3D(BOOL return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CwglDisableFrameLockI3D::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglDisableFrameLockI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDisableFrameLockI3D();
#endif
}

/* ***************************** WGL_IS_ENABLED_FRAME_LOCK_I3D *************************** */

gits::OpenGL::CwglIsEnabledFrameLockI3D::CwglIsEnabledFrameLockI3D() {}

gits::OpenGL::CwglIsEnabledFrameLockI3D::CwglIsEnabledFrameLockI3D(BOOL return_value, BOOL* pFlag)
    : _return_value(return_value), _pFlag(1, pFlag) {}

gits::CArgument& gits::OpenGL::CwglIsEnabledFrameLockI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _pFlag);
}

void gits::OpenGL::CwglIsEnabledFrameLockI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglIsEnabledFrameLockI3D(*_pFlag);
#endif
}

/* ***************************** WGL_QUERY_FRAME_LOCK_MASTER_I3D *************************** */

gits::OpenGL::CwglQueryFrameLockMasterI3D::CwglQueryFrameLockMasterI3D() {}

gits::OpenGL::CwglQueryFrameLockMasterI3D::CwglQueryFrameLockMasterI3D(BOOL return_value,
                                                                       BOOL* pFlag)
    : _return_value(return_value), _pFlag(1, pFlag) {}

gits::CArgument& gits::OpenGL::CwglQueryFrameLockMasterI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _pFlag);
}

void gits::OpenGL::CwglQueryFrameLockMasterI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryFrameLockMasterI3D(*_pFlag);
#endif
}

/* ***************************** WGL_GET_FRAME_USAGE_I3D *************************** */

gits::OpenGL::CwglGetFrameUsageI3D::CwglGetFrameUsageI3D() {}

gits::OpenGL::CwglGetFrameUsageI3D::CwglGetFrameUsageI3D(BOOL return_value, float* pUsage)
    : _return_value(return_value), _pUsage(1, pUsage) {}

gits::CArgument& gits::OpenGL::CwglGetFrameUsageI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _pUsage);
}

void gits::OpenGL::CwglGetFrameUsageI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetFrameUsageI3D(*_pUsage);
#endif
}

/* ***************************** WGL_BEGIN_FRAME_TRACKING_I3D *************************** */

gits::OpenGL::CwglBeginFrameTrackingI3D::CwglBeginFrameTrackingI3D() {}

gits::OpenGL::CwglBeginFrameTrackingI3D::CwglBeginFrameTrackingI3D(BOOL return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CwglBeginFrameTrackingI3D::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglBeginFrameTrackingI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBeginFrameTrackingI3D();
#endif
}

/* ***************************** WGL_END_FRAME_TRACKING_I3D *************************** */

gits::OpenGL::CwglEndFrameTrackingI3D::CwglEndFrameTrackingI3D() {}

gits::OpenGL::CwglEndFrameTrackingI3D::CwglEndFrameTrackingI3D(BOOL return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CwglEndFrameTrackingI3D::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglEndFrameTrackingI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEndFrameTrackingI3D();
#endif
}

/* ***************************** WGL_QUERY_FRAME_TRACKING_I3D *************************** */

gits::OpenGL::CwglQueryFrameTrackingI3D::CwglQueryFrameTrackingI3D() {}

gits::OpenGL::CwglQueryFrameTrackingI3D::CwglQueryFrameTrackingI3D(BOOL return_value,
                                                                   DWORD* pFrameCount,
                                                                   DWORD* pMissedFrames,
                                                                   float* pLastMissedUsage)
    : _return_value(return_value),
      _pFrameCount(1, pFrameCount),
      _pMissedFrames(1, pMissedFrames),
      _pLastMissedUsage(1, pLastMissedUsage) {}

gits::CArgument& gits::OpenGL::CwglQueryFrameTrackingI3D::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _pFrameCount, _pMissedFrames, _pLastMissedUsage);
}

void gits::OpenGL::CwglQueryFrameTrackingI3D::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryFrameTrackingI3D(*_pFrameCount, *_pMissedFrames, *_pLastMissedUsage);
#endif
}

/* ***************************** WGL_SET_STEREO_EMITTER_STATE3DL *************************** */

gits::OpenGL::CwglSetStereoEmitterState3DL::CwglSetStereoEmitterState3DL() {}

gits::OpenGL::CwglSetStereoEmitterState3DL::CwglSetStereoEmitterState3DL(BOOL return_value,
                                                                         HDC hDC,
                                                                         UINT uState)
    : _return_value(return_value), _hDC(hDC), _uState(uState), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglSetStereoEmitterState3DL::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uState, _hwnd);
}

void gits::OpenGL::CwglSetStereoEmitterState3DL::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSetStereoEmitterState3DL((HDC)UpdateHDC(_hDC, _hwnd), *_uState);
#endif
}

/* ***************************** WGL_ENUMERATE_VIDEO_DEVICES_NV *************************** */

gits::OpenGL::CwglEnumerateVideoDevicesNV::CwglEnumerateVideoDevicesNV() {}

gits::OpenGL::CwglEnumerateVideoDevicesNV::CwglEnumerateVideoDevicesNV(
    int return_value, HDC hDC, HVIDEOOUTPUTDEVICENV* phDeviceList)
    : _return_value(return_value), _hDC(hDC), _phDeviceList(), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglEnumerateVideoDevicesNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _phDeviceList, _hwnd);
}

void gits::OpenGL::CwglEnumerateVideoDevicesNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEnumerateVideoDevicesNV((HDC)UpdateHDC(_hDC, _hwnd),
                                     (HVIDEOOUTPUTDEVICENV*)*_phDeviceList);
#endif
}

/* ***************************** WGL_BIND_VIDEO_DEVICE_NV *************************** */

gits::OpenGL::CwglBindVideoDeviceNV::CwglBindVideoDeviceNV() {}

gits::OpenGL::CwglBindVideoDeviceNV::CwglBindVideoDeviceNV(BOOL return_value,
                                                           HDC hDC,
                                                           unsigned uVideoSlot,
                                                           HVIDEOOUTPUTDEVICENV hVideoDevice,
                                                           const int* piAttribList)
    : _return_value(return_value),
      _hDC(hDC),
      _uVideoSlot(uVideoSlot),
      _hVideoDevice(hVideoDevice),
      _piAttribList(piAttribList, 0, 2),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglBindVideoDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _uVideoSlot, _hVideoDevice, _piAttribList, _hwnd);
}

void gits::OpenGL::CwglBindVideoDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBindVideoDeviceNV((HDC)UpdateHDC(_hDC, _hwnd), *_uVideoSlot,
                               (HVIDEOOUTPUTDEVICENV)*_hVideoDevice, *_piAttribList);
#endif
}

/* ***************************** WGL_QUERY_CURRENT_CONTEXT_NV *************************** */

gits::OpenGL::CwglQueryCurrentContextNV::CwglQueryCurrentContextNV() {}

gits::OpenGL::CwglQueryCurrentContextNV::CwglQueryCurrentContextNV(BOOL return_value,
                                                                   int iAttribute,
                                                                   int* piValue)
    : _return_value(return_value), _iAttribute(iAttribute), _piValue(1, piValue) {}

gits::CArgument& gits::OpenGL::CwglQueryCurrentContextNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _iAttribute, _piValue);
}

void gits::OpenGL::CwglQueryCurrentContextNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryCurrentContextNV(*_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_GET_VIDEO_DEVICE_NV *************************** */

gits::OpenGL::CwglGetVideoDeviceNV::CwglGetVideoDeviceNV() {}

gits::OpenGL::CwglGetVideoDeviceNV::CwglGetVideoDeviceNV(BOOL return_value,
                                                         HDC hDC,
                                                         int numDevices,
                                                         HPVIDEODEV* hVideoDevice)
    : _return_value(return_value),
      _hDC(hDC),
      _numDevices(numDevices),
      _hVideoDevice(),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglGetVideoDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _numDevices, _hVideoDevice, _hwnd);
}

void gits::OpenGL::CwglGetVideoDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetVideoDeviceNV((HDC)UpdateHDC(_hDC, _hwnd), *_numDevices,
                              (HPVIDEODEV*)*_hVideoDevice);
#endif
}

/* ***************************** WGL_RELEASE_VIDEO_DEVICE_NV *************************** */

gits::OpenGL::CwglReleaseVideoDeviceNV::CwglReleaseVideoDeviceNV() {}

gits::OpenGL::CwglReleaseVideoDeviceNV::CwglReleaseVideoDeviceNV(BOOL return_value,
                                                                 HPVIDEODEV hVideoDevice)
    : _return_value(return_value), _hVideoDevice(hVideoDevice) {}

gits::CArgument& gits::OpenGL::CwglReleaseVideoDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hVideoDevice);
}

void gits::OpenGL::CwglReleaseVideoDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglReleaseVideoDeviceNV((HPVIDEODEV)*_hVideoDevice);
#endif
}

/* ***************************** WGL_BIND_VIDEO_IMAGE_NV *************************** */

gits::OpenGL::CwglBindVideoImageNV::CwglBindVideoImageNV() {}

gits::OpenGL::CwglBindVideoImageNV::CwglBindVideoImageNV(BOOL return_value,
                                                         HPVIDEODEV hVideoDevice,
                                                         HPBUFFERARB hPbuffer,
                                                         int iVideoBuffer)
    : _return_value(return_value),
      _hVideoDevice(hVideoDevice),
      _hPbuffer(hPbuffer),
      _iVideoBuffer(iVideoBuffer) {}

gits::CArgument& gits::OpenGL::CwglBindVideoImageNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hVideoDevice, _hPbuffer, _iVideoBuffer);
}

void gits::OpenGL::CwglBindVideoImageNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBindVideoImageNV((HPVIDEODEV)*_hVideoDevice, (HPBUFFERARB)*_hPbuffer, *_iVideoBuffer);
#endif
}

/* ***************************** WGL_RELEASE_VIDEO_IMAGE_NV *************************** */

gits::OpenGL::CwglReleaseVideoImageNV::CwglReleaseVideoImageNV() {}

gits::OpenGL::CwglReleaseVideoImageNV::CwglReleaseVideoImageNV(BOOL return_value,
                                                               HPBUFFERARB hPbuffer,
                                                               int iVideoBuffer)
    : _return_value(return_value), _hPbuffer(hPbuffer), _iVideoBuffer(iVideoBuffer) {}

gits::CArgument& gits::OpenGL::CwglReleaseVideoImageNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _iVideoBuffer);
}

void gits::OpenGL::CwglReleaseVideoImageNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglReleaseVideoImageNV((HPBUFFERARB)*_hPbuffer, *_iVideoBuffer);
#endif
}

/* ***************************** WGL_SEND_PBUFFER_TO_VIDEO_NV *************************** */

gits::OpenGL::CwglSendPbufferToVideoNV::CwglSendPbufferToVideoNV() {}

gits::OpenGL::CwglSendPbufferToVideoNV::CwglSendPbufferToVideoNV(BOOL return_value,
                                                                 HPBUFFERARB hPbuffer,
                                                                 int iBufferType,
                                                                 unsigned long* pulCounterPbuffer,
                                                                 BOOL bBlock)
    : _return_value(return_value),
      _hPbuffer(hPbuffer),
      _iBufferType(iBufferType),
      _pulCounterPbuffer(1, pulCounterPbuffer),
      _bBlock(bBlock) {}

gits::CArgument& gits::OpenGL::CwglSendPbufferToVideoNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hPbuffer, _iBufferType, _pulCounterPbuffer, _bBlock);
}

void gits::OpenGL::CwglSendPbufferToVideoNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglSendPbufferToVideoNV((HPBUFFERARB)*_hPbuffer, *_iBufferType, *_pulCounterPbuffer,
                                  *_bBlock);
#endif
}

/* ***************************** WGL_GET_VIDEO_INFO_NV *************************** */

gits::OpenGL::CwglGetVideoInfoNV::CwglGetVideoInfoNV() {}

gits::OpenGL::CwglGetVideoInfoNV::CwglGetVideoInfoNV(BOOL return_value,
                                                     HPVIDEODEV hpVideoDevice,
                                                     unsigned long* pulCounterOutputPbuffer,
                                                     unsigned long* pulCounterOutputVideo)
    : _return_value(return_value),
      _hpVideoDevice(hpVideoDevice),
      _pulCounterOutputPbuffer(1, pulCounterOutputPbuffer),
      _pulCounterOutputVideo(1, pulCounterOutputVideo) {}

gits::CArgument& gits::OpenGL::CwglGetVideoInfoNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hpVideoDevice, _pulCounterOutputPbuffer,
                       _pulCounterOutputVideo);
}

void gits::OpenGL::CwglGetVideoInfoNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetVideoInfoNV((HPVIDEODEV)*_hpVideoDevice, *_pulCounterOutputPbuffer,
                            *_pulCounterOutputVideo);
#endif
}

/* ***************************** WGL_JOIN_SWAP_GROUP_NV *************************** */

gits::OpenGL::CwglJoinSwapGroupNV::CwglJoinSwapGroupNV() {}

gits::OpenGL::CwglJoinSwapGroupNV::CwglJoinSwapGroupNV(BOOL return_value, HDC hDC, GLuint group)
    : _return_value(return_value), _hDC(hDC), _group(group), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglJoinSwapGroupNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _group, _hwnd);
}

void gits::OpenGL::CwglJoinSwapGroupNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglJoinSwapGroupNV((HDC)UpdateHDC(_hDC, _hwnd), *_group);
#endif
}

/* ***************************** WGL_BIND_SWAP_BARRIER_NV *************************** */

gits::OpenGL::CwglBindSwapBarrierNV::CwglBindSwapBarrierNV() {}

gits::OpenGL::CwglBindSwapBarrierNV::CwglBindSwapBarrierNV(BOOL return_value,
                                                           GLuint group,
                                                           GLuint barrier)
    : _return_value(return_value), _group(group), _barrier(barrier) {}

gits::CArgument& gits::OpenGL::CwglBindSwapBarrierNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _group, _barrier);
}

void gits::OpenGL::CwglBindSwapBarrierNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBindSwapBarrierNV(*_group, *_barrier);
#endif
}

/* ***************************** WGL_QUERY_SWAP_GROUP_NV *************************** */

gits::OpenGL::CwglQuerySwapGroupNV::CwglQuerySwapGroupNV() {}

gits::OpenGL::CwglQuerySwapGroupNV::CwglQuerySwapGroupNV(BOOL return_value,
                                                         HDC hDC,
                                                         GLuint* group,
                                                         GLuint* barrier)
    : _return_value(return_value),
      _hDC(hDC),
      _group(1, group),
      _barrier(1, barrier),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglQuerySwapGroupNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _group, _barrier, _hwnd);
}

void gits::OpenGL::CwglQuerySwapGroupNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQuerySwapGroupNV((HDC)UpdateHDC(_hDC, _hwnd), *_group, *_barrier);
#endif
}

/* ***************************** WGL_QUERY_MAX_SWAP_GROUPS_NV *************************** */

gits::OpenGL::CwglQueryMaxSwapGroupsNV::CwglQueryMaxSwapGroupsNV() {}

gits::OpenGL::CwglQueryMaxSwapGroupsNV::CwglQueryMaxSwapGroupsNV(BOOL return_value,
                                                                 HDC hDC,
                                                                 GLuint* maxGroups,
                                                                 GLuint* maxBarriers)
    : _return_value(return_value),
      _hDC(hDC),
      _maxGroups(1, maxGroups),
      _maxBarriers(1, maxBarriers),
      _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglQueryMaxSwapGroupsNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _maxGroups, _maxBarriers, _hwnd);
}

void gits::OpenGL::CwglQueryMaxSwapGroupsNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryMaxSwapGroupsNV((HDC)UpdateHDC(_hDC, _hwnd), *_maxGroups, *_maxBarriers);
#endif
}

/* ***************************** WGL_QUERY_FRAME_COUNT_NV *************************** */

gits::OpenGL::CwglQueryFrameCountNV::CwglQueryFrameCountNV() {}

gits::OpenGL::CwglQueryFrameCountNV::CwglQueryFrameCountNV(BOOL return_value,
                                                           HDC hDC,
                                                           GLuint* count)
    : _return_value(return_value), _hDC(hDC), _count(1, count), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglQueryFrameCountNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _count, _hwnd);
}

void gits::OpenGL::CwglQueryFrameCountNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryFrameCountNV((HDC)UpdateHDC(_hDC, _hwnd), *_count);
#endif
}

/* ***************************** WGL_RESET_FRAME_COUNT_NV *************************** */

gits::OpenGL::CwglResetFrameCountNV::CwglResetFrameCountNV() {}

gits::OpenGL::CwglResetFrameCountNV::CwglResetFrameCountNV(BOOL return_value, HDC hDC)
    : _return_value(return_value), _hDC(hDC), _hwnd(WindowFromDC(hDC)) {}

gits::CArgument& gits::OpenGL::CwglResetFrameCountNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDC, _hwnd);
}

void gits::OpenGL::CwglResetFrameCountNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglResetFrameCountNV((HDC)UpdateHDC(_hDC, _hwnd));
#endif
}

/* ***************************** WGL_ENUM_GPUS_NV *************************** */

gits::OpenGL::CwglEnumGpusNV::CwglEnumGpusNV() {}

gits::OpenGL::CwglEnumGpusNV::CwglEnumGpusNV(BOOL return_value, UINT iGpuIndex, HGPUNV* phGpu)
    : _return_value(return_value), _iGpuIndex(iGpuIndex), _phGpu() {}

gits::CArgument& gits::OpenGL::CwglEnumGpusNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _iGpuIndex, _phGpu);
}

void gits::OpenGL::CwglEnumGpusNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEnumGpusNV(*_iGpuIndex, (HGPUNV*)*_phGpu);
#endif
}

/* ***************************** WGL_ENUM_GPU_DEVICES_NV *************************** */

gits::OpenGL::CwglEnumGpuDevicesNV::CwglEnumGpuDevicesNV() {}

gits::OpenGL::CwglEnumGpuDevicesNV::CwglEnumGpuDevicesNV(BOOL return_value,
                                                         HGPUNV hGpu,
                                                         UINT iDeviceIndex,
                                                         PGPU_DEVICE lpGpuDevice)
    : _return_value(return_value), _hGpu(hGpu), _iDeviceIndex(iDeviceIndex) {}

gits::CArgument& gits::OpenGL::CwglEnumGpuDevicesNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hGpu, _iDeviceIndex);
}

void gits::OpenGL::CwglEnumGpuDevicesNV::Run() {
  throw ENotImplemented(EXCEPTION_MESSAGE);
  //  drv.wgl.wglEnumGpuDevicesNV(*_hGpu, *_iDeviceIndex, *_lpGpuDevice);
}

/* ***************************** WGL_CREATE_AFFINITY_DCNV *************************** */

gits::OpenGL::CwglCreateAffinityDCNV::CwglCreateAffinityDCNV() {}

gits::OpenGL::CwglCreateAffinityDCNV::CwglCreateAffinityDCNV(HDC return_value,
                                                             const HGPUNV* phGpuList)
    : _return_value(return_value), _phGpuList() {}

gits::CArgument& gits::OpenGL::CwglCreateAffinityDCNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _phGpuList);
}

void gits::OpenGL::CwglCreateAffinityDCNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  HDC hdc = drv.wgl.wglCreateAffinityDCNV((HGPUNV*)*_phGpuList);
  CHDC::AddMapping(_return_value.Original(), hdc);
#endif
}

/* ***************************** WGL_ENUM_GPUS_FROM_AFFINITY_DCNV *************************** */

gits::OpenGL::CwglEnumGpusFromAffinityDCNV::CwglEnumGpusFromAffinityDCNV() {}

gits::OpenGL::CwglEnumGpusFromAffinityDCNV::CwglEnumGpusFromAffinityDCNV(BOOL return_value,
                                                                         HDC hAffinityDC,
                                                                         UINT iGpuIndex,
                                                                         HGPUNV* hGpu)
    : _return_value(return_value), _hAffinityDC(hAffinityDC), _iGpuIndex(iGpuIndex), _hGpu() {}

gits::CArgument& gits::OpenGL::CwglEnumGpusFromAffinityDCNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hAffinityDC, _iGpuIndex, _hGpu);
}

void gits::OpenGL::CwglEnumGpusFromAffinityDCNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEnumGpusFromAffinityDCNV((HDC)*_hAffinityDC, *_iGpuIndex, (HGPUNV*)*_hGpu);
#endif
}

/* ***************************** WGL_DELETE_DCNV *************************** */

gits::OpenGL::CwglDeleteDCNV::CwglDeleteDCNV() {}

gits::OpenGL::CwglDeleteDCNV::CwglDeleteDCNV(BOOL return_value, HDC hdc)
    : _return_value(return_value), _hdc(hdc) {}

gits::CArgument& gits::OpenGL::CwglDeleteDCNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hdc);
}

void gits::OpenGL::CwglDeleteDCNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDeleteDCNV((HDC)*_hdc);
  _hdc.RemoveMapping();
#endif
}

/* ***************************** WGL_GET_GPUIDS_AMD *************************** */

gits::OpenGL::CwglGetGPUIDsAMD::CwglGetGPUIDsAMD() {}

gits::OpenGL::CwglGetGPUIDsAMD::CwglGetGPUIDsAMD(UINT return_value, UINT maxCount, UINT* ids)
    : _return_value(return_value), _maxCount(maxCount), _ids(ids, 0, 1) {}

gits::CArgument& gits::OpenGL::CwglGetGPUIDsAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _maxCount, _ids);
}

void gits::OpenGL::CwglGetGPUIDsAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGPUIDsAMD(*_maxCount, *_ids);
#endif
}

/* ***************************** WGL_GET_GPUINFO_AMD *************************** */

gits::OpenGL::CwglGetGPUInfoAMD::CwglGetGPUInfoAMD() : _data(size_t(0)) {}

gits::OpenGL::CwglGetGPUInfoAMD::CwglGetGPUInfoAMD(
    int return_value, UINT id, int property, GLenum dataType, UINT size, void* data)
    : _return_value(return_value),
      _id(id),
      _property(property),
      _dataType(dataType),
      _size(size),
      _data(size_t(0)) {}

gits::CArgument& gits::OpenGL::CwglGetGPUInfoAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _id, _property, _dataType, _size, _data);
}

void gits::OpenGL::CwglGetGPUInfoAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetGPUInfoAMD(*_id, *_property, *_dataType, *_size, *_data);
#endif
}

/* ***************************** WGL_GET_CONTEXT_GPUIDAMD *************************** */

gits::OpenGL::CwglGetContextGPUIDAMD::CwglGetContextGPUIDAMD() {}

gits::OpenGL::CwglGetContextGPUIDAMD::CwglGetContextGPUIDAMD(UINT return_value, HGLRC hglrc)
    : _return_value(return_value), _hglrc(hglrc) {}

gits::CArgument& gits::OpenGL::CwglGetContextGPUIDAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hglrc);
}

void gits::OpenGL::CwglGetContextGPUIDAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetContextGPUIDAMD((HGLRC)*_hglrc);
#endif
}

/* ***************************** WGL_CREATE_ASSOCIATED_CONTEXT_AMD *************************** */

gits::OpenGL::CwglCreateAssociatedContextAMD::CwglCreateAssociatedContextAMD() {}

gits::OpenGL::CwglCreateAssociatedContextAMD::CwglCreateAssociatedContextAMD(HGLRC return_value,
                                                                             UINT id)
    : _return_value(return_value), _id(id) {}

gits::CArgument& gits::OpenGL::CwglCreateAssociatedContextAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _id);
}

void gits::OpenGL::CwglCreateAssociatedContextAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglCreateAssociatedContextAMD(*_id);
#endif
}

/* ***************************** WGL_CREATE_ASSOCIATED_CONTEXT_ATTRIBS_AMD *************************** */

gits::OpenGL::CwglCreateAssociatedContextAttribsAMD::CwglCreateAssociatedContextAttribsAMD() {}

gits::OpenGL::CwglCreateAssociatedContextAttribsAMD::CwglCreateAssociatedContextAttribsAMD(
    HGLRC return_value, UINT id, HGLRC hShareContext, const int* attribList)
    : _return_value(return_value),
      _id(id),
      _hShareContext(hShareContext),
      _attribList(attribList, 0, 1) {}

gits::CArgument& gits::OpenGL::CwglCreateAssociatedContextAttribsAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _id, _hShareContext, _attribList);
}

void gits::OpenGL::CwglCreateAssociatedContextAttribsAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglCreateAssociatedContextAttribsAMD(*_id, (HGLRC)*_hShareContext, *_attribList);
#endif
}

/* ***************************** WGL_DELETE_ASSOCIATED_CONTEXT_AMD *************************** */

gits::OpenGL::CwglDeleteAssociatedContextAMD::CwglDeleteAssociatedContextAMD() {}

gits::OpenGL::CwglDeleteAssociatedContextAMD::CwglDeleteAssociatedContextAMD(BOOL return_value,
                                                                             HGLRC hglrc)
    : _return_value(return_value), _hglrc(hglrc) {}

gits::CArgument& gits::OpenGL::CwglDeleteAssociatedContextAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hglrc);
}

void gits::OpenGL::CwglDeleteAssociatedContextAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglDeleteAssociatedContextAMD((HGLRC)*_hglrc);
#endif
}

/* ***************************** WGL_MAKE_ASSOCIATED_CONTEXT_CURRENT_AMD *************************** */

gits::OpenGL::CwglMakeAssociatedContextCurrentAMD::CwglMakeAssociatedContextCurrentAMD() {}

gits::OpenGL::CwglMakeAssociatedContextCurrentAMD::CwglMakeAssociatedContextCurrentAMD(
    BOOL return_value, HGLRC hglrc)
    : _return_value(return_value), _hglrc(hglrc) {}

gits::CArgument& gits::OpenGL::CwglMakeAssociatedContextCurrentAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hglrc);
}

void gits::OpenGL::CwglMakeAssociatedContextCurrentAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglMakeAssociatedContextCurrentAMD((HGLRC)*_hglrc);
#endif
}

/* ***************************** WGL_GET_CURRENT_ASSOCIATED_CONTEXT_AMD *************************** */

gits::OpenGL::CwglGetCurrentAssociatedContextAMD::CwglGetCurrentAssociatedContextAMD() {}

gits::OpenGL::CwglGetCurrentAssociatedContextAMD::CwglGetCurrentAssociatedContextAMD(
    HGLRC return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CwglGetCurrentAssociatedContextAMD::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CwglGetCurrentAssociatedContextAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetCurrentAssociatedContextAMD();
#endif
}

/* ***************************** WGL_BLIT_CONTEXT_FRAMEBUFFER_AMD *************************** */

gits::OpenGL::CwglBlitContextFramebufferAMD::CwglBlitContextFramebufferAMD() {}

gits::OpenGL::CwglBlitContextFramebufferAMD::CwglBlitContextFramebufferAMD(HGLRC dstCtx,
                                                                           GLint srcX0,
                                                                           GLint srcY0,
                                                                           GLint srcX1,
                                                                           GLint srcY1,
                                                                           GLint dstX0,
                                                                           GLint dstY0,
                                                                           GLint dstX1,
                                                                           GLint dstY1,
                                                                           GLbitfield mask,
                                                                           GLenum filter)
    : _dstCtx(dstCtx),
      _srcX0(srcX0),
      _srcY0(srcY0),
      _srcX1(srcX1),
      _srcY1(srcY1),
      _dstX0(dstX0),
      _dstY0(dstY0),
      _dstX1(dstX1),
      _dstY1(dstY1),
      _mask(mask),
      _filter(filter) {}

gits::CArgument& gits::OpenGL::CwglBlitContextFramebufferAMD::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dstCtx, _srcX0, _srcY0, _srcX1, _srcY1, _dstX0, _dstY0,
                       _dstX1, _dstY1, _mask, _filter);
}

void gits::OpenGL::CwglBlitContextFramebufferAMD::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBlitContextFramebufferAMD(HGLRC(*_dstCtx), *_srcX0, *_srcY0, *_srcX1, *_srcY1, *_dstX0,
                                       *_dstY0, *_dstX1, *_dstY1, *_mask, *_filter);
#endif
}

/* ***************************** WGL_BIND_VIDEO_CAPTURE_DEVICE_NV *************************** */

gits::OpenGL::CwglBindVideoCaptureDeviceNV::CwglBindVideoCaptureDeviceNV() {}

gits::OpenGL::CwglBindVideoCaptureDeviceNV::CwglBindVideoCaptureDeviceNV(
    BOOL return_value, UINT uVideoSlot, HVIDEOINPUTDEVICENV hDevice)
    : _return_value(return_value), _uVideoSlot(uVideoSlot), _hDevice(hDevice) {}

gits::CArgument& gits::OpenGL::CwglBindVideoCaptureDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _uVideoSlot, _hDevice);
}

void gits::OpenGL::CwglBindVideoCaptureDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglBindVideoCaptureDeviceNV(*_uVideoSlot, (HVIDEOINPUTDEVICENV)*_hDevice);
#endif
}

/* ***************************** WGL_ENUMERATE_VIDEO_CAPTURE_DEVICES_NV *************************** */

gits::OpenGL::CwglEnumerateVideoCaptureDevicesNV::CwglEnumerateVideoCaptureDevicesNV() {}

gits::OpenGL::CwglEnumerateVideoCaptureDevicesNV::CwglEnumerateVideoCaptureDevicesNV(
    UINT return_value, HDC hDc, HVIDEOINPUTDEVICENV* phDeviceList)
    : _return_value(return_value), _hDc(hDc), _phDeviceList(), _hwnd(WindowFromDC(hDc)) {}

gits::CArgument& gits::OpenGL::CwglEnumerateVideoCaptureDevicesNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDc, _phDeviceList, _hwnd);
}

void gits::OpenGL::CwglEnumerateVideoCaptureDevicesNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglEnumerateVideoCaptureDevicesNV((HDC)UpdateHDC(_hDc, _hwnd),
                                            (HVIDEOINPUTDEVICENV*)*_phDeviceList);
#endif
}

/* ***************************** WGL_LOCK_VIDEO_CAPTURE_DEVICE_NV *************************** */

gits::OpenGL::CwglLockVideoCaptureDeviceNV::CwglLockVideoCaptureDeviceNV() {}

gits::OpenGL::CwglLockVideoCaptureDeviceNV::CwglLockVideoCaptureDeviceNV(
    BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice)
    : _return_value(return_value), _hDc(hDc), _hDevice(hDevice), _hwnd(WindowFromDC(hDc)) {}

gits::CArgument& gits::OpenGL::CwglLockVideoCaptureDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDc, _hDevice, _hwnd);
}

void gits::OpenGL::CwglLockVideoCaptureDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglLockVideoCaptureDeviceNV((HDC)UpdateHDC(_hDc, _hwnd), (HVIDEOINPUTDEVICENV)*_hDevice);
#endif
}

/* ***************************** WGL_QUERY_VIDEO_CAPTURE_DEVICE_NV *************************** */

gits::OpenGL::CwglQueryVideoCaptureDeviceNV::CwglQueryVideoCaptureDeviceNV() {}

gits::OpenGL::CwglQueryVideoCaptureDeviceNV::CwglQueryVideoCaptureDeviceNV(
    BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice, int iAttribute, int* piValue)
    : _return_value(return_value),
      _hDc(hDc),
      _hDevice(hDevice),
      _iAttribute(iAttribute),
      _piValue(1, piValue),
      _hwnd(WindowFromDC(hDc)) {}

gits::CArgument& gits::OpenGL::CwglQueryVideoCaptureDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDc, _hDevice, _iAttribute, _piValue, _hwnd);
}

void gits::OpenGL::CwglQueryVideoCaptureDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglQueryVideoCaptureDeviceNV((HDC)UpdateHDC(_hDc, _hwnd), (HVIDEOINPUTDEVICENV)*_hDevice,
                                       *_iAttribute, *_piValue);
#endif
}

/* ***************************** WGL_RELEASE_VIDEO_CAPTURE_DEVICE_NV *************************** */

gits::OpenGL::CwglReleaseVideoCaptureDeviceNV::CwglReleaseVideoCaptureDeviceNV() {}

gits::OpenGL::CwglReleaseVideoCaptureDeviceNV::CwglReleaseVideoCaptureDeviceNV(
    BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice)
    : _return_value(return_value), _hDc(hDc), _hDevice(hDevice), _hwnd(WindowFromDC(hDc)) {}

gits::CArgument& gits::OpenGL::CwglReleaseVideoCaptureDeviceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDc, _hDevice, _hwnd);
}

void gits::OpenGL::CwglReleaseVideoCaptureDeviceNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglReleaseVideoCaptureDeviceNV((HDC)UpdateHDC(_hDc, _hwnd),
                                         (HVIDEOINPUTDEVICENV)*_hDevice);
#endif
}

/* ***************************** WGL_COPY_IMAGE_SUB_DATA_NV *************************** */

gits::OpenGL::CwglCopyImageSubDataNV::CwglCopyImageSubDataNV() {}

gits::OpenGL::CwglCopyImageSubDataNV::CwglCopyImageSubDataNV(BOOL return_value,
                                                             HGLRC hSrcRC,
                                                             GLuint srcName,
                                                             GLenum srcTarget,
                                                             GLint srcLevel,
                                                             GLint srcX,
                                                             GLint srcY,
                                                             GLint srcZ,
                                                             HGLRC hDstRC,
                                                             GLuint dstName,
                                                             GLenum dstTarget,
                                                             GLint dstLevel,
                                                             GLint dstX,
                                                             GLint dstY,
                                                             GLint dstZ,
                                                             GLsizei width,
                                                             GLsizei height,
                                                             GLsizei depth)
    : _return_value(return_value),
      _hSrcRC(hSrcRC),
      _srcName(srcName),
      _srcTarget(srcTarget),
      _srcLevel(srcLevel),
      _srcX(srcX),
      _srcY(srcY),
      _srcZ(srcZ),
      _hDstRC(hDstRC),
      _dstName(dstName),
      _dstTarget(dstTarget),
      _dstLevel(dstLevel),
      _dstX(dstX),
      _dstY(dstY),
      _dstZ(dstZ),
      _width(width),
      _height(height),
      _depth(depth) {}

gits::CArgument& gits::OpenGL::CwglCopyImageSubDataNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hSrcRC, _srcName, _srcTarget, _srcLevel, _srcX, _srcY,
                       _srcZ, _hDstRC, _dstName, _dstTarget, _dstLevel, _dstX, _dstY, _dstZ, _width,
                       _height, _depth);
}

void gits::OpenGL::CwglCopyImageSubDataNV::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglCopyImageSubDataNV((HGLRC)*_hSrcRC, *_srcName, *_srcTarget, *_srcLevel, *_srcX, *_srcY,
                                *_srcZ, (HGLRC)*_hDstRC, *_dstName, *_dstTarget, *_dstLevel, *_dstX,
                                *_dstY, *_dstZ, *_width, *_height, *_depth);
#endif
}

/* *************************** WGL_GET_LAYER_PALETTE_ENTRIES ***************************** */
gits::OpenGL::CwglGetLayerPaletteEntries::CwglGetLayerPaletteEntries() {}

gits::OpenGL::CwglGetLayerPaletteEntries::CwglGetLayerPaletteEntries(
    HDC hDc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr)
    : _hDc(hDc),
      _iLayerPlane(iLayerPlane),
      _iStart(iStart),
      _cEntries(cEntries),
      _hwnd(WindowFromDC(hDc)) {}

gits::CArgument& gits::OpenGL::CwglGetLayerPaletteEntries::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hDc, _iLayerPlane, _iStart, _cEntries, _hwnd);
}

void gits::OpenGL::CwglGetLayerPaletteEntries::Run() {
#if defined GITS_PLATFORM_WINDOWS
  static std::vector<COLORREF> pcr;

  pcr.resize(std::max<size_t>(pcr.size(), *_cEntries));
  drv.wgl.wglGetLayerPaletteEntries(UpdateHDC(_hDc, _hwnd, true), *_iLayerPlane, *_iStart,
                                    *_cEntries, &pcr[0]);
#endif
}

/* ***************************** WGL_GET_DEFAULT_PROC_ADDRESS *************************** */

gits::OpenGL::CwglGetDefaultProcAddress::CwglGetDefaultProcAddress() {}

gits::OpenGL::CwglGetDefaultProcAddress::CwglGetDefaultProcAddress(LPCSTR lpszProc)
    : _lpszProc(lpszProc, 0, 1) {}

gits::CArgument& gits::OpenGL::CwglGetDefaultProcAddress::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _lpszProc);
}

void gits::OpenGL::CwglGetDefaultProcAddress::Run() {
#if defined GITS_PLATFORM_WINDOWS
  drv.wgl.wglGetDefaultProcAddress(*_lpszProc);
#endif
}

/* ***************************** HELPER_WGL_UPDATE_WINDOW *************************** */

gits::OpenGL::ChelperWglUpdateWindow::ChelperWglUpdateWindow() {}

gits::OpenGL::ChelperWglUpdateWindow::ChelperWglUpdateWindow(HWND hwnd) : _hwnd(hwnd) {
#if defined GITS_PLATFORM_WINDOWS
  std::vector<HWND> hwnd_del_list;
  CHWND::CSArray chwnd_del_list(hwnd_del_list);
  if (_hwnd.Original() != NULL) {
    UpdateWindowsRec(_hwnd.Original(), _winparams, chwnd_del_list);
  }
#endif
}

gits::CArgument& gits::OpenGL::ChelperWglUpdateWindow::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hwnd, _winparams);
}

void gits::OpenGL::ChelperWglUpdateWindow::Run() {
  std::vector<HWND> hwnd_del_list;
  CHWND::CSArray chwnd_del_list(hwnd_del_list);
  if (_hwnd.Original() != NULL) {
    UpdateWindows(_hwnd, _winparams, chwnd_del_list);
  }
}
