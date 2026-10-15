// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mappedDataMetaRunner.h"

#include "handleArgumentUpdaters.h"
#include "log.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"

#include <cstring>

namespace gits::opengl2 {

namespace {

// See mapTrackingWrappers.cpp's identical constants (recorder side) - kept as a separate,
// smaller copy here rather than shared, since the player only ever needs one target (see
// below), not the full binding-point table Decision 4's recorder-side resolution needs.
// Values confirmed against third_party/OpenGL-Registry/xml/gl.xml.
constexpr GLenum kCopyWriteBuffer = 0x8F37;
constexpr GLenum kCopyWriteBufferBinding = 0x8F37; // Same value as the target itself - a real
                                                   // gl.xml quirk, not a typo (see
                                                   // mapTrackingWrappers.cpp's identical note).
constexpr GLbitfield kMapWriteBit = 0x0002;

} // namespace

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
  realGetIntegerv(kCopyWriteBufferBinding, &previousBinding);

  auto realBindBuffer = player::g_DispatchTable.glBindBuffer.load(std::memory_order_acquire);
  realBindBuffer(kCopyWriteBuffer, buffer);

  auto realMapBufferRange =
      player::g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* pointer =
      realMapBufferRange(kCopyWriteBuffer, static_cast<GLintptr>(command.m_Offset.Value),
                         static_cast<GLsizeiptr>(command.m_Length.Value), kMapWriteBit);
  if (pointer != nullptr) {
    std::memcpy(pointer, command.m_Data.Data.data(), command.m_Data.Data.size());
    auto realUnmapBuffer = player::g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
    realUnmapBuffer(kCopyWriteBuffer);
  } else {
    LOG_WARNING << "MappedDataMetaRunner: glMapBufferRange failed for buffer " << buffer
                << " - skipping this buffer-map update";
  }

  realBindBuffer(kCopyWriteBuffer, static_cast<GLuint>(previousBinding));
}

} // namespace gits::opengl2
