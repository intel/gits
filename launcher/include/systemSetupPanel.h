// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

#include "basePanel.h"

namespace gits::gui {

class SystemSetupPanel : public BasePanel {
public:
  SystemSetupPanel();
  using BasePanel::BasePanel;

  void Render() override;

private:
  void RenderRegistry();
  void RenderRegistryEntries();
  void RenderRegistryButtons();
  void RenderClearShaderCacheButton();

  bool m_ShowClearShaderCacheConfirm = false;
  std::string m_ShaderCachePath;
};

} // namespace gits::gui
