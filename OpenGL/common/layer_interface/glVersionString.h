// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <utility>

namespace gits::opengl {

// Parses a GL_VERSION string's leading "<major>.<minor>" (e.g. "3.3.0 NVIDIA 555.85",
// "4.6 (Core Profile) Mesa 24.0.5") into (major, minor). Returns (0, 0) - older than every real
// version - if the string is null or does not start with two dot-separated integers, so a
// version that cannot be parsed is treated as supporting nothing rather than as new enough.
std::pair<int, int> ParseGlVersionMajorMinor(const char* versionString);

// True if the GL_EXTENSIONS space-separated legacy string contains `extensionName` as a whole
// word - not merely as a substring of some other, longer extension's name (e.g.
// "GL_ARB_query_buffer_object" must not match inside a hypothetical
// "GL_ARB_query_buffer_object_v2").
bool ExtensionsStringContainsWholeWord(const char* extensions, const char* extensionName);

// Whether `version` (from ParseGlVersionMajorMinor()) is at least major.minor.
inline bool GlVersionAtLeast(std::pair<int, int> version, int major, int minor) {
  return version.first > major || (version.first == major && version.second >= minor);
}

} // namespace gits::opengl
