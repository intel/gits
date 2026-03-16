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

#include "context.h"
#include "log.h"
#include <numeric>

class FileActions {
public:
  static bool CopyFile(const std::filesystem::path& source,
                       const std::filesystem::path& destination);
  static bool CopyFiles(const std::vector<std::filesystem::path>& sources,
                        const std::filesystem::path& destinationDir);
  static bool CopyDirectory(const std::filesystem::path& source,
                            const std::filesystem::path& destination);
  static bool CopyDirectoryContents(const std::filesystem::path& source,
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

  // Launches an executable as blocking but in a separate thread, so that we can easily know when the execution ends without holding up the main thread
  static void LaunchExecutableThreadCallbackOnExit(
      const std::filesystem::path& executablePath,
      const std::vector<std::string>& arguments = {},
      const std::filesystem::path& workingDirectory = "",
      std::function<void(const std::string&)> onOutput = nullptr,
      std::function<void()> callback = nullptr);

  template <typename T>
  static bool UpdateConfigYamlPath(std::filesystem::path configPath,
                                   std::vector<std::string> yamlPath,
                                   T value,
                                   bool addIfNotPresent = false) {
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

  // Helper function
  // Since the YAML::Node type is kind of like a pointer
  // Modifying the YAML by the given path is tricky
  // Because of how the assignment operator works, the YAML tree could get corrupted
  // That's why recursion is used here
  template <typename T>
  static bool SetYamlPathValue(YAML::Node node,
                               const std::vector<std::string>& path,
                               T value,
                               bool addIfNotPresent = false,
                               size_t index = 0) {
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
};
