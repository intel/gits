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

  virtual void loadLayers() = 0;

  Layer* getLayer(const std::string& name) {
    auto it = layers_.find(name);
    if (it == layers_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

protected:
  LayerGroup() = default;

  void addLayer(std::unique_ptr<Layer> layer) {
    if (!layer) {
      return;
    }
    const std::string& key = layer->GetName();
    assert(layers_.find(key) == layers_.end());
    layers_[key] = std::move(layer);
  }

private:
  std::unordered_map<std::string, std::unique_ptr<Layer>> layers_;
};

} // namespace DirectX
} // namespace gits
