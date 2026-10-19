// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <numeric>
#include <fstream>

#include <yaml-cpp/yaml.h>

#include "log.h"

namespace FileActions {

// Forward declaration for template function
template <typename T>
bool SetYamlPathValue(YAML::Node node,
                      const std::vector<std::string>& path,
                      const T value,
                      const bool addIfNotPresent = false,
                      const size_t index = 0);

bool CopyFileSafe(const std::filesystem::path& source, const std::filesystem::path& destination);
bool CopyFilesSafe(const std::vector<std::filesystem::path>& sources,
                   const std::filesystem::path& destinationDir);
bool CopyDirectorySafe(const std::filesystem::path& source,
                       const std::filesystem::path& destination);
bool CopyDirectoryContentsSafe(const std::filesystem::path& source,
                               const std::filesystem::path& destination,
                               const std::unordered_set<std::string>& excludedFilenames = {});
bool DeleteFileSafe(const std::filesystem::path& filePath);
bool DeleteFilesSafe(const std::vector<std::filesystem::path>& filePaths);
bool DeleteDirectorySafe(const std::filesystem::path& directoryPath);
bool CreateDirectorySafe(const std::filesystem::path& path);
bool Exists(const std::filesystem::path& path);

bool LaunchExecutable(
    const std::filesystem::path& executablePath,
    const std::vector<std::string>& arguments = {},
    bool waitForCompletion = false,
    const std::filesystem::path& workingDirectory = "",
    std::function<void(const std::string&)> onOutput = nullptr,
    const std::unordered_map<std::string, std::string>& environmentOverrides = {});

void LaunchExecutableAsync(
    const std::filesystem::path& executablePath,
    const std::vector<std::string>& arguments = {},
    const std::filesystem::path& workingDirectory = "",
    std::function<void(const std::string&)> onOutput = nullptr,
    const std::unordered_map<std::string, std::string>& environmentOverrides = {});

// Launches an executable as blocking but in a separate thread, so that we can easily know when the execution ends without holding up the main thread
void LaunchExecutableThreadCallbackOnExit(
    const std::filesystem::path& executablePath,
    const std::vector<std::string>& arguments = {},
    const std::filesystem::path& workingDirectory = "",
    std::function<void(const std::string&)> onOutput = nullptr,
    std::function<void()> callback = nullptr,
    std::function<void(uint32_t)> onStarted = nullptr,
    std::function<void(bool)> onResult = nullptr,
    const std::unordered_map<std::string, std::string>& environmentOverrides = {});

template <typename T>
bool UpdateConfigYamlPath(const std::filesystem::path configPath,
                          const std::vector<std::string> yamlPath,
                          const T value,
                          const bool addIfNotPresent = false) {
  if (configPath.empty()) {
    LOG_ERROR << "Couldn't update gits config. Provided config path is empty.";

    return false;
  }

  if (!std::filesystem::exists(configPath)) {
    LOG_ERROR << "Couldn't update gits config. Provided config path doesn't exist.";

    return false;
  }

  if (yamlPath.empty()) {
    LOG_ERROR << "Couldn't update gits config. YAML path is empty.";

    return false;
  }

  try {
    std::ifstream file(configPath);
    if (!file.is_open()) {
      LOG_ERROR << "Couldn't update gits config. Couldn't open the config file for reading.";

      return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    YAML::Node yaml = YAML::Load(content);

    if (!SetYamlPathValue<T>(yaml, yamlPath, value, addIfNotPresent)) {
      auto joined =
          std::accumulate(std::next(yamlPath.begin()), yamlPath.end(), yamlPath[0],
                          [](const std::string& a, const std::string& b) { return a + "." + b; });

      LOG_ERROR << "Couldn't update gits config. YAML path: " << joined << " doesn't exist";

      return false;
    }

    // Write the modified YAML back to the file
    std::ofstream outFile(configPath);
    if (!outFile.is_open()) {
      LOG_ERROR
          << "Error updating the capture output path. Couldn't open the config file for writing.";
      return false;
    }

    YAML::Emitter emitter;
    emitter << yaml;

    outFile << emitter.c_str();
    if (outFile.fail()) {
      LOG_ERROR << "Couldn't update gits config. Failed writing to file.";

      return false;
    }

    outFile.close();

  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't update gits config. Error: " << e.what();

    return false;
  }

  return true;
}

// Helper function
// Since the YAML::Node type is kind of like a pointer
// Modifying the YAML by the given path is tricky
// Because of how the assignment operator works, the YAML tree could get corrupted
// That's why recursion is used here
template <typename T>
bool SetYamlPathValue(YAML::Node node,
                      const std::vector<std::string>& path,
                      const T value,
                      const bool addIfNotPresent,
                      const size_t index) {
  if (index == path.size() - 1) {
    if (!node[path[index]] && !addIfNotPresent) {
      return false;
    }
    node[path[index]] = value;

    return true;
  }

  if (!node[path[index]]) {
    if (addIfNotPresent) {
      node[path[index]] = YAML::Node();
    } else {
      return false;
    }
  }

  return SetYamlPathValue<T>(node[path[index]], path, value, addIfNotPresent, index + 1);
}
}; // namespace FileActions
