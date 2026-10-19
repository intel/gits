// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "tracePanel.h"

#include "imGuiHelper.h"

#include "resource.h"
#include "studioContext.h"
#include "studioActions.h"
#include "fileActions.h"
#include "studioLabels.h"
#include "baseEventBus.h"
#include "guiActions.h"

#include "mainWindow.h"
#include "contextHelper.h"

#include "configurationYAMLAuto.h"

namespace {
using namespace gits::gui;

using Rule = gits::ImGuiHelper::TextStructureRule;
using MatchMode = gits::ImGuiHelper::TextMatchMode;
const auto TraceFile_Rules = {
    Rule{2, {"Frame # Frame Dispatch #"}, "Dispatch", MatchMode::NumberedWildcard},
    Rule{2, {"Frame # Frame Draw #"}, "Draw", MatchMode::NumberedWildcard},
    Rule{1, {"Frame # end"}, "Frame", MatchMode::NumberedWildcard, true},
};

float WidthLastButton() {
  auto labels = {Labels::CHOOSE_STREAM, Labels::CLEAR_ARGUMENTS, Labels::CHOOSE_CONFIG};

  float result = 0;
  for (const auto& label : labels) {
    auto width = gits::ImGuiHelper::WidthOf(gits::ImGuiHelper::Widgets::Button, label);
    if (width > result) {
      result = width;
    }
  }
  return result;
}
} // namespace

namespace gits::gui {
TracePanel::TracePanel() : BasePanel(), m_TraceEditor("TraceEditor"), m_TextTreeViewer() {
  m_TraceEditor.GetEditor().SetShowWhitespaces(false);
  m_TraceEditor.GetEditor().SetTabSize(4);
  m_TraceEditor.GetEditor().SetColorizerEnable(false);
  m_TraceEditor.SetConfig(TextEditorWidget::CONFIG_NO_TOOLBAR);

  m_TextTreeViewer.SetTextEditor(&m_TraceEditor.GetEditor());

  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&TracePanel::ActionCallback, this, std::placeholders::_1));
}

void TracePanel::Render() {
  if (!m_DidLoadTrace) {
    LoadTraceFile();
  }
  auto& context = StudioContext::GetInstance();

  ImVec2 available = ImGui::GetContentRegionAvail();
  const auto col1Ratio = std::min(256.0f / available.x, 0.25f);
  if (ImGui::BeginChild("TracePanel", ImVec2(available.x, available.y), true)) {
    const float contentHeight = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("TracePanelTable", 2,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("TreeViewColumn", ImGuiTableColumnFlags_WidthStretch, col1Ratio);
      ImGui::TableSetupColumn("EditorColumn", ImGuiTableColumnFlags_WidthStretch, 1.0f - col1Ratio);
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      if (ImGui::BeginChild("TraceTree", ImVec2(0.0f, contentHeight), false)) {
        m_TextTreeViewer.Render();
      }
      ImGui::EndChild();
      ImGui::TableSetColumnIndex(1);
      if (!context.traceFilePath.empty()) {
        m_TraceEditor.Render();
      } else {
        ImGui::Text(Labels::NO_TRACEFILE);
      }
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();
}

void TracePanel::LoadTraceFile() {
  auto& context = StudioContext::GetInstance();
  if (!context.traceFilePath.empty()) {
    if (m_TraceEditor.LoadFromFile(context.traceFilePath)) {
      m_TextTreeViewer.LoadFile(context.traceFilePath, ::TraceFile_Rules);
    } else {
      auto msg = "Could not load trace file: " + context.traceFilePath.string();
      LOG_ERROR << msg;
      m_TraceEditor.AppendText(msg);
    }
  }
  m_DidLoadTrace = true;
}

void TracePanel::ActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);
  auto& context = StudioContext::GetInstance();
  if (actionEvent.EventType == ActionEvent::Type::StudioAnalysisFinished) {
    if (context.Analysis.Trace && !context.traceFilePath.empty()) {
      m_DidLoadTrace = false;
    }
  }
}
} // namespace gits::gui
