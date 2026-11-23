// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "glTypes.h"

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gits::opengl {

// Hides the extensions in OpenGL.Recorder.SuppressExtensions and, with
// OpenGL.Recorder.SuppressProgramBinary, binaries only the recording driver can load, so the
// application compiles its shaders from source. Each answer leaves the driver's error state as the
// driver's own call would.
class CapabilitySuppressionService {
public:
  static CapabilitySuppressionService& Get();

  // Answers a state query in place of the driver: the number of extensions left after hiding, no
  // program binary formats, and only the SPIR-V shader binary formats. Returns false, leaving the
  // query to the driver, for any other `pname` or when there is nothing to hide. The format lists
  // are written to arrays sized by the reported counts, which may be empty.
  template <typename T>
  bool AnswerQuery(GLenum pname, T* data);

  // The driver's index of the extension at `index` in the current context's filtered list, or
  // nullopt when nothing is hidden from it. An index past the end of the filtered list stays past
  // the end of the driver's.
  std::optional<GLuint> DriverExtensionIndex(GLuint index);

  // The extension string without the hidden extensions. The result stays valid for the process
  // lifetime.
  const char* HideExtensions(const char* extensions);

  // Whether a binary load in a hidden format must be neither executed nor recorded. A new program
  // then stays unlinked, as after a load the driver rejects; one linked before keeps its
  // executable. SPIR-V shader binaries are portable and never hidden.
  bool SkipsProgramBinary(GLenum binaryFormat);
  bool SkipsShaderBinary(GLenum binaryFormat);

private:
  CapabilitySuppressionService() = default;

  // The extensions left in a context's list, by their driver indices.
  struct ContextExtensions {
    std::vector<GLuint> DriverIndices;
    GLuint Hidden{0};
  };

  // Called with m_ExtensionsMutex held.
  bool IsHidden(std::string_view extension);

  // The current context's extensions, or null when none are hidden from it. Entries are never
  // erased or changed once built.
  const ContextExtensions* CurrentContextExtensions();

  std::mutex m_ExtensionsMutex;
  std::map<std::string, std::string, std::less<>> m_ExtensionStrings;
  // Keyed by the context's GITSKey, which a later context reusing the handle does not get.
  std::unordered_map<GITSKey, ContextExtensions> m_ContextExtensions;
  std::set<std::string, std::less<>> m_LoggedExtensions;

  std::atomic<bool> m_LoggedProgramFormats{false};
  std::atomic<bool> m_LoggedShaderFormats{false};
  std::atomic<bool> m_LoggedProgramLoad{false};
  std::atomic<bool> m_LoggedShaderLoad{false};
};

} // namespace gits::opengl
