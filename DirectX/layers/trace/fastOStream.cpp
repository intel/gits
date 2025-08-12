// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "fastOStream.h"
#include "log2.h"
#include "configurationLib.h"
#include "ipc/common.h"
#include <filesystem>
#include <Windows.h>
#include <thread>

namespace gits {
namespace DirectX {

void FastOStringStream::open(const std::string& filePath,
                             const FlushMethod flushMethod,
                             const size_t bufferCapacity) {
  if (isOpen_) {
    close();
  }

  filePath_ = filePath;
  flushMethod_ = flushMethod;
  bufferCapacity_ = bufferCapacity;
  forcedFlushThreshold_ = bufferCapacity_ * forcedFlushCapacityRatio;

  bufferStreamBuffer_.resize(bufferCapacity_);
  if (flushMethod == FlushMethod::File) {
    bufferStreamFile_.open(filePath_);
  }
  bufferStream_ = fast_io::obuffer_view(bufferStreamBuffer_);

  isOpen_ = true;
}

void FastOStringStream::close() {
  isOpen_ = false;

  flush();
  bufferStream_.clear();
  bufferStreamFile_.close();
  bufferStreamBuffer_.clear();
  filePath_.clear();

  if (flushInfo_.pBuf) {
    UnmapViewOfFile(flushInfo_.pBuf);
  }
  if (flushInfo_.hMapFile) {
    CloseHandle(flushInfo_.hMapFile);
  }
  if (flushInfo_.hEvent) {
    CloseHandle(flushInfo_.hEvent);
  }
  flushInfo_.initialized = false;
}

void FastOStringStream::flush() {
  if (flushMethod_ == FlushMethod::Ipc) {
    initializeIpcFlush();
    writeAll(reinterpret_cast<SharedCircularBuffer*>(flushInfo_.pBuf), bufferStream_.cbegin(),
             bufferStream_.size());
    bufferStream_.clear();
  } else {
    bufferStreamFile_.write(bufferStream_.cbegin(), bufferStream_.size());
    bufferStreamFile_.flush();
    bufferStream_.clear();
  }
}

void FastOStringStream::initializeIpcFlush() {
  if (flushInfo_.initialized) {
    return;
  }

  auto executablePath =
      std::filesystem::path(Configurator::Get().common.recorder.installPath).parent_path() /
      "UtilityTools" / "DirectX_trace_ipc.exe";
  if (!std::filesystem::exists(executablePath)) {
    std::vector<char> moduleFilename(MAX_PATH + 1, 0);
    GetModuleFileNameA(nullptr, moduleFilename.data(), moduleFilename.size());

    // Try to load the trace executable from the directory next to the player
    auto playerPath = std::filesystem::absolute(moduleFilename.data());
    executablePath = std::filesystem::absolute(playerPath.parent_path() / "UtilityTools" /
                                               "DirectX_trace_ipc.exe");
    if (!std::filesystem::exists(executablePath)) {
      // Try to load the trace executable from the player's parent directory
      executablePath = std::filesystem::absolute(playerPath.parent_path().parent_path() /
                                                 "UtilityTools" / "DirectX_trace_ipc.exe");
    }
  }

  if (!std::filesystem::exists(executablePath)) {
    LOG_ERROR << "Could not locate the DirectX_trace_ipc.exe";
    return;
  }

  const std::string eventName =
      eventBaseName + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const std::string sharedMemoryName =
      sharedMemoryBaseName +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  std::stringstream ss;
  ss << '"' << executablePath.string() << "\" \"" << filePath_ << "\" " << GetCurrentProcessId()
     << " \"" << eventName << "\" \"" << sharedMemoryName << '"';

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  SECURITY_ATTRIBUTES securityAttributes;
  ZeroMemory(&securityAttributes, sizeof(securityAttributes));
  securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  securityAttributes.bInheritHandle = FALSE;

  const std::string& commandLine = ss.str();

  if (!CreateProcess(NULL, (char*)commandLine.c_str(), &securityAttributes, &securityAttributes,
                     FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
    LOG_ERROR << "CreateProcess failed: " << GetLastError();
    return;
  }

  flushInfo_.hEvent = OpenEventA(SYNCHRONIZE, FALSE, eventName.c_str());
  size_t numOfTries = 300;
  while (!flushInfo_.hEvent) {
    flushInfo_.hEvent = OpenEventA(SYNCHRONIZE, FALSE, eventName.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (--numOfTries == 0) {
      LOG_ERROR << "Could not open event: " << GetLastError();
      LOG_ERROR << "Ipc process failed to run, check ipc_log.txt";
      return;
    }
  }

  if (WaitForSingleObject(flushInfo_.hEvent, 3000) == WAIT_TIMEOUT) {
    LOG_ERROR << "WaitForSingleObject timed out";
    LOG_ERROR << "Ipc process failed to run, check ipc_log.txt";
    return;
  }

  flushInfo_.hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedMemoryName.c_str());
  if (flushInfo_.hMapFile == nullptr) {
    LOG_ERROR << "Could not open file mapping object: " << GetLastError();
    CloseHandle(flushInfo_.hEvent);
    return;
  }

  flushInfo_.pBuf =
      MapViewOfFile(flushInfo_.hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
  if (flushInfo_.pBuf == nullptr) {
    LOG_ERROR << "Could not map view of file: " << GetLastError();
    CloseHandle(flushInfo_.hMapFile);
    CloseHandle(flushInfo_.hEvent);
    return;
  }

  flushInfo_.initialized = true;
}

void FastOFileStream::flush() {
  fast_io::flush(fileStream_);
}

} // namespace DirectX
} // namespace gits
