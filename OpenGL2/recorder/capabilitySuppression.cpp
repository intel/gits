// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capabilitySuppression.h"

#include "configurator.h"
#include "glEnumsAuto.h"
#include "handleMapService.h"
#include "log.h"
#include "openglDriversAuto.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <iterator>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

extern "C" __declspec(dllimport) DWORD __stdcall GetLastError();
extern "C" __declspec(dllimport) void __stdcall SetLastError(DWORD dwErrCode);

namespace gits::opengl2 {

namespace {

// The extensions left in a context's list, by their driver indices.
struct ContextExtensions {
  std::vector<GLuint> DriverIndices;
  GLuint Hidden{0};
};

std::mutex g_ExtensionsMutex;
std::map<std::string, std::string, std::less<>> g_ExtensionStrings;
std::unordered_map<GITSKey, ContextExtensions> g_ContextExtensions;
std::set<std::string, std::less<>> g_LoggedExtensions;

std::atomic<bool> g_LoggedProgramFormats{false};
std::atomic<bool> g_LoggedShaderFormats{false};
std::atomic<bool> g_LoggedProgramLoad{false};
std::atomic<bool> g_LoggedShaderLoad{false};

void LogInfo(std::string_view message, const char* option) {
  const DWORD lastError = GetLastError();
  LOG_INFO << message << " (OpenGL.Recorder." << option << ")";
  SetLastError(lastError);
}

void LogOnce(std::atomic<bool>& logged, const char* message) {
  if (!logged.exchange(true, std::memory_order_relaxed)) {
    LogInfo(message, "SuppressProgramBinary");
  }
}

const std::vector<std::string>& SuppressedExtensions() {
  return Configurator::Get().opengl.recorder.suppressExtensions;
}

bool SuppressionEnabled() {
  return Configurator::Get().opengl.recorder.suppressProgramBinary;
}

// Called with g_ExtensionsMutex held.
bool IsHidden(std::string_view extension) {
  const auto& suppressed = SuppressedExtensions();
  if (std::find(suppressed.begin(), suppressed.end(), extension) == suppressed.end()) {
    return false;
  }
  if (g_LoggedExtensions.insert(std::string(extension)).second) {
    LogInfo("Hiding extension " + std::string(extension), "SuppressExtensions");
  }
  return true;
}

// Zero when the driver reports none or rejects `pname`, which then leaves `value` unwritten.
GLint DriverInteger(GLenum pname) {
  GLint value = 0;
  g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(pname, &value);
  return value;
}

// The current context's extensions, or null when none are hidden from it. Entries are never
// erased or changed once built.
const ContextExtensions* CurrentContextExtensions() {
  if (SuppressedExtensions().empty()) {
    return nullptr;
  }
  const auto getCurrentContext =
      g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  const HGLRC context = getCurrentContext != nullptr ? getCurrentContext() : nullptr;
  if (context == nullptr) {
    return nullptr;
  }
  // Keyed by the context's GITSKey, which a later context reusing the handle does not get.
  const GITSKey contextKey = ContextHandleMapService::Get().GetKey(context);
  if (contextKey == 0) {
    return nullptr;
  }
  // Queried on every call, so a query the driver rejects, as inside glBegin/glEnd, goes to it.
  const auto getStringi = g_DispatchTable.glGetStringi.load(std::memory_order_acquire);
  const GLint count = getStringi != nullptr ? DriverInteger(GL_NUM_EXTENSIONS) : 0;
  if (count <= 0) {
    return nullptr;
  }
  std::lock_guard lock(g_ExtensionsMutex);
  auto [it, inserted] = g_ContextExtensions.try_emplace(contextKey);
  if (inserted) {
    for (GLint index = 0; index < count; ++index) {
      const auto* extension =
          reinterpret_cast<const char*>(getStringi(GL_EXTENSIONS, static_cast<GLuint>(index)));
      if (extension != nullptr && IsHidden(extension)) {
        ++it->second.Hidden;
      } else {
        it->second.DriverIndices.push_back(static_cast<GLuint>(index));
      }
    }
  }
  return it->second.Hidden != 0 ? &it->second : nullptr;
}

std::vector<GLint> DriverFormats(GLenum countPname, GLenum listPname) {
  const GLint count = DriverInteger(countPname);
  if (count <= 0) {
    return {};
  }
  std::vector<GLint> formats(static_cast<size_t>(count));
  g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(listPname, formats.data());
  return formats;
}

// A load in a format the driver does not list fails with an error, which replays the same way.
bool IsDriverFormat(GLenum countPname, GLenum listPname, GLenum format) {
  const std::vector<GLint> formats = DriverFormats(countPname, listPname);
  return std::find(formats.begin(), formats.end(), static_cast<GLint>(format)) != formats.end();
}

// The driver's SPIR-V shader binary formats, or nullopt when it reports no others to hide.
std::optional<std::vector<GLint>> PortableShaderBinaryFormats() {
  const std::vector<GLint> formats =
      DriverFormats(GL_NUM_SHADER_BINARY_FORMATS, GL_SHADER_BINARY_FORMATS);
  std::vector<GLint> portable;
  std::copy_if(formats.begin(), formats.end(), std::back_inserter(portable),
               [](GLint format) { return format == GL_SHADER_BINARY_FORMAT_SPIR_V; });
  if (portable.size() == formats.size()) {
    return std::nullopt;
  }
  return portable;
}

template <typename T>
T ToStateValue(GLint value) {
  if constexpr (std::is_same_v<T, GLboolean>) {
    return value != 0 ? GL_TRUE : GL_FALSE;
  } else {
    return static_cast<T>(value);
  }
}

// The format lists are written to arrays sized by the reported counts, which may be empty.
template <typename T>
bool AnswerQuery(GLenum pname, T* data) {
  switch (pname) {
  case GL_NUM_EXTENSIONS: {
    const ContextExtensions* extensions = CurrentContextExtensions();
    if (extensions == nullptr) {
      return false;
    }
    if (data != nullptr) {
      *data = ToStateValue<T>(static_cast<GLint>(extensions->DriverIndices.size()));
    }
    return true;
  }
  case GL_NUM_PROGRAM_BINARY_FORMATS:
  case GL_PROGRAM_BINARY_FORMATS: {
    if (!SuppressionEnabled() || DriverInteger(GL_NUM_PROGRAM_BINARY_FORMATS) <= 0) {
      return false;
    }
    LogOnce(g_LoggedProgramFormats,
            "Reporting no program binary formats, so shaders are compiled from source");
    if (pname == GL_NUM_PROGRAM_BINARY_FORMATS && data != nullptr) {
      *data = ToStateValue<T>(0);
    }
    return true;
  }
  case GL_NUM_SHADER_BINARY_FORMATS:
  case GL_SHADER_BINARY_FORMATS: {
    const auto formats = SuppressionEnabled() ? PortableShaderBinaryFormats() : std::nullopt;
    if (!formats) {
      return false;
    }
    LogOnce(g_LoggedShaderFormats, "Reporting only SPIR-V shader binary formats");
    if (pname == GL_NUM_SHADER_BINARY_FORMATS) {
      if (data != nullptr) {
        *data = ToStateValue<T>(static_cast<GLint>(formats->size()));
      }
    } else if (data != nullptr) {
      std::transform(formats->begin(), formats->end(), data, ToStateValue<T>);
    }
    return true;
  }
  default:
    return false;
  }
}

} // namespace

bool AnswerStateQuery(GLenum pname, GLboolean* data) {
  return AnswerQuery(pname, data);
}

bool AnswerStateQuery(GLenum pname, GLint* data) {
  return AnswerQuery(pname, data);
}

bool AnswerStateQuery(GLenum pname, GLint64* data) {
  return AnswerQuery(pname, data);
}

bool AnswerStateQuery(GLenum pname, GLfloat* data) {
  return AnswerQuery(pname, data);
}

bool AnswerStateQuery(GLenum pname, GLdouble* data) {
  return AnswerQuery(pname, data);
}

const GLubyte* HideExtensions(GLenum name, const GLubyte* extensions) {
  if (name != GL_EXTENSIONS) {
    return extensions;
  }
  return reinterpret_cast<const GLubyte*>(
      HideExtensions(reinterpret_cast<const char*>(extensions)));
}

const char* HideExtensions(const char* extensions) {
  if (extensions == nullptr || SuppressedExtensions().empty()) {
    return extensions;
  }
  const std::string_view driverString(extensions);
  std::lock_guard lock(g_ExtensionsMutex);
  auto it = g_ExtensionStrings.find(driverString);
  if (it == g_ExtensionStrings.end()) {
    std::string kept;
    size_t begin = 0;
    while (begin < driverString.size()) {
      const size_t end = std::min(driverString.find(' ', begin), driverString.size());
      const std::string_view extension = driverString.substr(begin, end - begin);
      if (!extension.empty() && !IsHidden(extension)) {
        kept.append(extension).push_back(' ');
      }
      begin = end + 1;
    }
    if (!driverString.ends_with(' ') && !kept.empty()) {
      kept.pop_back();
    }
    it = g_ExtensionStrings.emplace(std::string(driverString), std::move(kept)).first;
  }
  return it->second.c_str();
}

GLuint DriverExtensionIndex(GLenum name, GLuint index) {
  if (name != GL_EXTENSIONS) {
    return index;
  }
  const ContextExtensions* extensions = CurrentContextExtensions();
  if (extensions == nullptr) {
    return index;
  }
  if (index < extensions->DriverIndices.size()) {
    return extensions->DriverIndices[index];
  }
  return static_cast<GLuint>(extensions->DriverIndices.size()) + extensions->Hidden;
}

bool SkipsProgramBinary(GLenum binaryFormat) {
  if (!SuppressionEnabled() ||
      !IsDriverFormat(GL_NUM_PROGRAM_BINARY_FORMATS, GL_PROGRAM_BINARY_FORMATS, binaryFormat)) {
    return false;
  }
  LogOnce(g_LoggedProgramLoad,
          "Skipping program binary loads, so shaders are compiled from source");
  return true;
}

bool SkipsShaderBinary(GLenum binaryFormat) {
  if (binaryFormat == GL_SHADER_BINARY_FORMAT_SPIR_V || !SuppressionEnabled() ||
      !IsDriverFormat(GL_NUM_SHADER_BINARY_FORMATS, GL_SHADER_BINARY_FORMATS, binaryFormat)) {
    return false;
  }
  LogOnce(g_LoggedShaderLoad, "Skipping shader binary loads in formats other than SPIR-V");
  return true;
}

} // namespace gits::opengl2
