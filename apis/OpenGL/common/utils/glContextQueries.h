// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <functional>
#include <utility>

namespace gits::opengl {

// The native context current on this thread, or nullptr, as reported by the real driver.
#if defined(_WIN32)
HGLRC CurrentContext();
#else
GLXContext CurrentContext();
#endif

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

// Checks the forward-compatible flag first, then the core profile (implicit on GL 3.1).
// Pass extensions to reuse an existing extension query for GL 3.1.
bool CurrentContextIsCoreOrForwardCompatible(std::pair<int, int> version,
                                             const CurrentContextExtensions* extensions = nullptr);

// Tests whether the context retains features deprecated in GL 3.0. On GL 3.2 and later this
// requires the compatibility bit and is not simply the inverse of the core profile bit.
bool CompatibilityContext(std::pair<int, int> version);

// Checks core support first, then the extensions in order. Does not cache the result.
bool SupportsCoreOrExtension(std::pair<int, int> version,
                             int coreMajor,
                             int coreMinor,
                             const char* extension,
                             const char* alternate = nullptr);

} // namespace gits::opengl
