// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace vulkan {

struct HelloPluginConfig {
  bool PrintFrames = false;
  bool PrintQueueSubmits = false;
};

class HelloPluginLayer : public Layer {
public:
  HelloPluginLayer(const HelloPluginConfig& cfg);
  ~HelloPluginLayer() = default;

  void Post(vkQueuePresentKHRCommand& command) override;
  void Post(vkQueueSubmitCommand& command) override;

private:
  void NewFrame();

  HelloPluginConfig m_Cfg;
};

} // namespace vulkan
} // namespace gits
