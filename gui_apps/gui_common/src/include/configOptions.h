// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>

#include "baseCommon.h"
#include <optional>
#include <string>
#include <vector>
#include <enumsAuto.h>

namespace gits::gui::config_options {

// This sets the values of select options to ones that we want to be the defaults when using the launcher
// these may be a different value compared to the actual GITS default
void SetLauncherDefaults(std::optional<GitsAction> gitsAction = std::nullopt);

std::filesystem::path& OutputDir(std::optional<GitsAction> gitsAction = std::nullopt);

std::filesystem::path& OutputTracePath(std::optional<GitsAction> gitsAction = std::nullopt);

std::filesystem::path& DumpPath(std::optional<GitsAction> gitsAction = std::nullopt);

std::filesystem::path& SubcapturePath(std::optional<GitsAction> gitsAction = std::nullopt);

bool& ExecutableNameOverrideEnabled(std::optional<GitsAction> gitsAction = std::nullopt);

std::string& ExecutableNameOverrideCustomName(std::optional<GitsAction> gitsAction = std::nullopt);

std::vector<ApiBool>& HudEnabled(std::optional<GitsAction> gitsAction = std::nullopt);

bool& ScreenshotsEnabled(std::optional<GitsAction> gitsAction = std::nullopt);

std::string& ScreenshotsFrames(std::optional<GitsAction> gitsAction = std::nullopt);

bool& TraceEnabled(std::optional<GitsAction> gitsAction = std::nullopt);

std::vector<std::string>& PlayerPlugins(std::optional<GitsAction> gitsAction = std::nullopt);

std::vector<std::string>& RecorderPlugins(std::optional<GitsAction> gitsAction = std::nullopt);

bool& RecorderEnabled(std::optional<GitsAction> gitsAction = std::nullopt);

bool& ShadowMemory(std::optional<GitsAction> gitsAction = std::nullopt);

// ResourcesDump
bool& ResourcesDumpEnabled(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& ResourcesDumpResourceKeys(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& ResourcesDumpCommandKeys(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& ResourcesDumpTextureRescaleRange(std::optional<GitsAction> gitsAction = std::nullopt);
ImageFormat& ResourcesDumpFormat(std::optional<GitsAction> gitsAction = std::nullopt);

// RenderTargetsDump
bool& RenderTargetsDumpEnabled(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& RenderTargetsDumpFrames(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& RenderTargetsDumpDraws(std::optional<GitsAction> gitsAction = std::nullopt);
ImageFormat& RenderTargetsDumpFormat(std::optional<GitsAction> gitsAction = std::nullopt);
bool& RenderTargetsDumpDryRun(std::optional<GitsAction> gitsAction = std::nullopt);

// DispatchOutputsDump
bool& DispatchOutputsDumpEnabled(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& DispatchOutputsDumpFrames(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& DispatchOutputsDumpDispatches(std::optional<GitsAction> gitsAction = std::nullopt);
ImageFormat& DispatchOutputsDumpFormat(std::optional<GitsAction> gitsAction = std::nullopt);
bool& DispatchOutputsDumpDryRun(std::optional<GitsAction> gitsAction = std::nullopt);

// RaytracingDump
bool& RaytracingDumpBindingTablesPre(std::optional<GitsAction> gitsAction = std::nullopt);
bool& RaytracingDumpBindingTablesPost(std::optional<GitsAction> gitsAction = std::nullopt);
bool& RaytracingDumpInstancesPre(std::optional<GitsAction> gitsAction = std::nullopt);
bool& RaytracingDumpInstancesPost(std::optional<GitsAction> gitsAction = std::nullopt);
bool& RaytracingDumpBlases(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& RaytracingDumpCommandKeys(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& RaytracingDumpCommandListModuloStep(
    std::optional<GitsAction> gitsAction = std::nullopt);

// ExecuteIndirectDump
bool& ExecuteIndirectDumpArgumentBufferPre(std::optional<GitsAction> gitsAction = std::nullopt);
bool& ExecuteIndirectDumpArgumentBufferPost(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& ExecuteIndirectDumpCommandKeys(std::optional<GitsAction> gitsAction = std::nullopt);

// RootSignatureDump
bool& RootSignatureDumpEnabled(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& RootSignatureDumpRootSignatureKeys(
    std::optional<GitsAction> gitsAction = std::nullopt);

bool& SubcaptureEnabled(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& SubcaptureFrames(std::optional<GitsAction> gitsAction = std::nullopt);
bool& SubcaptureOptimize(std::optional<GitsAction> gitsAction = std::nullopt);
bool& SubcaptureExecutionSerialization(std::optional<GitsAction> gitsAction = std::nullopt);
std::string& CommandListExecutions(std::optional<GitsAction> gitsAction = std::nullopt);
} // namespace gits::gui::config_options
