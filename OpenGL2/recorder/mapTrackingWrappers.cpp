// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingWrappers.h"

#include "mapTrackingService.h"
#include "openglDriversAuto.h"
#include "recursionGuard.h"

#include <cstring>

namespace gits::opengl2 {

namespace {

void* APIENTRY CustomWrapper_glMapBufferRange(GLenum target,
                                              GLintptr offset,
                                              GLsizeiptr length,
                                              GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* result = realFunction(target, offset, length, access);
  if (record && result != nullptr) {
    MapTrackingService::Get().OnMapBufferRange(target, offset, length, access, result);
  }
  return result;
}

GLboolean APIENTRY CustomWrapper_glUnmapBuffer(GLenum target) {
  const RecursionGuard guard;
  if (guard.IsNested()) {
    auto realFunction = g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
    return realFunction(target);
  }
  // The service makes the real call itself: it has to snapshot the mapped range either side of
  // that call, depending on the mapping's access flags.
  return MapTrackingService::Get().UnmapBufferAndCapture(target);
}

} // namespace

void* GetCustomRecorderWrapper(const char* name) {
  if (std::strcmp(name, "glMapBufferRange") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapBufferRange);
  }
  if (std::strcmp(name, "glUnmapBuffer") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glUnmapBuffer);
  }
  return nullptr;
}

} // namespace gits::opengl2
