// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"
#include "gits.h"

namespace gits {
namespace DirectX {

HelloHUDLayer::HelloHUDLayer(const HelloHUDConfig& cfg, CGits* gits)
    : Layer("HelloHUD"), cfg_(cfg), gits_(gits) {
  auto* hud = gits_->GetImGuiHUD();

  if (!hud) {
    LOG_ERROR << "ImGuiHUD not set, cannot create HelloHUDLayer";
    return;
  }

  token_ = hud->AddHUDPlugin(this, true);
}

HelloHUDLayer::~HelloHUDLayer() {
  auto* hud = gits_->GetImGuiHUD();
  if (hud) {
    hud->RemoveHUDPlugin(token_);
  }
}

const std::vector<ImGuiWidget>* HelloHUDLayer::HUDCallback() {
  widgets_.clear();

  widgets_.push_back(ImGuiWidget_Text{cfg_.Text});

  return &widgets_;
}

} // namespace DirectX
} // namespace gits
