// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"

#include <memory>
#include <mutex>

namespace gits {

class FastOStream;
class FastOStringStream;

namespace DirectX {

class TraceLayerGroup : public LayerGroup {
public:
  TraceLayerGroup();
  ~TraceLayerGroup() override;

  void loadLayers() override;

private:
  std::unique_ptr<FastOStream> traceStream_;
  std::unique_ptr<FastOStream> traceStreamPre_;
  std::unique_ptr<FastOStringStream> showExecutionStream_;
  std::mutex traceMutex_;
};

} // namespace DirectX
} // namespace gits
