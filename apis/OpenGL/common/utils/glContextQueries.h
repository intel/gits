// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <functional>
#include <utility>

namespace gits::opengl {

// Queries the current context's GL_VERSION. Returns (0, 0) if it cannot be parsed.
std::pair<int, int> CurrentContextGLVersion();

// Reads the extension count or legacy string once. Pass the current context's known GL version and
// use this object only while that context remains current on the calling thread.
class CurrentContextExtensions {
public:
  explicit CurrentContextExtensions(std::pair<int, int> version);

  bool Has(const char* name) const;

  // The name passed to the visitor is valid only for the duration of the call.
  void ForEach(const std::function<void(const char*)>& visit) const;

private:
  bool m_Indexed;
  int m_Count{0};
  const char* m_Legacy{nullptr};
};

// One-off extension lookup using the current context's already queried version.
bool CurrentContextHasExtension(std::pair<int, int> version, const char* extensionName);

// Checks core support first, then the extensions in order. Does not cache the result.
bool SupportsCoreOrExtension(std::pair<int, int> version,
                             int coreMajor,
                             int coreMinor,
                             const char* extension,
                             const char* alternate = nullptr);

} // namespace gits::opengl
