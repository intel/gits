// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

//GL types, taken from gl.h
#include "platform.h"

#include <stdint.h>
#include <stddef.h>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned long GLulong;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef int GLclampx;
typedef int GLfixed;
typedef void GLvoid;
typedef int64_t GLint64EXT;
typedef uint64_t GLuint64EXT;
typedef GLint64EXT GLint64;
typedef GLuint64EXT GLuint64;
typedef struct __GLsync* GLsync;
typedef char GLchar;

typedef char GLcharARB;
#if defined(_WIN64)
typedef signed long long int GLintptrARB;
typedef signed long long int GLsizeiptrARB;
typedef signed long long int GLintptr;
typedef signed long long int GLsizeiptr;
#else
typedef signed long int GLintptrARB;
typedef signed long int GLsizeiptrARB;
typedef signed long int GLintptr;
typedef signed long int GLsizeiptr;
#endif
typedef unsigned int GLhandleARB;
typedef unsigned short GLhalf;
typedef unsigned short GLhalfARB;
typedef unsigned short GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;
typedef void* GLeglImageOES;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLDisplay;
typedef void* EGLSurface;
typedef void* EGLClientBuffer;
typedef int32_t EGLint;
typedef void* EGLImageKHR;
typedef void* GLeglImageOES;
typedef void* EGLSyncNV;
typedef void* EGLSyncKHR;
typedef uint64_t EGLTimeKHR;
typedef uint64_t EGLTimeNV;
typedef uint64_t EGLuint64KHR;
typedef uint64_t EGLuint64NV;
typedef void* EGLStreamKHR;
typedef int EGLNativeFileDescriptorKHR;
typedef GLsizeiptr EGLsizeiANDROID;
typedef void* GLeglClientBufferEXT;
typedef void (*EGLSetBlobFuncANDROID)(const void* key,
                                      EGLsizeiANDROID keySize,
                                      const void* value,
                                      EGLsizeiANDROID valueSize);
typedef EGLsizeiANDROID (*EGLGetBlobFuncANDROID)(const void* key,
                                                 EGLsizeiANDROID keySize,
                                                 void* value,
                                                 EGLsizeiANDROID valueSize);
typedef void (*GLVULKANPROCNV)(void);

typedef struct HBITMAP__* HBITMAP;
typedef struct HWND__* HWND;
typedef struct HDC__* HDC;
typedef struct HGLRC__* HGLRC;
typedef struct HPBUFFERARB__* HPBUFFERARB;
typedef struct HPBUFFEREXT__* HPBUFFEREXT;
typedef struct HVIDEOOUTPUTDEVICENV__* HVIDEOOUTPUTDEVICENV;
typedef struct HGPUNV__* HGPUNV;
typedef struct HVIDEOINPUTDEVICENV__* HVIDEOINPUTDEVICENV;
typedef struct HPVIDEODEV__* HPVIDEODEV;
typedef int BOOL;
typedef unsigned short WORD;
#ifdef GITS_PLATFORM_WINDOWS
typedef unsigned long DWORD;
#else
typedef uint32_t DWORD;
#endif
typedef unsigned char BYTE;
typedef struct _WGLSWAP WGLSWAP;

#ifndef GITS_PLATFORM_WINDOWS
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
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR;
#else
typedef struct tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR;
typedef struct tagLAYERPLANEDESCRIPTOR* LPLAYERPLANEDESCRIPTOR;
#endif
typedef struct PIXELFORMATDESCRIPTOR_ {
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
} PIXELFORMATDESCRIPTOR__;

typedef int INT;
typedef int32_t INT32;
typedef int64_t INT64;
typedef unsigned short USHORT;
#ifndef VOID
typedef void VOID;
#endif
typedef VOID* LPVOID;
typedef unsigned UINT;
#ifdef GITS_PLATFORM_WINDOWS
typedef unsigned long DWORD;
#else
typedef uint32_t DWORD;
#endif
typedef const char* LPCSTR;
typedef DWORD COLORREF;
typedef void* HANDLE;
typedef struct _GLYPHMETRICSFLOAT* LPGLYPHMETRICSFLOAT;
typedef float FLOAT;

typedef struct _GPU_DEVICE* PGPU_DEVICE;

#if defined GITS_PLATFORM_WINDOWS
typedef HDC EGLNativeDisplayType;
typedef HBITMAP EGLNativePixmapType;
typedef HWND EGLNativeWindowType;
#else
typedef void* EGLNativeDisplayType;
typedef void* EGLNativeWindowType;
typedef void* EGLNativePixmapType;
#endif

typedef void(STDCALL* GLDEBUGPROC)(GLenum source,
                                   GLenum type,
                                   GLuint id,
                                   GLenum severity,
                                   GLsizei length,
                                   const GLchar* message,
                                   const GLvoid* userParam);
typedef void(STDCALL* GLDEBUGPROCARB)(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar* message,
                                      const GLvoid* userParam);
typedef void(STDCALL* GLDEBUGPROCKHR)(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar* message,
                                      const GLvoid* userParam);
typedef void(STDCALL* GLDEBUGPROCAMD)(GLuint id,
                                      GLenum category,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar* message,
                                      GLvoid* userParam);

typedef unsigned long XID;
typedef struct _XDisplay Display;
typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;
typedef struct __GLXFBConfigRec* GLXFBConfigSGIX;

typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXFBConfigID;
typedef XID GLXContextID;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
typedef XID Pixmap;
typedef XID Font;
typedef XID Window;
typedef XID GLXVideoCaptureDeviceNV;
typedef XID GLXPbufferSGIX;

typedef unsigned int GLXVideoDeviceNV;
//typedef struct GLXHyperpipeConfigSGIX GLXHyperpipeConfigSGIX;
//typedef struct GLXHyperpipeNetworkSGIX GLXHyperpipeNetworkSGIX;
typedef XID Colormap;

#ifndef Bool
#define Bool int
#endif
//typedef int Bool;
struct XVisualInfo;
#ifndef GITS_PLATFORM_X11
typedef void* XVisualInfo_;
#endif
