// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "sharedCircularBuffer.h"

#include <fcntl.h>
#include <semaphore.h>
#include <csignal>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

std::ofstream& Log() {
  static std::ofstream logFile{"ipc_log.txt"};
  return logFile;
}

bool IsProcessAlive(pid_t pid) {
  return kill(pid, 0) == 0 || errno != ESRCH;
}

void WaitForProcessToEnd(pid_t processID, std::atomic<bool>* ended) {
  while (IsProcessAlive(processID)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  *ended = true;
}

int MainImpl(const std::string& filepath,
             const pid_t processId,
             const std::string& semName,
             const std::string& sharedMemoryName) {
  sem_t* sem = sem_open(semName.c_str(), O_CREAT, 0666, 0);
  if (sem == SEM_FAILED) {
    Log() << "Could not create semaphore: " << strerror(errno) << std::endl;
    return EXIT_FAILURE;
  }

  int shmFd = shm_open(sharedMemoryName.c_str(), O_CREAT | O_RDWR, 0666);
  if (shmFd == -1) {
    Log() << "Could not create shared memory: " << strerror(errno) << std::endl;
    sem_close(sem);
    sem_unlink(semName.c_str());
    return EXIT_FAILURE;
  }

  if (ftruncate(shmFd, SHARED_MEMORY_SIZE) == -1) {
    Log() << "Could not set shared memory size: " << strerror(errno) << std::endl;
    close(shmFd);
    shm_unlink(sharedMemoryName.c_str());
    sem_close(sem);
    sem_unlink(semName.c_str());
    return EXIT_FAILURE;
  }

  void* pBuf = mmap(nullptr, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
  if (pBuf == MAP_FAILED) {
    Log() << "Could not mmap shared memory: " << strerror(errno) << std::endl;
    close(shmFd);
    shm_unlink(sharedMemoryName.c_str());
    sem_close(sem);
    sem_unlink(semName.c_str());
    return EXIT_FAILURE;
  }

  memset(pBuf, 0, SHARED_MEMORY_SIZE);

  auto* sharedBuffer = static_cast<SharedCircularBuffer*>(pBuf);
  sharedBuffer->Head = 1;

  std::ofstream file{filepath, std::ios::binary};
  std::vector<char> writeBuffer(SHARED_MEMORY_SIZE, '\0');

  std::atomic<bool> ended{false};
  std::thread thread{WaitForProcessToEnd, processId, &ended};

  size_t numEmptyReads = 0;
  constexpr size_t numEmptyReadsToSleep = 1000;

  sem_post(sem);

  while (true) {
    size_t bytes = Read(sharedBuffer, writeBuffer.data(), SHARED_MEMORY_SIZE);
    if (bytes) {
      file.write(writeBuffer.data(), bytes);
      numEmptyReads = 0;
    } else if (ended) {
      break;
    } else {
      ++numEmptyReads;
      if (numEmptyReads >= numEmptyReadsToSleep) {
        file.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        numEmptyReads = numEmptyReadsToSleep;
      }
    }
  }

  file.close();
  if (thread.joinable()) {
    thread.join();
  }
  munmap(pBuf, SHARED_MEMORY_SIZE);
  close(shmFd);
  shm_unlink(sharedMemoryName.c_str());
  sem_close(sem);
  sem_unlink(semName.c_str());
  return EXIT_SUCCESS;
}

void TopmostExceptionHandler(const char* funcName) {
  char msg[1024];

  try {
    throw;
  } catch (std::exception& ex) {
    snprintf(msg, sizeof(msg), "Unhandled exception caught in %s: %s", funcName, ex.what());
  } catch (...) {
    snprintf(msg, sizeof(msg), "Unhandled exception caught in %s", funcName);
  }

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
      Log() << "Usage: " << argv[0] << " <filepath> <processId> <semName> <sharedMemoryName>"
            << std::endl;
      return EXIT_FAILURE;
    }

    const size_t MAX_ARG_SIZE = 50000;
    for (int i = 1; i < argc; i++) {
      size_t size = strnlen(argv[i], MAX_ARG_SIZE);
      if (size >= MAX_ARG_SIZE) {
        Log() << "Argument number " << i << " is too long (over " << MAX_ARG_SIZE << " characters)"
              << std::endl;
        return EXIT_FAILURE;
      }
    }

    const std::string filepath = argv[1];
    const pid_t processId = std::stoi(argv[2]);
    const std::string semName = argv[3];
    const std::string sharedMemoryName = argv[4];

    return MainImpl(filepath, processId, semName, sharedMemoryName);
  } catch (...) {
    TopmostExceptionHandler("main");
  }
}
