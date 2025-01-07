// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   ptbl_wglLibrary.h
*
*/

#pragma once
#include "ptblLibrary.h"
#include "openglTypes.h"

namespace gits {
namespace OpenGL {

//****************** Portable interface *************************
int ptbl_wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd);
BOOL ptbl_wglChoosePixelFormatARB(HDC hdc,
                                  const int* piAttribIList,
                                  const FLOAT* pfAttribFList,
                                  UINT nMaxFormats,
                                  int* piFormats,
                                  UINT* nNumFormats);
BOOL ptbl_wglSetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* pfd);
HGLRC ptbl_wglCreateContext(HDC hdc);
HGLRC ptbl_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int* attribList);
BOOL ptbl_wglMakeCurrent(HDC hdc, HGLRC hglrc);
BOOL ptbl_wglDeleteContext(HGLRC hglrc);
BOOL ptbl_wglSwapBuffers(HDC hdc);
BOOL ptbl_wglShareLists(HGLRC hglrc1, HGLRC hglrc2);
HPBUFFERARB ptbl_wglCreatePbufferARB(
    HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
HDC ptbl_wglGetPbufferDCARB(HPBUFFERARB hPbuffer);

HGLRC ptbl_wglGetCurrentContext();
HDC ptblGetDC(HWND hwnd);
HWND ptblWindowFromDC(HDC dc);
int ptblReleaseDC(HWND hwnd, HDC hdc);

//****************** Portable execution *************************
void execSetContextWGL(PtblHandle ctx);
void execDelContextWGL(PtblHandle ctx);
void execBufferSwapWGL(PtblHandle ctx);
void execHelperCreatePBufferWGL(PtblHandle surf, PtblHandle format);

//****************** Helper functions ***************************
typedef std::vector<int> WGLARBPFAttribs;
typedef std::vector<int> WGLCtxParams;
void PtblToWGLPFDAttribs(PIXELFORMATDESCRIPTOR* pfd, const PtblPFAttribs& ptblattribs);
void WGLPFDToPtblAttribs(const PIXELFORMATDESCRIPTOR* pfd, PtblPFAttribs& ptblattribs);
WGLARBPFAttribs PtblToWGLARBAttribs(const PtblPFAttribs& ptblattribs);
PtblPFAttribs WGLARBToPtblAttribs(const int* wglattribs);
WGLCtxParams GetUpdatedWGLCtxParams(const int* params);
PtblCtxParams WGLToPtblCtxParams(const int* wglparams);
WGLCtxParams PtblToWGLCtxParams(const PtblCtxParams& ptblparams);
} // namespace OpenGL
} // namespace gits
