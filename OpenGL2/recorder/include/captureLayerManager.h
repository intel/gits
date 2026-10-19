// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <memory>
#include <vector>

namespace gits::opengl2 {

// Forward-declared, not #include "layerAuto.h": layerAuto.h pulls in glTypes.h, which conflicts
// with <windows.h>, and this header reaches recorder/unsupportedCommand.cpp, which needs
// <windows.h>, through captureManager.h.
class Layer;

// Builds and owns the recorder's Pre and Post layers, with EncoderLayer last in Post order.
class CaptureLayerManager {
public:
  // Defined in the .cpp file, as Layer is forward-declared and owned by unique_ptr.
  CaptureLayerManager();
  ~CaptureLayerManager();
  CaptureLayerManager(const CaptureLayerManager&) = delete;
  CaptureLayerManager& operator=(const CaptureLayerManager&) = delete;

  void LoadLayers();

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  std::vector<std::unique_ptr<Layer>> m_LayersOwner;
  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
};

} // namespace gits::opengl2
