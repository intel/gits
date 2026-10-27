// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// GL, WGL and GLX scalar, handle, and struct type declarations used across the
// full GL+WGL+GLX command set (core plus every vendor extension), matching the
// Khronos/Microsoft/X11 <GL/gl.h>/<GL/glext.h>/<GL/wglext.h>/<GL/glx.h>/
// <GL/glxext.h>/<windows.h>/<X11/Xlib.h> typedefs. Declared by hand here
// instead of including those headers, since they also declare every GL/WGL/GLX
// function - which would conflict with the interceptor's own definitions of
// those same functions in interceptorAuto.cpp. Self-declaring these types also
// avoids depending on <windows.h> or <X11/Xlib.h>, which each exist on one
// platform only, so the full command set builds everywhere.
//
// EGL types aren't declared here, as EGL isn't intercepted.

#pragma once

#include <cstdint>

#if defined(_WIN32)
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif

// ----- Base Win32 scalar types -----
// (real <windows.h> equivalents, redeclared so this header is self-contained)

typedef int BOOL;
typedef int INT;
typedef int32_t INT32;
typedef int64_t INT64;
typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned short WORD;
#if defined(_WIN32)
typedef unsigned long DWORD;
#else
// 32-bit like on Windows, so that WGL commands serialize identically.
typedef uint32_t DWORD;
#endif
typedef unsigned char BYTE;
typedef char CHAR;
#if defined(_WIN32)
typedef long LONG;
#else
typedef int32_t LONG;
#endif
typedef float FLOAT;
typedef void VOID;
typedef VOID* LPVOID;
typedef void* HANDLE;
typedef const char* LPCSTR;
typedef DWORD COLORREF;
typedef int(APIENTRY* PROC)();

typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT, *LPRECT;

// ----- WGL/GDI handles and structs -----
// (opaque handles and the structs used by WGL commands, not provided by any
// header we include)
//
// Struct tag names (tagFoo/_Foo below) match Microsoft's/wgl.xml's own
// declarations for each type. It's not our naming convention.

typedef struct HDC__* HDC;
typedef struct HGLRC__* HGLRC;
typedef struct HENHMETAFILE__* HENHMETAFILE;
typedef struct HPBUFFERARB__* HPBUFFERARB;
typedef struct HPBUFFEREXT__* HPBUFFEREXT;
typedef struct HGPUNV__* HGPUNV;
typedef struct HPGPUNV__* HPGPUNV;
typedef struct HVIDEOOUTPUTDEVICENV__* HVIDEOOUTPUTDEVICENV;
typedef struct HVIDEOINPUTDEVICENV__* HVIDEOINPUTDEVICENV;
typedef struct HPVIDEODEV__* HPVIDEODEV;

typedef struct _GPU_DEVICE {
  DWORD cb;
  CHAR DeviceName[32];
  CHAR DeviceString[128];
  DWORD Flags;
  RECT rcVirtualScreen;
} GPU_DEVICE, *PGPU_DEVICE;

typedef struct tagPIXELFORMATDESCRIPTOR {
  WORD nSize;
  WORD nVersion;
  DWORD dwFlags;
  BYTE iPixelType;
  BYTE cColorBits;
  BYTE cRedBits;
  BYTE cRedShift;
  BYTE cGreenBits;
  BYTE cGreenShift;
  BYTE cBlueBits;
  BYTE cBlueShift;
  BYTE cAlphaBits;
  BYTE cAlphaShift;
  BYTE cAccumBits;
  BYTE cAccumRedBits;
  BYTE cAccumGreenBits;
  BYTE cAccumBlueBits;
  BYTE cAccumAlphaBits;
  BYTE cDepthBits;
  BYTE cStencilBits;
  BYTE cAuxBuffers;
  BYTE iLayerType;
  BYTE bReserved;
  DWORD dwLayerMask;
  DWORD dwVisibleMask;
  DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

typedef struct tagLAYERPLANEDESCRIPTOR {
  WORD nSize;
  WORD nVersion;
  DWORD dwFlags;
  BYTE iPixelType;
  BYTE cColorBits;
  BYTE cRedBits;
  BYTE cRedShift;
  BYTE cGreenBits;
  BYTE cGreenShift;
  BYTE cBlueBits;
  BYTE cBlueShift;
  BYTE cAlphaBits;
  BYTE cAlphaShift;
  BYTE cAccumBits;
  BYTE cAccumRedBits;
  BYTE cAccumGreenBits;
  BYTE cAccumBlueBits;
  BYTE cAccumAlphaBits;
  BYTE cDepthBits;
  BYTE cStencilBits;
  BYTE cAuxBuffers;
  BYTE iLayerPlane;
  BYTE bReserved;
  COLORREF crTransparent;
} LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, *LPLAYERPLANEDESCRIPTOR;

typedef struct _WGLSWAP {
  HDC hdc;
  UINT uiFlags;
} WGLSWAP, *PWGLSWAP, *LPWGLSWAP;

// Opaque forward declaration: only ever passed through as a pointer, never
// read or written by this DLL.
typedef struct _GLYPHMETRICSFLOAT* LPGLYPHMETRICSFLOAT;

// ----- GLX/X11 handles and structs -----
// XID is `unsigned long` in Xlib, 64-bit on Linux. It is 64-bit everywhere here, so that GLX
// commands serialize identically on every platform. The structs are only ever passed through as
// pointers.

typedef uint64_t XID;
typedef XID Window;
typedef XID Pixmap;
typedef XID Colormap;
typedef XID Font;
typedef int Bool;
typedef int Status;
typedef struct _XDisplay Display;
typedef struct XVisualInfo XVisualInfo;

typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;
typedef struct __GLXFBConfigRec* GLXFBConfigSGIX;
typedef XID GLXDrawable;
typedef XID GLXPixmap;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
typedef XID GLXContextID;
typedef XID GLXPbufferSGIX;
typedef XID GLXVideoCaptureDeviceNV;
typedef unsigned int GLXVideoDeviceNV;
typedef struct GLXHyperpipeNetworkSGIX GLXHyperpipeNetworkSGIX;
typedef struct GLXHyperpipeConfigSGIX GLXHyperpipeConfigSGIX;
typedef void (*__GLXextFuncPtr)(void);

// ----- GL scalar and handle types -----

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

typedef int32_t GLclampx;
typedef int32_t GLfixed;
typedef char GLchar;
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
typedef uint16_t GLhalf;
typedef uint16_t GLhalfARB;
typedef uint16_t GLhalfNV;
typedef int64_t GLint64;
typedef int64_t GLint64EXT;
typedef uint64_t GLuint64;
typedef uint64_t GLuint64EXT;
typedef intptr_t GLintptr;
typedef intptr_t GLintptrARB;
typedef intptr_t GLsizeiptr;
typedef intptr_t GLsizeiptrARB;
typedef GLintptr GLvdpauSurfaceNV;
typedef struct __GLsync* GLsync;
typedef void* GLeglImageOES;
typedef void* GLeglClientBufferEXT;
typedef void(APIENTRY* GLVULKANPROCNV)(void);

struct _cl_context;
struct _cl_event;

typedef void(APIENTRY* GLDEBUGPROC)(GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar* message,
                                    const void* userParam);
typedef void(APIENTRY* GLDEBUGPROCARB)(GLenum source,
                                       GLenum type,
                                       GLuint id,
                                       GLenum severity,
                                       GLsizei length,
                                       const GLchar* message,
                                       const void* userParam);
typedef void(APIENTRY* GLDEBUGPROCKHR)(GLenum source,
                                       GLenum type,
                                       GLuint id,
                                       GLenum severity,
                                       GLsizei length,
                                       const GLchar* message,
                                       const void* userParam);
typedef void(APIENTRY* GLDEBUGPROCAMD)(GLuint id,
                                       GLenum category,
                                       GLenum severity,
                                       GLsizei length,
                                       const GLchar* message,
                                       void* userParam);
