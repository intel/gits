// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpPanel.h"

#include "imgui.h"
#include "imGuiHelper.h"
#include "launcherActions.h"
#include "labels.h"
#include "mainWindow.h"
#include "eventBus.h"
#include "contextHelper.h"
#include "nlohmann/json.hpp"

#include <ranges>
#include <algorithm>

namespace gits::gui {

ResourceDumpPanel::ResourceDumpPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&ResourceDumpPanel::ContextCallback, this, std::placeholders::_1));
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&ResourceDumpPanel::PathCallback, this, std::placeholders::_1));
}

void ResourceDumpPanel::Render() {
  using LB = Labels::ResourceDumpPanel;

  auto& context = Context::GetInstance();
  const float indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");

  // Calculate widths for range input fields (similar to PlaybackOptionsPanel)
  auto inputWidth =
      2 * ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "123456789012345678901234567890");
  auto rangeFieldInputWidth = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "12345678901234");

  bool changed = false;

  auto comboFormat = [&](const char* label, float labelWidth, const char* id, const char* tooltip,
                         int& currentIdx) -> bool {
    static const char* kItems[] = {LB::FORMAT_PNG, LB::FORMAT_JPEG};
    bool localChanged = false;

    ImGui::SetNextItemWidth(labelWidth);
    ImGui::TextUnformatted(label);
    ImGuiHelper::AddTooltip(tooltip);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(inputWidth);

    if (currentIdx < 0) {
      currentIdx = 0;
    }
    if (currentIdx > 1) {
      currentIdx = 1;
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + labelWidth -
                         ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label));

    localChanged |= ImGui::Combo(id, &currentIdx, kItems, IM_ARRAYSIZE(kItems));
    return localChanged;
  };

  auto calcMaxLabelWidth = [](std::initializer_list<const char*> labels) -> float {
    return std::ranges::max(labels | std::views::transform([](const auto& label) {
                              return ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
                            }));
  };

  // =======================================================================
  // DirectX.Features.ResourcesDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::RES_CHECKBOX, &ResourcesDumpConfig.Enabled);
  ImGuiHelper::AddTooltip(LB::RES_CHECKBOX_TOOLTIP);

  if (ResourcesDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::RES_RESOURCE_KEYS_LABEL, LB::COMMAND_KEYS_LABEL,
                           LB::RES_TEXTURE_RESCALE_LABEL, LB::RES_TEXTURE_RESCALE_LABEL});

    changed |= ImGuiHelper::LabelInputStringTooltip(
        LB::RES_RESOURCE_KEYS_LABEL, LB::RES_RESOURCE_KEYS_ID, LB::RES_RESOURCE_KEYS_TOOLTIP,
        ResourcesDumpConfig.ResourceKeys, labelWidth, inputWidth);

    changed |= ImGuiHelper::LabelInputStringTooltip(
        LB::COMMAND_KEYS_LABEL, LB::RES_COMMAND_KEYS_ID, LB::RES_COMMAND_KEYS_TOOLTIP,
        ResourcesDumpConfig.CommandKeys, labelWidth, inputWidth);

    changed |= ImGuiHelper::LabelInputStringTooltip(
        LB::RES_TEXTURE_RESCALE_LABEL, LB::RES_TEXTURE_RESCALE_ID, LB::RES_TEXTURE_RESCALE_TOOLTIP,
        ResourcesDumpConfig.TextureRescaleRange, labelWidth, inputWidth);

    changed |= comboFormat(LB::FORMAT_LABEL, labelWidth, LB::RES_FORMAT_ID, LB::FORMAT_TOOLTIP,
                           ResourcesDumpConfig.FormatIdx);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.RenderTargetsDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::RENDER_CHECKBOX, &RenderTargetsDumpConfig.Enabled);
  ImGuiHelper::AddTooltip(LB::RENDER_CHECKBOX_TOOLTIP);

  if (RenderTargetsDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::FRAMES_LABEL, LB::RENDER_DRAWS_LABEL, LB::FORMAT_LABEL});

    // Frames range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::FRAMES_LABEL, labelWidth, inputWidth, rangeFieldInputWidth, LB::RENDER_FRAMES_ID,
        LB::FRAMES_TOOLTIP, RenderTargetsDumpConfig.Frames, RenderTargetsDumpConfig.TmpFrameStart,
        RenderTargetsDumpConfig.TmpFrameEnd, RenderTargetsDumpConfig.TmpFrameStep,
        "Add Frame Range", "Add Frame");

    // Draws range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::RENDER_DRAWS_LABEL, labelWidth, inputWidth, rangeFieldInputWidth, LB::RENDER_DRAWS_ID,
        LB::RENDER_DRAWS_TOOLTIP, RenderTargetsDumpConfig.Draws,
        RenderTargetsDumpConfig.TmpDrawStart, RenderTargetsDumpConfig.TmpDrawEnd,
        RenderTargetsDumpConfig.TmpDrawStep, "Add Draw Range", "Add Draw");

    changed |= comboFormat(LB::FORMAT_LABEL, labelWidth, LB::RENDER_FORMAT_ID, LB::FORMAT_TOOLTIP,
                           RenderTargetsDumpConfig.FormatIdx);

    changed |= ImGui::Checkbox(LB::RENDER_DRY_RUN_CHECKBOX, &RenderTargetsDumpConfig.DryRun);
    ImGuiHelper::AddTooltip(LB::DRY_RUN_TOOLTIP);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.DispatchOutputsDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::DISPATCH_CHECKBOX, &DispatchOutputsDumpConfig.Enabled);
  ImGuiHelper::AddTooltip(LB::DISPATCH_CHECKBOX_TOOLTIP);

  if (DispatchOutputsDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth = calcMaxLabelWidth({LB::FRAMES_LABEL, LB::DISPATCH_DISPATCHES_LABEL,
                                         LB::FORMAT_LABEL, LB::DISPATCH_DRY_RUN_CHECKBOX});

    // Frames range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::FRAMES_LABEL, labelWidth, inputWidth, rangeFieldInputWidth, LB::DISPATCH_FRAMES_ID,
        LB::FRAMES_TOOLTIP, DispatchOutputsDumpConfig.Frames,
        DispatchOutputsDumpConfig.TmpFrameStart, DispatchOutputsDumpConfig.TmpFrameEnd,
        DispatchOutputsDumpConfig.TmpFrameStep, "Add Frame Range", "Add Frame");

    // Dispatches range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::DISPATCH_DISPATCHES_LABEL, labelWidth, inputWidth, rangeFieldInputWidth,
        LB::DISPATCH_DISPATCHES_ID, LB::DISPATCH_DISPATCHES_TOOLTIP,
        DispatchOutputsDumpConfig.Dispatches, DispatchOutputsDumpConfig.TmpDispatchStart,
        DispatchOutputsDumpConfig.TmpDispatchEnd, DispatchOutputsDumpConfig.TmpDispatchStep,
        "Add Dispatch Range", "Add Dispatch");

    changed |= comboFormat(LB::FORMAT_LABEL, labelWidth, LB::DISPATCH_FORMAT_ID, LB::FORMAT_TOOLTIP,
                           DispatchOutputsDumpConfig.FormatIdx);

    changed |= ImGui::Checkbox(LB::DISPATCH_DRY_RUN_CHECKBOX, &DispatchOutputsDumpConfig.DryRun);
    ImGuiHelper::AddTooltip(LB::DRY_RUN_TOOLTIP);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.RaytracingDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::RT_CHECKBOX, &RaytracingDumpConfig.Enabled);
  ImGuiHelper::AddTooltip(LB::RT_CHECKBOX_TOOLTIP);

  if (RaytracingDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::RT_BINDING_TABLES_PRE_CHECKBOX, LB::RT_BINDING_TABLES_POST_CHECKBOX,
                           LB::RT_INSTANCES_PRE_CHECKBOX, LB::RT_INSTANCES_POST_CHECKBOX,
                           LB::RT_COMMAND_LIST_MODULO_STEP_LABEL});

    changed |=
        ImGui::Checkbox(LB::RT_BINDING_TABLES_PRE_CHECKBOX, &RaytracingDumpConfig.BindingTablesPre);
    ImGuiHelper::AddTooltip(LB::RT_BINDING_TABLES_PRE_TOOLTIP);

    changed |= ImGui::Checkbox(LB::RT_BINDING_TABLES_POST_CHECKBOX,
                               &RaytracingDumpConfig.BindingTablesPost);
    ImGuiHelper::AddTooltip(LB::RT_BINDING_TABLES_POST_TOOLTIP);

    changed |= ImGui::Checkbox(LB::RT_INSTANCES_PRE_CHECKBOX, &RaytracingDumpConfig.InstancesPre);
    ImGuiHelper::AddTooltip(LB::RT_INSTANCES_PRE_TOOLTIP);

    changed |= ImGui::Checkbox(LB::RT_INSTANCES_POST_CHECKBOX, &RaytracingDumpConfig.InstancesPost);
    ImGuiHelper::AddTooltip(LB::RT_INSTANCES_POST_TOOLTIP);

    changed |= ImGui::Checkbox(LB::RT_BLASES_CHECKBOX, &RaytracingDumpConfig.Blases);
    ImGuiHelper::AddTooltip(LB::RT_BLASES_TOOLTIP);

    changed |= ImGuiHelper::LabelInputStringTooltip(
        LB::COMMAND_KEYS_LABEL, LB::RT_COMMAND_KEYS_ID, LB::RT_COMMAND_KEYS_TOOLTIP,
        RaytracingDumpConfig.CommandKeys, labelWidth, inputWidth);

    // CommandListModuloStep range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::RT_COMMAND_LIST_MODULO_STEP_LABEL, labelWidth, inputWidth, rangeFieldInputWidth,
        LB::RT_COMMAND_LIST_MODULO_STEP_ID, LB::RT_COMMAND_LIST_MODULO_STEP_TOOLTIP,
        RaytracingDumpConfig.CommandListModuloStep, RaytracingDumpConfig.TmpModuloStart,
        RaytracingDumpConfig.TmpModuloEnd, RaytracingDumpConfig.TmpModuloStep, "Add Range",
        "Add Item");

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.ExecuteIndirectDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::XI_CHECKBOX, &ExecuteIndirectDumpConfig.Enabled);
  ImGuiHelper::AddTooltip(LB::XI_CHECKBOX_TOOLTIP);

  if (ExecuteIndirectDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::XI_ARGUMENT_BUFFER_PRE_CHECKBOX,
                           LB::XI_ARGUMENT_BUFFER_POST_CHECKBOX, LB::COMMAND_KEYS_LABEL});

    changed |= ImGui::Checkbox(LB::XI_ARGUMENT_BUFFER_PRE_CHECKBOX,
                               &ExecuteIndirectDumpConfig.ArgumentBufferPre);
    ImGuiHelper::AddTooltip(LB::XI_ARGUMENT_BUFFER_PRE_TOOLTIP);

    changed |= ImGui::Checkbox(LB::XI_ARGUMENT_BUFFER_POST_CHECKBOX,
                               &ExecuteIndirectDumpConfig.ArgumentBufferPost);
    ImGuiHelper::AddTooltip(LB::XI_ARGUMENT_BUFFER_POST_TOOLTIP);

    changed |= ImGuiHelper::LabelInputStringTooltip(
        LB::COMMAND_KEYS_LABEL, LB::XI_COMMAND_KEYS_ID, LB::XI_COMMAND_KEYS_TOOLTIP,
        ExecuteIndirectDumpConfig.CommandKeys, labelWidth, inputWidth);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.RootSignatureDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::ROOTSIG_CHECKBOX, &RootSignatureDumpConfig.Enabled);
  ImGuiHelper::AddTooltip(LB::ROOTSIG_CHECKBOX_TOOLTIP);

  if (RootSignatureDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth = calcMaxLabelWidth({LB::ROOTSIG_KEYS_LABEL});

    changed |= ImGuiHelper::LabelInputStringTooltip(LB::ROOTSIG_KEYS_LABEL, LB::ROOTSIG_KEYS_ID,
                                                    LB::ROOTSIG_KEYS_TOOLTIP,
                                                    RootSignatureDumpConfig.Keys, 0, inputWidth);

    ImGui::Unindent(indent);
  }

  if (changed) {
    UpdateCLICall();
  }
}

const std::string ResourceDumpPanel::GetCLIArguments() const {
  std::string args;

  auto addBool = [&](const char* key) { args += std::string("--") + key + " "; };
  auto addStr = [&](const char* key, const std::string& value) {
    if (!value.empty()) {
      args += std::string("--") + key + "=\"" + value + "\" ";
    }
  };
  auto addFormat = [&](const char* key, int idx) {
    // Keep in sync with UI combo (0=PNG, 1=JPEG)
    const char* fmt =
        (idx == 1) ? Labels::ResourceDumpPanel::FORMAT_JPEG : Labels::ResourceDumpPanel::FORMAT_PNG;
    args += std::string("--") + key + "=\"" + fmt + "\" ";
  };

  // DirectX.Features.ResourcesDump
  if (ResourcesDumpConfig.Enabled) {
    addBool("DirectX.Features.ResourcesDump.Enabled");
    addStr("DirectX.Features.ResourcesDump.ResourceKeys", ResourcesDumpConfig.ResourceKeys);
    addStr("DirectX.Features.ResourcesDump.CommandKeys", ResourcesDumpConfig.CommandKeys);
    addStr("DirectX.Features.ResourcesDump.TextureRescaleRange",
           ResourcesDumpConfig.TextureRescaleRange);
    addFormat("DirectX.Features.ResourcesDump.Format", ResourcesDumpConfig.FormatIdx);
  }

  // DirectX.Features.RenderTargetsDump
  if (RenderTargetsDumpConfig.Enabled) {
    addBool("DirectX.Features.RenderTargetsDump.Enabled");
    addStr("DirectX.Features.RenderTargetsDump.Frames", RenderTargetsDumpConfig.Frames);
    addStr("DirectX.Features.RenderTargetsDump.Draws", RenderTargetsDumpConfig.Draws);
    addFormat("DirectX.Features.RenderTargetsDump.Format", RenderTargetsDumpConfig.FormatIdx);
    if (RenderTargetsDumpConfig.DryRun) {
      addBool("DirectX.Features.RenderTargetsDump.DryRun");
    }
  }

  // DirectX.Features.DispatchOutputsDump
  if (DispatchOutputsDumpConfig.Enabled) {
    addBool("DirectX.Features.DispatchOutputsDump.Enabled");
    addStr("DirectX.Features.DispatchOutputsDump.Frames", DispatchOutputsDumpConfig.Frames);
    addStr("DirectX.Features.DispatchOutputsDump.Dispatches", DispatchOutputsDumpConfig.Dispatches);
    addFormat("DirectX.Features.DispatchOutputsDump.Format", DispatchOutputsDumpConfig.FormatIdx);
    if (DispatchOutputsDumpConfig.DryRun) {
      addBool("DirectX.Features.DispatchOutputsDump.DryRun");
    }
  }

  // DirectX.Features.RaytracingDump
  if (RaytracingDumpConfig.Enabled) {
    if (RaytracingDumpConfig.BindingTablesPre) {
      addBool("DirectX.Features.RaytracingDump.BindingTablesPre");
    }
    if (RaytracingDumpConfig.BindingTablesPost) {
      addBool("DirectX.Features.RaytracingDump.BindingTablesPost");
    }
    if (RaytracingDumpConfig.InstancesPre) {
      addBool("DirectX.Features.RaytracingDump.InstancesPre");
    }
    if (RaytracingDumpConfig.InstancesPost) {
      addBool("DirectX.Features.RaytracingDump.InstancesPost");
    }
    if (RaytracingDumpConfig.Blases) {
      addBool("DirectX.Features.RaytracingDump.Blases");
    }
    addStr("DirectX.Features.RaytracingDump.CommandKeys", RaytracingDumpConfig.CommandKeys);
    addStr("DirectX.Features.RaytracingDump.CommandListModuloStep",
           RaytracingDumpConfig.CommandListModuloStep);
  }

  // DirectX.Features.ExecuteIndirectDump
  if (ExecuteIndirectDumpConfig.Enabled) {
    if (ExecuteIndirectDumpConfig.ArgumentBufferPre) {
      addBool("DirectX.Features.ExecuteIndirectDump.ArgumentBufferPre");
    }
    if (ExecuteIndirectDumpConfig.ArgumentBufferPost) {
      addBool("DirectX.Features.ExecuteIndirectDump.ArgumentBufferPost");
    }
    addStr("DirectX.Features.ExecuteIndirectDump.CommandKeys",
           ExecuteIndirectDumpConfig.CommandKeys);
  }

  // DirectX.Features.RootSignatureDump
  if (RootSignatureDumpConfig.Enabled) {
    addBool("DirectX.Features.RootSignatureDump.Enabled");
    addStr("DirectX.Features.RootSignatureDump.RootSignatureKeys", RootSignatureDumpConfig.Keys);
  }

  return args;
}

void ResourceDumpPanel::ContextCallback(const Event& /*e*/) {}

void ResourceDumpPanel::PathCallback(const Event& /*e*/) {}
} // namespace gits::gui
