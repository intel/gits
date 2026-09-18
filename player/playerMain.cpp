// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#include "log.h"
#include "playerLib.h"

#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#if defined GITS_PLATFORM_LINUX
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <filesystem>
#include <vector>
#include <string>

namespace gits {

#if defined GITS_PLATFORM_WINDOWS
int RelaunchWindows(const std::filesystem::path& newPlayerPath,
                    const std::vector<std::string>& args) {
  std::string cmdLine = "\"" + newPlayerPath.string() + "\"";
  for (const auto& arg : args) {
    cmdLine += " \"" + arg + "\"";
  }

  STARTUPINFOA si{};
  PROCESS_INFORMATION pi{};
  si.cb = sizeof(si);

  BOOL success = CreateProcessA(nullptr,        // lpApplicationName
                                cmdLine.data(), // lpCommandLine (needs to be mutable)
                                nullptr,        // lpProcessAttributes
                                nullptr,        // lpThreadAttributes
                                FALSE,          // bInheritHandles
                                0,              // dwCreationFlags
                                nullptr,        // lpEnvironment
                                nullptr,        // lpCurrentDirectory
                                &si,            // lpStartupInfo
                                &pi             // lpProcessInformation
  );

  if (!success) {
    LOG_ERROR << "Failed to create renamed player executable process. Error: " << GetLastError();
    return 1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return static_cast<int>(exitCode);
};
#endif

#if defined GITS_PLATFORM_LINUX
int RelaunchLinux(const std::filesystem::path& newPlayerPath,
                  const std::vector<std::string>& args) {
  std::vector<char*> argv;                           // Execv needs char* array
  auto newPlayerPathString = newPlayerPath.string(); // We need a copy for lifetime purpose
  argv.push_back(newPlayerPathString.data());

  for (const auto& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    execv(newPlayerPathString.c_str(), argv.data());
    // If we get here, execv failed
    _exit(1);
  } else if (pid > 0) {
    // Parent process - wait for child
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
  } else {
    LOG_ERROR << "Failed to relaunch the renamed player executable. Failed to fork process";
    return 1;
  }
}
#endif

int RenameAndRelaunch(const std::string& newPlayerName,
                      std::filesystem::path originalPlayerPath,
                      std::vector<std::string> args) {

#if defined GITS_PLATFORM_WINDOWS
  if (originalPlayerPath.extension() != ".exe") {
    originalPlayerPath += ".exe";
  }
#endif
  std::filesystem::path newPlayerPath = originalPlayerPath.parent_path() / newPlayerName;

  std::filesystem::copy_file(originalPlayerPath, newPlayerPath,
                             std::filesystem::copy_options::overwrite_existing);

  int result = EXIT_FAILURE;
#if defined GITS_PLATFORM_WINDOWS
  result = RelaunchWindows(newPlayerPath, args);
#endif
#if defined GITS_PLATFORM_LINUX
  result = RelaunchLinux(newPlayerPath, args);
#endif

  LOG_INFO << "Removing the renamed player executable: " << newPlayerPath;
  try {
    std::filesystem::remove(newPlayerPath);
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Failed to remove the renamed player executable. Error: " << e.what();
  }

  return result;
}

} // namespace gits

int main(int argc, char* argv[]) {

#ifdef GITS_PLATFORM_WINDOWS
  // Prevent OS from scaling our windows.
  SetProcessDPIAware();
#ifdef _NDEBUG
  // This is a workaround for older fullscreen streams that most probably cause
  // heap corruption without noticeable effect until program termination - after
  // leaving main, during process cleanup access violation results I'll leave
  // this in debug builds - as this should not happen for new streams
  SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_NOGPFAULTERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT |
               SEM_FAILCRITICALERRORS);
#endif
#endif

  int ret = Initialize(argc, argv);
  if (ret) {
    return ret;
  }

  std::filesystem::path playerPath = "";
  auto argsVector = std::vector<std::string>(argv, argv + argc);
  if (argsVector.size() >= 1) {
    playerPath = argsVector[0];
    argsVector.erase(argsVector.begin());
  }

  auto trimExtension = [](std::string name) {
    constexpr std::string_view ext = ".exe";
    if (name.size() > ext.size() &&
        std::equal(ext.rbegin(), ext.rend(), name.rbegin(), [](char a, char b) {
          return std::tolower(static_cast<unsigned char>(a)) ==
                 std::tolower(static_cast<unsigned char>(b));
        })) {
      name.resize(name.size() - ext.size());
    }
    return name;
  };

  std::string requestedPlayerName = GetApplicationName();
  if (!requestedPlayerName.empty()) {
    if (trimExtension(playerPath.filename().string()) == trimExtension(requestedPlayerName)) {
      LOG_INFO << "Player name matches requested name.";
    } else {
      LOG_INFO << "Player name differs from the requested name, Player will be renamed and "
                  "relaunched.";
      return gits::RenameAndRelaunch(requestedPlayerName, std::filesystem::absolute(playerPath),
                                     std::move(argsVector));
    }
  }

  return Play();
}
