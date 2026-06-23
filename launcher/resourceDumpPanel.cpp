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
#include "configOptions.h"
#include "configOptionsGuiHelpers.h"
#include "configMetadataAuto.h"

#include "nlohmann/json.hpp"

#include <ranges>
#include <algorithm>

namespace {
// Helper functions for mapping between gits::ImageFormat and combo index
inline int ImageFormatToComboIdx(gits::ImageFormat fmt) {
  switch (fmt) {
  case gits::ImageFormat::PNG:
    return 0;
  case gits::ImageFormat::JPEG:
    return 1;
  case gits::ImageFormat::DDS:
    return 2;
  default:
    return 0;
  }
}

inline gits::ImageFormat ComboIdxToImageFormat(int idx) {
  switch (idx) {
  case 0:
    return gits::ImageFormat::PNG;
  case 1:
    return gits::ImageFormat::JPEG;
  case 2:
    return gits::ImageFormat::DDS;
  default:
    return gits::ImageFormat::PNG;
  }
}

inline const char* ImageFormatToString(gits::ImageFormat fmt) {
  switch (fmt) {
  case gits::ImageFormat::PNG:
    return gits::gui::Labels::ResourceDumpPanel::FORMAT_PNG;
  case gits::ImageFormat::JPEG:
    return gits::gui::Labels::ResourceDumpPanel::FORMAT_JPEG;
  case gits::ImageFormat::DDS:
    return gits::gui::Labels::ResourceDumpPanel::FORMAT_DDS;
  default:
    return gits::gui::Labels::ResourceDumpPanel::FORMAT_PNG;
  }
}
} // namespace

namespace gits::gui {

ResourceDumpPanel::ResourceDumpPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&ResourceDumpPanel::OnPlaybackEnded, this, std::placeholders::_1));
}

void ResourceDumpPanel::Render() {
  using LB = Labels::ResourceDumpPanel;

  auto& context = Context::GetInstance();
  const float indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");

  auto inputWidth =
      2 * ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "123456789012345678901234567890");
  auto rangeFieldInputWidth = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "12345678901234");

  bool changed = false;

  auto comboFormat = [&](const char* label, float labelWidth, const char* id, const char* tooltip,
                         int& currentIdx) -> bool {
    static const char* kItems[] = {LB::FORMAT_PNG, LB::FORMAT_JPEG, LB::FORMAT_DDS};
    bool localChanged = false;

    ImGui::SetNextItemWidth(labelWidth);
    ImGui::TextUnformatted(label);
    if (tooltip) {
      ImGuiHelper::AddTooltip(tooltip);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(inputWidth);

    if (currentIdx < 0) {
      currentIdx = 0;
    }
    if (currentIdx > 2) {
      currentIdx = 2;
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
  auto& resourcesDumpEnabled = config_options::ResourcesDumpEnabled();
  changed |= ImGui::Checkbox(LB::RES_CHECKBOX, &resourcesDumpEnabled);
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Features::ResourcesDump::enabled);

  if (resourcesDumpEnabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::RES_RESOURCE_KEYS_LABEL, LB::COMMAND_KEYS_LABEL,
                           LB::RES_TEXTURE_RESCALE_LABEL, LB::RES_TEXTURE_RESCALE_LABEL});

    changed |= ImGuiHelper::LabelInputString(LB::RES_RESOURCE_KEYS_LABEL, LB::RES_RESOURCE_KEYS_ID,
                                             config_options::ResourcesDumpResourceKeys(),
                                             labelWidth, inputWidth);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ResourcesDump::resourceKeys);

    changed |= ImGuiHelper::LabelInputString(LB::COMMAND_KEYS_LABEL, LB::RES_COMMAND_KEYS_ID,
                                             config_options::ResourcesDumpCommandKeys(), labelWidth,
                                             inputWidth);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ResourcesDump::commandKeys);

    changed |= ImGuiHelper::LabelInputString(
        LB::RES_TEXTURE_RESCALE_LABEL, LB::RES_TEXTURE_RESCALE_ID,
        config_options::ResourcesDumpTextureRescaleRange(), labelWidth, inputWidth);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ResourcesDump::textureRescaleRange);

    auto& formatString = config_options::ResourcesDumpFormat();
    ResourcesDumpConfig.FormatIdx = ImageFormatToComboIdx(formatString);
    changed |= comboFormat(LB::FORMAT_LABEL, labelWidth, LB::RES_FORMAT_ID, nullptr,
                           ResourcesDumpConfig.FormatIdx);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ResourcesDump::format);
    formatString = ComboIdxToImageFormat(ResourcesDumpConfig.FormatIdx);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.RenderTargetsDump
  // =======================================================================
  auto& renderTargetsDumpEnabled = config_options::RenderTargetsDumpEnabled();
  changed |= ImGui::Checkbox(LB::RENDER_CHECKBOX, &renderTargetsDumpEnabled);
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Features::RenderTargetsDump::enabled);

  if (renderTargetsDumpEnabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::FRAMES_LABEL, LB::RENDER_DRAWS_LABEL, LB::FORMAT_LABEL});

    // Frames range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::FRAMES_LABEL, labelWidth, inputWidth, rangeFieldInputWidth, LB::RENDER_FRAMES_ID,
        nullptr, config_options::RenderTargetsDumpFrames(), RenderTargetsDumpConfig.TmpFrameStart,
        RenderTargetsDumpConfig.TmpFrameEnd, RenderTargetsDumpConfig.TmpFrameStep,
        "Add Frame Range", "Add Frame");
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RenderTargetsDump::frames);

    // Draws range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::RENDER_DRAWS_LABEL, labelWidth, inputWidth, rangeFieldInputWidth, LB::RENDER_DRAWS_ID,
        nullptr, config_options::RenderTargetsDumpDraws(), RenderTargetsDumpConfig.TmpDrawStart,
        RenderTargetsDumpConfig.TmpDrawEnd, RenderTargetsDumpConfig.TmpDrawStep, "Add Draw Range",
        "Add Draw");
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RenderTargetsDump::draws);

    changed |= ImGui::Checkbox(LB::RENDER_INCREASE_STEP_CHECKBOX,
                               &RenderTargetsDumpConfig.IncreaseStepOnOutOfMemoryError);
    ImGuiHelper::AddTooltip(LB::RENDER_INCREASE_STEP_TOOLTIP);

    auto& formatString = config_options::RenderTargetsDumpFormat();
    RenderTargetsDumpConfig.FormatIdx = ImageFormatToComboIdx(formatString);
    changed |= comboFormat(LB::FORMAT_LABEL, labelWidth, LB::RENDER_FORMAT_ID, nullptr,
                           RenderTargetsDumpConfig.FormatIdx);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RenderTargetsDump::format);
    formatString = ComboIdxToImageFormat(RenderTargetsDumpConfig.FormatIdx);

    changed |=
        ImGui::Checkbox(LB::RENDER_DRY_RUN_CHECKBOX, &config_options::RenderTargetsDumpDryRun());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RenderTargetsDump::dryRun);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.DispatchOutputsDump
  // =======================================================================
  auto& dispatchOutputsDumpEnabled = config_options::DispatchOutputsDumpEnabled();
  changed |= ImGui::Checkbox(LB::DISPATCH_CHECKBOX, &dispatchOutputsDumpEnabled);
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Features::DispatchOutputsDump::enabled);

  if (dispatchOutputsDumpEnabled) {
    ImGui::Indent(indent);

    auto labelWidth = calcMaxLabelWidth({LB::FRAMES_LABEL, LB::DISPATCH_DISPATCHES_LABEL,
                                         LB::FORMAT_LABEL, LB::DISPATCH_DRY_RUN_CHECKBOX});

    // Frames range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::FRAMES_LABEL, labelWidth, inputWidth, rangeFieldInputWidth, LB::DISPATCH_FRAMES_ID,
        nullptr, config_options::DispatchOutputsDumpFrames(),
        DispatchOutputsDumpConfig.TmpFrameStart, DispatchOutputsDumpConfig.TmpFrameEnd,
        DispatchOutputsDumpConfig.TmpFrameStep, "Add Frame Range", "Add Frame");
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::DispatchOutputsDump::frames);

    // Dispatches range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::DISPATCH_DISPATCHES_LABEL, labelWidth, inputWidth, rangeFieldInputWidth,
        LB::DISPATCH_DISPATCHES_ID, nullptr, config_options::DispatchOutputsDumpDispatches(),
        DispatchOutputsDumpConfig.TmpDispatchStart, DispatchOutputsDumpConfig.TmpDispatchEnd,
        DispatchOutputsDumpConfig.TmpDispatchStep, "Add Dispatch Range", "Add Dispatch");
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::DispatchOutputsDump::dispatches);

    auto& formatString = config_options::DispatchOutputsDumpFormat();
    DispatchOutputsDumpConfig.FormatIdx = ImageFormatToComboIdx(formatString);
    changed |= comboFormat(LB::FORMAT_LABEL, labelWidth, LB::DISPATCH_FORMAT_ID, nullptr,
                           DispatchOutputsDumpConfig.FormatIdx);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::DispatchOutputsDump::format);
    formatString = ComboIdxToImageFormat(DispatchOutputsDumpConfig.FormatIdx);

    changed |= ImGui::Checkbox(LB::DISPATCH_DRY_RUN_CHECKBOX,
                               &config_options::DispatchOutputsDumpDryRun());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::DispatchOutputsDump::dryRun);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.RaytracingDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::RT_CHECKBOX, &RaytracingDumpConfig.Enabled);
  ImGuiHelper::HelpButton(LB::RT_CHECKBOX_TOOLTIP, LB::RT_CHECKBOX_TOOLTIP,
                          LB::RT_CHECKBOX_TOOLTIP);

  if (RaytracingDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::RT_BINDING_TABLES_PRE_CHECKBOX, LB::RT_BINDING_TABLES_POST_CHECKBOX,
                           LB::RT_INSTANCES_PRE_CHECKBOX, LB::RT_INSTANCES_POST_CHECKBOX,
                           LB::RT_COMMAND_LIST_MODULO_STEP_LABEL});

    changed |= ImGui::Checkbox(LB::RT_BINDING_TABLES_PRE_CHECKBOX,
                               &config_options::RaytracingDumpBindingTablesPre());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::bindingTablesPre);

    changed |= ImGui::Checkbox(LB::RT_BINDING_TABLES_POST_CHECKBOX,
                               &config_options::RaytracingDumpBindingTablesPost());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::bindingTablesPost);

    changed |= ImGui::Checkbox(LB::RT_INSTANCES_PRE_CHECKBOX,
                               &config_options::RaytracingDumpInstancesPre());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::instancesPre);

    changed |= ImGui::Checkbox(LB::RT_INSTANCES_POST_CHECKBOX,
                               &config_options::RaytracingDumpInstancesPost());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::instancesPost);

    changed |= ImGui::Checkbox(LB::RT_BLASES_CHECKBOX, &config_options::RaytracingDumpBlases());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::blases);

    changed |= ImGuiHelper::LabelInputString(LB::COMMAND_KEYS_LABEL, LB::RT_COMMAND_KEYS_ID,
                                             config_options::RaytracingDumpCommandKeys(),
                                             labelWidth, inputWidth);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::commandKeys);

    // CommandListModuloStep range with UI controls
    changed |= ImGuiHelper::RangeControls(
        LB::RT_COMMAND_LIST_MODULO_STEP_LABEL, labelWidth, inputWidth, rangeFieldInputWidth,
        LB::RT_COMMAND_LIST_MODULO_STEP_ID, nullptr,
        config_options::RaytracingDumpCommandListModuloStep(), RaytracingDumpConfig.TmpModuloStart,
        RaytracingDumpConfig.TmpModuloEnd, RaytracingDumpConfig.TmpModuloStep, "Add Range",
        "Add Item");
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RaytracingDump::commandListModuloStep);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.ExecuteIndirectDump
  // =======================================================================
  changed |= ImGui::Checkbox(LB::XI_CHECKBOX, &ExecuteIndirectDumpConfig.Enabled);
  ImGuiHelper::HelpButton(LB::XI_CHECKBOX_TOOLTIP, LB::XI_CHECKBOX_TOOLTIP,
                          LB::XI_CHECKBOX_TOOLTIP);

  if (ExecuteIndirectDumpConfig.Enabled) {
    ImGui::Indent(indent);

    auto labelWidth =
        calcMaxLabelWidth({LB::XI_ARGUMENT_BUFFER_PRE_CHECKBOX,
                           LB::XI_ARGUMENT_BUFFER_POST_CHECKBOX, LB::COMMAND_KEYS_LABEL});

    changed |= ImGui::Checkbox(LB::XI_ARGUMENT_BUFFER_PRE_CHECKBOX,
                               &config_options::ExecuteIndirectDumpArgumentBufferPre());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ExecuteIndirectDump::argumentBufferPre);

    changed |= ImGui::Checkbox(LB::XI_ARGUMENT_BUFFER_POST_CHECKBOX,
                               &config_options::ExecuteIndirectDumpArgumentBufferPost());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ExecuteIndirectDump::argumentBufferPost);

    changed |= ImGuiHelper::LabelInputString(LB::COMMAND_KEYS_LABEL, LB::XI_COMMAND_KEYS_ID,
                                             config_options::ExecuteIndirectDumpCommandKeys(),
                                             labelWidth, inputWidth);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::ExecuteIndirectDump::commandKeys);

    ImGui::Unindent(indent);
  }

  // =======================================================================
  // DirectX.Features.RootSignatureDump
  // =======================================================================
  auto& rootSignatureDumpEnabled = config_options::RootSignatureDumpEnabled();
  changed |= ImGui::Checkbox(LB::ROOTSIG_CHECKBOX, &rootSignatureDumpEnabled);
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Features::RootSignatureDump::enabled);

  if (rootSignatureDumpEnabled) {
    ImGui::Indent(indent);

    auto labelWidth = calcMaxLabelWidth({LB::ROOTSIG_KEYS_LABEL});

    changed |= ImGuiHelper::LabelInputString(LB::ROOTSIG_KEYS_LABEL, LB::ROOTSIG_KEYS_ID,
                                             config_options::RootSignatureDumpRootSignatureKeys(),
                                             0, inputWidth);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::RootSignatureDump::rootSignatureKeys);

    ImGui::Unindent(indent);
  }

  if (changed) {
    context.UpdateInMemoryConfig(gits::gui::Mode::PLAYBACK);
    EventBus::GetInstance().publish<ContextEvent>(
        {ContextEvent::Type::InMemoryConfigurationChanged, Mode::PLAYBACK});
  }
}

std::string ResourceDumpPanel::DoubleRangeSteps(const std::string& rangeString) {
  if (rangeString.empty()) {
    return rangeString;
  }

  std::string result;
  std::istringstream stream(rangeString);
  std::string segment;
  while (std::getline(stream, segment, ',')) {
    if (!result.empty()) {
      result += ',';
    }
    auto dashPos = segment.find('-');
    if (dashPos == std::string::npos) {
      // Single value — keep as-is
      result += segment;
      continue;
    }
    auto colonPos = segment.find(':');
    if (colonPos != std::string::npos) {
      int step = std::stoi(segment.substr(colonPos + 1));
      step *= 2;
      result += segment.substr(0, colonPos + 1) + std::to_string(step);
    } else {
      // Range with implicit step=1 → make it step=2
      result += segment + ":2";
    }
  }
  return result;
}

void ResourceDumpPanel::OnPlaybackEnded(const Event& e) {
  const ActionEvent& action = static_cast<const ActionEvent&>(e);
  if (action.EventType != ActionEvent::Type::Playback ||
      action.ActionState != ActionEvent::State::Ended) {
    return;
  }

  if (!RenderTargetsDumpConfig.IncreaseStepOnOutOfMemoryError) {
    return;
  }

  bool isOom = action.ActionStatus == ActionEvent::Status::Failure &&
               action.Details.find("E_OUTOFMEMORY") != std::string::npos;
  if (!isOom) {
    oomRetryCount_ = 0;
    return;
  }

  if (oomRetryCount_ >= kMaxOomRetries) {
    LOG_ERROR << "OOM retry limit reached (" << kMaxOomRetries << "). Giving up.";
    return;
  }
  oomRetryCount_++;

  bool anyModified = false;
  if (config_options::RenderTargetsDumpEnabled()) {
    // We change only the draws and leave frames as-is
    config_options::RenderTargetsDumpDraws() =
        DoubleRangeSteps(config_options::RenderTargetsDumpDraws());
    gits::gui::Context::GetInstance().UpdateInMemoryConfig(gits::gui::Mode::PLAYBACK);
    EventBus::GetInstance().publish<ContextEvent>(
        {ContextEvent::Type::InMemoryConfigurationChanged, Mode::PLAYBACK});
    anyModified = true;
  }
  if (!anyModified) {
    return;
  }
  PlaybackStream();
}

} // namespace gits::gui
