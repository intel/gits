// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER.replace('#', '/')}

/**
 * @file   glEnumsAuto.cpp
 *
 * @brief  Contains implementations of functions to get GLenum names as strings.
 */

#include "glEnumsAuto.h"

#include "exception.h"
#include "log.h"
#include "openglTypes.h"
#include "platform.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <span>
#include <unordered_map>

<%
    sorted_bitmask_groups: list[str] = sorted(bitmask_groups)
%>\

namespace {

struct EnumEntry {
  GLenumGroup group;
  GLenum value;
  std::string_view name;
};

// Per-API enum data, generated from gl.xml, wgl.xml, egl.xml, glx.xml.

% for api in Api:
static constexpr std::array<EnumEntry, ${len(enum_entries_by_api[api])}> ${api.lower()}Enums = {{
  % for group_str, value, name in enum_entries_by_api[api]:
    % if value < 0:
  {GLenumGroup::${group_str}, static_cast<GLenum>(${name}), "${name}"},
    % else:
  {GLenumGroup::${group_str}, ${name}, "${name}"},
    % endif  # value < 0
  % endfor
}};

% endfor

// Per-API bitflag data: single-bit values from bitmask groups only.
// Multi-bit and zero-value constants are resolved via the regular enum data.
// When this template was written, only GL had group annotations, so other arrays can be empty.

% for api in Api:
static constexpr std::array<EnumEntry, ${len(bitflag_entries_by_api[api])}> ${api.lower()}Bitflags = {{
  % for group_str, value, name in bitflag_entries_by_api[api]:
  {GLenumGroup::${group_str}, ${name}, "${name}"},
  % endfor
}};

% endfor

// Maps GLenum's (group, value) -> name.
// Includes mapping for GLenumGroup::Unknown, to values of any group.
// Group and value are both 32-bit. We pack them into one 64-bit key.
using GLenumNameMap = std::unordered_map<uint64_t, std::string_view>;

GLenumNameMap buildMap(std::span<const EnumEntry> entries) {
  GLenumNameMap map;
  // Reserve twice the space: once for real entries, once for entries with
  // group set to GLenumGroup::Unknown, to allow group-agnostic value lookup.
  map.reserve(entries.size() * 2);

  // Pre-shifted to avoid recomputing the shift for every insertion.
  const uint64_t unknownGroup = uint64_t(GLenumGroup::Unknown) << 32;

  for (const auto& e : entries) {
    map[(uint64_t(e.group) << 32) | e.value] = e.name;
    // On collisions with GLenumGroup::Unknown: when enums from multiple groups
    // collide, the last insertion wins, which is arbitrary but acceptable.
    map[unknownGroup | e.value] = e.name;
  }

  return map;
}

std::string to_string(GLenumApi api) {
  switch (api) {
  case GLenumApi::GL:      return "GL";
  case GLenumApi::WGL:     return "WGL";
  case GLenumApi::EGL:     return "EGL";
  case GLenumApi::GLX:     return "GLX";
  case GLenumApi::Unknown: return "Unknown";
  default:
    LOG_ERROR << "Unexpected GLenumApi value.";
    throw std::logic_error(EXCEPTION_MESSAGE);
  }
}

const GLenumNameMap& getEnumMap(GLenumApi api) {
  static const GLenumNameMap glMap  = buildMap(glEnums);
  static const GLenumNameMap wglMap = buildMap(wglEnums);
  static const GLenumNameMap eglMap = buildMap(eglEnums);
  static const GLenumNameMap glxMap = buildMap(glxEnums);
  switch (api) {
  case GLenumApi::GL:  return glMap;
  case GLenumApi::WGL: return wglMap;
  case GLenumApi::EGL: return eglMap;
  case GLenumApi::GLX: return glxMap;
  default:             return glMap;
  }
}

const GLenumNameMap& getBitflagMap(GLenumApi api) {
  static const GLenumNameMap glMap  = buildMap(glBitflags);
  static const GLenumNameMap wglMap = buildMap(wglBitflags);
  static const GLenumNameMap eglMap = buildMap(eglBitflags);
  static const GLenumNameMap glxMap = buildMap(glxBitflags);
  switch (api) {
  case GLenumApi::GL:  return glMap;
  case GLenumApi::WGL: return wglMap;
  case GLenumApi::EGL: return eglMap;
  case GLenumApi::GLX: return glxMap;
  default:             return glMap;
  }
}

} // namespace

bool IsBitmaskGroup(GLenumGroup group) {
  switch (group) {
% for g in sorted_bitmask_groups:
  case GLenumGroup::${g}:
% endfor
    return true;
  default:
    return false;
  }
}

std::string_view GetGLenumName(GLenumApi api, GLenum value, GLenumGroup group) {
  const uint64_t key = (uint64_t(group) << 32) | value;

  if (api != GLenumApi::Unknown) {
    const GLenumNameMap& map = getEnumMap(api);
    const auto it = map.find(key);
    if (it != map.end()) {
      return it->second;
    }
  }

  // Not found in the requested API; try fallback APIs in platform-dependent
  // order. GL is always tried first as it has the most complete annotations.
  // TODO: Detect the active API at runtime and prioritize it after GL.

#if defined(GITS_PLATFORM_WINDOWS)
  constexpr std::array fallbackOrder = {
    GLenumApi::GL, GLenumApi::WGL, GLenumApi::EGL, GLenumApi::GLX
  };
#elif defined(GITS_PLATFORM_LINUX)
  constexpr std::array fallbackOrder = {
    GLenumApi::GL, GLenumApi::EGL, GLenumApi::GLX, GLenumApi::WGL
  };
#else
  // Unknown platform; defaulting to Linux/EGL order.
  #warning "Unknown platform in GetGLenumName fallback order. Defaulting to Linux order."
  constexpr std::array fallbackOrder = {
    GLenumApi::GL, GLenumApi::EGL, GLenumApi::GLX, GLenumApi::WGL
  };
#endif

  for (const GLenumApi fallback : fallbackOrder) {
    if (fallback == api) {
      continue;
    }
    const GLenumNameMap& map = getEnumMap(fallback);
    const auto it = map.find(key);
    if (it != map.end()) {
      if (api != GLenumApi::Unknown) {
        LOG_WARNING << "GLenum 0x" << std::hex << value
                    << " (group " << static_cast<uint32_t>(group) << ")"
                    << " not found in " << to_string(api)
                    << ", found in " << to_string(fallback) << " instead.\n";
      }
      return it->second;
    }
  }

  return {};
}

std::string GetBitmaskString(GLenumApi api, GLenum value, GLenumGroup group) {
  // Check for exact match first to handle all-bits-set constants
  // (e.g. GL_ALL_ATTRIB_BITS) and zero-value named constants.
  const std::string_view exactMatch = GetGLenumName(api, value, group);
  if (!exactMatch.empty()) {
    return std::string(exactMatch);
  }

  if (value == 0u) {
    return "0";
  }

  // Decompose the bitmask into individual flags, from lowest to highest bit.
  // Accumulate set bits that don't match any flags and add them as a single
  // hex value at the end.
  // Currently only GL has group annotations on <enum> XML elements, so we
  // don't do any cross-API fallback here. Not sure if there even are any
  // bitmasks with cross-API bitflags.
  const GLenumNameMap& bitflagMap = getBitflagMap(api);
  std::string result;
  uint32_t unknownBits = 0u;
  uint32_t remaining = static_cast<uint32_t>(value);

  while (remaining != 0u) {
    const uint32_t lowestSetBit = 1u << std::countr_zero(remaining);
    remaining &= ~lowestSetBit;

    const auto it = bitflagMap.find((uint64_t(group) << 32) | lowestSetBit);
    if (it != bitflagMap.end()) {
      if (!result.empty()) {
        result += " | ";
      }
      result += it->second;
    } else {
      unknownBits |= lowestSetBit;
    }
  }

  if (unknownBits != 0u) {
    if (!result.empty()) {
      result += " | ";
    }
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << unknownBits;
    result += oss.str();
  }

  return result;
}

std::string GetGLenumString(GLenumApi api, GLenum value, GLenumGroup group) {
  if (IsBitmaskGroup(group)) {
    return GetBitmaskString(api, value, group);
  }

  const std::string_view name = GetGLenumName(api, value, group);
  if (!name.empty()) {
    return std::string(name);
  }

  // Fallback: return hex representation.
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase << value;
  return oss.str();
}
