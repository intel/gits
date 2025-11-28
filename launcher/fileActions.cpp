// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "fileActions.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <plog/Log.h>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

// plog adaptor for process output
class ProcessOutputLogger {
public:
  static void log(const std::string& msg, std::function<void(const std::string&)> onOutput) {
    if (msg.empty()) {
      PLOG_INFO << "Process output message is empty";
      return;
    }

    if (onOutput) {
      onOutput(msg);
    } else {
      PLOG_INFO << msg;
    }
  };
};

// File operations
bool FileActions::CopyFile(const fs::path& source, const fs::path& destination) {
  try {
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
    PLOG_INFO << "Copied: " << source << " -> " << destination;
    return true;
  } catch (const fs::filesystem_error& e) {
    PLOG_ERROR << "Error copying file: " << e.what();
    return false;
  }
}

bool FileActions::CopyFiles(const std::vector<fs::path>& sources, const fs::path& destinationDir) {
  bool allSucceeded = true;

  // Ensure destination directory exists
  if (!CreateDirectory(destinationDir)) {
    PLOG_ERROR << "Failed to create destination directory: " << destinationDir;
    return false;
  }

  for (const auto& source : sources) {
    if (!exists(source)) {
      PLOG_WARNING << "Source file does not exist: " << source;
      allSucceeded = false;
      continue;
    }

    fs::path destination = destinationDir / source.filename();
    if (!CopyFile(source, destination)) {
      allSucceeded = false;
    }
  }

  return allSucceeded;
}

bool FileActions::CopyDirectory(const fs::path& source, const fs::path& destination) {
  try {
    fs::copy(source, destination,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    PLOG_INFO << "Copied directory: " << source << " -> " << destination;
    return true;
  } catch (const fs::filesystem_error& e) {
    PLOG_ERROR << "Error copying directory: " << e.what();
    return false;
  }
}

bool FileActions::DeleteFile(const fs::path& filePath) {
  try {
    if (!exists(filePath)) {
      PLOG_WARNING << "File does not exist: " << filePath;
      return true; // Consider non-existent file as successfully "deleted"
    }

    fs::remove(filePath);
    PLOG_INFO << "Deleted file: " << filePath;
    return true;
  } catch (const fs::filesystem_error& e) {
    PLOG_ERROR << "Error deleting file: " << e.what();
    return false;
  }
}

bool FileActions::DeleteFiles(const std::vector<fs::path>& filePaths) {
  bool allSucceeded = true;

  for (const auto& filePath : filePaths) {
    if (!DeleteFile(filePath)) {
      allSucceeded = false;
    }
  }

  return allSucceeded;
}

bool FileActions::DeleteDirectory(const fs::path& directoryPath) {
  try {
    if (!exists(directoryPath)) {
      PLOG_WARNING << "Directory does not exist: " << directoryPath;
      return true; // Consider non-existent directory as successfully "deleted"
    }

    std::uintmax_t removedCount = fs::remove_all(directoryPath);
    PLOG_INFO << "Deleted directory and " << removedCount << " items: " << directoryPath;
    return true;
  } catch (const fs::filesystem_error& e) {
    PLOG_ERROR << "Error deleting directory: " << e.what();
    return false;
  }
}

bool FileActions::CreateDirectory(const fs::path& path) {
  try {
    fs::create_directories(path);
    return true;
  } catch (const fs::filesystem_error& e) {
    PLOG_ERROR << "Error creating directory: " << e.what();
    return false;
  }
}

bool FileActions::Exists(const fs::path& path) {
  return fs::exists(path);
}

bool FileActions::LaunchExecutable(const fs::path& executablePath,
                                   const std::vector<std::string>& arguments,
                                   bool waitForCompletion,
                                   const fs::path& workingDirectory,
                                   std::function<void(const std::string&)> onOutput) {
#ifdef _WIN32
  return LaunchExecutableWindows(executablePath, arguments, waitForCompletion, workingDirectory,
                                 onOutput);
#else
  return LaunchExecutableUnix(executablePath, arguments, waitForCompletion, workingDirectory,
                              onOutput);
#endif
}

void FileActions::LaunchExecutableAsync(const fs::path& executablePath,
                                        const std::vector<std::string>& arguments,
                                        const fs::path& workingDirectory,
                                        std::function<void(const std::string&)> onOutput) {
#ifdef _WIN32
  std::thread([=]() {
    LaunchExecutableWindows(executablePath, arguments, false, workingDirectory, onOutput);
  }).detach();
#else
  std::thread([=]() {
    LaunchExecutableUnix(executablePath, arguments, false, workingDirectory, onOutput);
  }).detach();
#endif
}

#ifdef _WIN32
bool FileActions::LaunchExecutableWindows(const fs::path& executablePath,
                                          const std::vector<std::string>& arguments,
                                          bool waitForCompletion,
                                          const fs::path& workingDirectory,
                                          std::function<void(const std::string&)> onOutput) {
  std::string cmdLine = "\"" + executablePath.string() + "\"";
  for (const auto& arg : arguments) {
    cmdLine += " \"" + arg + "\"";
  }

  // Create pipes for stdout/stderr
  HANDLE hStdOutRead = nullptr, hStdOutWrite = nullptr;
  HANDLE hStdErrRead = nullptr, hStdErrWrite = nullptr;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

  CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
  CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0);

  STARTUPINFOA si = {};
  PROCESS_INFORMATION pi = {};
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdOutput = hStdOutWrite;
  si.hStdError = hStdErrWrite;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  const char* workDir = nullptr;
  std::string workDirStr;
  if (!workingDirectory.empty()) {
    workDirStr = workingDirectory.string();
    workDir = workDirStr.c_str();
    PLOG_INFO << "Setting working directory to: " << workingDirectory;
  } else {
    PLOG_INFO << "Using current working directory: " << fs::current_path();
  }

  BOOL result = CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()), nullptr, nullptr,
                               TRUE, // inherit handles
                               0, nullptr, workDir, &si, &pi);

  // Close write ends in parent
  CloseHandle(hStdOutWrite);
  CloseHandle(hStdErrWrite);

  if (!result) {
    PLOG_ERROR << "Failed to launch executable: " << executablePath;
    PLOG_ERROR << "Error code: " << GetLastError();
    CloseHandle(hStdOutRead);
    CloseHandle(hStdErrRead);
    return false;
  }

  PLOG_INFO << "Launched: " << executablePath;

  // Read output
  char buffer[4096];
  DWORD bytesRead;
  while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
    const std::string msg(buffer, bytesRead);
    PLOG_INFO << "!" << msg;
    ProcessOutputLogger::log(msg, onOutput);
  }
  while (ReadFile(hStdErrRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
    const std::string msg(buffer, bytesRead);
    ProcessOutputLogger::log(msg, onOutput);
  }

  if (waitForCompletion) {
    WaitForSingleObject(pi.hProcess, INFINITE);
  }

  CloseHandle(hStdOutRead);
  CloseHandle(hStdErrRead);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}
#else
bool FileActions::LaunchExecutableUnix(const fs::path& executablePath,
                                       const std::vector<std::string>& arguments,
                                       bool waitForCompletion,
                                       const fs::path& workingDirectory,
                                       std::function<void(const std::string&)> onOutput) {
  std::string execPathStr = executablePath.string();
  std::vector<char*> args;
  args.push_back(const_cast<char*>(execPathStr.c_str()));
  for (const auto& arg : arguments) {
    args.push_back(const_cast<char*>(arg.c_str()));
  }
  args.push_back(nullptr);

  int pipefd[2];
  pipe(pipefd);

  pid_t pid = fork();
  if (pid == 0) {
    // Child
    close(pipefd[0]); // close read end
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    if (!workingDirectory.empty()) {
      if (chdir(workingDirectory.string().c_str()) != 0) {
        ProcessOutputLogger::log(
            "Failed to change working directory to: " + workingDirectory.string(), onOutput);
        exit(1);
      }
    }

    execv(execPathStr.c_str(), args.data());
    ProcessOutputLogger::log("Failed to execute: " + execPathStr, onOutput);
    exit(1);
  } else if (pid > 0) {
    // Parent
    close(pipefd[1]); // close write end
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytesRead] = '\0';

      ProcessOutputLogger::log(std::string(buffer), onOutput);
    }
    close(pipefd[0]);

    if (waitForCompletion) {
      int status;
      waitpid(pid, &status, 0);
    }
    return true;
  } else {
    // Fork failed
    ProcessOutputLogger::log("Failed to fork process for: " + execPathStr, onOutput);
    return false;
  }
}
#endif
