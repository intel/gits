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
