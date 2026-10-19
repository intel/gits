//
// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configOptions.h"
#include "baseContext.h"
#include "baseEventBus.h"

namespace {

void SetCaptureDefaults(){};

void SetPlaybackDefaults() {
  gits::gui::config_options::ExecutableNameOverrideEnabled(gits::gui::GitsAction::PLAYBACK) = true;

  gits::gui::BaseContext::GetBaseInstance().UpdateInMemoryConfig(gits::gui::GitsAction::PLAYBACK);
};

void SetSubcaptureDefaults() {
  gits::gui::config_options::SubcaptureEnabled(gits::gui::GitsAction::SUBCAPTURE) = true;
  gits::gui::config_options::ExecutableNameOverrideEnabled(gits::gui::GitsAction::SUBCAPTURE) =
      true;
  gits::gui::config_options::HudEnabled(gits::gui::GitsAction::SUBCAPTURE) = {};

  gits::gui::BaseContext::GetBaseInstance().UpdateInMemoryConfig(gits::gui::GitsAction::SUBCAPTURE);
};

} // namespace

namespace gits::gui::config_options {
void SetLauncherDefaults(std::optional<GitsAction> gitsAction) {
  if (!gitsAction.has_value()) {
    gitsAction = BaseContext::GetBaseInstance().SelectedAction;
  }

  switch (gitsAction.value()) {
  case GitsAction::CAPTURE:
    SetCaptureDefaults();
    break;
  case GitsAction::PLAYBACK:
    SetPlaybackDefaults();
    break;
  case GitsAction::SUBCAPTURE:
    SetSubcaptureDefaults();
    break;
  default:
    break;
  }
}

std::filesystem::path& OutputDir(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.outputDir;
}

std::filesystem::path& OutputTracePath(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.outputTracePath;
}

std::filesystem::path& DumpPath(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.recorder.dumpPath;
}

std::filesystem::path& SubcapturePath(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.subcapturePath;
}

bool& ExecutableNameOverrideEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.executableNameOverride.enabled;
}

std::string& ExecutableNameOverrideCustomName(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.executableNameOverride.customName;
}

std::vector<ApiBool>& HudEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.shared.hud.enabled;
}

bool& ScreenshotsEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.shared.screenshots.enabled;
}

std::string& ScreenshotsFrames(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.shared.screenshots.frames;
}

bool& TraceEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.shared.trace.enabled;
}

std::vector<std::string>& PlayerPlugins(std::optional<GitsAction> gitsAction) {
  auto& config =
      BaseContext::GetBaseInstance().ConfigurationForAction(gitsAction).ModifiedGitsConfiguration;
  return config.common.player.plugins;
}

std::vector<std::string>& RecorderPlugins(std::optional<GitsAction> gitsAction) {
  auto& config =
      BaseContext::GetBaseInstance().ConfigurationForAction(gitsAction).ModifiedGitsConfiguration;
  return config.common.recorder.plugins;
}

bool& RecorderEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.recorder.enabled;
}

bool& ShadowMemory(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.recorder.shadowMemory;
}

// ResourcesDump
bool& ResourcesDumpEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.enabled;
}

std::string& ResourcesDumpResourceKeys(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.resourceKeys;
}

std::string& ResourcesDumpCommandKeys(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.commandKeys;
}

std::string& ResourcesDumpTextureRescaleRange(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.textureRescaleRange;
}

ImageFormat& ResourcesDumpFormat(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.format;
}

// RenderTargetsDump
bool& RenderTargetsDumpEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.enabled;
}

std::string& RenderTargetsDumpFrames(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.frames;
}

std::string& RenderTargetsDumpDraws(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.draws;
}

ImageFormat& RenderTargetsDumpFormat(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.format;
}

bool& RenderTargetsDumpDryRun(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.dryRun;
}

// DispatchOutputsDump
bool& DispatchOutputsDumpEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.enabled;
}

std::string& DispatchOutputsDumpFrames(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.frames;
}

std::string& DispatchOutputsDumpDispatches(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.dispatches;
}

ImageFormat& DispatchOutputsDumpFormat(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.format;
}

bool& DispatchOutputsDumpDryRun(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.dryRun;
}

// RaytracingDump
bool& RaytracingDumpBindingTablesPre(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.bindingTablesPre;
}

bool& RaytracingDumpBindingTablesPost(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.bindingTablesPost;
}

bool& RaytracingDumpInstancesPre(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.instancesPre;
}

bool& RaytracingDumpInstancesPost(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.instancesPost;
}

bool& RaytracingDumpBlases(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.blases;
}

std::string& RaytracingDumpCommandKeys(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.commandKeys;
}

std::string& RaytracingDumpCommandListModuloStep(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.commandListModuloStep;
}

// ExecuteIndirectDump
bool& ExecuteIndirectDumpArgumentBufferPre(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.executeIndirectDump.argumentBufferPre;
}

bool& ExecuteIndirectDumpArgumentBufferPost(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.executeIndirectDump.argumentBufferPost;
}

std::string& ExecuteIndirectDumpCommandKeys(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.executeIndirectDump.commandKeys;
}

// RootSignatureDump
bool& RootSignatureDumpEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.rootSignatureDump.enabled;
}

std::string& RootSignatureDumpRootSignatureKeys(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.directx.features.rootSignatureDump.rootSignatureKeys;
}
bool& SubcaptureEnabled(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.subcapture.enabled;
}
std::string& SubcaptureFrames(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.subcapture.frames;
}
bool& SubcaptureOptimize(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.subcapture.optimize;
}
bool& SubcaptureExecutionSerialization(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.subcapture.directx.executionSerialization;
}
std::string& CommandListExecutions(std::optional<GitsAction> gitsAction) {
  return BaseContext::GetBaseInstance()
      .ConfigurationForAction(gitsAction)
      .ModifiedGitsConfiguration.common.player.subcapture.directx.commandListExecutions;
}
} // namespace gits::gui::config_options
