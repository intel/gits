// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "fileActions.h"

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <thread>
#include <numeric>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

#include "log.h"

namespace fs = std::filesystem;

// plog adaptor for process output
class ProcessOutputLogger {
public:
  static void log(const std::string& msg, std::function<void(const std::string&)> onOutput) {
    if (msg.empty()) {
      LOG_INFO << "Process output message is empty";
      return;
    }

    if (onOutput) {
      onOutput(msg);
    } else {
      LOG_INFO << msg;
    }
  };
};

namespace {
#ifdef _WIN32
bool LaunchExecutableWindows(
    const fs::path& executablePath,
    const std::vector<std::string>& arguments,
    bool waitForCompletion,
    const fs::path& workingDirectory,
    std::function<void(const std::string&)> onOutput,
    const std::unordered_map<std::string, std::string>& environmentOverrides,
    std::function<void(uint32_t)> onStarted = nullptr) {
  std::string cmdLine = "\"" + executablePath.string() + "\"";
  for (const auto& arg : arguments) {
    cmdLine += " " + arg;
  }

  if (!environmentOverrides.empty()) {
    LOG_DEBUG << "LaunchExecutableWindows got environment overrides but they are not yet supported "
                 "on Windows";
  }

  // Create pipes for stdout/stderr
  HANDLE hStdOutRead = nullptr, hStdOutWrite = nullptr;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

  if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
    return false;
  }
  SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si = {};
  PROCESS_INFORMATION pi = {};
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdOutput = hStdOutWrite;
  si.hStdError = hStdOutWrite;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  const char* workDir = nullptr;
  std::string workDirStr;
  if (!workingDirectory.empty()) {
    workDirStr = workingDirectory.string();
    workDir = workDirStr.c_str();
    LOG_INFO << "Setting working directory to: " << workingDirectory;
  } else {
    LOG_INFO << "Using current working directory: " << fs::current_path();
  }

  BOOL result = CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()), nullptr, nullptr,
                               TRUE, // inherit handles
                               0, nullptr, workDir, &si, &pi);

  // Close write ends in parent
  CloseHandle(hStdOutWrite);

  if (!result) {
    LOG_ERROR << "Failed to launch executable: " << executablePath;
    LOG_ERROR << "Error code: " << GetLastError();
    CloseHandle(hStdOutRead);
    return false;
  }

  LOG_INFO << "Launched: " << executablePath;
  if (onStarted) {
    onStarted(pi.dwProcessId);
  }

  // Read output
  char buffer[4096];
  DWORD bytesRead = 0;
  while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
    if (bytesRead >= sizeof(buffer)) {
      break;
    }
    const std::string msg(buffer, bytesRead);
    ProcessOutputLogger::log(msg, onOutput);
  }
  bool success = true;
  if (waitForCompletion) {
    DWORD exitCode = 0;
    success = WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0 &&
              GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == 0;
  }

  CloseHandle(hStdOutRead);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return success;
}
#else
bool LaunchExecutableUnix(const fs::path& executablePath,
                          const std::vector<std::string>& arguments,
                          bool waitForCompletion,
                          const fs::path& workingDirectory,
                          std::function<void(const std::string&)> onOutput,
                          const std::unordered_map<std::string, std::string>& environmentOverrides,
                          std::function<void(uint32_t)> onStarted = nullptr) {
  const bool captureOutput = static_cast<bool>(onOutput);
  std::string execPathStr = executablePath.string();
  std::vector<char*> args;
  args.push_back(const_cast<char*>(execPathStr.c_str()));
  for (const auto& arg : arguments) {
    args.push_back(const_cast<char*>(arg.c_str()));
  }
  args.push_back(nullptr);

  int pipefd[2] = {-1, -1};
  if (captureOutput && pipe(pipefd) != 0) {
    ProcessOutputLogger::log("Failed to create output pipe for: " + execPathStr, onOutput);
    return false;
  }

  pid_t pid = fork();
  if (pid == 0) {
    // Child
    if (captureOutput) {
      close(pipefd[0]); // close read end
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      close(pipefd[1]);
    }

    if (!workingDirectory.empty()) {
      if (chdir(workingDirectory.string().c_str()) != 0) {
        ProcessOutputLogger::log(
            "Failed to change working directory to: " + workingDirectory.string(), onOutput);
        _exit(126);
      }
    }

    for (const auto& [key, value] : environmentOverrides) {
      const int result =
          value.empty() ? unsetenv(key.c_str()) : setenv(key.c_str(), value.c_str(), 1);
      if (result != 0) {
        ProcessOutputLogger::log("Failed to set environment variable: " + key, onOutput);
        _exit(1);
      }
    }

    execv(execPathStr.c_str(), args.data());
    _exit(127);
  } else if (pid > 0) {
    // Parent
    if (onStarted) {
      onStarted(static_cast<uint32_t>(pid));
    }
    if (captureOutput) {
      close(pipefd[1]); // close write end
      char buffer[4096];
      ssize_t bytesRead;
      while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';

        ProcessOutputLogger::log(std::string(buffer), onOutput);
      }
      close(pipefd[0]);
    }

    if (waitForCompletion) {
      int status = 0;
      pid_t result;
      do {
        result = waitpid(pid, &status, 0);
      } while (result < 0 && errno == EINTR);
      return result == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    return true;
  } else {
    // Fork failed
    close(pipefd[0]);
    close(pipefd[1]);
    ProcessOutputLogger::log("Failed to fork process for: " + execPathStr, onOutput);
    return false;
  }
}
#endif
} // namespace

namespace FileActions {
// File operations
bool CopyFileSafe(const fs::path& source, const fs::path& destination) {
  try {
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
    LOG_VERBOSE << "Copied: " << source << " -> " << destination;
    return true;
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << "Error copying file: " << e.what();
    return false;
  }
}

bool CopyFilesSafe(const std::vector<fs::path>& sources, const fs::path& destinationDir) {
  bool allSucceeded = true;

  // Ensure destination directory exists
  if (!CreateDirectorySafe(destinationDir)) {
    LOG_ERROR << "Failed to create destination directory: " << destinationDir;
    return false;
  }

  for (const auto& source : sources) {
    if (!exists(source)) {
      LOG_WARNING << "Source file does not exist: " << source;
      allSucceeded = false;
      continue;
    }

    fs::path destination = destinationDir / source.filename();
    if (!CopyFileSafe(source, destination)) {
      allSucceeded = false;
    }
  }

  return allSucceeded;
}

bool CopyDirectorySafe(const fs::path& source, const fs::path& destination) {
  try {
    fs::copy(source, destination,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    LOG_INFO << "Copied directory: " << source << " -> " << destination;
    return true;
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << "Error copying directory: " << e.what();
    return false;
  }
}

bool CopyDirectoryContentsSafe(const std::filesystem::path& source,
                               const std::filesystem::path& destination,
                               const std::unordered_set<std::string>& excludedFilenames) {
  try {
    for (const auto& item : fs::directory_iterator(source)) {
      const auto& source_path = item.path();
      const auto filename = source_path.filename().string();
      if (excludedFilenames.count(filename) > 0) {
        LOG_DEBUG << "Skipping excluded file: " << filename;
        continue;
      }
      const auto destination_path = destination / source_path.filename();
      fs::copy(source_path, destination_path,
               fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }
    LOG_INFO << "Copied contents of directory: " << source << " -> " << destination;
    return true;
  } catch (const fs::filesystem_error& e) {
    LOG_INFO << "Error copying contents of directory: " << source << " Error: " << e.what();
    return false;
  }
}

bool DeleteFileSafe(const fs::path& filePath) {
  try {
    if (!exists(filePath)) {
      LOG_WARNING << "File does not exist: " << filePath;
      return true; // Consider non-existent file as successfully "deleted"
    }

    fs::remove(filePath);
    LOG_INFO << "Deleted file: " << filePath;
    return true;
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << "Error deleting file: " << e.what();
    return false;
  }
}

bool DeleteFilesSafe(const std::vector<fs::path>& filePaths) {
  bool allSucceeded = true;

  for (const auto& filePath : filePaths) {
    if (!DeleteFileSafe(filePath)) {
      allSucceeded = false;
    }
  }

  return allSucceeded;
}

bool DeleteDirectorySafe(const fs::path& directoryPath) {
  try {
    if (!exists(directoryPath)) {
      LOG_WARNING << "Directory does not exist: " << directoryPath;
      return true; // Consider non-existent directory as successfully "deleted"
    }

    std::uintmax_t removedCount = fs::remove_all(directoryPath);
    LOG_INFO << "Deleted directory and " << removedCount << " items: " << directoryPath;
    return true;
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << "Error deleting directory: " << e.what();
    return false;
  }
}

bool CreateDirectorySafe(const fs::path& path) {
  try {
    fs::create_directories(path);
    return true;
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << "Error creating directory: " << e.what();
    return false;
  }
}

bool Exists(const fs::path& path) {
  return fs::exists(path);
}

bool LaunchExecutable(const fs::path& executablePath,
                      const std::vector<std::string>& arguments,
                      bool waitForCompletion,
                      const fs::path& workingDirectory,
                      std::function<void(const std::string&)> onOutput,
                      const std::unordered_map<std::string, std::string>& environmentOverrides) {
#ifdef _WIN32
  return LaunchExecutableWindows(executablePath, arguments, waitForCompletion, workingDirectory,
                                 std::move(onOutput), environmentOverrides);
#else
  return LaunchExecutableUnix(executablePath, arguments, waitForCompletion, workingDirectory,
                              std::move(onOutput), environmentOverrides);
#endif
}

void LaunchExecutableAsync(
    const fs::path& executablePath,
    const std::vector<std::string>& arguments,
    const fs::path& workingDirectory,
    std::function<void(const std::string&)> onOutput,
    const std::unordered_map<std::string, std::string>& environmentOverrides) {
  auto sharedArgs = std::make_shared<std::vector<std::string>>(arguments);
  auto sharedEnvironmentOverrides =
      std::make_shared<std::unordered_map<std::string, std::string>>(environmentOverrides);
#ifdef _WIN32
  std::thread([executablePath, sharedArgs = std::move(sharedArgs),
               sharedEnvironmentOverrides = std::move(sharedEnvironmentOverrides), workingDirectory,
               onOutput = std::move(onOutput)]() {
    LaunchExecutableWindows(executablePath, *sharedArgs, false, workingDirectory,
                            std::move(onOutput), *sharedEnvironmentOverrides);
  }).detach();
#else
  std::thread([executablePath, sharedArgs = std::move(sharedArgs),
               sharedEnvironmentOverrides = std::move(sharedEnvironmentOverrides), workingDirectory,
               onOutput = std::move(onOutput)]() {
    LaunchExecutableUnix(executablePath, *sharedArgs, false, workingDirectory, std::move(onOutput),
                         *sharedEnvironmentOverrides);
  }).detach();
#endif
}

void LaunchExecutableThreadCallbackOnExit(
    const std::filesystem::path& executablePath,
    const std::vector<std::string>& arguments,
    const std::filesystem::path& workingDirectory,
    std::function<void(const std::string&)> onOutput,
    std::function<void()> callback,
    std::function<void(uint32_t)> onStarted,
    std::function<void(bool)> onResult,
    const std::unordered_map<std::string, std::string>& environmentOverrides) {
  auto sharedArgs = std::make_shared<std::vector<std::string>>(arguments);
  auto sharedEnvironmentOverrides =
      std::make_shared<std::unordered_map<std::string, std::string>>(environmentOverrides);
#ifdef _WIN32
  std::thread([executablePath, sharedArgs = std::move(sharedArgs), workingDirectory,
               onOutput = std::move(onOutput), callback = std::move(callback),
               onStarted = std::move(onStarted), onResult = std::move(onResult),
               sharedEnvironmentOverrides = std::move(sharedEnvironmentOverrides)]() {
    const bool success = LaunchExecutableWindows(executablePath, *sharedArgs, true,
                                                 workingDirectory, std::move(onOutput),
                                                 *sharedEnvironmentOverrides, std::move(onStarted));
    if (callback) {
      callback();
    }
    if (onResult) {
      onResult(success);
    }
  }).detach();
#else
  std::thread([executablePath, sharedArgs = std::move(sharedArgs), workingDirectory,
               onOutput = std::move(onOutput), callback = std::move(callback),
               onStarted = std::move(onStarted), onResult = std::move(onResult),
               sharedEnvironmentOverrides = std::move(sharedEnvironmentOverrides)]() {
    const bool success = LaunchExecutableUnix(executablePath, *sharedArgs, true, workingDirectory,
                                              std::move(onOutput), *sharedEnvironmentOverrides,
                                              std::move(onStarted));
    if (callback) {
      callback();
    }
    if (onResult) {
      onResult(success);
    }
  }).detach();
#endif
}
} // namespace FileActions
