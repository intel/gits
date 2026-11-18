// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureCustomizationLayer.h"

#include "capabilitySuppressionService.h"
#include "externalHandleService.h"
#include "glEnumsAuto.h"
#include "log.h"
#include "openglDriversAuto.h"

namespace gits::opengl {

namespace {

template <typename CommandT>
void AnswerStateQuery(CommandT& command) {
  if (CapabilitySuppressionService::Get().AnswerQuery(command.m_pname.Value,
                                                      command.m_data.Pointer)) {
    command.m_Skip = true;
  }
}

void SkipCall(Command& command) {
  command.m_Skip = true;
  command.m_SkipRecording = true;
}

// The driver is called with the application's handle, not with the command's arguments.
template <typename CommandT>
void RecordExportKey(CommandT& command, const char* commandName) {
  const uint64_t exportKey = stream::ExternalHandleService::Get().GetRecordedExportKey(
      reinterpret_cast<void*>(command.m_handle.Value), command.m_Key);
  if (exportKey == 0) {
    LOG_ERROR << commandName << " imports a handle that no recorded command exported, so the "
              << "stream cannot replay the import. Only handles exported by Vulkan are supported.";
  }
  command.m_handle.Value = static_cast<uintptr_t>(exportKey);
}

} // namespace

void CaptureCustomizationLayer::Pre(glGetBooleanvCommand& command) {
  AnswerStateQuery(command);
}

void CaptureCustomizationLayer::Pre(glGetIntegervCommand& command) {
  AnswerStateQuery(command);
}

void CaptureCustomizationLayer::Pre(glGetInteger64vCommand& command) {
  AnswerStateQuery(command);
}

void CaptureCustomizationLayer::Pre(glGetFloatvCommand& command) {
  AnswerStateQuery(command);
}

void CaptureCustomizationLayer::Pre(glGetDoublevCommand& command) {
  AnswerStateQuery(command);
}

void CaptureCustomizationLayer::Pre(glGetStringiCommand& command) {
  if (command.m_name.Value != GL_EXTENSIONS) {
    return;
  }
  const auto driverIndex =
      CapabilitySuppressionService::Get().DriverExtensionIndex(command.m_index.Value);
  if (!driverIndex) {
    return;
  }
  command.m_Skip = true;
  command.m_Return.Value =
      g_DispatchTable.glGetStringi.load(std::memory_order_acquire)(GL_EXTENSIONS, *driverIndex);
}

void CaptureCustomizationLayer::Post(glGetStringCommand& command) {
  if (command.m_name.Value == GL_EXTENSIONS) {
    command.m_Return.Value =
        reinterpret_cast<const GLubyte*>(CapabilitySuppressionService::Get().HideExtensions(
            reinterpret_cast<const char*>(command.m_Return.Value)));
  }
}

void CaptureCustomizationLayer::Post(wglGetExtensionsStringARBCommand& command) {
  command.m_Return.Value =
      CapabilitySuppressionService::Get().HideExtensions(command.m_Return.Value);
}

void CaptureCustomizationLayer::Post(wglGetExtensionsStringEXTCommand& command) {
  command.m_Return.Value =
      CapabilitySuppressionService::Get().HideExtensions(command.m_Return.Value);
}

void CaptureCustomizationLayer::Pre(glProgramBinaryCommand& command) {
  if (CapabilitySuppressionService::Get().SkipsProgramBinary(command.m_binaryFormat.Value)) {
    SkipCall(command);
  }
}

void CaptureCustomizationLayer::Pre(glProgramBinaryOESCommand& command) {
  if (CapabilitySuppressionService::Get().SkipsProgramBinary(command.m_binaryFormat.Value)) {
    SkipCall(command);
  }
}

void CaptureCustomizationLayer::Pre(glShaderBinaryCommand& command) {
  if (CapabilitySuppressionService::Get().SkipsShaderBinary(command.m_binaryFormat.Value)) {
    SkipCall(command);
  }
}

void CaptureCustomizationLayer::Pre(glImportMemoryWin32HandleEXTCommand& command) {
  RecordExportKey(command, "glImportMemoryWin32HandleEXT");
}

void CaptureCustomizationLayer::Pre(glImportSemaphoreWin32HandleEXTCommand& command) {
  RecordExportKey(command, "glImportSemaphoreWin32HandleEXT");
}

void CaptureCustomizationLayer::Post(wglCreateContextAttribsARBCommand& command) {
  if (command.m_Return.Value == nullptr) {
    return;
  }
  const auto& attributes = command.m_attribList.Data;
  for (size_t index = 0; index + 1 < attributes.size(); index += 2) {
    if (attributes[index] == WGL_CONTEXT_PROFILE_MASK_ARB) {
      GITS_ASSERT((attributes[index + 1] & WGL_CONTEXT_ES_PROFILE_BIT_EXT) == 0,
                  "OpenGL ES contexts are not supported by the recorder");
    }
  }
}

} // namespace gits::opengl
