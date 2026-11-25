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
#include "mapTrackingService.h"
#include "glDispatchAuto.h"

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

void RefreshAfterSignaledWait(GLenum result) {
  if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
    MapTrackingService::Get().RefreshGpuWrittenMaps();
  }
}

// A query result read into client memory waits until the query completes, and the reported
// availability tells that it did. Written into a query buffer, it does neither.
template <typename CommandT>
void RefreshAfterQueryResult(CommandT& command) {
  const auto& values = command.m_params.Data;
  if (values.empty()) {
    return;
  }
  const GLenum pname = command.m_pname.Value;
  if (pname == GL_QUERY_RESULT || (pname == GL_QUERY_RESULT_AVAILABLE && values[0] != 0)) {
    MapTrackingService::Get().RefreshGpuWrittenMaps();
  }
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

void CaptureCustomizationLayer::Post(glClientWaitSyncCommand& command) {
  RefreshAfterSignaledWait(command.m_Return.Value);
}

void CaptureCustomizationLayer::Post(glClientWaitSyncAPPLECommand& command) {
  RefreshAfterSignaledWait(command.m_Return.Value);
}

void CaptureCustomizationLayer::Post(glGetSyncivCommand& command) {
  // The generated wrapper does not copy `values`, whose length is only the caller's capacity; the
  // driver writes the first element. The player waits for a sync object the application saw
  // signaled, since the application may then overwrite memory the GPU read through an
  // unsynchronized map.
  const GLint* values = command.m_values.Pointer;
  if (command.m_count.Value <= 0 || values == nullptr) {
    return;
  }
  command.m_values.Data.assign(values, values + 1);
  command.m_values.HasValue = true;
  if (command.m_pname.Value == GL_SYNC_STATUS && values[0] == GL_SIGNALED) {
    MapTrackingService::Get().RefreshGpuWrittenMaps();
  }
}

void CaptureCustomizationLayer::Post(glFinishCommand&) {
  MapTrackingService::Get().RefreshGpuWrittenMaps();
}

void CaptureCustomizationLayer::Post(glGetQueryObjectuivCommand& command) {
  RefreshAfterQueryResult(command);
}

void CaptureCustomizationLayer::Post(glGetQueryObjectuivARBCommand& command) {
  RefreshAfterQueryResult(command);
}

void CaptureCustomizationLayer::Post(glGetQueryObjecti64vCommand& command) {
  RefreshAfterQueryResult(command);
}

void CaptureCustomizationLayer::Post(glGetQueryObjecti64vEXTCommand& command) {
  RefreshAfterQueryResult(command);
}

void CaptureCustomizationLayer::Post(glGetQueryObjectui64vCommand& command) {
  RefreshAfterQueryResult(command);
}

void CaptureCustomizationLayer::Post(glGetQueryObjectui64vEXTCommand& command) {
  RefreshAfterQueryResult(command);
}

} // namespace gits::opengl
