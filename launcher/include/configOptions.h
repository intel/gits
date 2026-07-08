// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>

#include "common.h"
#include <optional>
#include <string>
#include <vector>
#include <enumsAuto.h>

namespace gits::gui::config_options {

// This sets the values of select options to ones that we want to be the defaults when using the launcher
// these may be a different value compared to the actual GITS default
void SetLauncherDefaults(std::optional<Mode> mode = std::nullopt);

std::filesystem::path& OutputDir(std::optional<Mode> mode = std::nullopt);

std::filesystem::path& OutputTracePath(std::optional<Mode> mode = std::nullopt);

std::filesystem::path& DumpPath(std::optional<Mode> mode = std::nullopt);

std::filesystem::path& SubcapturePath(std::optional<Mode> mode = std::nullopt);

bool& ExecutableNameOverrideEnabled(std::optional<Mode> mode = std::nullopt);

std::string& ExecutableNameOverrideCustomName(std::optional<Mode> mode = std::nullopt);

std::vector<ApiBool>& HudEnabled(std::optional<Mode> mode = std::nullopt);

bool& ScreenshotsEnabled(std::optional<Mode> mode = std::nullopt);

std::string& ScreenshotsFrames(std::optional<Mode> mode = std::nullopt);

bool& TraceEnabled(std::optional<Mode> mode = std::nullopt);

std::vector<std::string>& PlayerPlugins(std::optional<Mode> mode = std::nullopt);

std::vector<std::string>& RecorderPlugins(std::optional<Mode> mode = std::nullopt);

bool& RecorderEnabled(std::optional<Mode> mode = std::nullopt);

bool& ShadowMemory(std::optional<Mode> mode = std::nullopt);

// ResourcesDump
bool& ResourcesDumpEnabled(std::optional<Mode> mode = std::nullopt);
std::string& ResourcesDumpResourceKeys(std::optional<Mode> mode = std::nullopt);
std::string& ResourcesDumpCommandKeys(std::optional<Mode> mode = std::nullopt);
std::string& ResourcesDumpTextureRescaleRange(std::optional<Mode> mode = std::nullopt);
ImageFormat& ResourcesDumpFormat(std::optional<Mode> mode = std::nullopt);

// RenderTargetsDump
bool& RenderTargetsDumpEnabled(std::optional<Mode> mode = std::nullopt);
std::string& RenderTargetsDumpFrames(std::optional<Mode> mode = std::nullopt);
std::string& RenderTargetsDumpDraws(std::optional<Mode> mode = std::nullopt);
ImageFormat& RenderTargetsDumpFormat(std::optional<Mode> mode = std::nullopt);
bool& RenderTargetsDumpDryRun(std::optional<Mode> mode = std::nullopt);

// DispatchOutputsDump
bool& DispatchOutputsDumpEnabled(std::optional<Mode> mode = std::nullopt);
std::string& DispatchOutputsDumpFrames(std::optional<Mode> mode = std::nullopt);
std::string& DispatchOutputsDumpDispatches(std::optional<Mode> mode = std::nullopt);
ImageFormat& DispatchOutputsDumpFormat(std::optional<Mode> mode = std::nullopt);
bool& DispatchOutputsDumpDryRun(std::optional<Mode> mode = std::nullopt);

// RaytracingDump
bool& RaytracingDumpBindingTablesPre(std::optional<Mode> mode = std::nullopt);
bool& RaytracingDumpBindingTablesPost(std::optional<Mode> mode = std::nullopt);
bool& RaytracingDumpInstancesPre(std::optional<Mode> mode = std::nullopt);
bool& RaytracingDumpInstancesPost(std::optional<Mode> mode = std::nullopt);
bool& RaytracingDumpBlases(std::optional<Mode> mode = std::nullopt);
std::string& RaytracingDumpCommandKeys(std::optional<Mode> mode = std::nullopt);
std::string& RaytracingDumpCommandListModuloStep(std::optional<Mode> mode = std::nullopt);

// ExecuteIndirectDump
bool& ExecuteIndirectDumpArgumentBufferPre(std::optional<Mode> mode = std::nullopt);
bool& ExecuteIndirectDumpArgumentBufferPost(std::optional<Mode> mode = std::nullopt);
std::string& ExecuteIndirectDumpCommandKeys(std::optional<Mode> mode = std::nullopt);

// RootSignatureDump
bool& RootSignatureDumpEnabled(std::optional<Mode> mode = std::nullopt);
std::string& RootSignatureDumpRootSignatureKeys(std::optional<Mode> mode = std::nullopt);

bool& SubcaptureEnabled(std::optional<Mode> mode = std::nullopt);
std::string& SubcaptureFrames(std::optional<Mode> mode = std::nullopt);
bool& SubcaptureOptimize(std::optional<Mode> mode = std::nullopt);
bool& SubcaptureExecutionSerialization(std::optional<Mode> mode = std::nullopt);
std::string& CommandListExecutions(std::optional<Mode> mode = std::nullopt);
} // namespace gits::gui::config_options
