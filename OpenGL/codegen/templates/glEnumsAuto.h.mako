// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER.replace('#', '/')}

#pragma once

// For name string getters at the end of this file.
#include "openglTypes.h"
#include <string>
#include <string_view>

/**
 * @file   glEnumsAuto.h
 *
 * @brief  Contains definitions of GLenums, and functions to get a name/string
 * of a GLenum value.
 */

// ##### Macro definitions #####

#ifndef EGL_CAST
// C++ typecast macro for special EGL handle values.
#define EGL_CAST(type, value) (static_cast<type>(value))
#endif

% for enum in enums:
<%
    comment = f'\n// {enum.comment}' if enum.comment else ''
    alias = f'  // AKA {enum.alias}' if enum.alias else ''
    # Note: enum.type is printed as C literal suffix (like "u" or "ull")
%>\
#ifndef ${enum.name}${comment}
#define ${enum.name} ${enum.value}${enum.type}${alias}
#endif

% endfor\


// Macros for WGL's PIXELFORMATDESCRIPTOR bitflags and other constants.
// For compatibility with the old, manually written glEnums.h.
// TODO: A proper fix is to ensure that Linux never builds the related code.
// Then those macros can be removed.
#define PFD_TYPE_RGBA                   0
#define PFD_TYPE_COLORINDEX             1
#define PFD_MAIN_PLANE                  0
#define PFD_OVERLAY_PLANE               1
#define PFD_UNDERLAY_PLANE              (-1)
#define PFD_DOUBLEBUFFER                0x00000001
#define PFD_STEREO                      0x00000002
#define PFD_DRAW_TO_WINDOW              0x00000004
#define PFD_DRAW_TO_BITMAP              0x00000008
#define PFD_SUPPORT_GDI                 0x00000010
#define PFD_SUPPORT_OPENGL              0x00000020
#define PFD_GENERIC_FORMAT              0x00000040
#define PFD_NEED_PALETTE                0x00000080
#define PFD_NEED_SYSTEM_PALETTE         0x00000100
#define PFD_SWAP_EXCHANGE               0x00000200
#define PFD_SWAP_COPY                   0x00000400
#define PFD_SWAP_LAYER_BUFFERS          0x00000800
#define PFD_GENERIC_ACCELERATED         0x00001000
#define PFD_SUPPORT_DIRECTDRAW          0x00002000
#define PFD_DIRECT3D_ACCELERATED        0x00004000
#define PFD_SUPPORT_COMPOSITION         0x00008000
#define PFD_DEPTH_DONTCARE              0x20000000
#define PFD_DOUBLEBUFFER_DONTCARE       0x40000000
#define PFD_STEREO_DONTCARE             0x80000000


// ##### Name strings #####

<%
    all_groups: list[str] = sorted({key.group for key in enum_name_map if key.group})
%>\

enum class GLenumApi : uint32_t {
  Unknown = 0,
  GL,
  WGL,
  EGL,
  GLX,
};

// WARNING: GLenumGroup enumerant values are determined by alphabetical sort
// order of group names from the XML. Adding new groups shifts later values.
// Never persist these numeric values across builds (e.g. in saved captures).
enum class GLenumGroup : uint32_t {
  NoGroup = 0, // Can't be named "None", as Xlib.h has a macro called "None".
  Unknown,  // Group is unknown, all groups (and no group) must be considered.
% for group in all_groups:
  ${group},
% endfor
};

// Returns true if the given group is a bitmask group (contains bitflags
// instead of normal enums).
bool IsBitmaskGroup(GLenumGroup group);

// NOTE: A GLenum value can map to multiple names. The following functions try
// their best to return a sensible name for a given value, but it's not
// guaranteed to be what you want.

// Returns the name of a GLenum value, or an empty string_view if the value is
// unknown for this api and group. If you give it a bitmask, it will only
// return a name if the value is an exact match for a single GLenum constant.
std::string_view GetGLenumName(GLenumApi api, GLenum value, GLenumGroup group);

// Decomposes a bitmask into bitflags, e.g. "GL_FOO_BIT | GL_BAR_BIT".
// Unknown bits are appended as hex. Zero with no named constant returns "0".
std::string GetBitmaskString(GLenumApi api, GLenum value, GLenumGroup group);

// Dispatches to GetGLenumName or GetBitmaskString based on the group.
// Always returns a non-empty string; falls back to hex if no name is found.
std::string GetGLenumString(GLenumApi api, GLenum value, GLenumGroup group);
