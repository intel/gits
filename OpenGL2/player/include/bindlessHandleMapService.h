// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <utility>

namespace gits::opengl2 {

// Bindless texture and image handles are values the driver assigns, so a captured handle is
// replaced by the one the replay driver returned for the same texture, sampler or image. A handle
// the replay has not created is used as captured. Handles in buffer contents are not replaced.
// Independent share groups can be given the same handle for different objects, so handles are
// mapped per share group of the current context (ShareGroupService).
class BindlessHandleMapService {
public:
  static BindlessHandleMapService& Get();

  // A captured handle of 0 is a failed call and is not registered.
  void Register(GLuint64 capturedHandle, GLuint64 replayHandle);
  GLuint64 Resolve(GLuint64 capturedHandle) const;

private:
  BindlessHandleMapService() = default;

  mutable std::mutex m_Mutex;
  std::map<std::pair<uint64_t, GLuint64>, GLuint64> m_Handles;
};

} // namespace gits::opengl2
