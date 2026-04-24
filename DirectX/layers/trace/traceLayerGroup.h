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

  void LoadLayers() override;

private:
  std::unique_ptr<FastOStream> m_TraceStream;
  std::unique_ptr<FastOStream> m_TraceStreamPre;
  std::unique_ptr<FastOStringStream> m_ShowExecutionStream;
  std::mutex m_TraceMutex;
};

} // namespace DirectX
} // namespace gits
