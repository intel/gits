// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/** Windows Type definitions for Linux**/

#pragma once

#include "platform.h"

#if !defined GITS_PLATFORM_WINDOWS

typedef struct tagPIXELFORMATDESCRIPTOR {
  int nSize;
  int nVersion;
  int dwFlags;
  int iPixelType;
  int cColorBits;
  int cRedBits;
  int cRedShift;
  int cGreenBits;
  int cGreenShift;
  int cBlueBits;
  int cBlueShift;
  int cAlphaBits;
  int cAlphaShift;
  int cAccumBits;
  int cAccumRedBits;
  int cAccumGreenBits;
  int cAccumBlueBits;
  int cAccumAlphaBits;
  int cDepthBits;
  int cStencilBits;
  int cAuxBuffers;
  int iLayerType;
  int bReserved;
  int dwLayerMask;
  int dwVisibleMask;
  int dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR;

#endif
