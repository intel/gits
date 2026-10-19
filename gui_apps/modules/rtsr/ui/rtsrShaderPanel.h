// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/ui/rtsrEvents.h"
#include "rtsr/ui/rtsrModule.h"
#include "rtsr/ui/rtsrShaderSourceCache.h"

#include "textEditorWidget.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "buttonGroup.h"
#include "yamlTreeViewer.h"

namespace gits::gui {

// The Shaders tab: the connected application, the pipelines it has created, and
// the shaders and serialized state of the selected one. Reconstructed DXIL
// source is editable while its client session is live.
class RtsrShaderPanel {
public:
  explicit RtsrShaderPanel(RtsrModule& module);

  void Render();

  // Both are fanned in from RtsrPanel, which owns the single event
  // subscription for the whole module.
  void OnRtsrEvent(const RtsrEvent& event);
  void OnThemeChanged();

  // Selects a pipeline on behalf of another tab. Clears the filter if it would
  // hide the row and centres the row in the list, because otherwise the jump
  // leaves the user looking at an unrelated part of a list thousands of rows
  // long.
  void RevealPipeline(rtsr::RTSR_ID id);

  // Drops the harvested run. Called when the next one starts, and by RtsrPanel
  // when the module is disabled.
  void ClearSession();

private:
  // Ended keeps the finished run's rows, and the store behind them, so it can
  // still be browsed once the process being measured has exited.
  enum class SessionState {
    Idle,
    Open,
    Ended
  };

  // One row of the pipeline table. Built from RtsrEvent rather than read back
  // from the store every frame, which keeps rendering independent of how much
  // the session holds.
  struct PipelineRow {
    rtsr::RTSR_ID Id{};
    rtsr::RTSR_PIPELINE_TYPE Type{rtsr::RTSR_PIPELINE_TYPE_UNKNOWN};
    size_t StageCount{};
    std::string Name;
  };

  using ShaderDraftKey = std::pair<rtsr::RTSR_ID, rtsr::RTSR_SHADER_STAGE>;

  struct ShaderDraft {
    std::shared_ptr<const rtsr::ShaderSource> Base;
    std::string Text;
    // Exact source captured by the pending operation. The editor can process
    // another input event later in the same ImGui frame.
    std::string SubmittedText;
    std::string AppliedText;
    std::string Diagnostic;
    std::optional<RtsrReplacementService::OperationId> Operation;
    bool Dirty{false};
  };

  void AddOrUpdateRow(const RtsrEvent& event);
  void RemoveRow(rtsr::RTSR_ID id);

  void RebuildVisibleRows();
  std::optional<size_t> VisiblePositionOf(rtsr::RTSR_ID id) const;
  void SelectPipeline(std::optional<rtsr::RTSR_ID> id);
  // Re-reads the store for the selected pipeline. Called when the selection
  // changes and when what it points at does.
  void RefreshSelection();

  void RenderSession();
  void RenderPipelineList();
  void RenderPipelineDetail();
  void RenderShader(const rtsr::PipelineRecord& pipeline, const rtsr::ShaderSummary& shader);
  void ProcessApplyResult(ShaderDraft& draft);
  void ProcessResetResult(rtsr::RTSR_ID pipelineId);
  void ClearPipelineDrafts(rtsr::RTSR_ID pipelineId);

  void PipelineToggleButton(const std::string& label,
                            const std::string& hint,
                            rtsr::RTSR_PIPELINE_TYPE type);

  RtsrModule& m_Module;
  const rtsr::SessionStore& m_Store;

  SessionState m_State{SessionState::Idle};
  std::string m_ApplicationName;
  std::string m_ApplicationPath;
  rtsr::RTSR_API m_UsedApi{rtsr::RTSR_API_UNKNOWN};

  std::vector<PipelineRow> m_Pipelines;
  std::unordered_map<rtsr::RTSR_ID, size_t> m_RowByPipelineId;
  // Indices into m_Pipelines that pass the filter, rebuilt only when the filter
  // or the rows change.
  std::vector<size_t> m_VisibleRows;
  bool m_VisibleRowsDirty{true};
  char m_Filter[128]{};
  // Renders left in which to keep asking for the selected row to be centred,
  // counted down by the list render, which is the only place that can move the
  // table's scroll. More than one is needed because the tab switch that brings
  // this panel back only takes effect a frame later, and that first render is
  // also the first time the nested panes are laid out since the panel was last
  // visible. A scroll target computed from geometry that has not settled lands
  // nowhere, so the request is retried until the row really is on screen.
  int m_RevealAttemptsLeft{0};

  std::optional<rtsr::RTSR_ID> m_Selected;
  std::optional<rtsr::PipelineRecord> m_SelectedRecord;
  std::vector<rtsr::ShaderSummary> m_SelectedShaders;

  std::map<ShaderDraftKey, ShaderDraft> m_Drafts;
  std::optional<ShaderDraftKey> m_EditorKey;
  std::unordered_map<rtsr::RTSR_ID, RtsrReplacementService::OperationId> m_ResetOperations;
  std::unordered_map<rtsr::RTSR_ID, std::string> m_PipelineNotices;
  std::shared_ptr<const rtsr::ShaderSource> m_DisplayedSource;

  gits::ImGuiHelper::TextEditorWidget m_PipelineViewer;
  gits::ImGuiHelper::YamlTreeViewer m_YamlTreeViewer;
  gits::ImGuiHelper::TextEditorWidget m_SourceViewer;
  gits::ImGuiHelper::TextEditorWidget m_Log;
  RtsrShaderSourceCache m_SourceCache;
  std::map<rtsr::RTSR_PIPELINE_TYPE, bool> m_PipelineTypeFilters = {
      {rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_COMPUTE, true},
      {rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_GRAPHICS, true},
      {rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_RAY_TRACING, true},
      {rtsr::RTSR_PIPELINE_TYPE::RTSR_PIPELINE_TYPE_UNKNOWN, true},
  };
};

} // namespace gits::gui
