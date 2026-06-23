// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "eventBus.h"
#include "textEditorWidget.h"
#include "metaDataPanel.h"
#include "pluginsPanel.h"
#include "captureOptionsPanel.h"
#include "playbackOptionsPanel.h"
#include "subcaptureOptionsPanel.h"
#include "yamlTreeViewer.h"

namespace gits::gui {

class YAMLPanel : public BasePanel {

  enum class YAMLEditorConfig {
    NO_CONFIG = -1,
    FULL_CONFIG,
    BASE_CONFIG
  };

public:
  YAMLPanel();
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  void Render() override;

private:
  void UpdateContent();
  bool UpdateInMemoryConfig(std::filesystem::path filePath);
  void SaveInMemoryConfigToFile(std::filesystem::path filePath);

  gits::ImGuiHelper::TextEditorWidget m_ConfigEditor;

  YAMLEditorConfig m_ActiveConfig = YAMLEditorConfig::NO_CONFIG;
  gits::ImGuiHelper::YamlTreeViewer m_YamlTreeViewer;
  bool m_ShowOnlyDelta = true;

  void ModeChangedCallback(const Event& e);
  void InMemoryConfigChangedCallback(const Event& e);
  void ConfigFileChangedCallback(const Event& e);
  void ThemeChangedCallback(const Event& e);
};

} // namespace gits::gui
