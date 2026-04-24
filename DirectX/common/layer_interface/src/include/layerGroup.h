// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

namespace gits {
namespace DirectX {

class LayerGroup {
public:
  virtual ~LayerGroup() = default;
  LayerGroup(const LayerGroup&) = delete;
  LayerGroup& operator=(const LayerGroup&) = delete;

  virtual void LoadLayers() = 0;

  Layer* GetLayer(const std::string& name) {
    auto it = m_Layers.find(name);
    if (it == m_Layers.end()) {
      return nullptr;
    }
    return it->second.get();
  }

protected:
  LayerGroup() = default;

  void AddLayer(std::unique_ptr<Layer> layer) {
    if (!layer) {
      return;
    }
    const std::string& key = layer->GetName();
    assert(m_Layers.find(key) == m_Layers.end());
    m_Layers[key] = std::move(layer);
  }

private:
  std::unordered_map<std::string, std::unique_ptr<Layer>> m_Layers;
};

} // namespace DirectX
} // namespace gits
