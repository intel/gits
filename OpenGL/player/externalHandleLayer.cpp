// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "externalHandleLayer.h"

#include "externalHandleService.h"
#include "playerManager.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace gits::opengl {

namespace {

thread_local uintptr_t tl_ExportKey{};

template <typename CommandT>
void ImportReplayedExport(CommandT& command, const char* commandName) {
  const uintptr_t exportKey = command.m_handle.Value;
  tl_ExportKey = exportKey;
  if (!PlayerManager::Get().ExecuteCommands() || command.m_Skip) {
    return;
  }
  void* handle = stream::ExternalHandleService::Get().GetReplayedExport(exportKey);
  if (handle == nullptr) {
    throw std::runtime_error(std::string(commandName) + " (key " + std::to_string(command.m_Key) +
                             ") imports a handle of command " + std::to_string(exportKey) +
                             ", which did not export one in this replay");
  }
  command.m_handle.Value = reinterpret_cast<uintptr_t>(handle);
}

} // namespace

void ExternalHandleLayer::Pre(glImportMemoryWin32HandleEXTCommand& command) {
  ImportReplayedExport(command, "glImportMemoryWin32HandleEXT");
}

void ExternalHandleLayer::Post(glImportMemoryWin32HandleEXTCommand& command) {
  command.m_handle.Value = tl_ExportKey;
}

void ExternalHandleLayer::Pre(glImportSemaphoreWin32HandleEXTCommand& command) {
  ImportReplayedExport(command, "glImportSemaphoreWin32HandleEXT");
}

void ExternalHandleLayer::Post(glImportSemaphoreWin32HandleEXTCommand& command) {
  command.m_handle.Value = tl_ExportKey;
}

} // namespace gits::opengl
