// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bindlessHandleMapService.h"
#include "handleArgumentUpdaters.h"
#include "shareGroupService.h"

namespace gits::opengl2 {
namespace {

uint64_t CurrentShareGroup() {
  return ShareGroupService::Get().GroupFor(CurrentContext());
}

} // namespace

BindlessHandleMapService& BindlessHandleMapService::Get() {
  static BindlessHandleMapService instance;
  return instance;
}

void BindlessHandleMapService::Register(GLuint64 capturedHandle, GLuint64 replayHandle) {
  if (capturedHandle == 0) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Handles[{shareGroup, capturedHandle}] = replayHandle;
}

GLuint64 BindlessHandleMapService::Resolve(GLuint64 capturedHandle) const {
  const uint64_t shareGroup = CurrentShareGroup();
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto handle = m_Handles.find({shareGroup, capturedHandle});
  return handle != m_Handles.end() ? handle->second : capturedHandle;
}

} // namespace gits::opengl2
