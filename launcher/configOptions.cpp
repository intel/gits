//
// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configOptions.h"
#include "context.h"
#include "eventBus.h"

namespace {

void SetCaptureDefaults(){};

void SetPlaybackDefaults() {
  gits::gui::config_options::ExecutableNameOverrideEnabled(gits::gui::Mode::PLAYBACK) = true;

  gits::gui::Context::GetInstance().UpdateInMemoryConfig(gits::gui::Mode::PLAYBACK);
};

void SetSubcaptureDefaults() {
  gits::gui::config_options::SubcaptureEnabled(gits::gui::Mode::SUBCAPTURE) = true;
  gits::gui::config_options::ExecutableNameOverrideEnabled(gits::gui::Mode::SUBCAPTURE) = true;

  gits::gui::Context::GetInstance().UpdateInMemoryConfig(gits::gui::Mode::SUBCAPTURE);
};

} // namespace

namespace gits::gui::config_options {
void SetLauncherDefaults(std::optional<Mode> mode) {
  if (!mode.has_value()) {
    mode = Context::GetInstance().AppMode;
  }

  switch (mode.value()) {
  case Mode::CAPTURE:
    SetCaptureDefaults();
    break;
  case Mode::PLAYBACK:
    SetPlaybackDefaults();
    break;
  case Mode::SUBCAPTURE:
    SetSubcaptureDefaults();
    break;
  default:
    break;
  }
}

std::filesystem::path& OutputDir(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.outputDir;
}

std::filesystem::path& OutputTracePath(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.outputTracePath;
}

std::filesystem::path& DumpPath(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.recorder.dumpPath;
}

std::filesystem::path& SubcapturePath(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.subcapturePath;
}

bool& ExecutableNameOverrideEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.executableNameOverride.enabled;
}

std::string& ExecutableNameOverrideCustomName(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.executableNameOverride.customName;
}

std::vector<ApiBool>& HudEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.shared.hud.enabled;
}

bool& ScreenshotsEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.shared.screenshots.enabled;
}

std::string& ScreenshotsFrames(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.shared.screenshots.frames;
}

bool& TraceEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.shared.trace.enabled;
}

std::vector<std::string>& PlayerPlugins(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.player.plugins;
}

std::vector<std::string>& RecorderPlugins(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.recorder.plugins;
}

bool& RecorderEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.recorder.enabled;
}

bool& ShadowMemory(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.recorder.shadowMemory;
}

// ResourcesDump
bool& ResourcesDumpEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.enabled;
}

std::string& ResourcesDumpResourceKeys(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.resourceKeys;
}

std::string& ResourcesDumpCommandKeys(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.commandKeys;
}

std::string& ResourcesDumpTextureRescaleRange(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.textureRescaleRange;
}

ImageFormat& ResourcesDumpFormat(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.resourcesDump.format;
}

// RenderTargetsDump
bool& RenderTargetsDumpEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.enabled;
}

std::string& RenderTargetsDumpFrames(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.frames;
}

std::string& RenderTargetsDumpDraws(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.draws;
}

ImageFormat& RenderTargetsDumpFormat(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.format;
}

bool& RenderTargetsDumpDryRun(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.renderTargetsDump.dryRun;
}

// DispatchOutputsDump
bool& DispatchOutputsDumpEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.enabled;
}

std::string& DispatchOutputsDumpFrames(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.frames;
}

std::string& DispatchOutputsDumpDispatches(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.dispatches;
}

ImageFormat& DispatchOutputsDumpFormat(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.format;
}

bool& DispatchOutputsDumpDryRun(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.dispatchOutputsDump.dryRun;
}

// RaytracingDump
bool& RaytracingDumpBindingTablesPre(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.bindingTablesPre;
}

bool& RaytracingDumpBindingTablesPost(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.bindingTablesPost;
}

bool& RaytracingDumpInstancesPre(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.instancesPre;
}

bool& RaytracingDumpInstancesPost(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.instancesPost;
}

bool& RaytracingDumpBlases(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.blases;
}

std::string& RaytracingDumpCommandKeys(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.commandKeys;
}

std::string& RaytracingDumpCommandListModuloStep(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.raytracingDump.commandListModuloStep;
}

// ExecuteIndirectDump
bool& ExecuteIndirectDumpArgumentBufferPre(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.executeIndirectDump.argumentBufferPre;
}

bool& ExecuteIndirectDumpArgumentBufferPost(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.executeIndirectDump.argumentBufferPost;
}

std::string& ExecuteIndirectDumpCommandKeys(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.executeIndirectDump.commandKeys;
}

// RootSignatureDump
bool& RootSignatureDumpEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.rootSignatureDump.enabled;
}

std::string& RootSignatureDumpRootSignatureKeys(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.directx.features.rootSignatureDump.rootSignatureKeys;
}
bool& SubcaptureEnabled(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.subcapture.enabled;
}
std::string& SubcaptureFrames(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.subcapture.frames;
}
bool& SubcaptureOptimize(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.subcapture.optimize;
}
bool& SubcaptureExecutionSerialization(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.subcapture.directx.executionSerialization;
}
std::string& CommandListExecutions(std::optional<Mode> mode) {
  return Context::GetInstance()
      .ConfigurationForMode(mode)
      .ModifiedGitsConfiguration.common.player.subcapture.directx.commandListExecutions;
}
} // namespace gits::gui::config_options
