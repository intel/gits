// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools.cpp
 *
 * @brief Common tools for GITS project.
 *
 */

#include "tools.h"
#include "xxhash.h"

#include <cstdint>
#include <fstream>

#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace gits {

uint64_t ComputeHash(const void* data, size_t size) {
  union u32u64 {
    uint64_t u64;
    uint32_t u32[2];
  };

  u32u64 val;
  val.u32[0] = XXH32(data, size, 0);
  val.u32[1] = static_cast<uint32_t>(size);
  return val.u64;
}

std::string CommandOutput(const std::string& command, bool isRecorder) {
#ifdef GITS_PLATFORM_WINDOWS
  // Windows can't handle popen correctly in non-console applications.
  if (isRecorder) {
    return "<couldn't exec: " + command + ">";
  }
#define popen  _popen
#define pclose _pclose
#endif
  std::string content;
  char buffer[256];

  errno = 0;
  FILE* out = popen(command.c_str(), "r");
  if (out == nullptr) {
    const auto msg = "Couldn't open process, errno: " + std::to_string(errno);
    throw std::runtime_error(std::string(EXCEPTION_MESSAGE) + msg);
  }
  while (!feof(out)) {
    // Linux generates warning for not using the return value of fgets function.
    if (fgets(buffer, sizeof(buffer), out) != nullptr) {
      content += buffer;
    }
  }
  pclose(out);

  return content;
}

#ifdef GITS_PLATFORM_WINDOWS
std::string GetWindowsProcessName(int processID) {
  std::string processName;
  HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
  if (handle) {
    DWORD buffSize = 1024;
    CHAR buffer[1024];
    BOOL retVal = QueryFullProcessImageNameA(handle, 0, buffer, &buffSize);
    CloseHandle(handle);
    if (retVal == false) {
      LOG_ERROR << "Can't get name of the process.";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    processName = buffer;
  } else {
    LOG_ERROR << "Can't open handle to the process.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  std::string::size_type startNamePos = processName.find_last_of("/\\");
  processName = processName.substr(startNamePos + 1);
  std::string::size_type endNamePos = processName.find_last_of('.');
  processName = processName.substr(0, endNamePos);
  return processName;
}
#endif

#ifdef GITS_PLATFORM_LINUX
std::string GetLinuxProcessName(pid_t processID) {
  std::string processName;
  std::ifstream file("/proc/" + std::to_string(processID) + "/comm");
  file >> processName;
  return processName;
}
#endif

} //namespace gits
