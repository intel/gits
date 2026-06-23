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
#include "yamlTreeViewer.h"
#include "textEditorWidget.h"

#include <string>
#include <mutex>
#include <atomic>

namespace gits::gui {
class MetaDataPanel : public BasePanel {

  enum class SubPanel {
    Unset = -1,
    Config,
    Diags,
    Stats
  };

public:
  MetaDataPanel();
  void Render() override;

private:
  void ClearStats();
  void AppendStats(std::string str);
  void FillStatsEditor();

  void UpdateContent();
  // Event callbacks
  void StreamPathCallback(const Event& e);
  void ThemeChangedCallback(const Event& e);

  gits::ImGuiHelper::TextEditorWidget m_MetaDataStatsEditor;
  gits::ImGuiHelper::TextEditorWidget m_MetaDataEditor;
  gits::ImGuiHelper::YamlTreeViewer m_YamlTreeViewer;

  SubPanel m_ActiveSubPanel = SubPanel::Unset;
  std::string m_StatsStr = "";
  std::mutex m_StatsMutex;
  std::atomic<bool> m_StatsGatheringInProgress = false;
};
} // namespace gits::gui
