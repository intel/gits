// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "common.h"

#include <windows.h>

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

std::ofstream& Log() {
  static std::ofstream logFile{"ipc_log.txt"};
  return logFile;
}

void waitForProcessToEnd(DWORD processID, std::atomic<bool>* ended) {
  while (true) {
    HANDLE hProcess =
        OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
    if (!hProcess) {
      break;
    }
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
      if (exitCode != STILL_ACTIVE) {
        CloseHandle(hProcess);
        break;
      }
    }
    WaitForSingleObject(hProcess, 10);
    CloseHandle(hProcess);
  }
  *ended = true;
}

int main2(const std::string& filepath,
          const DWORD processId,
          const std::string& eventName,
          const std::string& sharedMemoryName) {
  HANDLE hEvent = CreateEventA(nullptr, FALSE, FALSE, eventName.c_str());
  if (hEvent == nullptr) {
    Log() << "Could not create event: " << GetLastError() << std::endl;
    return EXIT_FAILURE;
  }

  HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                       SHARED_MEMORY_SIZE, sharedMemoryName.c_str());
  if (hMapFile == nullptr) {
    Log() << "Could not create file mapping object: " << GetLastError() << std::endl;
    CloseHandle(hEvent);
    return EXIT_FAILURE;
  }

  LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
  if (pBuf == nullptr) {
    Log() << "Could not map view of file: " << GetLastError() << std::endl;
    CloseHandle(hMapFile);
    CloseHandle(hEvent);
    return EXIT_FAILURE;
  }

  memset(pBuf, 0, SHARED_MEMORY_SIZE);

  SharedCircularBuffer* sharedBuffer = (SharedCircularBuffer*)pBuf;
  sharedBuffer->head = 1;

  std::ofstream file{filepath, std::ios::binary};
  std::vector<char> writeBuffer(SHARED_MEMORY_SIZE, '\0');

  std::atomic<bool> ended;
  std::thread thread{waitForProcessToEnd, processId, &ended};

  size_t numEmptyReads = 0;
  constexpr size_t numEmptyReadsToSleep = 1000;

  SetEvent(hEvent);

  while (true) {
    size_t bytes = read(sharedBuffer, writeBuffer.data(), SHARED_MEMORY_SIZE);
    if (bytes) {
      file.write(writeBuffer.data(), bytes);
      numEmptyReads = 0;
    } else if (ended) {
      break;
    } else {
      ++numEmptyReads;
      if (numEmptyReads >= numEmptyReadsToSleep) {
        if (file.rdbuf()->in_avail()) {
          file.flush();
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        numEmptyReads = numEmptyReadsToSleep;
      }
    }
  }

  file.close();
  if (thread.joinable()) {
    thread.join();
  }
  UnmapViewOfFile(pBuf);
  CloseHandle(hMapFile);
  CloseHandle(hEvent);
  return EXIT_SUCCESS;
}

void topmost_exception_handler(const char* funcName) {
  char msg[1024];

  try {
    throw;
  } catch (std::exception& ex) {
    sprintf(msg, "Unhandled exception caught in %s: %s", funcName, ex.what());
  } catch (...) {
    sprintf(msg, "Unhandled exception caught in %s", funcName);
  } // No GITS exception support; we don't want to include exception.h/log.h

  try {
    Log() << "Err: " << msg;
  } catch (...) {
    fprintf(stderr, "%s: Exception during handling exception caught in %s:\n     %s\n",
            __FUNCTION__, funcName, msg);
  }

  std::abort();
}

int main(int argc, char* argv[]) {
  try {
    if (argc != 5) {
      Log() << "Usage: " << argv[0] << " <filepath> <processId> <eventName> <sharedMemoryName>"
            << std::endl;
      return EXIT_FAILURE;
    }

    // We could potentially use PATH_MAX/MAX_PATH instead, but on Windows it's complicated.
    const size_t MAX_ARG_SIZE = 50000; // A generous limit.
    for (size_t i = 1; i < argc; i++) {
      size_t size = strnlen(argv[i], MAX_ARG_SIZE);
      if (size > MAX_ARG_SIZE) {
        Log() << "Argument number " << i << " is too long (over " << MAX_ARG_SIZE << " characters)"
              << std::endl;
        return EXIT_FAILURE;
      }
    }

    const std::string filepath = argv[1];
    const DWORD processId = std::stoi(argv[2]);
    const std::string eventName = argv[3];
    const std::string sharedMemoryName = argv[4];

    return main2(filepath, processId, eventName, sharedMemoryName);
  } catch (...) {
    topmost_exception_handler("main");
  }
}
