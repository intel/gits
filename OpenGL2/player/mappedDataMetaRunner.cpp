// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mappedDataMetaRunner.h"

#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "layerAuto.h"
#include "log.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "playerManager.h"

#include <cstring>

namespace gits::opengl2 {

namespace {
void ReplayMappedData(MappedDataMetaCommand& command) {
  const GLuint buffer = command.m_Buffer.Value;
  if (buffer == 0) {
    LOG_WARNING << "MappedDataMetaRunner: buffer key " << command.m_Buffer.Key
                << " did not resolve to a live buffer - skipping this buffer-map update";
    return;
  }

  // Temporarily bind the resolved buffer to GL_COPY_WRITE_BUFFER (no rendering-pipeline
  // meaning of its own) rather than whichever target originally recorded this map, so this
  // update never perturbs a binding point a later replayed command might depend on - see
  // Decision 6's Consequences. Save/restore whatever was previously bound there.
  GLint previousBinding = 0;
  auto realGetIntegerv = player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_COPY_WRITE_BUFFER_BINDING, &previousBinding);

  auto realBindBuffer = player::g_DispatchTable.glBindBuffer.load(std::memory_order_acquire);
  realBindBuffer(GL_COPY_WRITE_BUFFER, buffer);

  auto realMapBufferRange =
      player::g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* pointer =
      realMapBufferRange(GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(command.m_Offset.Value),
                         static_cast<GLsizeiptr>(command.m_Length.Value), GL_MAP_WRITE_BIT);
  if (pointer != nullptr) {
    std::memcpy(pointer, command.m_Data.Data.data(), command.m_Data.Data.size());
    auto realUnmapBuffer = player::g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
    realUnmapBuffer(GL_COPY_WRITE_BUFFER);
  } else {
    LOG_WARNING << "MappedDataMetaRunner: glMapBufferRange failed for buffer " << buffer
                << " - skipping this buffer-map update";
  }

  realBindBuffer(GL_COPY_WRITE_BUFFER, static_cast<GLuint>(previousBinding));
}
} // namespace

void MappedDataMetaRunner::Run() {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    ReplayMappedData(command);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
