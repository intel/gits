// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrShaderPanel.h"
#include "rtsr/toolchain/rtsrShaderToolchain.h"
#include "rtsr/ui/rtsrLabels.h"
#include "baseContext.h"
#include "imGuiHelper.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <sstream>

#include "metaDataActions.h"

namespace gits::gui {

namespace {

using LB = RtsrLabels;

// How many renders a reveal request survives. Two are always needed, one to set
// the scroll target and one for it to take effect; the rest cover the renders
// the nested panes take to settle after the tab switch. Small enough that a row
// which can never be shown stops being chased almost immediately.
constexpr int REVEAL_ATTEMPTS = 8;

// Stage names are the DirectX ones, which is what the module's only client
// speaks. The wire enum aliases the Vulkan spellings onto the same values, so
// these cannot be listed as separate cases.
const char* StageText(rtsr::RTSR_SHADER_STAGE stage) {
  switch (stage) {
  case rtsr::RTSR_SHADER_STAGE_VERTEX:
    return "Vertex";
  case rtsr::RTSR_SHADER_STAGE_TESSELLATION_CONTROL:
    return "Hull";
  case rtsr::RTSR_SHADER_STAGE_TESSELLATION_EVALUATION:
    return "Domain";
  case rtsr::RTSR_SHADER_STAGE_GEOMETRY:
    return "Geometry";
  case rtsr::RTSR_SHADER_STAGE_FRAGMENT:
    return "Pixel";
  case rtsr::RTSR_SHADER_STAGE_COMPUTE:
    return "Compute";
  case rtsr::RTSR_SHADER_STAGE_RAY_TRACING_RAYGEN:
    return "Ray generation";
  case rtsr::RTSR_SHADER_STAGE_RAY_TRACING_ANY_HIT:
    return "Any hit";
  case rtsr::RTSR_SHADER_STAGE_RAY_TRACING_CLOSEST_HIT:
    return "Closest hit";
  case rtsr::RTSR_SHADER_STAGE_RAY_TRACING_MISS:
    return "Miss";
  case rtsr::RTSR_SHADER_STAGE_RAY_TRACING_INTERSECTION:
    return "Intersection";
  default:
    return LB::UNKNOWN;
  }
}

const char* PipelineTypeText(rtsr::RTSR_PIPELINE_TYPE type) {
  switch (type) {
  case rtsr::RTSR_PIPELINE_TYPE_GRAPHICS:
    return "Graphics";
  case rtsr::RTSR_PIPELINE_TYPE_COMPUTE:
    return "Compute";
  case rtsr::RTSR_PIPELINE_TYPE_RAY_TRACING:
    return "Ray tracing";
  default:
    return LB::UNKNOWN;
  }
}

const char* BytecodeTypeText(rtsr::RTSR_BYTECODE_TYPE type) {
  switch (type) {
  case rtsr::RTSR_BYTECODE_TYPE_SPIRV:
    return "SPIR-V";
  case rtsr::RTSR_BYTECODE_TYPE_DXBC:
    return "DXBC";
  case rtsr::RTSR_BYTECODE_TYPE_DXIL:
    return "DXIL";
  default:
    return LB::UNKNOWN;
  }
}

const char* ApiText(rtsr::RTSR_API api) {
  switch (api) {
  case rtsr::RTSR_API_VULKAN_1_0:
    return "Vulkan 1.0";
  case rtsr::RTSR_API_VULKAN_1_1:
    return "Vulkan 1.1";
  case rtsr::RTSR_API_VULKAN_1_2:
    return "Vulkan 1.2";
  case rtsr::RTSR_API_VULKAN_1_3:
    return "Vulkan 1.3";
  case rtsr::RTSR_API_DX12:
    return "DirectX 12";
  default:
    return LB::UNKNOWN;
  }
}

// Uppercase, unpadded and without a prefix, which is how RTSREditor prints the
// same hashes. Keeping the two identical is what makes cross-checking a session
// against the old tool possible.
std::string HashText(rtsr::RTSR_HASH hash) {
  std::ostringstream oss;
  oss << std::uppercase << std::hex << hash;
  return oss.str();
}

// The client sends the shader model as major * 10 + minor, or zero when it could
// not tell.
std::string ShaderModelText(size_t version) {
  if (version == 0) {
    return LB::NOT_AVAILABLE;
  }
  return std::to_string(version / 10) + "." + std::to_string(version % 10);
}

std::string ToLower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

// The client serializes the pipeline state without indentation, which is one
// enormous line. Reformatting is the only way the viewer is of any use; a state
// that does not parse is shown as it arrived rather than hidden.
std::string PrettyJson(const std::string& text) {
  try {
    return nlohmann::json::parse(text).dump(2);
  } catch (const nlohmann::json::exception&) {
    return text;
  }
}

} // namespace

RtsrShaderPanel::RtsrShaderPanel(RtsrModule& module)
    : m_Module(module),
      m_Store(module.Store()),
      m_PipelineViewer("RTSRPipelineViewer"),
      m_YamlTreeViewer(),
      m_SourceViewer("RTSRSourceViewer"),
      m_Log("RTSRLog"),
      m_SourceCache(m_Store) {
  m_YamlTreeViewer.SetTextEditor(&m_PipelineViewer.GetEditor());
  m_PipelineViewer.GetEditor().SetShowWhitespaces(false);
  m_PipelineViewer.GetEditor().SetTabSize(4);
  m_PipelineViewer.GetEditor().SetLanguageDefinition(TextEditorWidget::GetYamlLanguageDefinition());
  m_PipelineViewer.SetConfig(gits::ImGuiHelper::TextEditorWidget::CONFIG_NO_TOOLBAR);
  m_SourceViewer.SetConfig(gits::ImGuiHelper::TextEditorWidget::CONFIG_NO_TOOLBAR);
  m_SourceViewer.GetEditor().SetLanguageDefinition(TextEditor::LanguageDefinition::HLSL());
  m_SourceViewer.GetEditor().SetReadOnly(true);

  auto logConfig = gits::ImGuiHelper::TextEditorWidget::CONFIG_NO_TOOLBAR;
  logConfig.ScrollToBottom = true;
  m_Log.SetConfig(logConfig);

  OnThemeChanged();
}

void RtsrShaderPanel::OnThemeChanged() {
  m_PipelineViewer.UpdatePalette();
  m_SourceViewer.UpdatePalette();
  //m_Log.UpdatePalette();
}

// **********************************************************************************************
// * Session state, driven by events                                                            *
// **********************************************************************************************

void RtsrShaderPanel::OnRtsrEvent(const RtsrEvent& event) {
  switch (event.EventType) {
  case RtsrEvent::Type::SessionOpened:
    ClearSession();
    m_State = SessionState::Open;
    m_ApplicationName = event.ApplicationName;
    m_ApplicationPath = event.ApplicationPath;
    m_UsedApi = event.UsedApi;
    break;

  case RtsrEvent::Type::ShaderSourceReady:
    // Notification only; the visible stage reads the current cache in RenderShader.
    break;

  case RtsrEvent::Type::ReplacementUpdated:
    // Notification only; the visible stage reads its operation from the
    // replacement service.
    break;

  case RtsrEvent::Type::SessionClosed:
    // Everything stays: which rows there are, which one is selected, and where
    // the list is scrolled. Only the run is over.
    m_State = SessionState::Ended;
    for (auto& entry : m_Drafts) {
      entry.second.Operation.reset();
    }
    m_ResetOperations.clear();
    m_PipelineNotices.clear();
    break;

  case RtsrEvent::Type::PipelineCreated:
    AddOrUpdateRow(event);
    break;

  case RtsrEvent::Type::PipelineEligibilityChanged:
    if (m_Selected == event.PipelineId && m_SelectedRecord) {
      const auto pipeline = m_Store.Pipeline(event.PipelineId);
      if (pipeline && pipeline->Incarnation == m_SelectedRecord->Incarnation) {
        m_SelectedRecord->ReplacementEligible = pipeline->ReplacementEligible;
        m_SelectedRecord->ReplacementDiagnostic = pipeline->ReplacementDiagnostic;
      }
    }
    break;

  case RtsrEvent::Type::PipelineNamed: {
    const auto it = m_RowByPipelineId.find(event.PipelineId);
    if (it != m_RowByPipelineId.end()) {
      m_Pipelines[it->second].Name = event.Name;
      m_VisibleRowsDirty = true;
      if (m_Selected == event.PipelineId) {
        RefreshSelection();
      }
    }
    break;
  }

  case RtsrEvent::Type::PipelineDeleted:
    RemoveRow(event.PipelineId);
    break;

  case RtsrEvent::Type::ShaderCreated:
    // Nothing to keep: shaders are shown per pipeline, read from the store, and
    // the count in the banner comes from there too.
    break;

  case RtsrEvent::Type::TimeStatsUpdated:
    // The performance tab's business.
    break;

  case RtsrEvent::Type::Log:
    LOG_INFO << event.Text;
    break;
  }
}

void RtsrShaderPanel::AddOrUpdateRow(const RtsrEvent& event) {
  const auto it = m_RowByPipelineId.find(event.PipelineId);
  if (it != m_RowByPipelineId.end()) {
    // The client reused an id, so the row describes a different pipeline now.
    ClearPipelineDrafts(event.PipelineId);
    auto& row = m_Pipelines[it->second];
    row.Type = event.PipelineType;
    row.StageCount = event.StageCount;
    if (m_Selected == event.PipelineId) {
      RefreshSelection();
    }
  } else {
    m_RowByPipelineId.emplace(event.PipelineId, m_Pipelines.size());
    m_Pipelines.push_back(
        PipelineRow{event.PipelineId, event.PipelineType, event.StageCount, std::string{}});
  }
  m_VisibleRowsDirty = true;
}

void RtsrShaderPanel::RemoveRow(rtsr::RTSR_ID id) {
  const auto it = m_RowByPipelineId.find(id);
  if (it == m_RowByPipelineId.end()) {
    return;
  }

  const size_t index = it->second;
  m_Pipelines.erase(m_Pipelines.begin() + static_cast<std::ptrdiff_t>(index));
  m_RowByPipelineId.erase(it);

  // Rows stay in creation order, which is the order the user has been watching
  // them appear in, so everything behind the hole shifts down by one.
  for (auto& entry : m_RowByPipelineId) {
    if (entry.second > index) {
      --entry.second;
    }
  }

  m_VisibleRowsDirty = true;
  ClearPipelineDrafts(id);
  if (m_Selected == id) {
    SelectPipeline(std::nullopt);
  }
}

void RtsrShaderPanel::ClearPipelineDrafts(rtsr::RTSR_ID pipelineId) {
  for (auto draft = m_Drafts.begin(); draft != m_Drafts.end();) {
    draft = draft->first.first == pipelineId ? m_Drafts.erase(draft) : std::next(draft);
  }
  m_ResetOperations.erase(pipelineId);
  m_PipelineNotices.erase(pipelineId);
  if (m_EditorKey.has_value() && m_EditorKey->first == pipelineId) {
    m_EditorKey.reset();
    m_DisplayedSource.reset();
    m_SourceViewer.SetText({});
  }
}

void RtsrShaderPanel::ClearSession() {
  m_SourceCache.Clear();
  m_Drafts.clear();
  m_EditorKey.reset();
  m_ResetOperations.clear();
  m_PipelineNotices.clear();
  m_DisplayedSource.reset();
  m_SourceViewer.SetText({});
  m_State = SessionState::Idle;
  m_ApplicationName.clear();
  m_ApplicationPath.clear();
  m_UsedApi = rtsr::RTSR_API_UNKNOWN;

  m_Pipelines.clear();
  m_RowByPipelineId.clear();
  m_VisibleRows.clear();
  m_VisibleRowsDirty = true;
  m_RevealAttemptsLeft = 0;

  SelectPipeline(std::nullopt);
}

void RtsrShaderPanel::RebuildVisibleRows() {
  m_VisibleRowsDirty = false;
  m_VisibleRows.clear();
  m_VisibleRows.reserve(m_Pipelines.size());

  const std::string filter = ToLower(m_Filter);
  for (size_t i = 0; i < m_Pipelines.size(); ++i) {
    if (!m_PipelineTypeFilters[m_Pipelines[i].Type]) {
      continue;
    }
    if (filter.empty() || ToLower(m_Pipelines[i].Name).find(filter) != std::string::npos ||
        std::to_string(m_Pipelines[i].Id).find(filter) != std::string::npos) {
      m_VisibleRows.push_back(i);
    }
  }
}

std::optional<size_t> RtsrShaderPanel::VisiblePositionOf(rtsr::RTSR_ID id) const {
  for (size_t position = 0; position < m_VisibleRows.size(); ++position) {
    if (m_Pipelines[m_VisibleRows[position]].Id == id) {
      return position;
    }
  }
  return std::nullopt;
}

void RtsrShaderPanel::SelectPipeline(std::optional<rtsr::RTSR_ID> id) {
  m_Selected = id;
  RefreshSelection();
}

void RtsrShaderPanel::RefreshSelection() {
  m_SelectedRecord.reset();
  m_SelectedShaders.clear();
  m_PipelineViewer.SetText({});

  if (!m_Selected.has_value()) {
    return;
  }

  m_SelectedRecord = m_Store.Pipeline(*m_Selected);
  if (!m_SelectedRecord.has_value()) {
    // It went away between the click and the read.
    m_Selected.reset();
    return;
  }

  m_SelectedShaders = m_Store.PipelineShaders(*m_Selected);
  const auto& pipelineJson = PrettyJson(m_SelectedRecord->State);
  auto yml = orderedJsonStrToYaml(pipelineJson);
  auto ymlStr = YAML::Dump(yml);
  m_YamlTreeViewer.SetYAMLText(ymlStr);
  m_PipelineViewer.SetText(ymlStr);
}

void RtsrShaderPanel::RevealPipeline(rtsr::RTSR_ID id) {
  const auto it = m_RowByPipelineId.find(id);
  if (it == m_RowByPipelineId.end()) {
    return;
  }
  SelectPipeline(id);

  RebuildVisibleRows();
  if (!VisiblePositionOf(id).has_value()) {
    // The active filters hide the row we were asked to show.
    m_Filter[0] = '\0';
    m_PipelineTypeFilters[m_Pipelines[it->second].Type] = true;
    RebuildVisibleRows();
  }
  m_RevealAttemptsLeft = REVEAL_ATTEMPTS;
}

// **********************************************************************************************
// * Rendering                                                                                  *
// **********************************************************************************************

void RtsrShaderPanel::Render() {
  RenderSession();
  ImGui::Separator();

  // Each pane is a child window so that the tables inside them have a definite
  // height to scroll within.
  ImGui::BeginChild("RtsrBrowser");
  if (ImGui::BeginTable("RtsrPanes", 2,
                        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild("RtsrListPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Border);

    RenderPipelineList();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::TableNextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::BeginChild("RtsrDetailPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Border);
    RenderPipelineDetail();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndTable();
  }
  ImGui::EndChild();
}

void RtsrShaderPanel::RenderSession() {
  if (m_State == SessionState::Idle) {
    ImGui::TextUnformatted(LB::WAITING_FOR_CLIENT);
    return;
  }

  if (m_State == SessionState::Ended) {
    ImGui::TextUnformatted(LB::SESSION_ENDED);
  }

  ImGui::Text("%s: %s", LB::SESSION_APPLICATION, m_ApplicationName.c_str());
  ImGui::Text("%s: %s", LB::SESSION_PATH, m_ApplicationPath.c_str());
  ImGui::Text("%s: %s", LB::SESSION_API, ApiText(m_UsedApi));
  // Read from the store rather than counted here, so this reports everything
  // received even when the tables are a frame behind during a burst.
  ImGui::Text("%s: %zu pipelines, %zu shaders", LB::SESSION_CONTENTS, m_Store.PipelineCount(),
              m_Store.ShaderCount());
}

void RtsrShaderPanel::PipelineToggleButton(const std::string& label,
                                           const std::string& hint,
                                           rtsr::RTSR_PIPELINE_TYPE type) {
  if (m_PipelineTypeFilters[type]) {
    ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::BUTTON_STYLE_SUCCESS);
  } else {
    ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::BUTTON_STYLE_FAILURE);
  }
  if (ImGui::Button(label.c_str())) {
    m_PipelineTypeFilters[type] = !m_PipelineTypeFilters[type];
    m_VisibleRowsDirty = true;
  }
  ImGuiHelper::AddTooltip(hint);
  ImGuiHelper::PopButtonStyle();
}

void RtsrShaderPanel::RenderPipelineList() {
  auto area = ImGui::GetContentRegionAvail();
  auto size = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, RtsrLabels::PIPELINES_FILTER_CMP) +
              ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, RtsrLabels::PIPELINES_FILTER_GFX) +
              ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, RtsrLabels::PIPELINES_FILTER_RAY) +
              ImGui::GetStyle().ItemSpacing.x * 2.0f;
  auto width = area.x - size - ImGui::GetStyle().ItemSpacing.x;
  ImGui::PushItemWidth(width);
  if (ImGui::InputTextWithHint("##RtsrFilter", LB::FILTER_HINT, m_Filter, sizeof(m_Filter))) {
    m_VisibleRowsDirty = true;
  }

  ImGui::SameLine();
  PipelineToggleButton(RtsrLabels::PIPELINES_FILTER_CMP, RtsrLabels::PIPELINES_FILTER_CMP_HINT,
                       rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_COMPUTE);
  ImGui::SameLine();
  PipelineToggleButton(RtsrLabels::PIPELINES_FILTER_GFX, RtsrLabels::PIPELINES_FILTER_GFX_HINT,
                       rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_GRAPHICS);
  ImGui::SameLine();
  PipelineToggleButton(RtsrLabels::PIPELINES_FILTER_RAY, RtsrLabels::PIPELINES_FILTER_RAY_HINT,
                       rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_RAY_TRACING);

  if (m_VisibleRowsDirty) {
    RebuildVisibleRows();
    if (m_Selected.has_value() && !VisiblePositionOf(*m_Selected).has_value()) {
      SelectPipeline(std::nullopt);
    }
  }

  constexpr auto flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                         ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
                         ImGuiTableFlags_Resizable;
  if (!ImGui::BeginTable("RtsrPipelines", 4, flags)) {
    return;
  }

  ImGui::TableSetupScrollFreeze(0, 1);
  ImGui::TableSetupColumn(LB::COLUMN_ID);
  ImGui::TableSetupColumn(LB::COLUMN_NAME);
  ImGui::TableSetupColumn(LB::COLUMN_TYPE);
  ImGui::TableSetupColumn(LB::COLUMN_STAGES);
  ImGui::TableHeadersRow();

  // Which row to reveal, resolved here rather than when it was asked for,
  // because pipelines arriving meanwhile move it.
  std::optional<int> reveal;
  if (m_RevealAttemptsLeft > 0 && m_Selected.has_value()) {
    if (const auto position = VisiblePositionOf(*m_Selected)) {
      reveal = static_cast<int>(*position);
    }
  }
  // Counted down here rather than where the row is submitted, so a row that
  // never gets submitted cannot keep the request alive indefinitely.
  m_RevealAttemptsLeft = reveal.has_value() ? m_RevealAttemptsLeft - 1 : 0;

  // A replay creates thousands of pipelines, so only the rows on screen are
  // built.
  ImGuiListClipper clipper;
  clipper.Begin(static_cast<int>(m_VisibleRows.size()));
  // A row to reveal is almost always outside the range the clipper would build,
  // and a row that is never submitted has no position to scroll to. Forcing it
  // in lets the row ask to be centred itself, which is how ImGui's own list box
  // reveals its selection.
  if (reveal.has_value()) {
    clipper.IncludeItemByIndex(*reveal);
  }
  while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
      const auto& row = m_Pipelines[m_VisibleRows[static_cast<size_t>(i)]];
      const std::string id = std::to_string(row.Id);

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      // The id is unique, so it identifies the row to ImGui as well and the
      // selection survives the list growing underneath it.
      const bool selected = m_Selected == row.Id;
      if (ImGui::Selectable(id.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
        SelectPipeline(row.Id);
      }
      if (reveal == i) {
        // Derived from the row's own rectangle, so the frozen header row and the
        // real row height are accounted for. Halfway down the list, except for a
        // row close enough to an end that the list cannot scroll that far, which
        // ImGui snaps to the edge instead.
        ImGui::SetScrollHereY(0.5f);
        // A scroll only takes effect on the next render, so the row being on
        // screen is what says the target was computed from settled geometry and
        // has since been applied. Until then, keep asking.
        if (ImGui::IsItemVisible()) {
          m_RevealAttemptsLeft = 0;
        }
      }

      ImGui::TableNextColumn();
      ImGui::TextUnformatted(row.Name.empty() ? LB::UNNAMED_PIPELINE : row.Name.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(PipelineTypeText(row.Type));
      ImGui::TableNextColumn();
      ImGui::Text("%zu", row.StageCount);
    }
  }

  ImGui::EndTable();
}

void RtsrShaderPanel::RenderPipelineDetail() {
  if (!m_SelectedRecord.has_value()) {
    ImGui::TextUnformatted(LB::NO_SELECTION);
    return;
  }
  const auto& record = *m_SelectedRecord;

  ImGui::Text("%s: %s", LB::STATE_HASH, HashText(record.StateHash).c_str());

  ImGui::PushID(std::to_string(record.Id).c_str());
  if (ImGui::BeginTabBar("RtsrShaderStages")) {
    for (size_t i = 0; i < m_SelectedShaders.size(); ++i) {
      const auto& shader = m_SelectedShaders[i];
      // Ray tracing and other unsupported stages are outside this viewer's scope.
      if (shader.Stage < rtsr::RTSR_SHADER_STAGE_VERTEX ||
          shader.Stage > rtsr::RTSR_SHADER_STAGE_COMPUTE) {
        continue;
      }
      ImGui::PushID(static_cast<int>(i));
      if (ImGui::BeginTabItem(StageText(shader.Stage))) {
        RenderShader(record, shader);
        ImGui::EndTabItem();
      }
      ImGui::PopID();
    }
    if (ImGui::BeginTabItem(LB::TAB_PIPELINE_STATE)) {
      ImVec2 available = ImGui::GetContentRegionAvail();
      const auto col1Ratio = std::min(256.0f / available.x, 0.25f);
      if (ImGui::BeginTable("MetaDataTable", 2,
                            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("TreeViewColumn", ImGuiTableColumnFlags_WidthStretch, col1Ratio);
        ImGui::TableSetupColumn("EditorColumn", ImGuiTableColumnFlags_WidthStretch,
                                1.0f - col1Ratio);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        m_YamlTreeViewer.Render();
        ImGui::TableSetColumnIndex(1);
        m_PipelineViewer.Render(ImVec2(0.0f, 0.0f));
        ImGui::EndTable();
      }

      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::PopID();
}

void RtsrShaderPanel::ProcessApplyResult(ShaderDraft& draft) {
  if (!draft.Operation.has_value()) {
    return;
  }

  const auto result = m_Module.Replacements().Result(*draft.Operation);
  if (!result.has_value()) {
    draft.Operation.reset();
    draft.Diagnostic = LB::OPERATION_CANCELLED;
    return;
  }
  if (result->State == RtsrReplacementService::OperationState::Pending) {
    return;
  }

  if (result->State == RtsrReplacementService::OperationState::Succeeded) {
    draft.AppliedText = draft.SubmittedText;
    draft.Diagnostic = LB::APPLY_SUCCEEDED;
    if (!result->Diagnostic.empty()) {
      draft.Diagnostic += '\n';
      draft.Diagnostic += result->Diagnostic;
    }
  } else {
    draft.Diagnostic = LB::APPLY_FAILED;
    if (!result->Diagnostic.empty()) {
      draft.Diagnostic += '\n';
      draft.Diagnostic += result->Diagnostic;
    }
  }
  draft.Operation.reset();
}

void RtsrShaderPanel::ProcessResetResult(rtsr::RTSR_ID pipelineId) {
  const auto operation = m_ResetOperations.find(pipelineId);
  if (operation == m_ResetOperations.end()) {
    return;
  }

  const auto result = m_Module.Replacements().Result(operation->second);
  if (!result.has_value()) {
    m_PipelineNotices[pipelineId] = LB::OPERATION_CANCELLED;
    m_ResetOperations.erase(operation);
    return;
  }
  if (result->State == RtsrReplacementService::OperationState::Pending) {
    m_PipelineNotices[pipelineId] = LB::RESET_PENDING;
    return;
  }

  if (result->State == RtsrReplacementService::OperationState::Succeeded) {
    m_PipelineNotices[pipelineId] = LB::RESET_SUCCEEDED;
    // Completed apply results from a tab that was not visible must not later
    // announce an active replacement after this reset has superseded them.
    for (auto& [draftKey, draft] : m_Drafts) {
      if (draftKey.first == pipelineId) {
        draft.Operation.reset();
        draft.Diagnostic.clear();
      }
    }
  } else {
    m_PipelineNotices[pipelineId] = std::string(LB::RESET_FAILED) +
                                    (result->Diagnostic.empty() ? "" : "\n" + result->Diagnostic);
  }
  m_ResetOperations.erase(operation);
}

void RtsrShaderPanel::RenderShader(const rtsr::PipelineRecord& pipeline,
                                   const rtsr::ShaderSummary& shader) {
  ImGui::Text("%s: %s | %s: %s", LB::COLUMN_HASH, HashText(shader.BytecodeHash).c_str(),
              LB::COLUMN_ENTRYPOINT, shader.EntrypointName.c_str());
  ImGui::Text("%s: %s | %s: %s | %s: %zu", LB::COLUMN_BYTECODE,
              BytecodeTypeText(shader.BytecodeType), LB::COLUMN_MODEL,
              ShaderModelText(shader.D3DShaderVersion).c_str(), LB::COLUMN_BYTES,
              shader.BytecodeSize);

  const auto entry = m_SourceCache.Request(shader.BytecodeHash, shader.EntrypointName);
  if (entry.Status == RtsrShaderSourceCache::State::Pending) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::CRITICAL);
    ImGui::TextUnformatted(LB::SOURCE_PENDING);
    ImGui::PopStyleColor();
    return;
  }
  if (entry.Status == RtsrShaderSourceCache::State::Failed) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::FAILURE);
    ImGui::TextUnformatted(LB::SOURCE_FAILED);
    ImGui::TextWrapped("%s", entry.Source->Diagnostic.c_str());
    ImGui::PopStyleColor();
    return;
  }

  const bool hlsl = entry.Source->Kind == rtsr::SourceKind::Hlsl;
  const bool glsl = entry.Source->Kind == rtsr::SourceKind::Glsl;
  const bool vulkan =
      m_UsedApi >= rtsr::RTSR_API_VULKAN_1_0 && m_UsedApi <= rtsr::RTSR_API_VULKAN_1_3;
  const bool canEdit = m_State == SessionState::Open && pipeline.ReplacementEligible &&
                       ((hlsl && shader.BytecodeType == rtsr::RTSR_BYTECODE_TYPE_DXIL &&
                         shader.D3DShaderVersion != 0) ||
                        (glsl && shader.BytecodeType == rtsr::RTSR_BYTECODE_TYPE_SPIRV && vulkan));
  const char* sourceLabel = hlsl   ? (canEdit ? LB::SOURCE_HLSL_EDITABLE : LB::SOURCE_HLSL)
                            : glsl ? (canEdit ? LB::SOURCE_GLSL_EDITABLE : LB::SOURCE_GLSL)
                                   : LB::SOURCE_DISASSEMBLY;
  ImGui::TextUnformatted(sourceLabel);
  if (!pipeline.ReplacementEligible && !pipeline.ReplacementDiagnostic.empty()) {
    ImGui::TextWrapped("Replacement unavailable: %s", pipeline.ReplacementDiagnostic.c_str());
  }
  if (!entry.Source->Diagnostic.empty()) {
    ImGui::TextWrapped("%s", entry.Source->Diagnostic.c_str());
  }

  const ShaderDraftKey key{*m_Selected, shader.Stage};
  auto [draftIt, inserted] = m_Drafts.try_emplace(key);
  ShaderDraft& draft = draftIt->second;
  if (inserted) {
    draft.Base = entry.Source;
    draft.Text = entry.Source->Text;
  }

  ProcessApplyResult(draft);
  ProcessResetResult(*m_Selected);

  const bool pipelineBusy = m_Module.Replacements().HasPendingOperation(*m_Selected);
  const bool replacementActive = m_Module.Replacements().HasActiveReplacement(*m_Selected);
  const bool stageReplacementActive =
      m_Module.Replacements().HasActiveReplacement(*m_Selected, shader.Stage);
  const bool editorEditable = canEdit && !pipelineBusy;
  if (m_EditorKey != key || m_DisplayedSource != entry.Source) {
    m_EditorKey = key;
    m_DisplayedSource = entry.Source;
    if (glsl) {
      m_SourceViewer.GetEditor().SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    } else if (hlsl) {
      m_SourceViewer.GetEditor().SetLanguageDefinition(TextEditor::LanguageDefinition::HLSL());
    }
    m_SourceViewer.GetEditor().SetColorizerEnable(hlsl || glsl);
    m_SourceViewer.SetText(draft.Text);
    m_SourceViewer.GetEditor().SetCursorPosition(TextEditor::Coordinates(0, 0));
  }
  const bool shouldBeReadOnly = !editorEditable;
  if (m_SourceViewer.GetEditor().IsReadOnly() != shouldBeReadOnly) {
    m_SourceViewer.GetEditor().SetReadOnly(shouldBeReadOnly);
  }

  const bool canApply =
      editorEditable && draft.Dirty && (!stageReplacementActive || draft.Text != draft.AppliedText);
  ImGui::BeginDisabled(!canApply);
  if (ImGui::Button(LB::COMPILE_AND_APPLY)) {
    RtsrReplacementService::ApplyRequest request;
    request.PipelineId = *m_Selected;
    request.Stage = shader.Stage;
    request.OriginalShaderHash = shader.BytecodeHash;
    request.D3DShaderVersion = shader.D3DShaderVersion;
    request.EntrypointName = entry.Source->EntrypointName;
    request.Source = draft.Text;
    request.PlayerPath = BaseContext::GetBaseInstance().GetGITSPlayerPath();
    draft.SubmittedText = draft.Text;
    draft.Operation = m_Module.Replacements().Apply(std::move(request));
    draft.Diagnostic.clear();
    m_PipelineNotices.erase(*m_Selected);
  }
  ImGui::EndDisabled();
  gits::ImGuiHelper::AddTooltip(LB::COMPILE_AND_APPLY_HINT);

  ImGui::SameLine();
  ImGui::BeginDisabled(!editorEditable || !draft.Dirty);
  if (ImGui::Button(LB::DISCARD_EDITS)) {
    draft.Text = draft.Base->Text;
    draft.Dirty = false;
    draft.Diagnostic.clear();
    m_SourceViewer.SetText(draft.Text);
    m_SourceViewer.GetEditor().SetCursorPosition(TextEditor::Coordinates(0, 0));
  }
  ImGui::EndDisabled();
  gits::ImGuiHelper::AddTooltip(LB::DISCARD_EDITS_HINT);

  ImGui::SameLine();
  ImGui::BeginDisabled(m_State != SessionState::Open || pipelineBusy || !replacementActive);
  if (ImGui::Button(LB::RESTORE_PIPELINE)) {
    m_ResetOperations[*m_Selected] = m_Module.Replacements().ResetPipeline(*m_Selected);
    m_PipelineNotices[*m_Selected] = LB::RESET_PENDING;
    for (auto& [draftKey, pipelineDraft] : m_Drafts) {
      if (draftKey.first == *m_Selected) {
        pipelineDraft.Diagnostic.clear();
      }
    }
  }
  ImGui::EndDisabled();
  gits::ImGuiHelper::AddTooltip(LB::RESTORE_PIPELINE_HINT);

  if (pipelineBusy) {
    const bool resetPending = m_ResetOperations.find(*m_Selected) != m_ResetOperations.end();
    ImGui::TextUnformatted(resetPending ? LB::RESET_PENDING : LB::APPLY_PENDING);
  } else if (replacementActive) {
    ImGui::TextUnformatted(LB::REPLACEMENT_ACTIVE);
  }
  const auto pipelineNotice = m_PipelineNotices.find(*m_Selected);
  if (pipelineNotice != m_PipelineNotices.end() && !pipelineNotice->second.empty()) {
    ImGui::TextWrapped("%s", pipelineNotice->second.c_str());
  }
  if (!draft.Diagnostic.empty()) {
    ImGui::TextWrapped("%s", draft.Diagnostic.c_str());
  }

  m_SourceViewer.Render(ImVec2(0.0f, 0.0f));
  if (editorEditable && m_SourceViewer.GetEditor().IsTextChanged()) {
    draft.Text = m_SourceViewer.GetText();
    draft.Dirty = draft.Text != draft.Base->Text;
    draft.Diagnostic.clear();
  }
}

} // namespace gits::gui
