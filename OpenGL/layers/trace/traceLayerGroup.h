// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "fastOStream.h"

#include <memory>
#include <mutex>

namespace gits::opengl {

class TraceLayer;

class TraceLayerGroup {
public:
  TraceLayerGroup();
  ~TraceLayerGroup();

  bool IsEnabled() const;
  std::unique_ptr<TraceLayer> CreateLayer();

private:
  std::unique_ptr<FastOStream> m_PreStream;
  std::mutex m_Mutex;
  bool m_Flush{};
};

} // namespace gits::opengl
