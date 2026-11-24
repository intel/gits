// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glContextQueries.h"

#include "glDispatchAuto.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"

#include <atomic>
#include <cstring>
#include <sstream>
#include <string>

namespace gits::opengl {

namespace {

GLint GetInteger(GLenum pname) {
  GLint value = 0;
  g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(pname, &value);
  return value;
}

bool IsForwardCompatible() {
  return (GetInteger(GL_CONTEXT_FLAGS) & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0;
}

bool HasArbCompatibility(std::pair<int, int> version, const CurrentContextExtensions* extensions) {
  constexpr char extension[] = "GL_ARB_compatibility";
  return extensions ? extensions->Has(extension) : CurrentContextHasExtension(version, extension);
}

template <typename Predicate>
bool VisitIndexedExtensions(GLint count, Predicate stop) {
  for (GLint i = 0; i < count; ++i) {
    // The first call may replace the lazy thunk with the resolved function.
    auto getStringi = g_DispatchTable.glGetStringi.load(std::memory_order_acquire);
    const auto* name =
        reinterpret_cast<const char*>(getStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
    if (name != nullptr && stop(name)) {
      return true;
    }
  }
  return false;
}

} // namespace

#if defined(_WIN32)
HGLRC CurrentContext() {
  auto realFunction = g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}
#else
GLXContext CurrentContext() {
  auto realFunction = g_DispatchTable.glXGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}
#endif

std::pair<int, int> CurrentContextGLVersion() {
  auto realGetString = g_DispatchTable.glGetString.load(std::memory_order_acquire);
  return ParseGlVersionMajorMinor(reinterpret_cast<const char*>(realGetString(GL_VERSION)));
}

CurrentContextExtensions::CurrentContextExtensions(std::pair<int, int> version)
    : m_Indexed(GlVersionAtLeast(version, 3, 0)) {
  // Indexed enumeration requires GL 3.0; the legacy string is not legal in core profiles.
  if (m_Indexed) {
    g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_NUM_EXTENSIONS, &m_Count);
  } else {
    m_Legacy = reinterpret_cast<const char*>(
        g_DispatchTable.glGetString.load(std::memory_order_acquire)(GL_EXTENSIONS));
  }
}

bool CurrentContextExtensions::Has(const char* name) const {
  if (!m_Indexed) {
    return ExtensionsStringContainsWholeWord(m_Legacy, name);
  }
  return VisitIndexedExtensions(
      m_Count, [name](const char* extension) { return std::strcmp(extension, name) == 0; });
}

void CurrentContextExtensions::ForEach(const std::function<void(const char*)>& visit) const {
  if (m_Indexed) {
    VisitIndexedExtensions(m_Count, [&visit](const char* name) {
      visit(name);
      return false;
    });
  } else if (m_Legacy != nullptr) {
    std::istringstream stream(m_Legacy);
    for (std::string name; stream >> name;) {
      visit(name.c_str());
    }
  }
}

bool CurrentContextHasExtension(std::pair<int, int> version, const char* extensionName) {
  return CurrentContextExtensions(version).Has(extensionName);
}

bool CurrentContextIsCoreOrForwardCompatible(std::pair<int, int> version,
                                             const CurrentContextExtensions* extensions) {
  if (GlVersionAtLeast(version, 3, 0) && IsForwardCompatible()) {
    return true;
  }
  if (GlVersionAtLeast(version, 3, 2)) {
    return (GetInteger(GL_CONTEXT_PROFILE_MASK) & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
  }
  return version == std::pair{3, 1} && !HasArbCompatibility(version, extensions);
}

bool CompatibilityContext(std::pair<int, int> version) {
  if (version.first < 3) {
    return true;
  }
  if (IsForwardCompatible()) {
    return false;
  }
  if (version == std::pair{3, 0}) {
    return true;
  }
  if (version == std::pair{3, 1}) {
    return HasArbCompatibility(version, nullptr);
  }
  return (GetInteger(GL_CONTEXT_PROFILE_MASK) & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;
}

bool SupportsCoreOrExtension(std::pair<int, int> version,
                             int coreMajor,
                             int coreMinor,
                             const char* extension,
                             const char* alternate) {
  return GlVersionAtLeast(version, coreMajor, coreMinor) ||
         CurrentContextHasExtension(version, extension) ||
         (alternate != nullptr && CurrentContextHasExtension(version, alternate));
}

} // namespace gits::opengl
