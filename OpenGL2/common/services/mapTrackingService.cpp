// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"

namespace gits::opengl2 {

MapTrackingService& MapTrackingService::Get() {
  // Meyer's singleton - same rationale as ContextHandleMapService::Get()/
  // ObjectNameMapService::Get(): no MessageBus/OrderingRecorder ownership here, so none of
  // CaptureManager's static-destruction-order hazards apply.
  static MapTrackingService instance;
  return instance;
}

void MapTrackingService::StoreData(uint64_t shareGroup, GLuint buffer, const MappedRange& range) {
  if (buffer == 0) {
    return;
  }
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_MappedBuffers[key] = range;
}

std::optional<MappedRange> MapTrackingService::TakeData(uint64_t shareGroup, GLuint buffer) {
  if (buffer == 0) {
    return std::nullopt;
  }
  const Key key{shareGroup, buffer};
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_MappedBuffers.find(key);
  if (it == m_MappedBuffers.end()) {
    return std::nullopt;
  }
  MappedRange range = it->second;
  m_MappedBuffers.erase(it);
  return range;
}

} // namespace gits::opengl2
