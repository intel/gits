// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <memory>

namespace gits {

class TraceFiles;

namespace opengl {

class TraceLayer;

class TraceLayerGroup {
public:
  TraceLayerGroup();
  ~TraceLayerGroup();

  // The layer created by CreateLayer() is registered as a Pre layer only if TracesPreCalls() and
  // as a Post layer only if TracesPostCalls().
  bool TracesPreCalls() const;
  bool TracesPostCalls() const;
  std::unique_ptr<TraceLayer> CreateLayer();

private:
  std::shared_ptr<TraceFiles> m_Files;
  bool m_Flush{};
};

} // namespace opengl
} // namespace gits
