// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <vector>
#include <string>

#include "context.h"

class FileActions {
public:
  static bool CopyFile(const std::filesystem::path& source,
                       const std::filesystem::path& destination);
  static bool CopyFiles(const std::vector<std::filesystem::path>& sources,
                        const std::filesystem::path& destinationDir);
  static bool CopyDirectory(const std::filesystem::path& source,
                            const std::filesystem::path& destination);
  static bool DeleteFile(const std::filesystem::path& filePath);
  static bool DeleteFiles(const std::vector<std::filesystem::path>& filePaths);
  static bool DeleteDirectory(const std::filesystem::path& directoryPath);
  static bool CreateDirectory(const std::filesystem::path& path);
  static bool Exists(const std::filesystem::path& path);

  static bool LaunchExecutable(const std::filesystem::path& executablePath,
                               const std::vector<std::string>& arguments = {},
                               bool waitForCompletion = false,
                               const std::filesystem::path& workingDirectory = "",
                               std::function<void(const std::string&)> onOutput = nullptr);

  static void LaunchExecutableAsync(const std::filesystem::path& executablePath,
                                    const std::vector<std::string>& arguments = {},
                                    const std::filesystem::path& workingDirectory = "",
                                    std::function<void(const std::string&)> onOutput = nullptr);

private:
#ifdef _WIN32
  static bool LaunchExecutableWindows(const std::filesystem::path& executablePath,
                                      const std::vector<std::string>& arguments,
                                      bool waitForCompletion,
                                      const std::filesystem::path& workingDirectory,
                                      std::function<void(const std::string&)> onOutput);
#else
  static bool LaunchExecutableUnix(const std::filesystem::path& executablePath,
                                   const std::vector<std::string>& arguments,
                                   bool waitForCompletion,
                                   const std::filesystem::path& workingDirectory,
                                   std::function<void(const std::string&)> onOutput);
#endif
};
