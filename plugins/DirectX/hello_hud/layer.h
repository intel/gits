// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include "gits.h"
#include "imGuiHUD.h"

#include <string>

namespace gits {
namespace DirectX {

struct HelloHUDConfig {
  std::string Text;
};

class HelloHUDLayer : public Layer, public IHUDPlugin {
public:
  HelloHUDLayer(const HelloHUDConfig& cfg, CGits* gits);
  ~HelloHUDLayer();

  const std::vector<ImGuiWidget>* HUDCallback() override;

private:
  std::vector<ImGuiWidget> widgets_;
  HelloHUDConfig cfg_;
  CGits* gits_;
  int token_;
};

} // namespace DirectX
} // namespace gits
