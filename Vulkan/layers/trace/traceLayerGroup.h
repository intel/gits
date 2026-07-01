// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <memory>
#include <mutex>

namespace gits {

class FastOStream;
class FastOStringStream;

namespace vulkan {

/*
 * Encapsulates creation logic of tracing-related Layers.
 * Assembles file paths, creates output streams and decides which layers to create.
 */
class TraceLayerGroup {
public:
  TraceLayerGroup();
  ~TraceLayerGroup();

  TraceLayerGroup(const TraceLayerGroup&) = delete;
  TraceLayerGroup& operator=(const TraceLayerGroup&) = delete;

  std::unique_ptr<Layer> GetTraceLayer() {
    return std::move(m_TraceLayer);
  }

private:
  std::unique_ptr<FastOStream> m_TraceStream;
  std::unique_ptr<FastOStream> m_TraceStreamPre;
  std::unique_ptr<Layer> m_TraceLayer;
  std::mutex m_TraceMutex;
};

} // namespace vulkan
} // namespace gits
