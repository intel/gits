// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   pfattribs.cpp
 *
 * @brief Definition of pixel format attributes translation functions.
 *
 */

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "pfattribs.h"
#include "log.h"
#include "openglEnums.h"
#include <unordered_map>

void gits::GetAttribsFromPFD(const PIXELFORMATDESCRIPTOR* pfd,
                             int size,
                             int* attribs,
                             int* values) {
  //arb attribs -> pfd map definition
  int pfdAccelTranslation = 0;
  if (pfd->dwFlags & PFD_GENERIC_ACCELERATED) {
    pfdAccelTranslation = WGL_FULL_ACCELERATION_ARB;
  } else if (pfd->dwFlags & PFD_GENERIC_FORMAT) {
    pfdAccelTranslation = WGL_GENERIC_ACCELERATION_ARB;
  } else {
    pfdAccelTranslation = WGL_NO_ACCELERATION_ARB;
  }

  int pfdPixTypeTranslation = 0;
  if (pfd->iPixelType & PFD_TYPE_RGBA) {
    pfdPixTypeTranslation = WGL_TYPE_RGBA_ARB;
  }
  if (pfd->iPixelType & PFD_TYPE_COLORINDEX) {
    pfdPixTypeTranslation = WGL_TYPE_COLORINDEX_ARB;
  }

  int pfdSwapMethodsTranslation = 0;
  if (pfd->dwFlags & PFD_SWAP_EXCHANGE) {
    pfdSwapMethodsTranslation = WGL_SWAP_EXCHANGE_ARB;
  } else if (pfd->dwFlags & PFD_SWAP_COPY) {
    pfdSwapMethodsTranslation = WGL_SWAP_COPY_ARB;
  } else {
    pfdSwapMethodsTranslation = WGL_SWAP_UNDEFINED_ARB; // ??????
  }

  typedef std::pair<int, int> pair_t;

  pair_t tmparray[] = {
      pair_t(WGL_DRAW_TO_WINDOW_ARB, (int)(pfd->dwFlags & PFD_DRAW_TO_WINDOW)),
      pair_t(WGL_DRAW_TO_BITMAP_ARB, (int)(pfd->dwFlags & PFD_DRAW_TO_BITMAP)),
      pair_t(WGL_ACCELERATION_ARB, pfdAccelTranslation),
      pair_t(WGL_NEED_PALETTE_ARB, (int)(pfd->dwFlags & PFD_NEED_PALETTE)),
      pair_t(WGL_NEED_SYSTEM_PALETTE_ARB, (int)(pfd->dwFlags & PFD_NEED_SYSTEM_PALETTE)),
      pair_t(WGL_SWAP_LAYER_BUFFERS_ARB, (int)(pfd->dwFlags & PFD_SWAP_LAYER_BUFFERS)),
      pair_t(WGL_SWAP_METHOD_ARB, pfdSwapMethodsTranslation),
      pair_t(WGL_SUPPORT_GDI_ARB, (int)(pfd->dwFlags & PFD_SUPPORT_GDI)),
      pair_t(WGL_SUPPORT_OPENGL_ARB, (int)(pfd->dwFlags & PFD_SUPPORT_OPENGL)),
      pair_t(WGL_DOUBLE_BUFFER_ARB, (int)(pfd->dwFlags & PFD_DOUBLEBUFFER)),
      pair_t(WGL_STEREO_ARB, (int)(pfd->dwFlags & PFD_STEREO)),
      pair_t(WGL_PIXEL_TYPE_ARB, pfdPixTypeTranslation),
      pair_t(WGL_COLOR_BITS_ARB, pfd->cColorBits),
      pair_t(WGL_RED_BITS_ARB, pfd->cRedBits),
      pair_t(WGL_RED_SHIFT_ARB, pfd->cRedShift),
      pair_t(WGL_GREEN_BITS_ARB, pfd->cGreenBits),
      pair_t(WGL_GREEN_SHIFT_ARB, pfd->cGreenShift),
      pair_t(WGL_BLUE_BITS_ARB, pfd->cBlueBits),
      pair_t(WGL_BLUE_SHIFT_ARB, pfd->cBlueShift),
      pair_t(WGL_ALPHA_BITS_ARB, pfd->cAlphaBits),
      pair_t(WGL_ALPHA_SHIFT_ARB, pfd->cAlphaShift),
      pair_t(WGL_ACCUM_BITS_ARB, pfd->cAccumBits),
      pair_t(WGL_ACCUM_RED_BITS_ARB, pfd->cAccumRedBits),
      pair_t(WGL_ACCUM_GREEN_BITS_ARB, pfd->cAccumGreenBits),
      pair_t(WGL_ACCUM_BLUE_BITS_ARB, pfd->cAccumBlueBits),
      pair_t(WGL_ACCUM_ALPHA_BITS_ARB, pfd->cAccumAlphaBits),
      pair_t(WGL_DEPTH_BITS_ARB, pfd->cDepthBits),
      pair_t(WGL_STENCIL_BITS_ARB, pfd->cStencilBits),
      pair_t(WGL_AUX_BUFFERS_ARB, pfd->cAuxBuffers)};

  std::unordered_map<int, int> translationMap(tmparray,
                                              tmparray + sizeof(tmparray) / sizeof(tmparray[0]));

  //Get arb pixel format attribs values from created map
  for (int i = 0; i < size; i++, values++, attribs++) {
    if (translationMap.find(*attribs) == translationMap.end()) {
      //attribs which are not translatable are set to 0
      *attribs = 0;
      *values = 0;
    } else {
      *values = translationMap[*attribs];
    }
  }
}

void gits::GetPFDFromAttribs(int size,
                             const int* attribs,
                             const int* values,
                             PIXELFORMATDESCRIPTOR* pfd) {
  pfd->dwFlags = 0;

  for (int i = 0; i < size; i++, attribs++, values++) {
    switch (*attribs) {
    case WGL_DRAW_TO_WINDOW_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_DRAW_TO_WINDOW;
      }
      break;
    case WGL_DRAW_TO_BITMAP_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_DRAW_TO_BITMAP;
      }
      break;
    case WGL_ACCELERATION_ARB:
      switch (*values) {
      case WGL_FULL_ACCELERATION_ARB:
        pfd->dwFlags = pfd->dwFlags | PFD_GENERIC_ACCELERATED;
        break;
      case WGL_GENERIC_ACCELERATION_ARB:
        pfd->dwFlags = pfd->dwFlags | PFD_GENERIC_FORMAT;
        break;
      }
      break;
    case WGL_NEED_PALETTE_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_NEED_PALETTE;
      }
      break;
    case WGL_NEED_SYSTEM_PALETTE_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_NEED_SYSTEM_PALETTE;
      }
      break;
    case WGL_SWAP_LAYER_BUFFERS_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_SWAP_LAYER_BUFFERS;
      }
      break;
    case WGL_SWAP_METHOD_ARB:
      switch (*values) {
      case WGL_SWAP_EXCHANGE_ARB:
        pfd->dwFlags = pfd->dwFlags | PFD_SWAP_EXCHANGE;
        break;
      case WGL_SWAP_COPY_ARB:
        pfd->dwFlags = pfd->dwFlags | PFD_SWAP_COPY;
        break;
      }
      break;
    case WGL_SUPPORT_GDI_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_SUPPORT_GDI;
      }
      break;
    case WGL_SUPPORT_OPENGL_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_SUPPORT_OPENGL;
      }
      break;
    case WGL_DOUBLE_BUFFER_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_DOUBLEBUFFER;
      }
      break;
    case WGL_STEREO_ARB:
      if (*values > 0) {
        pfd->dwFlags = pfd->dwFlags | PFD_STEREO;
      }
      break;
    case WGL_PIXEL_TYPE_ARB:
      switch (*values) {
      case WGL_TYPE_RGBA_ARB:
        pfd->dwFlags = pfd->dwFlags | PFD_TYPE_RGBA;
        break;
      case WGL_TYPE_COLORINDEX_ARB:
        pfd->dwFlags = pfd->dwFlags | PFD_TYPE_COLORINDEX;
        break;
      }
      break;
    case WGL_COLOR_BITS_ARB:
      pfd->cColorBits = (BYTE)*values;
      break;
    case WGL_RED_BITS_ARB:
      pfd->cRedBits = (BYTE)*values;
      break;
    case WGL_RED_SHIFT_ARB:
      pfd->cRedShift = (BYTE)*values;
      break;
    case WGL_GREEN_BITS_ARB:
      pfd->cGreenBits = (BYTE)*values;
      break;
    case WGL_GREEN_SHIFT_ARB:
      pfd->cGreenShift = (BYTE)*values;
      break;
    case WGL_BLUE_BITS_ARB:
      pfd->cBlueBits = (BYTE)*values;
      break;
    case WGL_BLUE_SHIFT_ARB:
      pfd->cBlueShift = (BYTE)*values;
      break;
    case WGL_ALPHA_BITS_ARB:
      pfd->cAlphaBits = (BYTE)*values;
      break;
    case WGL_ALPHA_SHIFT_ARB:
      pfd->cAlphaShift = (BYTE)*values;
      break;
    case WGL_ACCUM_BITS_ARB:
      pfd->cAccumBits = (BYTE)*values;
      break;
    case WGL_ACCUM_RED_BITS_ARB:
      pfd->cAccumRedBits = (BYTE)*values;
      break;
    case WGL_ACCUM_GREEN_BITS_ARB:
      pfd->cAccumGreenBits = (BYTE)*values;
      break;
    case WGL_ACCUM_BLUE_BITS_ARB:
      pfd->cAccumBlueBits = (BYTE)*values;
      break;
    case WGL_ACCUM_ALPHA_BITS_ARB:
      pfd->cAccumAlphaBits = (BYTE)*values;
      break;
    case WGL_DEPTH_BITS_ARB:
      pfd->cDepthBits = (BYTE)*values;
      break;
    case WGL_STENCIL_BITS_ARB:
      pfd->cStencilBits = (BYTE)*values;
      break;
    case WGL_AUX_BUFFERS_ARB:
      pfd->cAuxBuffers = (BYTE)*values;
      break;
    case 0:
      break;
    default:
      // This is unknown attribute, its not necessarily an error as pfd does not
      // support all pixel formats (eg. ones with multisampling).
      Log(WARN) << "Unknown pixelformat attribute '" << *attribs << "' with value '" << *values;
    }
  }
}
