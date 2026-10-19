// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <optional>
#include "baseCommon.h"
#include "baseEventBus.h"
#include "configurationAuto.h"

namespace gits::gui::gui_actions {
void RestoreOriginalConfigIfNeeded(const TemporaryConfigInfo& tmpConfigInfo);
bool IsValidGITSBasePath(const std::filesystem::path& path);
bool DetectBasePaths();
void ResetBasePaths();

void LoadConfigFile(GitsAction gitsAction);
void LoadConfigFromMemory(GitsAction gitsAction);

std::optional<PathEvent> FileDialogs();
void ShowFileDialog(FileDialogKey key, const std::filesystem::path& path = "");

void PlaybackStreamAsync(const std::filesystem::path streamPath,
                         const Configuration& configuration,
                         std::function<void(bool)> onFinished = nullptr,
                         std::function<void(const std::string&)> onOutput = nullptr);
void AnalyzeStreamAsync(const std::filesystem::path streamPath,
                        const std::filesystem::path outputPath,
                        Configuration configuration,
                        bool generateScreenshots,
                        bool generateTrace = false);

std::string CreateEmailBodyWithLog(const std::string& bodyText, const std::string& logText);
void SendEmail(const std::string& recipient, const std::string& subject, const std::string& body);
void SendLogByEmail(const std::string& recipient,
                    const std::string& subject,
                    const std::string& body);

void OpenURL(const std::string& url);
bool OpenFolder(const std::filesystem::path& path);
bool OpenFolder(const std::string& path);

//void SubcaptureStream(streamPath, configuration, outputPath);
//void CaptureTarget(targetPath, configuration, outputPath);
} // namespace gits::gui::gui_actions
