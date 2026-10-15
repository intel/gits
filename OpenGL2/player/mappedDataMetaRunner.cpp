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
#include "log.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"

#include <cstring>

namespace gits::opengl2 {

void MappedDataMetaRunner::Run() {
  // Fills command.m_Buffer.Value from the decoded .Key via the pre-existing key->live-GLuint
  // direction (player/include/handleArgumentUpdaters.h) - no service changes needed, per the
  // plan doc's Decision 6. A never-registered key logs its own warning inside UpdateHandle/
  // ObjectNameMapService::GetHandle and leaves Value at 0.
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
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

} // namespace gits::opengl2
