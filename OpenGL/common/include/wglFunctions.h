// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   wglFunctions.h
*
*/

#pragma once

#include "openglFunction.h"
#include "nonglArguments.h"
#include "wglArguments.h"
#include "platform.h"
#include "openglTypes.h"
#include "mapping.h"

namespace gits {
namespace OpenGL {

#if defined GITS_PLATFORM_WINDOWS
void UpdateWindowsRec(HWND hwnd, Cint::CSArray& winparams, CHWND::CSArray& hwnd_del_list);
void UpdateWindows(const CHWND& hwnd,
                   const Cint::CSArray& winparams,
                   const CHWND::CSArray& hwnd_del_list);
#endif

/**
    * @brief OpenGL wglChoosePixelFormat() function call wrapper.
    *
    * OpenGL wglChoosePixelFormat() function call wrapper.
    * Returns: int
    */
class CwglChoosePixelFormat : public CFunction {
  CHDC _hdc;
  CHWND _hwnd;
  CPIXELFORMATDESCRIPTOR _pfd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CwglChoosePixelFormat();
  CwglChoosePixelFormat(int return_value, HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd);
  virtual unsigned Id() const {
    return ID_WGL_CHOOSE_PIXEL_FORMAT;
  }
  virtual const char* Name() const {
    return "wglChoosePixelFormat";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSetPixelFormat() function call wrapper.
    *
    * OpenGL wglSetPixelFormat() function call wrapper.
    * Returns: BOOL
    */
class CwglSetPixelFormat : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CHWND _hwnd;
  Cint _format;
  Cint::CSArray _attribs;
  Cint::CSArray _values;
  Cint::CSArray _winparams;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSetPixelFormat();
  CwglSetPixelFormat(BOOL return_value, HDC hdc, int format, const PIXELFORMATDESCRIPTOR* ppfd);
  virtual unsigned Id() const {
    return ID_WGL_SET_PIXEL_FORMAT;
  }
  virtual const char* Name() const {
    return "wglSetPixelFormat";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPixelFormat() function call wrapper.
    *
    * OpenGL wglGetPixelFormat() function call wrapper.
    * Returns: int
    */
class CwglGetPixelFormat : public CFunction {
  Cint _return_value;
  CHDC _hdc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetPixelFormat();
  CwglGetPixelFormat(int return_value, HDC hdc);
  virtual unsigned Id() const {
    return ID_WGL_GET_PIXEL_FORMAT;
  }
  virtual const char* Name() const {
    return "wglGetPixelFormat";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Write(CCodeOStream& stream) const {}
  virtual void Run();
};

/**
    * @brief OpenGL wglDescribePixelFormat() function call wrapper.
    *
    * OpenGL wglDescribePixelFormat() function call wrapper.
    * Returns: int
    */
class CwglDescribePixelFormat : public CFunction {
  Cint _return_value;
  CHDC _hdc;
  Cint _format;
  Cunsigned _nBytes;
  CPIXELFORMATDESCRIPTOR::CSArray _ppfd;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDescribePixelFormat();
  CwglDescribePixelFormat(
      int return_value, HDC hdc, int format, unsigned nBytes, PIXELFORMATDESCRIPTOR* ppfd);
  virtual unsigned Id() const {
    return ID_WGL_DESCRIBE_PIXEL_FORMAT;
  }
  virtual const char* Name() const {
    return "wglDescribePixelFormat";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCopyContext() function call wrapper.
    *
    * OpenGL wglCopyContext() function call wrapper.
    * Returns: BOOL
    */
class CwglCopyContext : public CFunction {
  CBOOL _return_value;
  CHGLRC _hglrcSrc;
  CHGLRC _hglrcDst;
  CUINT _mask;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCopyContext();
  CwglCopyContext(BOOL return_value, HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask);
  virtual unsigned Id() const {
    return ID_WGL_COPY_CONTEXT;
  }
  virtual const char* Name() const {
    return "wglCopyContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateContext() function call wrapper.
    *
    * OpenGL wglCreateContext() function call wrapper.
    * Returns: HGLRC
    */
class CwglCreateContext : public CFunction {
  CHGLRC _return_value;
  CHDC _hdc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateContext();
  CwglCreateContext(HGLRC return_value, HDC hdc);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_CONTEXT;
  }
  virtual const char* Name() const {
    return "wglCreateContext";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateLayerContext() function call wrapper.
    *
    * OpenGL wglCreateLayerContext() function call wrapper.
    * Returns: HGLRC
    */
class CwglCreateLayerContext : public CFunction {
  CHGLRC _return_value;
  CHDC _hdc;
  Cint _iLayerPlane;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateLayerContext();
  CwglCreateLayerContext(HGLRC return_value, HDC hdc, int iLayerPlane);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_LAYER_CONTEXT;
  }
  virtual const char* Name() const {
    return "wglCreateLayerContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDeleteContext() function call wrapper.
    *
    * OpenGL wglDeleteContext() function call wrapper.
    * Returns: BOOL
    */
class CwglDeleteContext : public CFunction {
  CBOOL _return_value;
  CHGLRC _hglrc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDeleteContext();
  CwglDeleteContext(BOOL return_value, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_DELETE_CONTEXT;
  }
  virtual const char* Name() const {
    return "wglDeleteContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetCurrentContext() function call wrapper.
    *
    * OpenGL wglGetCurrentContext() function call wrapper.
    * Returns: HGLRC
    */
class CwglGetCurrentContext : public CFunction {
  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CwglGetCurrentContext();
  CwglGetCurrentContext(HGLRC return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_CURRENT_CONTEXT;
  }
  virtual const char* Name() const {
    return "wglGetCurrentContext";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetCurrentDC() function call wrapper.
    *
    * OpenGL wglGetCurrentDC() function call wrapper.
    * Returns: HDC
    */
class CwglGetCurrentDC : public CFunction {
  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CwglGetCurrentDC();
  CwglGetCurrentDC(HDC return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_CURRENT_DC;
  }
  virtual const char* Name() const {
    return "wglGetCurrentDC";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetProcAddress() function call wrapper.
    *
    * OpenGL wglGetProcAddress() function call wrapper.
    * Returns: PROC
    */
class CwglGetProcAddress : public CFunction {
  Cchar::CSArray _lpszProc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CwglGetProcAddress();
  CwglGetProcAddress(LPCSTR lpszProc);
  virtual unsigned Id() const {
    return ID_WGL_GET_PROC_ADDRESS;
  }
  virtual const char* Name() const {
    return "wglGetProcAddress";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglMakeCurrent() function call wrapper.
    *
    * OpenGL wglMakeCurrent() function call wrapper.
    * Returns: BOOL
    */
class CwglMakeCurrent : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CHGLRC _hglrc;
  CHWND _hwnd;
  Cint::CSArray _winparams;
  CHWND::CSArray _hwnd_del_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglMakeCurrent();
  CwglMakeCurrent(BOOL return_value, HDC hdc, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_MAKE_CURRENT;
  }
  virtual const char* Name() const {
    return "wglMakeCurrent";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglShareLists() function call wrapper.
    *
    * OpenGL wglShareLists() function call wrapper.
    * Returns: BOOL
    */
class CwglShareLists : public CFunction {
  CBOOL _return_value;
  CHGLRC _hglrc1;
  CHGLRC _hglrc2;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglShareLists();
  CwglShareLists(BOOL return_value, HGLRC hglrc1, HGLRC hglrc2);
  virtual unsigned Id() const {
    return ID_WGL_SHARE_LISTS;
  }
  virtual const char* Name() const {
    return "wglShareLists";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglUseFontBitmapsA() function call wrapper.
    *
    * OpenGL wglUseFontBitmapsA() function call wrapper.
    * Returns: BOOL
    */
class CwglUseFontBitmapsA : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CDWORD _first;
  CDWORD _count;
  CDWORD _listBase;
  CHWND _hwnd;
  Cchar::CSArray _fontName;
  Cint::CSArray _fontArgs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglUseFontBitmapsA();
  CwglUseFontBitmapsA(BOOL return_value,
                      HDC hdc,
                      DWORD first,
                      DWORD count,
                      DWORD listBase,
                      std::string str,
                      std::vector<int> args);
  virtual unsigned Id() const {
    return ID_WGL_USE_FONT_BITMAPS_A;
  }
  virtual const char* Name() const {
    return "wglUseFontBitmapsA";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglUseFontBitmapsW() function call wrapper.
    *
    * OpenGL wglUseFontBitmapsW() function call wrapper.
    * Returns: BOOL
    */
class CwglUseFontBitmapsW : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CDWORD _first;
  CDWORD _count;
  CDWORD _listBase;
  CHWND _hwnd;
  Cchar::CSArray _fontName;
  Cint::CSArray _fontArgs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglUseFontBitmapsW();
  CwglUseFontBitmapsW(BOOL return_value,
                      HDC hdc,
                      DWORD first,
                      DWORD count,
                      DWORD listBase,
                      std::string str,
                      std::vector<int> args);
  virtual unsigned Id() const {
    return ID_WGL_USE_FONT_BITMAPS_W;
  }
  virtual const char* Name() const {
    return "wglUseFontBitmapsW";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSwapLayerBuffers() function call wrapper.
    *
    * OpenGL wglSwapLayerBuffers() function call wrapper.
    * Returns: BOOL
    */
class CwglSwapLayerBuffers : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CUINT _fuPlanes;
  CHWND _hwnd;
  Cint::CSArray _winparams;
  CHWND::CSArray _hwnd_del_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSwapLayerBuffers();
  CwglSwapLayerBuffers(BOOL return_value, HDC hdc, UINT fuPlanes);
  virtual unsigned Id() const {
    return ID_WGL_SWAP_LAYER_BUFFERS;
  }
  virtual const char* Name() const {
    return "wglSwapLayerBuffers";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSwapBuffers() function call wrapper.
    *
    * OpenGL wglSwapBuffers() function call wrapper.
    * Returns: BOOL
    */
class CwglSwapBuffers : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CHWND _hwnd;
  Cint::CSArray _winparams;
  CHWND::CSArray _hwnd_del_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSwapBuffers();
  CwglSwapBuffers(BOOL return_value, HDC hdc);
  virtual unsigned Id() const {
    return ID_WGL_SWAP_BUFFERS;
  }
  virtual const char* Name() const {
    return "wglSwapBuffers";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSwapMultipleBuffers() function call wrapper.
    *
    * OpenGL wglSwapMultipleBuffers() function call wrapper.
    * Returns: BOOL
    */
class CwglSwapMultipleBuffers : public CFunction {
  CBOOL _return_value;
  CGLuint _buffers;
  CHDC::CSArray _hdc;
  CHWND _hwnd;
  Cint::CSArray _winparams;
  CHWND::CSArray _hwnd_del_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSwapMultipleBuffers();
  CwglSwapMultipleBuffers(BOOL return_value, GLuint buffers, HDC* hdc);
  virtual unsigned Id() const {
    return ID_WGL_SWAP_MULTIPLE_BUFFERS;
  }
  virtual const char* Name() const {
    return "wglSwapBuffers";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglUseFontOutlinesA() function call wrapper.
    *
    * OpenGL wglUseFontOutlinesA() function call wrapper.
    * Returns: BOOL
    */
class CwglUseFontOutlinesA : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CDWORD _first;
  CDWORD _count;
  CDWORD _listBase;
  CFLOAT _deviation;
  CFLOAT _extrusion;
  Cint _format;
  CHWND _hwnd;
  Cchar::CSArray _fontName;
  Cint::CSArray _fontArgs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 10;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglUseFontOutlinesA();
  CwglUseFontOutlinesA(BOOL return_value,
                       HDC hdc,
                       DWORD first,
                       DWORD count,
                       DWORD listBase,
                       FLOAT deviation,
                       FLOAT extrusion,
                       int format,
                       LPGLYPHMETRICSFLOAT lpgmf,
                       std::string str,
                       std::vector<int> args);
  virtual unsigned Id() const {
    return ID_WGL_USE_FONT_OUTLINES_A;
  }
  virtual const char* Name() const {
    return "wglUseFontOutlinesA";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglUseFontOutlinesW() function call wrapper.
    *
    * OpenGL wglUseFontOutlinesW() function call wrapper.
    * Returns: BOOL
    */
class CwglUseFontOutlinesW : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CDWORD _first;
  CDWORD _count;
  CDWORD _listBase;
  CFLOAT _deviation;
  CFLOAT _extrusion;
  Cint _format;
  CHWND _hwnd;
  Cchar::CSArray _fontName;
  Cint::CSArray _fontArgs;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 10;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglUseFontOutlinesW();
  CwglUseFontOutlinesW(BOOL return_value,
                       HDC hdc,
                       DWORD first,
                       DWORD count,
                       DWORD listBase,
                       FLOAT deviation,
                       FLOAT extrusion,
                       int format,
                       LPGLYPHMETRICSFLOAT lpgmf,
                       std::string str,
                       std::vector<int> args);
  virtual unsigned Id() const {
    return ID_WGL_USE_FONT_OUTLINES_W;
  }
  virtual const char* Name() const {
    return "wglUseFontOutlinesW";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateBufferRegionARB() function call wrapper.
    *
    * OpenGL wglCreateBufferRegionARB() function call wrapper.
    * Returns: HANDLE
    */
class CwglCreateBufferRegionARB : public CFunction {
  CHANDLE _return_value;
  CHDC _hDC;
  Cint _iLayerPlane;
  CUINT _uType;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateBufferRegionARB();
  CwglCreateBufferRegionARB(HANDLE return_value, HDC hDC, int iLayerPlane, UINT uType);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_BUFFER_REGION_ARB;
  }
  virtual const char* Name() const {
    return "wglCreateBufferRegionARB";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDeleteBufferRegionARB() function call wrapper.
    *
    * OpenGL wglDeleteBufferRegionARB() function call wrapper.
    * Returns: VOID
    */
class CwglDeleteBufferRegionARB : public CFunction {
  CHANDLE _hRegion;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CwglDeleteBufferRegionARB();
  CwglDeleteBufferRegionARB(HANDLE hRegion);
  virtual unsigned Id() const {
    return ID_WGL_DELETE_BUFFER_REGION_ARB;
  }
  virtual const char* Name() const {
    return "wglDeleteBufferRegionARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSaveBufferRegionARB() function call wrapper.
    *
    * OpenGL wglSaveBufferRegionARB() function call wrapper.
    * Returns: BOOL
    */
class CwglSaveBufferRegionARB : public CFunction {
  CBOOL _return_value;
  CHANDLE _hRegion;
  Cint _x;
  Cint _y;
  Cint _width;
  Cint _height;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSaveBufferRegionARB();
  CwglSaveBufferRegionARB(BOOL return_value, HANDLE hRegion, int x, int y, int width, int height);
  virtual unsigned Id() const {
    return ID_WGL_SAVE_BUFFER_REGION_ARB;
  }
  virtual const char* Name() const {
    return "wglSaveBufferRegionARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglRestoreBufferRegionARB() function call wrapper.
    *
    * OpenGL wglRestoreBufferRegionARB() function call wrapper.
    * Returns: BOOL
    */
class CwglRestoreBufferRegionARB : public CFunction {
  CBOOL _return_value;
  CHANDLE _hRegion;
  Cint _x;
  Cint _y;
  Cint _width;
  Cint _height;
  Cint _xSrc;
  Cint _ySrc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglRestoreBufferRegionARB();
  CwglRestoreBufferRegionARB(
      BOOL return_value, HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
  virtual unsigned Id() const {
    return ID_WGL_RESTORE_BUFFER_REGION_ARB;
  }
  virtual const char* Name() const {
    return "wglRestoreBufferRegionARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetExtensionsStringARB() function call wrapper.
    *
    * OpenGL wglGetExtensionsStringARB() function call wrapper.
    * Returns: const char *
    */
class CwglGetExtensionsStringARB : public CFunction {
  CHDC _hdc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  CwglGetExtensionsStringARB();
  CwglGetExtensionsStringARB(const char* return_value, HDC hdc);
  virtual unsigned Id() const {
    return ID_WGL_GET_EXTENSIONS_STRING_ARB;
  }
  virtual const char* Name() const {
    return "wglGetExtensionsStringARB";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPixelFormatAttribivARB() function call wrapper.
    *
    * OpenGL wglGetPixelFormatAttribivARB() function call wrapper.
    * Returns: BOOL
    */
class CwglGetPixelFormatAttribivARB : public CFunction {
  CHDC _hdc;
  Cint _iPixelFormat;
  Cint _iLayerPlane;
  CUINT _nAttributes;
  COutArgument<0> _piAttributes;
  COutArgument<1> _piValues;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }

public:
  CwglGetPixelFormatAttribivARB();
  CwglGetPixelFormatAttribivARB(BOOL return_value,
                                HDC hdc,
                                int iPixelFormat,
                                int iLayerPlane,
                                UINT nAttributes,
                                const int* piAttributes,
                                int* piValues);
  virtual unsigned Id() const {
    return ID_WGL_GET_PIXEL_FORMAT_ATTRIBIV_ARB;
  }
  virtual const char* Name() const {
    return "wglGetPixelFormatAttribivARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPixelFormatAttribfvARB() function call wrapper.
    *
    * OpenGL wglGetPixelFormatAttribfvARB() function call wrapper.
    * Returns: BOOL
    */
class CwglGetPixelFormatAttribfvARB : public CFunction {
  CHDC _hdc;
  Cint _iPixelFormat;
  Cint _iLayerPlane;
  CUINT _nAttributes;
  COutArgument<0> _piAttributes;
  COutArgument<1> _pfValues;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }

public:
  CwglGetPixelFormatAttribfvARB();
  CwglGetPixelFormatAttribfvARB(BOOL return_value,
                                HDC hdc,
                                int iPixelFormat,
                                int iLayerPlane,
                                UINT nAttributes,
                                const int* piAttributes,
                                FLOAT* pfValues);
  virtual unsigned Id() const {
    return ID_WGL_GET_PIXEL_FORMAT_ATTRIBFV_ARB;
  }
  virtual const char* Name() const {
    return "wglGetPixelFormatAttribfvARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglChoosePixelFormatARB() function call wrapper.
    *
    * OpenGL wglChoosePixelFormatARB() function call wrapper.
    * Returns: BOOL
    */
class CwglChoosePixelFormatARB : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  Cint::CSArray _piAttribIList;
  CFLOAT::CSArray _pfAttribFList;
  CUINT _nMaxFormats;
  Cint::CSArray _piFormats;
  CUINT::CSArray _nNumFormats;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglChoosePixelFormatARB();
  CwglChoosePixelFormatARB(BOOL return_value,
                           HDC hdc,
                           const int* piAttribIList,
                           const FLOAT* pfAttribFList,
                           UINT nMaxFormats,
                           int* piFormats,
                           UINT* nNumFormats);
  virtual unsigned Id() const {
    return ID_WGL_CHOOSE_PIXEL_FORMAT_ARB;
  }
  virtual const char* Name() const {
    return "wglChoosePixelFormatARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglMakeContextCurrentARB() function call wrapper.
    *
    * OpenGL wglMakeContextCurrentARB() function call wrapper.
    * Returns: BOOL
    */
class CwglMakeContextCurrentARB : public CFunction {
  CBOOL _return_value;
  CHDC _hDrawDC;
  CHDC _hReadDC;
  CHGLRC _hglrc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglMakeContextCurrentARB();
  CwglMakeContextCurrentARB(BOOL return_value, HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_MAKE_CONTEXT_CURRENT_ARB;
  }
  virtual const char* Name() const {
    return "wglMakeContextCurrentARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetCurrentReadDCARB() function call wrapper.
    *
    * OpenGL wglGetCurrentReadDCARB() function call wrapper.
    * Returns: HDC
    */
class CwglGetCurrentReadDCARB : public CFunction {
  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CwglGetCurrentReadDCARB();
  CwglGetCurrentReadDCARB(HDC return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_CURRENT_READ_DC_ARB;
  }
  virtual const char* Name() const {
    return "wglGetCurrentReadDCARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreatePbufferARB() function call wrapper.
    *
    * OpenGL wglCreatePbufferARB() function call wrapper.
    * Returns: HPBUFFERARB
    */
class CwglCreatePbufferARB : public CFunction {
  CHPBUFFERARB _return_value;
  CHDC _hDC;
  Cint _iPixelFormat;
  Cint _iWidth;
  Cint _iHeight;
  Cint::CSArray _piAttribList;
  Cint::CSArray _attribs;
  Cint::CSArray _values;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreatePbufferARB();
  CwglCreatePbufferARB(HPBUFFERARB return_value,
                       HDC hDC,
                       int iPixelFormat,
                       int iWidth,
                       int iHeight,
                       const int* piAttribList);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_PBUFFER_ARB;
  }
  virtual const char* Name() const {
    return "wglCreatePbufferARB";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPbufferDCARB() function call wrapper.
    *
    * OpenGL wglGetPbufferDCARB() function call wrapper.
    * Returns: HDC
    */
class CwglGetPbufferDCARB : public CFunction {
  CHDC _return_value;
  CHPBUFFERARB _hPbuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetPbufferDCARB();
  CwglGetPbufferDCARB(HDC return_value, HPBUFFERARB hPbuffer);
  virtual unsigned Id() const {
    return ID_WGL_GET_PBUFFER_DC_ARB;
  }
  virtual const char* Name() const {
    return "wglGetPbufferDCARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleasePbufferDCARB() function call wrapper.
    *
    * OpenGL wglReleasePbufferDCARB() function call wrapper.
    * Returns: int
    */
class CwglReleasePbufferDCARB : public CFunction {
  Cint _return_value;
  CHPBUFFERARB _hPbuffer;
  CHDC _hDC;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleasePbufferDCARB();
  CwglReleasePbufferDCARB(int return_value, HPBUFFERARB hPbuffer, HDC hDC);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_PBUFFER_DC_ARB;
  }
  virtual const char* Name() const {
    return "wglReleasePbufferDCARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDestroyPbufferARB() function call wrapper.
    *
    * OpenGL wglDestroyPbufferARB() function call wrapper.
    * Returns: BOOL
    */
class CwglDestroyPbufferARB : public CFunction {
  CBOOL _return_value;
  CHPBUFFERARB _hPbuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDestroyPbufferARB();
  CwglDestroyPbufferARB(BOOL return_value, HPBUFFERARB hPbuffer);
  virtual unsigned Id() const {
    return ID_WGL_DESTROY_PBUFFER_ARB;
  }
  virtual const char* Name() const {
    return "wglDestroyPbufferARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryPbufferARB() function call wrapper.
    *
    * OpenGL wglQueryPbufferARB() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryPbufferARB : public CFunction {
  CHPBUFFERARB _hPbuffer;
  Cint _iAttribute;
  COutArgument<0> _piValue;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CwglQueryPbufferARB();
  CwglQueryPbufferARB(BOOL return_value, HPBUFFERARB hPbuffer, int iAttribute, int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_PBUFFER_ARB;
  }
  virtual const char* Name() const {
    return "wglQueryPbufferARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBindTexImageARB() function call wrapper.
    *
    * OpenGL wglBindTexImageARB() function call wrapper.
    * Returns: BOOL
    */
class CwglBindTexImageARB : public CFunction {
  CBOOL _return_value;
  CHPBUFFERARB _hPbuffer;
  Cint _iBuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBindTexImageARB();
  CwglBindTexImageARB(BOOL return_value, HPBUFFERARB hPbuffer, int iBuffer);
  virtual unsigned Id() const {
    return ID_WGL_BIND_TEX_IMAGE_ARB;
  }
  virtual const char* Name() const {
    return "wglBindTexImageARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleaseTexImageARB() function call wrapper.
    *
    * OpenGL wglReleaseTexImageARB() function call wrapper.
    * Returns: BOOL
    */
class CwglReleaseTexImageARB : public CFunction {
  CBOOL _return_value;
  CHPBUFFERARB _hPbuffer;
  Cint _iBuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleaseTexImageARB();
  CwglReleaseTexImageARB(BOOL return_value, HPBUFFERARB hPbuffer, int iBuffer);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_TEX_IMAGE_ARB;
  }
  virtual const char* Name() const {
    return "wglReleaseTexImageARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSetPbufferAttribARB() function call wrapper.
    *
    * OpenGL wglSetPbufferAttribARB() function call wrapper.
    * Returns: BOOL
    */
class CwglSetPbufferAttribARB : public CFunction {
  CBOOL _return_value;
  CHPBUFFERARB _hPbuffer;
  Cint::CSArray _piAttribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSetPbufferAttribARB();
  CwglSetPbufferAttribARB(BOOL return_value, HPBUFFERARB hPbuffer, const int* piAttribList);
  virtual unsigned Id() const {
    return ID_WGL_SET_PBUFFER_ATTRIB_ARB;
  }
  virtual const char* Name() const {
    return "wglSetPbufferAttribARB";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateContextAttribsARB() function call wrapper.
    *
    * OpenGL wglCreateContextAttribsARB() function call wrapper.
    * Returns: HGLRC
    */
class CwglCreateContextAttribsARB : public CFunction {
  CHGLRC _return_value;
  CHDC _hDC;
  CHGLRC _hShareContext;
  Cint::CSArray _attribList;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateContextAttribsARB();
  CwglCreateContextAttribsARB(HGLRC return_value,
                              HDC hDC,
                              HGLRC hShareContext,
                              const int* attribList);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_CONTEXT_ATTRIBS_ARB;
  }
  virtual const char* Name() const {
    return "wglCreateContextAttribsARB";
  }
  virtual const char* Suffix() const {
    return "_wrap";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateDisplayColorTableEXT() function call wrapper.
    *
    * OpenGL wglCreateDisplayColorTableEXT() function call wrapper.
    * Returns: GLboolean
    */
class CwglCreateDisplayColorTableEXT : public CFunction {
  CGLboolean _return_value;
  CGLushort _id;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateDisplayColorTableEXT();
  CwglCreateDisplayColorTableEXT(GLboolean return_value, GLushort id);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_DISPLAY_COLOR_TABLE_EXT;
  }
  virtual const char* Name() const {
    return "wglCreateDisplayColorTableEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglLoadDisplayColorTableEXT() function call wrapper.
    *
    * OpenGL wglLoadDisplayColorTableEXT() function call wrapper.
    * Returns: GLboolean
    */
class CwglLoadDisplayColorTableEXT : public CFunction {
  CGLboolean _return_value;
  CGLushort::CSArray _table;
  CGLuint _length;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglLoadDisplayColorTableEXT();
  CwglLoadDisplayColorTableEXT(GLboolean return_value, const GLushort* table, GLuint length);
  virtual unsigned Id() const {
    return ID_WGL_LOAD_DISPLAY_COLOR_TABLE_EXT;
  }
  virtual const char* Name() const {
    return "wglLoadDisplayColorTableEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBindDisplayColorTableEXT() function call wrapper.
    *
    * OpenGL wglBindDisplayColorTableEXT() function call wrapper.
    * Returns: GLboolean
    */
class CwglBindDisplayColorTableEXT : public CFunction {
  CGLboolean _return_value;
  CGLushort _id;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBindDisplayColorTableEXT();
  CwglBindDisplayColorTableEXT(GLboolean return_value, GLushort id);
  virtual unsigned Id() const {
    return ID_WGL_BIND_DISPLAY_COLOR_TABLE_EXT;
  }
  virtual const char* Name() const {
    return "wglBindDisplayColorTableEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDestroyDisplayColorTableEXT() function call wrapper.
    *
    * OpenGL wglDestroyDisplayColorTableEXT() function call wrapper.
    * Returns: VOID
    */
class CwglDestroyDisplayColorTableEXT : public CFunction {
  CGLushort _id;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CwglDestroyDisplayColorTableEXT();
  CwglDestroyDisplayColorTableEXT(GLushort id);
  virtual unsigned Id() const {
    return ID_WGL_DESTROY_DISPLAY_COLOR_TABLE_EXT;
  }
  virtual const char* Name() const {
    return "wglDestroyDisplayColorTableEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetExtensionsStringEXT() function call wrapper.
    *
    * OpenGL wglGetExtensionsStringEXT() function call wrapper.
    * Returns: const char *
    */
class CwglGetExtensionsStringEXT : public CFunction {
  Cchar::CSArray _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetExtensionsStringEXT();
  CwglGetExtensionsStringEXT(const char* return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_EXTENSIONS_STRING_EXT;
  }
  virtual const char* Name() const {
    return "wglGetExtensionsStringEXT";
  }
  virtual void Write(CCodeOStream& stream) const {}
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglMakeContextCurrentEXT() function call wrapper.
    *
    * OpenGL wglMakeContextCurrentEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglMakeContextCurrentEXT : public CFunction {
  CBOOL _return_value;
  CHDC _hDrawDC;
  CHDC _hReadDC;
  CHGLRC _hglrc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglMakeContextCurrentEXT();
  CwglMakeContextCurrentEXT(BOOL return_value, HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_MAKE_CONTEXT_CURRENT_EXT;
  }
  virtual const char* Name() const {
    return "wglMakeContextCurrentEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetCurrentReadDCEXT() function call wrapper.
    *
    * OpenGL wglGetCurrentReadDCEXT() function call wrapper.
    * Returns: HDC
    */
class CwglGetCurrentReadDCEXT : public CFunction {
  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }

public:
  CwglGetCurrentReadDCEXT();
  CwglGetCurrentReadDCEXT(HDC return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_CURRENT_READ_DC_EXT;
  }
  virtual const char* Name() const {
    return "wglGetCurrentReadDCEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreatePbufferEXT() function call wrapper.
    *
    * OpenGL wglCreatePbufferEXT() function call wrapper.
    * Returns: HPBUFFEREXT
    */
class CwglCreatePbufferEXT : public CFunction {
  CHPBUFFEREXT _return_value;
  CHDC _hDC;
  Cint _iPixelFormat;
  Cint _iWidth;
  Cint _iHeight;
  Cint::CSArray _piAttribList;
  Cint::CSArray _attribs;
  Cint::CSArray _values;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreatePbufferEXT();
  CwglCreatePbufferEXT(HPBUFFEREXT return_value,
                       HDC hDC,
                       int iPixelFormat,
                       int iWidth,
                       int iHeight,
                       const int* piAttribList);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_PBUFFER_EXT;
  }
  virtual const char* Name() const {
    return "wglCreatePbufferEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPbufferDCEXT() function call wrapper.
    *
    * OpenGL wglGetPbufferDCEXT() function call wrapper.
    * Returns: HDC
    */
class CwglGetPbufferDCEXT : public CFunction {
  CHDC _return_value;
  CHPBUFFEREXT _hPbuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetPbufferDCEXT();
  CwglGetPbufferDCEXT(HDC return_value, HPBUFFEREXT hPbuffer);
  virtual unsigned Id() const {
    return ID_WGL_GET_PBUFFER_DC_EXT;
  }
  virtual const char* Name() const {
    return "wglGetPbufferDCEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleasePbufferDCEXT() function call wrapper.
    *
    * OpenGL wglReleasePbufferDCEXT() function call wrapper.
    * Returns: int
    */
class CwglReleasePbufferDCEXT : public CFunction {
  Cint _return_value;
  CHPBUFFEREXT _hPbuffer;
  CHDC _hDC;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleasePbufferDCEXT();
  CwglReleasePbufferDCEXT(int return_value, HPBUFFEREXT hPbuffer, HDC hDC);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_PBUFFER_DC_EXT;
  }
  virtual const char* Name() const {
    return "wglReleasePbufferDCEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDestroyPbufferEXT() function call wrapper.
    *
    * OpenGL wglDestroyPbufferEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglDestroyPbufferEXT : public CFunction {
  CBOOL _return_value;
  CHPBUFFEREXT _hPbuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDestroyPbufferEXT();
  CwglDestroyPbufferEXT(BOOL return_value, HPBUFFEREXT hPbuffer);
  virtual unsigned Id() const {
    return ID_WGL_DESTROY_PBUFFER_EXT;
  }
  virtual const char* Name() const {
    return "wglDestroyPbufferEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryPbufferEXT() function call wrapper.
    *
    * OpenGL wglQueryPbufferEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryPbufferEXT : public CFunction {
  CHPBUFFEREXT _hPbuffer;
  Cint _iAttribute;
  COutArgument<0> _piValue;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CwglQueryPbufferEXT();
  CwglQueryPbufferEXT(BOOL return_value, HPBUFFEREXT hPbuffer, int iAttribute, int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_PBUFFER_EXT;
  }
  virtual const char* Name() const {
    return "wglQueryPbufferEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPixelFormatAttribivEXT() function call wrapper.
    *
    * OpenGL wglGetPixelFormatAttribivEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglGetPixelFormatAttribivEXT : public CFunction {
  CHDC _hdc;
  Cint _iPixelFormat;
  Cint _iLayerPlane;
  CUINT _nAttributes;
  COutArgument<0> _piAttributes;
  COutArgument<1> _piValues;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }

public:
  CwglGetPixelFormatAttribivEXT();
  CwglGetPixelFormatAttribivEXT(BOOL return_value,
                                HDC hdc,
                                int iPixelFormat,
                                int iLayerPlane,
                                UINT nAttributes,
                                int* piAttributes,
                                int* piValues);
  virtual unsigned Id() const {
    return ID_WGL_GET_PIXEL_FORMAT_ATTRIBIV_EXT;
  }
  virtual const char* Name() const {
    return "wglGetPixelFormatAttribivEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetPixelFormatAttribfvEXT() function call wrapper.
    *
    * OpenGL wglGetPixelFormatAttribfvEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglGetPixelFormatAttribfvEXT : public CFunction {
  CHDC _hdc;
  Cint _iPixelFormat;
  Cint _iLayerPlane;
  CUINT _nAttributes;
  COutArgument<0> _piAttributes;
  COutArgument<1> _pfValues;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 7;
  }

public:
  CwglGetPixelFormatAttribfvEXT();
  CwglGetPixelFormatAttribfvEXT(BOOL return_value,
                                HDC hdc,
                                int iPixelFormat,
                                int iLayerPlane,
                                UINT nAttributes,
                                int* piAttributes,
                                FLOAT* pfValues);
  virtual unsigned Id() const {
    return ID_WGL_GET_PIXEL_FORMAT_ATTRIBFV_EXT;
  }
  virtual const char* Name() const {
    return "wglGetPixelFormatAttribfvEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglChoosePixelFormatEXT() function call wrapper.
    *
    * OpenGL wglChoosePixelFormatEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglChoosePixelFormatEXT : public CFunction {
  CHDC _hdc;
  Cint::CSArray _piAttribIList;
  CFLOAT::CSArray _pfAttribFList;
  CUINT _nMaxFormats;
  Cint::CSArray _piFormats;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }

public:
  CwglChoosePixelFormatEXT();
  CwglChoosePixelFormatEXT(BOOL return_value,
                           HDC hdc,
                           const int* piAttribIList,
                           const FLOAT* pfAttribFList,
                           UINT nMaxFormats,
                           int* piFormats,
                           UINT* nNumFormats);
  virtual unsigned Id() const {
    return ID_WGL_CHOOSE_PIXEL_FORMAT_EXT;
  }
  virtual const char* Name() const {
    return "wglChoosePixelFormatEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSwapIntervalEXT() function call wrapper.
    *
    * OpenGL wglSwapIntervalEXT() function call wrapper.
    * Returns: BOOL
    */
class CwglSwapIntervalEXT : public CFunction {
  CBOOL _return_value;
  Cint _interval;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSwapIntervalEXT();
  CwglSwapIntervalEXT(BOOL return_value, int interval);
  virtual unsigned Id() const {
    return ID_WGL_SWAP_INTERVAL_EXT;
  }
  virtual const char* Name() const {
    return "wglSwapIntervalEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetSwapIntervalEXT() function call wrapper.
    *
    * OpenGL wglGetSwapIntervalEXT() function call wrapper.
    * Returns: int
    */
class CwglGetSwapIntervalEXT : public CFunction {
  Cint _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetSwapIntervalEXT();
  CwglGetSwapIntervalEXT(int return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_SWAP_INTERVAL_EXT;
  }
  virtual const char* Name() const {
    return "wglGetSwapIntervalEXT";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglAllocateMemoryNV() function call wrapper.
    *
    * OpenGL wglAllocateMemoryNV() function call wrapper.
    * Returns: void*
    */
class CwglAllocateMemoryNV : public CFunction {
  CvoidPtr _return_value;
  CGLsizei _size;
  CGLfloat _readfreq;
  CGLfloat _writefreq;
  CGLfloat _priority;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglAllocateMemoryNV();
  CwglAllocateMemoryNV(
      void* return_value, GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
  virtual unsigned Id() const {
    return ID_WGL_ALLOCATE_MEMORY_NV;
  }
  virtual const char* Name() const {
    return "wglAllocateMemoryNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglFreeMemoryNV() function call wrapper.
    *
    * OpenGL wglFreeMemoryNV() function call wrapper.
    * Returns: void
    */
class CwglFreeMemoryNV : public CFunction {
  CvoidPtr _pointer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CwglFreeMemoryNV();
  CwglFreeMemoryNV(void* pointer);
  virtual unsigned Id() const {
    return ID_WGL_FREE_MEMORY_NV;
  }
  virtual const char* Name() const {
    return "wglFreeMemoryNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetSyncValuesOML() function call wrapper.
    *
    * OpenGL wglGetSyncValuesOML() function call wrapper.
    * Returns: BOOL
    */
class CwglGetSyncValuesOML : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CINT64::CSArray _ust;
  CINT64::CSArray _msc;
  CINT64::CSArray _sbc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetSyncValuesOML();
  CwglGetSyncValuesOML(BOOL return_value, HDC hdc, INT64* ust, INT64* msc, INT64* sbc);
  virtual unsigned Id() const {
    return ID_WGL_GET_SYNC_VALUES_OML;
  }
  virtual const char* Name() const {
    return "wglGetSyncValuesOML";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetMscRateOML() function call wrapper.
    *
    * OpenGL wglGetMscRateOML() function call wrapper.
    * Returns: BOOL
    */
class CwglGetMscRateOML : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CINT32::CSArray _numerator;
  CINT32::CSArray _denominator;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetMscRateOML();
  CwglGetMscRateOML(BOOL return_value, HDC hdc, INT32* numerator, INT32* denominator);
  virtual unsigned Id() const {
    return ID_WGL_GET_MSC_RATE_OML;
  }
  virtual const char* Name() const {
    return "wglGetMscRateOML";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSwapBuffersMscOML() function call wrapper.
    *
    * OpenGL wglSwapBuffersMscOML() function call wrapper.
    * Returns: INT64
    */
class CwglSwapBuffersMscOML : public CFunction {
  CINT64 _return_value;
  CHDC _hdc;
  CINT64 _target_msc;
  CINT64 _divisor;
  CINT64 _remainder;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSwapBuffersMscOML();
  CwglSwapBuffersMscOML(
      INT64 return_value, HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
  virtual unsigned Id() const {
    return ID_WGL_SWAP_BUFFERS_MSC_OML;
  }
  virtual const char* Name() const {
    return "wglSwapBuffersMscOML";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSwapLayerBuffersMscOML() function call wrapper.
    *
    * OpenGL wglSwapLayerBuffersMscOML() function call wrapper.
    * Returns: INT64
    */
class CwglSwapLayerBuffersMscOML : public CFunction {
  CINT64 _return_value;
  CHDC _hdc;
  Cint _fuPlanes;
  CINT64 _target_msc;
  CINT64 _divisor;
  CINT64 _remainder;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSwapLayerBuffersMscOML();
  CwglSwapLayerBuffersMscOML(
      INT64 return_value, HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
  virtual unsigned Id() const {
    return ID_WGL_SWAP_LAYER_BUFFERS_MSC_OML;
  }
  virtual const char* Name() const {
    return "wglSwapLayerBuffersMscOML";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglWaitForMscOML() function call wrapper.
    *
    * OpenGL wglWaitForMscOML() function call wrapper.
    * Returns: BOOL
    */
class CwglWaitForMscOML : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CINT64 _target_msc;
  CINT64 _divisor;
  CINT64 _remainder;
  CINT64::CSArray _ust;
  CINT64::CSArray _msc;
  CINT64::CSArray _sbc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 8;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglWaitForMscOML();
  CwglWaitForMscOML(BOOL return_value,
                    HDC hdc,
                    INT64 target_msc,
                    INT64 divisor,
                    INT64 remainder,
                    INT64* ust,
                    INT64* msc,
                    INT64* sbc);
  virtual unsigned Id() const {
    return ID_WGL_WAIT_FOR_MSC_OML;
  }
  virtual const char* Name() const {
    return "wglWaitForMscOML";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglWaitForSbcOML() function call wrapper.
    *
    * OpenGL wglWaitForSbcOML() function call wrapper.
    * Returns: BOOL
    */
class CwglWaitForSbcOML : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;
  CINT64 _target_sbc;
  CINT64::CSArray _ust;
  CINT64::CSArray _msc;
  CINT64::CSArray _sbc;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglWaitForSbcOML();
  CwglWaitForSbcOML(
      BOOL return_value, HDC hdc, INT64 target_sbc, INT64* ust, INT64* msc, INT64* sbc);
  virtual unsigned Id() const {
    return ID_WGL_WAIT_FOR_SBC_OML;
  }
  virtual const char* Name() const {
    return "wglWaitForSbcOML";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetDigitalVideoParametersI3D() function call wrapper.
    *
    * OpenGL wglGetDigitalVideoParametersI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetDigitalVideoParametersI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _iAttribute;
  Cint::CSArray _piValue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetDigitalVideoParametersI3D();
  CwglGetDigitalVideoParametersI3D(BOOL return_value, HDC hDC, int iAttribute, int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_GET_DIGITAL_VIDEO_PARAMETERS_I3D;
  }
  virtual const char* Name() const {
    return "wglGetDigitalVideoParametersI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSetDigitalVideoParametersI3D() function call wrapper.
    *
    * OpenGL wglSetDigitalVideoParametersI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglSetDigitalVideoParametersI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _iAttribute;
  Cint::CSArray _piValue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSetDigitalVideoParametersI3D();
  CwglSetDigitalVideoParametersI3D(BOOL return_value, HDC hDC, int iAttribute, const int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_SET_DIGITAL_VIDEO_PARAMETERS_I3D;
  }
  virtual const char* Name() const {
    return "wglSetDigitalVideoParametersI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
    * @brief OpenGL wglGetGammaTableParametersI3D() function call wrapper.
    *
    * OpenGL wglGetGammaTableParametersI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetGammaTableParametersI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _iAttribute;
  Cint::CSArray _piValue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGammaTableParametersI3D();
  CwglGetGammaTableParametersI3D(BOOL return_value, HDC hDC, int iAttribute, int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_GET_GAMMA_TABLE_PARAMETERS_I3D;
  }
  virtual const char* Name() const {
    return "wglGetGammaTableParametersI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSetGammaTableParametersI3D() function call wrapper.
    *
    * OpenGL wglSetGammaTableParametersI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglSetGammaTableParametersI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _iAttribute;
  Cint::CSArray _piValue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSetGammaTableParametersI3D();
  CwglSetGammaTableParametersI3D(BOOL return_value, HDC hDC, int iAttribute, const int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_SET_GAMMA_TABLE_PARAMETERS_I3D;
  }
  virtual const char* Name() const {
    return "wglSetGammaTableParametersI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGammaTableI3D() function call wrapper.
    *
    * OpenGL wglGetGammaTableI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetGammaTableI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _iEntries;
  CUSHORT::CSArray _puRed;
  CUSHORT::CSArray _puGreen;
  CUSHORT::CSArray _puBlue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGammaTableI3D();
  CwglGetGammaTableI3D(
      BOOL return_value, HDC hDC, int iEntries, USHORT* puRed, USHORT* puGreen, USHORT* puBlue);
  virtual unsigned Id() const {
    return ID_WGL_GET_GAMMA_TABLE_I3D;
  }
  virtual const char* Name() const {
    return "wglGetGammaTableI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSetGammaTableI3D() function call wrapper.
    *
    * OpenGL wglSetGammaTableI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglSetGammaTableI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _iEntries;
  CUSHORT::CSArray _puRed;
  CUSHORT::CSArray _puGreen;
  CUSHORT::CSArray _puBlue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSetGammaTableI3D();
  CwglSetGammaTableI3D(BOOL return_value,
                       HDC hDC,
                       int iEntries,
                       const USHORT* puRed,
                       const USHORT* puGreen,
                       const USHORT* puBlue);
  virtual unsigned Id() const {
    return ID_WGL_SET_GAMMA_TABLE_I3D;
  }
  virtual const char* Name() const {
    return "wglSetGammaTableI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnableGenlockI3D() function call wrapper.
    *
    * OpenGL wglEnableGenlockI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglEnableGenlockI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnableGenlockI3D();
  CwglEnableGenlockI3D(BOOL return_value, HDC hDC);
  virtual unsigned Id() const {
    return ID_WGL_ENABLE_GENLOCK_I3D;
  }
  virtual const char* Name() const {
    return "wglEnableGenlockI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
    * @brief OpenGL wglDisableGenlockI3D() function call wrapper.
    *
    * OpenGL wglDisableGenlockI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglDisableGenlockI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDisableGenlockI3D();
  CwglDisableGenlockI3D(BOOL return_value, HDC hDC);
  virtual unsigned Id() const {
    return ID_WGL_DISABLE_GENLOCK_I3D;
  }
  virtual const char* Name() const {
    return "wglDisableGenlockI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglIsEnabledGenlockI3D() function call wrapper.
    *
    * OpenGL wglIsEnabledGenlockI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglIsEnabledGenlockI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CBOOL::CSArray _pFlag;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglIsEnabledGenlockI3D();
  CwglIsEnabledGenlockI3D(BOOL return_value, HDC hDC, BOOL* pFlag);
  virtual unsigned Id() const {
    return ID_WGL_IS_ENABLED_GENLOCK_I3D;
  }
  virtual const char* Name() const {
    return "wglIsEnabledGenlockI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGenlockSourceI3D() function call wrapper.
    *
    * OpenGL wglGenlockSourceI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGenlockSourceI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT _uSource;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGenlockSourceI3D();
  CwglGenlockSourceI3D(BOOL return_value, HDC hDC, UINT uSource);
  virtual unsigned Id() const {
    return ID_WGL_GENLOCK_SOURCE_I3D;
  }
  virtual const char* Name() const {
    return "wglGenlockSourceI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGenlockSourceI3D() function call wrapper.
    *
    * OpenGL wglGetGenlockSourceI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetGenlockSourceI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT::CSArray _uSource;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGenlockSourceI3D();
  CwglGetGenlockSourceI3D(BOOL return_value, HDC hDC, UINT* uSource);
  virtual unsigned Id() const {
    return ID_WGL_GET_GENLOCK_SOURCE_I3D;
  }
  virtual const char* Name() const {
    return "wglGetGenlockSourceI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGenlockSourceEdgeI3D() function call wrapper.
    *
    * OpenGL wglGenlockSourceEdgeI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGenlockSourceEdgeI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT _uEdge;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGenlockSourceEdgeI3D();
  CwglGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT uEdge);
  virtual unsigned Id() const {
    return ID_WGL_GENLOCK_SOURCE_EDGE_I3D;
  }
  virtual const char* Name() const {
    return "wglGenlockSourceEdgeI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGenlockSourceEdgeI3D() function call wrapper.
    *
    * OpenGL wglGetGenlockSourceEdgeI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetGenlockSourceEdgeI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT::CSArray _uEdge;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGenlockSourceEdgeI3D();
  CwglGetGenlockSourceEdgeI3D(BOOL return_value, HDC hDC, UINT* uEdge);
  virtual unsigned Id() const {
    return ID_WGL_GET_GENLOCK_SOURCE_EDGE_I3D;
  }
  virtual const char* Name() const {
    return "wglGetGenlockSourceEdgeI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGenlockSampleRateI3D() function call wrapper.
    *
    * OpenGL wglGenlockSampleRateI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGenlockSampleRateI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT _uRate;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGenlockSampleRateI3D();
  CwglGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT uRate);
  virtual unsigned Id() const {
    return ID_WGL_GENLOCK_SAMPLE_RATE_I3D;
  }
  virtual const char* Name() const {
    return "wglGenlockSampleRateI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGenlockSampleRateI3D() function call wrapper.
    *
    * OpenGL wglGetGenlockSampleRateI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetGenlockSampleRateI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT::CSArray _uRate;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGenlockSampleRateI3D();
  CwglGetGenlockSampleRateI3D(BOOL return_value, HDC hDC, UINT* uRate);
  virtual unsigned Id() const {
    return ID_WGL_GET_GENLOCK_SAMPLE_RATE_I3D;
  }
  virtual const char* Name() const {
    return "wglGetGenlockSampleRateI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGenlockSourceDelayI3D() function call wrapper.
    *
    * OpenGL wglGenlockSourceDelayI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGenlockSourceDelayI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT _uDelay;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGenlockSourceDelayI3D();
  CwglGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT uDelay);
  virtual unsigned Id() const {
    return ID_WGL_GENLOCK_SOURCE_DELAY_I3D;
  }
  virtual const char* Name() const {
    return "wglGenlockSourceDelayI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGenlockSourceDelayI3D() function call wrapper.
    *
    * OpenGL wglGetGenlockSourceDelayI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetGenlockSourceDelayI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT::CSArray _uDelay;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGenlockSourceDelayI3D();
  CwglGetGenlockSourceDelayI3D(BOOL return_value, HDC hDC, UINT* uDelay);
  virtual unsigned Id() const {
    return ID_WGL_GET_GENLOCK_SOURCE_DELAY_I3D;
  }
  virtual const char* Name() const {
    return "wglGetGenlockSourceDelayI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryGenlockMaxSourceDelayI3D() function call wrapper.
    *
    * OpenGL wglQueryGenlockMaxSourceDelayI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryGenlockMaxSourceDelayI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT::CSArray _uMaxLineDelay;
  CUINT::CSArray _uMaxPixelDelay;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryGenlockMaxSourceDelayI3D();
  CwglQueryGenlockMaxSourceDelayI3D(BOOL return_value,
                                    HDC hDC,
                                    UINT* uMaxLineDelay,
                                    UINT* uMaxPixelDelay);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_GENLOCK_MAX_SOURCE_DELAY_I3D;
  }
  virtual const char* Name() const {
    return "wglQueryGenlockMaxSourceDelayI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateImageBufferI3D() function call wrapper.
    *
    * OpenGL wglCreateImageBufferI3D() function call wrapper.
    * Returns: LPVOID
    */
class CwglCreateImageBufferI3D : public CFunction {
  CLPVOID _return_value;
  CHDC _hDC;
  CDWORD _dwSize;
  CUINT _uFlags;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateImageBufferI3D();
  CwglCreateImageBufferI3D(LPVOID return_value, HDC hDC, DWORD dwSize, UINT uFlags);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_IMAGE_BUFFER_I3D;
  }
  virtual const char* Name() const {
    return "wglCreateImageBufferI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDestroyImageBufferI3D() function call wrapper.
    *
    * OpenGL wglDestroyImageBufferI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglDestroyImageBufferI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CLPVOID _pAddress;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDestroyImageBufferI3D();
  CwglDestroyImageBufferI3D(BOOL return_value, HDC hDC, LPVOID pAddress);
  virtual unsigned Id() const {
    return ID_WGL_DESTROY_IMAGE_BUFFER_I3D;
  }
  virtual const char* Name() const {
    return "wglDestroyImageBufferI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglAssociateImageBufferEventsI3D() function call wrapper.
    *
    * OpenGL wglAssociateImageBufferEventsI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglAssociateImageBufferEventsI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CHANDLE::CSArray _pEvent;
  CLPVOID::CSArray _pAddress;
  CDWORD::CSArray _pSize;
  CUINT _count;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 6;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglAssociateImageBufferEventsI3D();
  CwglAssociateImageBufferEventsI3D(BOOL return_value,
                                    HDC hDC,
                                    const HANDLE* pEvent,
                                    const LPVOID* pAddress,
                                    const DWORD* pSize,
                                    UINT count);
  virtual unsigned Id() const {
    return ID_WGL_ASSOCIATE_IMAGE_BUFFER_EVENTS_I3D;
  }
  virtual const char* Name() const {
    return "wglAssociateImageBufferEventsI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleaseImageBufferEventsI3D() function call wrapper.
    *
    * OpenGL wglReleaseImageBufferEventsI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglReleaseImageBufferEventsI3D : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CLPVOID::CSArray _pAddress;
  CUINT _count;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleaseImageBufferEventsI3D();
  CwglReleaseImageBufferEventsI3D(BOOL return_value, HDC hDC, const LPVOID* pAddress, UINT count);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_IMAGE_BUFFER_EVENTS_I3D;
  }
  virtual const char* Name() const {
    return "wglReleaseImageBufferEventsI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnableFrameLockI3D() function call wrapper.
    *
    * OpenGL wglEnableFrameLockI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglEnableFrameLockI3D : public CFunction {
  CBOOL _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnableFrameLockI3D();
  CwglEnableFrameLockI3D(BOOL return_value);
  virtual unsigned Id() const {
    return ID_WGL_ENABLE_FRAME_LOCK_I3D;
  }
  virtual const char* Name() const {
    return "wglEnableFrameLockI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDisableFrameLockI3D() function call wrapper.
    *
    * OpenGL wglDisableFrameLockI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglDisableFrameLockI3D : public CFunction {
  CBOOL _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDisableFrameLockI3D();
  CwglDisableFrameLockI3D(BOOL return_value);
  virtual unsigned Id() const {
    return ID_WGL_DISABLE_FRAME_LOCK_I3D;
  }
  virtual const char* Name() const {
    return "wglDisableFrameLockI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglIsEnabledFrameLockI3D() function call wrapper.
    *
    * OpenGL wglIsEnabledFrameLockI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglIsEnabledFrameLockI3D : public CFunction {
  CBOOL _return_value;
  CBOOL::CSArray _pFlag;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglIsEnabledFrameLockI3D();
  CwglIsEnabledFrameLockI3D(BOOL return_value, BOOL* pFlag);
  virtual unsigned Id() const {
    return ID_WGL_IS_ENABLED_FRAME_LOCK_I3D;
  }
  virtual const char* Name() const {
    return "wglIsEnabledFrameLockI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryFrameLockMasterI3D() function call wrapper.
    *
    * OpenGL wglQueryFrameLockMasterI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryFrameLockMasterI3D : public CFunction {
  CBOOL _return_value;
  CBOOL::CSArray _pFlag;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryFrameLockMasterI3D();
  CwglQueryFrameLockMasterI3D(BOOL return_value, BOOL* pFlag);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_FRAME_LOCK_MASTER_I3D;
  }
  virtual const char* Name() const {
    return "wglQueryFrameLockMasterI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetFrameUsageI3D() function call wrapper.
    *
    * OpenGL wglGetFrameUsageI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglGetFrameUsageI3D : public CFunction {
  CBOOL _return_value;
  CFLOAT::CSArray _pUsage;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetFrameUsageI3D();
  CwglGetFrameUsageI3D(BOOL return_value, float* pUsage);
  virtual unsigned Id() const {
    return ID_WGL_GET_FRAME_USAGE_I3D;
  }
  virtual const char* Name() const {
    return "wglGetFrameUsageI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBeginFrameTrackingI3D() function call wrapper.
    *
    * OpenGL wglBeginFrameTrackingI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglBeginFrameTrackingI3D : public CFunction {
  CBOOL _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBeginFrameTrackingI3D();
  CwglBeginFrameTrackingI3D(BOOL return_value);
  virtual unsigned Id() const {
    return ID_WGL_BEGIN_FRAME_TRACKING_I3D;
  }
  virtual const char* Name() const {
    return "wglBeginFrameTrackingI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEndFrameTrackingI3D() function call wrapper.
    *
    * OpenGL wglEndFrameTrackingI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglEndFrameTrackingI3D : public CFunction {
  CBOOL _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEndFrameTrackingI3D();
  CwglEndFrameTrackingI3D(BOOL return_value);
  virtual unsigned Id() const {
    return ID_WGL_END_FRAME_TRACKING_I3D;
  }
  virtual const char* Name() const {
    return "wglEndFrameTrackingI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryFrameTrackingI3D() function call wrapper.
    *
    * OpenGL wglQueryFrameTrackingI3D() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryFrameTrackingI3D : public CFunction {
  CBOOL _return_value;
  CDWORD::CSArray _pFrameCount;
  CDWORD::CSArray _pMissedFrames;
  CFLOAT::CSArray _pLastMissedUsage;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryFrameTrackingI3D();
  CwglQueryFrameTrackingI3D(BOOL return_value,
                            DWORD* pFrameCount,
                            DWORD* pMissedFrames,
                            float* pLastMissedUsage);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_FRAME_TRACKING_I3D;
  }
  virtual const char* Name() const {
    return "wglQueryFrameTrackingI3D";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSetStereoEmitterState3DL() function call wrapper.
    *
    * OpenGL wglSetStereoEmitterState3DL() function call wrapper.
    * Returns: BOOL
    */
class CwglSetStereoEmitterState3DL : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CUINT _uState;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSetStereoEmitterState3DL();
  CwglSetStereoEmitterState3DL(BOOL return_value, HDC hDC, UINT uState);
  virtual unsigned Id() const {
    return ID_WGL_SET_STEREO_EMITTER_STATE3DL;
  }
  virtual const char* Name() const {
    return "wglSetStereoEmitterState3DL";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnumerateVideoDevicesNV() function call wrapper.
    *
    * OpenGL wglEnumerateVideoDevicesNV() function call wrapper.
    * Returns: int
    */
class CwglEnumerateVideoDevicesNV : public CFunction {
  Cint _return_value;
  CHDC _hDC;
  CHVIDEOOUTPUTDEVICENV::CSArray _phDeviceList;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnumerateVideoDevicesNV();
  CwglEnumerateVideoDevicesNV(int return_value, HDC hDC, HVIDEOOUTPUTDEVICENV* phDeviceList);
  virtual unsigned Id() const {
    return ID_WGL_ENUMERATE_VIDEO_DEVICES_NV;
  }
  virtual const char* Name() const {
    return "wglEnumerateVideoDevicesNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBindVideoDeviceNV() function call wrapper.
    *
    * OpenGL wglBindVideoDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglBindVideoDeviceNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cunsigned _uVideoSlot;
  CHVIDEOOUTPUTDEVICENV _hVideoDevice;
  Cint::CSArray _piAttribList;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBindVideoDeviceNV();
  CwglBindVideoDeviceNV(BOOL return_value,
                        HDC hDC,
                        unsigned uVideoSlot,
                        HVIDEOOUTPUTDEVICENV hVideoDevice,
                        const int* piAttribList);
  virtual unsigned Id() const {
    return ID_WGL_BIND_VIDEO_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglBindVideoDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryCurrentContextNV() function call wrapper.
    *
    * OpenGL wglQueryCurrentContextNV() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryCurrentContextNV : public CFunction {
  CBOOL _return_value;
  Cint _iAttribute;
  Cint::CSArray _piValue;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryCurrentContextNV();
  CwglQueryCurrentContextNV(BOOL return_value, int iAttribute, int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_CURRENT_CONTEXT_NV;
  }
  virtual const char* Name() const {
    return "wglQueryCurrentContextNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetVideoDeviceNV() function call wrapper.
    *
    * OpenGL wglGetVideoDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglGetVideoDeviceNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  Cint _numDevices;
  CHPVIDEODEV::CSArray _hVideoDevice;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetVideoDeviceNV();
  CwglGetVideoDeviceNV(BOOL return_value, HDC hDC, int numDevices, HPVIDEODEV* hVideoDevice);
  virtual unsigned Id() const {
    return ID_WGL_GET_VIDEO_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglGetVideoDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleaseVideoDeviceNV() function call wrapper.
    *
    * OpenGL wglReleaseVideoDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglReleaseVideoDeviceNV : public CFunction {
  CBOOL _return_value;
  CHPVIDEODEV _hVideoDevice;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleaseVideoDeviceNV();
  CwglReleaseVideoDeviceNV(BOOL return_value, HPVIDEODEV hVideoDevice);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_VIDEO_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglReleaseVideoDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBindVideoImageNV() function call wrapper.
    *
    * OpenGL wglBindVideoImageNV() function call wrapper.
    * Returns: BOOL
    */
class CwglBindVideoImageNV : public CFunction {
  CBOOL _return_value;
  CHPVIDEODEV _hVideoDevice;
  CHPBUFFERARB _hPbuffer;
  Cint _iVideoBuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBindVideoImageNV();
  CwglBindVideoImageNV(BOOL return_value,
                       HPVIDEODEV hVideoDevice,
                       HPBUFFERARB hPbuffer,
                       int iVideoBuffer);
  virtual unsigned Id() const {
    return ID_WGL_BIND_VIDEO_IMAGE_NV;
  }
  virtual const char* Name() const {
    return "wglBindVideoImageNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleaseVideoImageNV() function call wrapper.
    *
    * OpenGL wglReleaseVideoImageNV() function call wrapper.
    * Returns: BOOL
    */
class CwglReleaseVideoImageNV : public CFunction {
  CBOOL _return_value;
  CHPBUFFERARB _hPbuffer;
  Cint _iVideoBuffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleaseVideoImageNV();
  CwglReleaseVideoImageNV(BOOL return_value, HPBUFFERARB hPbuffer, int iVideoBuffer);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_VIDEO_IMAGE_NV;
  }
  virtual const char* Name() const {
    return "wglReleaseVideoImageNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglSendPbufferToVideoNV() function call wrapper.
    *
    * OpenGL wglSendPbufferToVideoNV() function call wrapper.
    * Returns: BOOL
    */
class CwglSendPbufferToVideoNV : public CFunction {
  CBOOL _return_value;
  CHPBUFFERARB _hPbuffer;
  Cint _iBufferType;
  Cunsigned_long::CSArray _pulCounterPbuffer;
  CBOOL _bBlock;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglSendPbufferToVideoNV();
  CwglSendPbufferToVideoNV(BOOL return_value,
                           HPBUFFERARB hPbuffer,
                           int iBufferType,
                           unsigned long* pulCounterPbuffer,
                           BOOL bBlock);
  virtual unsigned Id() const {
    return ID_WGL_SEND_PBUFFER_TO_VIDEO_NV;
  }
  virtual const char* Name() const {
    return "wglSendPbufferToVideoNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetVideoInfoNV() function call wrapper.
    *
    * OpenGL wglGetVideoInfoNV() function call wrapper.
    * Returns: BOOL
    */
class CwglGetVideoInfoNV : public CFunction {
  CBOOL _return_value;
  CHPVIDEODEV _hpVideoDevice;
  Cunsigned_long::CSArray _pulCounterOutputPbuffer;
  Cunsigned_long::CSArray _pulCounterOutputVideo;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetVideoInfoNV();
  CwglGetVideoInfoNV(BOOL return_value,
                     HPVIDEODEV hpVideoDevice,
                     unsigned long* pulCounterOutputPbuffer,
                     unsigned long* pulCounterOutputVideo);
  virtual unsigned Id() const {
    return ID_WGL_GET_VIDEO_INFO_NV;
  }
  virtual const char* Name() const {
    return "wglGetVideoInfoNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglJoinSwapGroupNV() function call wrapper.
    *
    * OpenGL wglJoinSwapGroupNV() function call wrapper.
    * Returns: BOOL
    */
class CwglJoinSwapGroupNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CGLuint _group;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglJoinSwapGroupNV();
  CwglJoinSwapGroupNV(BOOL return_value, HDC hDC, GLuint group);
  virtual unsigned Id() const {
    return ID_WGL_JOIN_SWAP_GROUP_NV;
  }
  virtual const char* Name() const {
    return "wglJoinSwapGroupNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBindSwapBarrierNV() function call wrapper.
    *
    * OpenGL wglBindSwapBarrierNV() function call wrapper.
    * Returns: BOOL
    */
class CwglBindSwapBarrierNV : public CFunction {
  CBOOL _return_value;
  CGLuint _group;
  CGLuint _barrier;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBindSwapBarrierNV();
  CwglBindSwapBarrierNV(BOOL return_value, GLuint group, GLuint barrier);
  virtual unsigned Id() const {
    return ID_WGL_BIND_SWAP_BARRIER_NV;
  }
  virtual const char* Name() const {
    return "wglBindSwapBarrierNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQuerySwapGroupNV() function call wrapper.
    *
    * OpenGL wglQuerySwapGroupNV() function call wrapper.
    * Returns: BOOL
    */
class CwglQuerySwapGroupNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CGLuint::CSArray _group;
  CGLuint::CSArray _barrier;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQuerySwapGroupNV();
  CwglQuerySwapGroupNV(BOOL return_value, HDC hDC, GLuint* group, GLuint* barrier);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_SWAP_GROUP_NV;
  }
  virtual const char* Name() const {
    return "wglQuerySwapGroupNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryMaxSwapGroupsNV() function call wrapper.
    *
    * OpenGL wglQueryMaxSwapGroupsNV() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryMaxSwapGroupsNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CGLuint::CSArray _maxGroups;
  CGLuint::CSArray _maxBarriers;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 4;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryMaxSwapGroupsNV();
  CwglQueryMaxSwapGroupsNV(BOOL return_value, HDC hDC, GLuint* maxGroups, GLuint* maxBarriers);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_MAX_SWAP_GROUPS_NV;
  }
  virtual const char* Name() const {
    return "wglQueryMaxSwapGroupsNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryFrameCountNV() function call wrapper.
    *
    * OpenGL wglQueryFrameCountNV() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryFrameCountNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CGLuint::CSArray _count;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryFrameCountNV();
  CwglQueryFrameCountNV(BOOL return_value, HDC hDC, GLuint* count);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_FRAME_COUNT_NV;
  }
  virtual const char* Name() const {
    return "wglQueryFrameCountNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglResetFrameCountNV() function call wrapper.
    *
    * OpenGL wglResetFrameCountNV() function call wrapper.
    * Returns: BOOL
    */
class CwglResetFrameCountNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDC;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglResetFrameCountNV();
  CwglResetFrameCountNV(BOOL return_value, HDC hDC);
  virtual unsigned Id() const {
    return ID_WGL_RESET_FRAME_COUNT_NV;
  }
  virtual const char* Name() const {
    return "wglResetFrameCountNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnumGpusNV() function call wrapper.
    *
    * OpenGL wglEnumGpusNV() function call wrapper.
    * Returns: BOOL
    */
class CwglEnumGpusNV : public CFunction {
  CBOOL _return_value;
  CUINT _iGpuIndex;
  CHGPUNV::CSArray _phGpu;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnumGpusNV();
  CwglEnumGpusNV(BOOL return_value, UINT iGpuIndex, HGPUNV* phGpu);
  virtual unsigned Id() const {
    return ID_WGL_ENUM_GPUS_NV;
  }
  virtual const char* Name() const {
    return "wglEnumGpusNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnumGpuDevicesNV() function call wrapper.
    *
    * OpenGL wglEnumGpuDevicesNV() function call wrapper.
    * Returns: BOOL
    */
class CwglEnumGpuDevicesNV : public CFunction {
  CBOOL _return_value;
  CHGPUNV _hGpu;
  CUINT _iDeviceIndex;
  //CPGPU_DEVICE _lpGpuDevice;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnumGpuDevicesNV();
  CwglEnumGpuDevicesNV(BOOL return_value, HGPUNV hGpu, UINT iDeviceIndex, PGPU_DEVICE lpGpuDevice);
  virtual unsigned Id() const {
    return ID_WGL_ENUM_GPU_DEVICES_NV;
  }
  virtual const char* Name() const {
    return "wglEnumGpuDevicesNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateAffinityDCNV() function call wrapper.
    *
    * OpenGL wglCreateAffinityDCNV() function call wrapper.
    * Returns: HDC
    */
class CwglCreateAffinityDCNV : public CFunction {
  CHDC _return_value;
  CHGPUNV::CSArray _phGpuList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateAffinityDCNV();
  CwglCreateAffinityDCNV(HDC return_value, const HGPUNV* phGpuList);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_AFFINITY_DCNV;
  }
  virtual const char* Name() const {
    return "wglCreateAffinityDCNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnumGpusFromAffinityDCNV() function call wrapper.
    *
    * OpenGL wglEnumGpusFromAffinityDCNV() function call wrapper.
    * Returns: BOOL
    */
class CwglEnumGpusFromAffinityDCNV : public CFunction {
  CBOOL _return_value;
  CHDC _hAffinityDC;
  CUINT _iGpuIndex;
  CHGPUNV::CSArray _hGpu;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnumGpusFromAffinityDCNV();
  CwglEnumGpusFromAffinityDCNV(BOOL return_value, HDC hAffinityDC, UINT iGpuIndex, HGPUNV* hGpu);
  virtual unsigned Id() const {
    return ID_WGL_ENUM_GPUS_FROM_AFFINITY_DCNV;
  }
  virtual const char* Name() const {
    return "wglEnumGpusFromAffinityDCNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDeleteDCNV() function call wrapper.
    *
    * OpenGL wglDeleteDCNV() function call wrapper.
    * Returns: BOOL
    */
class CwglDeleteDCNV : public CFunction {
  CBOOL _return_value;
  CHDC _hdc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDeleteDCNV();
  CwglDeleteDCNV(BOOL return_value, HDC hdc);
  virtual unsigned Id() const {
    return ID_WGL_DELETE_DCNV;
  }
  virtual const char* Name() const {
    return "wglDeleteDCNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGPUIDsAMD() function call wrapper.
    *
    * OpenGL wglGetGPUIDsAMD() function call wrapper.
    * Returns: UINT
    */
class CwglGetGPUIDsAMD : public CFunction {
  CUINT _return_value;
  CUINT _maxCount;
  CUINT::CSArray _ids;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGPUIDsAMD();
  CwglGetGPUIDsAMD(UINT return_value, UINT maxCount, UINT* ids);
  virtual unsigned Id() const {
    return ID_WGL_GET_GPUIDS_AMD;
  }
  virtual const char* Name() const {
    return "wglGetGPUIDsAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetGPUInfoAMD() function call wrapper.
    *
    * OpenGL wglGetGPUInfoAMD() function call wrapper.
    * Returns: INT
    */
class CwglGetGPUInfoAMD : public CFunction {
  Cint _return_value;
  CUINT _id;
  Cint _property;
  CGLenum _dataType;
  CUINT _size;
  CvoidPtr::CSArray _data;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetGPUInfoAMD();
  CwglGetGPUInfoAMD(
      int return_value, UINT id, int property, GLenum dataType, UINT size, void* data);
  virtual unsigned Id() const {
    return ID_WGL_GET_GPUINFO_AMD;
  }
  virtual const char* Name() const {
    return "wglGetGPUInfoAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetContextGPUIDAMD() function call wrapper.
    *
    * OpenGL wglGetContextGPUIDAMD() function call wrapper.
    * Returns: UINT
    */
class CwglGetContextGPUIDAMD : public CFunction {
  CUINT _return_value;
  CHGLRC _hglrc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetContextGPUIDAMD();
  CwglGetContextGPUIDAMD(UINT return_value, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_GET_CONTEXT_GPUIDAMD;
  }
  virtual const char* Name() const {
    return "wglGetContextGPUIDAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateAssociatedContextAMD() function call wrapper.
    *
    * OpenGL wglCreateAssociatedContextAMD() function call wrapper.
    * Returns: HGLRC
    */
class CwglCreateAssociatedContextAMD : public CFunction {
  CHGLRC _return_value;
  CUINT _id;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateAssociatedContextAMD();
  CwglCreateAssociatedContextAMD(HGLRC return_value, UINT id);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_ASSOCIATED_CONTEXT_AMD;
  }
  virtual const char* Name() const {
    return "wglCreateAssociatedContextAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCreateAssociatedContextAttribsAMD() function call wrapper.
    *
    * OpenGL wglCreateAssociatedContextAttribsAMD() function call wrapper.
    * Returns: HGLRC
    */
class CwglCreateAssociatedContextAttribsAMD : public CFunction {
  CHGLRC _return_value;
  CUINT _id;
  CHGLRC _hShareContext;
  Cint::CSArray _attribList;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglCreateAssociatedContextAttribsAMD();
  CwglCreateAssociatedContextAttribsAMD(HGLRC return_value,
                                        UINT id,
                                        HGLRC hShareContext,
                                        const int* attribList);
  virtual unsigned Id() const {
    return ID_WGL_CREATE_ASSOCIATED_CONTEXT_ATTRIBS_AMD;
  }
  virtual const char* Name() const {
    return "wglCreateAssociatedContextAttribsAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglDeleteAssociatedContextAMD() function call wrapper.
    *
    * OpenGL wglDeleteAssociatedContextAMD() function call wrapper.
    * Returns: BOOL
    */
class CwglDeleteAssociatedContextAMD : public CFunction {
  CBOOL _return_value;
  CHGLRC _hglrc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglDeleteAssociatedContextAMD();
  CwglDeleteAssociatedContextAMD(BOOL return_value, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_DELETE_ASSOCIATED_CONTEXT_AMD;
  }
  virtual const char* Name() const {
    return "wglDeleteAssociatedContextAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglMakeAssociatedContextCurrentAMD() function call wrapper.
    *
    * OpenGL wglMakeAssociatedContextCurrentAMD() function call wrapper.
    * Returns: BOOL
    */
class CwglMakeAssociatedContextCurrentAMD : public CFunction {
  CBOOL _return_value;
  CHGLRC _hglrc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglMakeAssociatedContextCurrentAMD();
  CwglMakeAssociatedContextCurrentAMD(BOOL return_value, HGLRC hglrc);
  virtual unsigned Id() const {
    return ID_WGL_MAKE_ASSOCIATED_CONTEXT_CURRENT_AMD;
  }
  virtual const char* Name() const {
    return "wglMakeAssociatedContextCurrentAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetCurrentAssociatedContextAMD() function call wrapper.
    *
    * OpenGL wglGetCurrentAssociatedContextAMD() function call wrapper.
    * Returns: HGLRC
    */
class CwglGetCurrentAssociatedContextAMD : public CFunction {
  CHGLRC _return_value;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 0;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglGetCurrentAssociatedContextAMD();
  CwglGetCurrentAssociatedContextAMD(HGLRC return_value);
  virtual unsigned Id() const {
    return ID_WGL_GET_CURRENT_ASSOCIATED_CONTEXT_AMD;
  }
  virtual const char* Name() const {
    return "wglGetCurrentAssociatedContextAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBlitContextFramebufferAMD() function call wrapper.
    *
    * OpenGL wglBlitContextFramebufferAMD() function call wrapper.
    * Returns: VOID
    */
class CwglBlitContextFramebufferAMD : public CFunction {
  CHGLRC _dstCtx;
  CGLint _srcX0;
  CGLint _srcY0;
  CGLint _srcX1;
  CGLint _srcY1;
  CGLint _dstX0;
  CGLint _dstY0;
  CGLint _dstX1;
  CGLint _dstY1;
  CGLbitfield _mask;
  CGLenum _filter;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 11;
  }

public:
  CwglBlitContextFramebufferAMD();
  CwglBlitContextFramebufferAMD(HGLRC dstCtx,
                                GLint srcX0,
                                GLint srcY0,
                                GLint srcX1,
                                GLint srcY1,
                                GLint dstX0,
                                GLint dstY0,
                                GLint dstX1,
                                GLint dstY1,
                                GLbitfield mask,
                                GLenum filter);
  virtual unsigned Id() const {
    return ID_WGL_BLIT_CONTEXT_FRAMEBUFFER_AMD;
  }
  virtual const char* Name() const {
    return "wglBlitContextFramebufferAMD";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglBindVideoCaptureDeviceNV() function call wrapper.
    *
    * OpenGL wglBindVideoCaptureDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglBindVideoCaptureDeviceNV : public CFunction {
  CBOOL _return_value;
  CUINT _uVideoSlot;
  CHVIDEOINPUTDEVICENV _hDevice;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglBindVideoCaptureDeviceNV();
  CwglBindVideoCaptureDeviceNV(BOOL return_value, UINT uVideoSlot, HVIDEOINPUTDEVICENV hDevice);
  virtual unsigned Id() const {
    return ID_WGL_BIND_VIDEO_CAPTURE_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglBindVideoCaptureDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglEnumerateVideoCaptureDevicesNV() function call wrapper.
    *
    * OpenGL wglEnumerateVideoCaptureDevicesNV() function call wrapper.
    * Returns: UINT
    */
class CwglEnumerateVideoCaptureDevicesNV : public CFunction {
  CUINT _return_value;
  CHDC _hDc;
  CHVIDEOINPUTDEVICENV::CSArray _phDeviceList;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglEnumerateVideoCaptureDevicesNV();
  CwglEnumerateVideoCaptureDevicesNV(UINT return_value, HDC hDc, HVIDEOINPUTDEVICENV* phDeviceList);
  virtual unsigned Id() const {
    return ID_WGL_ENUMERATE_VIDEO_CAPTURE_DEVICES_NV;
  }
  virtual const char* Name() const {
    return "wglEnumerateVideoCaptureDevicesNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglLockVideoCaptureDeviceNV() function call wrapper.
    *
    * OpenGL wglLockVideoCaptureDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglLockVideoCaptureDeviceNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDc;
  CHVIDEOINPUTDEVICENV _hDevice;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglLockVideoCaptureDeviceNV();
  CwglLockVideoCaptureDeviceNV(BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice);
  virtual unsigned Id() const {
    return ID_WGL_LOCK_VIDEO_CAPTURE_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglLockVideoCaptureDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglQueryVideoCaptureDeviceNV() function call wrapper.
    *
    * OpenGL wglQueryVideoCaptureDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglQueryVideoCaptureDeviceNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDc;
  CHVIDEOINPUTDEVICENV _hDevice;
  Cint _iAttribute;
  Cint::CSArray _piValue;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglQueryVideoCaptureDeviceNV();
  CwglQueryVideoCaptureDeviceNV(
      BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice, int iAttribute, int* piValue);
  virtual unsigned Id() const {
    return ID_WGL_QUERY_VIDEO_CAPTURE_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglQueryVideoCaptureDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglReleaseVideoCaptureDeviceNV() function call wrapper.
    *
    * OpenGL wglReleaseVideoCaptureDeviceNV() function call wrapper.
    * Returns: BOOL
    */
class CwglReleaseVideoCaptureDeviceNV : public CFunction {
  CBOOL _return_value;
  CHDC _hDc;
  CHVIDEOINPUTDEVICENV _hDevice;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }
  virtual MaybeConstCArgRef Return() const {
    return _return_value;
  }

public:
  CwglReleaseVideoCaptureDeviceNV();
  CwglReleaseVideoCaptureDeviceNV(BOOL return_value, HDC hDc, HVIDEOINPUTDEVICENV hDevice);
  virtual unsigned Id() const {
    return ID_WGL_RELEASE_VIDEO_CAPTURE_DEVICE_NV;
  }
  virtual const char* Name() const {
    return "wglReleaseVideoCaptureDeviceNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglCopyImageSubDataNV() function call wrapper.
    *
    * OpenGL wglCopyImageSubDataNV() function call wrapper.
    * Returns: BOOL
    */
class CwglCopyImageSubDataNV : public CFunction {
  CBOOL _return_value;
  CHGLRC _hSrcRC;
  CGLuint _srcName;
  CGLenum _srcTarget;
  CGLint _srcLevel;
  CGLint _srcX;
  CGLint _srcY;
  CGLint _srcZ;
  CHGLRC _hDstRC;
  CGLuint _dstName;
  CGLenum _dstTarget;
  CGLint _dstLevel;
  CGLint _dstX;
  CGLint _dstY;
  CGLint _dstZ;
  CGLsizei _width;
  CGLsizei _height;
  CGLsizei _depth;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 18;
  }

public:
  CwglCopyImageSubDataNV();
  CwglCopyImageSubDataNV(BOOL return_value,
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
                         GLsizei depth);
  virtual unsigned Id() const {
    return ID_WGL_COPY_IMAGE_SUB_DATA_NV;
  }
  virtual const char* Name() const {
    return "wglCopyImageSubDataNV";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }

  virtual void Run();
};

/**
    * @brief OpenGL wglGetDefaultProcAddress() function call wrapper.
    *
    * OpenGL wglGetDefaultProcAddress() function call wrapper.
    * Returns: PROC
    */
class CwglGetDefaultProcAddress : public CFunction {
  Cchar::CSArray _lpszProc;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 1;
  }

public:
  CwglGetDefaultProcAddress();
  CwglGetDefaultProcAddress(LPCSTR lpszProc);
  virtual unsigned Id() const {
    return ID_WGL_GET_DEFAULT_PROC_ADDRESS;
  }
  virtual const char* Name() const {
    return "wglGetDefaultProcAddress";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL wglGetLayerPaletteEntries() function call wrapper.
    *
    * OpenGL wglGetLayerPaletteEntries() function call wrapper.
    * Returns: BOOL
    */
class CwglGetLayerPaletteEntries : public CFunction {
  CHDC _hDc;
  Cint _iLayerPlane;
  Cint _iStart;
  Cint _cEntries;
  CHWND _hwnd;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 5;
  }

public:
  CwglGetLayerPaletteEntries();
  CwglGetLayerPaletteEntries(HDC hDc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr);
  virtual unsigned Id() const {
    return ID_WGL_GET_LAYER_PALETTE_ENTRIES;
  }
  virtual const char* Name() const {
    return "wglGetLayerPaletteEntries";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};

/**
    * @brief OpenGL helperWglUpdateWindow() function call wrapper.
    *
    * OpenGL helperWglUpdateWindow() function call wrapper.
    * Returns: BOOL
    */
class ChelperWglUpdateWindow : public CFunction {
  CHWND _hwnd;
  Cint::CSArray _winparams;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 2;
  }

public:
  ChelperWglUpdateWindow();
  ChelperWglUpdateWindow(HWND hwnd);
  virtual unsigned Id() const {
    return ID_HELPER_WGL_UPDATE_WINDOW;
  }
  virtual const char* Name() const {
    return "helperWglUpdateWindow";
  }
  virtual unsigned Version() const {
    return VERSION_UNKNOWN;
  }
  virtual void Run();
};
} // namespace OpenGL
} // namespace gits
