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
    : Layer("HelloHUD"), m_Cfg(cfg), m_Gits(gits) {
  auto* hud = m_Gits->GetImGuiHUD();

  if (!hud) {
    LOG_ERROR << "ImGuiHUD not set, cannot create HelloHUDLayer";
    return;
  }

  m_Token = hud->AddHUDPlugin(this, true);
}

HelloHUDLayer::~HelloHUDLayer() {
  auto* hud = m_Gits->GetImGuiHUD();
  if (hud && m_Token != -1) {
    hud->RemoveHUDPlugin(m_Token);
  }
}

const std::vector<ImGuiWidget>* HelloHUDLayer::HUDCallback() {
  m_Widgets.clear();

  m_Widgets.push_back(ImGuiWidget_Text{m_Cfg.Text});

  return &m_Widgets;
}

} // namespace DirectX
} // namespace gits
