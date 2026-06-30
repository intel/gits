// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "fastOStream.h"
#include "log.h"
#include "configurationLib.h"
#include <filesystem>
#ifdef WIN32
#include <Windows.h>
#include "traceIpc/sharedCircularBuffer.h"
#elif defined(__linux__)
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "traceIpcLinux/sharedCircularBuffer.h"
#endif
#include <thread>

namespace gits {

void FastOStringStream::Open(const std::string& filePath,
                             const FlushMethod flushMethod,
                             const size_t bufferCapacity) {
  if (m_IsOpen) {
    Close();
  }

  m_FilePath = filePath;
  m_FlushMethod = flushMethod;
  m_BufferCapacity = bufferCapacity;
  m_ForcedFlushThreshold = m_BufferCapacity * FORCED_FLUSH_CAPACITY_RATIO;

  m_BufferStreamBuffer.resize(m_BufferCapacity);
  if (flushMethod == FlushMethod::File) {
    m_BufferStreamFile.open(m_FilePath);
  }
  m_BufferStream = fast_io::obuffer_view(m_BufferStreamBuffer);

  m_IsOpen = true;
}

void FastOStringStream::Close() {
  m_IsOpen = false;

  Flush();
  m_BufferStream.clear();
  m_BufferStreamFile.close();
  m_BufferStreamBuffer.clear();
  m_FilePath.clear();

#ifdef WIN32
  if (m_FlushInfo.Buf) {
    UnmapViewOfFile(m_FlushInfo.Buf);
  }
  if (m_FlushInfo.MapFile) {
    CloseHandle(m_FlushInfo.MapFile);
  }
  if (m_FlushInfo.Event) {
    CloseHandle(m_FlushInfo.Event);
  }
  m_FlushInfo.Initialized = false;
#elif defined(__linux__)
  if (m_FlushInfo.Buf && m_FlushInfo.Buf != MAP_FAILED) {
    munmap(m_FlushInfo.Buf, SHARED_MEMORY_SIZE);
    m_FlushInfo.Buf = MAP_FAILED;
  }
  if (m_FlushInfo.ShmFd >= 0) {
    close(m_FlushInfo.ShmFd);
    m_FlushInfo.ShmFd = -1;
  }
  if (m_FlushInfo.Sem && m_FlushInfo.Sem != SEM_FAILED) {
    sem_close(m_FlushInfo.Sem);
    m_FlushInfo.Sem = SEM_FAILED;
  }
  m_FlushInfo.Initialized = false;
#endif
}

void FastOStringStream::Flush() {
  auto fallbackToDirectFileWrite = [this]() {
    LOG_ERROR << "IPC flush not initialized, falling back to file flush";
    m_FlushMethod = FlushMethod::File;
    m_BufferStreamFile.open(m_FilePath);
    m_BufferStreamFile.write(m_BufferStream.cbegin(), m_BufferStream.size());
    m_BufferStreamFile.flush();
    m_BufferStream.clear();
  };

  if (m_FlushMethod == FlushMethod::Ipc) {
#ifdef WIN32
    InitializeIpcFlush();
    if (!m_FlushInfo.Initialized) {
      fallbackToDirectFileWrite();
      return;
    }
    WriteAll(reinterpret_cast<SharedCircularBuffer*>(m_FlushInfo.Buf), m_BufferStream.cbegin(),
             m_BufferStream.size());
#elif defined(__linux__)
    InitializeIpcFlush();
    if (!m_FlushInfo.Initialized) {
      fallbackToDirectFileWrite();
      return;
    }
    WriteAll(reinterpret_cast<SharedCircularBuffer*>(m_FlushInfo.Buf), m_BufferStream.cbegin(),
             m_BufferStream.size());
#endif
    m_BufferStream.clear();
  } else {
    m_BufferStreamFile.write(m_BufferStream.cbegin(), m_BufferStream.size());
    m_BufferStreamFile.flush();
    m_BufferStream.clear();
  }
}

#ifdef WIN32
void FastOStringStream::InitializeIpcFlush() {
  if (m_FlushInfo.Initialized) {
    return;
  }

  auto executablePath =
      std::filesystem::path(Configurator::Get().common.recorder.installPath).parent_path() /
      "UtilityTools" / "traceIpc.exe";
  if (!std::filesystem::exists(executablePath)) {
    std::vector<char> moduleFilename(MAX_PATH + 1, 0);
    GetModuleFileNameA(nullptr, moduleFilename.data(), moduleFilename.size());

    // Try to load the trace executable from the directory next to the player
    auto playerPath = std::filesystem::absolute(moduleFilename.data());
    executablePath =
        std::filesystem::absolute(playerPath.parent_path() / "UtilityTools" / "traceIpc.exe");
    if (!std::filesystem::exists(executablePath)) {
      // Try to load the trace executable from the player's parent directory
      executablePath = std::filesystem::absolute(playerPath.parent_path().parent_path() /
                                                 "UtilityTools" / "traceIpc.exe");
    }
  }

  if (!std::filesystem::exists(executablePath)) {
    LOG_ERROR << "Could not locate the traceIpc.exe";
    return;
  }

  const std::string eventName =
      EVENT_BASE_NAME + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const std::string sharedMemoryName =
      SHARED_MEMORY_BASE_NAME +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  std::stringstream ss;
  ss << '"' << executablePath.string() << "\" \"" << m_FilePath << "\" " << GetCurrentProcessId()
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

  m_FlushInfo.Event = OpenEventA(SYNCHRONIZE, FALSE, eventName.c_str());
  size_t numOfTries = 300;
  while (!m_FlushInfo.Event) {
    m_FlushInfo.Event = OpenEventA(SYNCHRONIZE, FALSE, eventName.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (--numOfTries == 0) {
      LOG_ERROR << "Could not open event: " << GetLastError();
      LOG_ERROR << "Ipc process failed to run, check ipc_log.txt";
      return;
    }
  }

  if (WaitForSingleObject(m_FlushInfo.Event, 3000) == WAIT_TIMEOUT) {
    LOG_ERROR << "WaitForSingleObject timed out";
    LOG_ERROR << "Ipc process failed to run, check ipc_log.txt";
    return;
  }

  m_FlushInfo.MapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, sharedMemoryName.c_str());
  if (m_FlushInfo.MapFile == nullptr) {
    LOG_ERROR << "Could not open file mapping object: " << GetLastError();
    CloseHandle(m_FlushInfo.Event);
    return;
  }

  m_FlushInfo.Buf =
      MapViewOfFile(m_FlushInfo.MapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
  if (m_FlushInfo.Buf == nullptr) {
    LOG_ERROR << "Could not map view of file: " << GetLastError();
    CloseHandle(m_FlushInfo.MapFile);
    CloseHandle(m_FlushInfo.Event);
    return;
  }

  m_FlushInfo.Initialized = true;
}
#endif

#ifdef __linux__
void FastOStringStream::InitializeIpcFlush() {
  if (m_FlushInfo.Initialized) {
    return;
  }

  auto executablePath =
      std::filesystem::path(Configurator::Get().common.recorder.installPath).parent_path() /
      "UtilityTools" / "traceIpcLinux";
  if (!std::filesystem::exists(executablePath)) {
    auto selfPath = std::filesystem::canonical("/proc/self/exe");
    executablePath =
        std::filesystem::absolute(selfPath.parent_path() / "UtilityTools" / "traceIpcLinux");
    if (!std::filesystem::exists(executablePath)) {
      executablePath = std::filesystem::absolute(selfPath.parent_path().parent_path() /
                                                 "UtilityTools" / "traceIpcLinux");
    }
  }

  if (!std::filesystem::exists(executablePath)) {
    LOG_ERROR << "Could not locate the traceIpcLinux executable";
    return;
  }

  const std::string timestamp =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const std::string semName = SEM_BASE_NAME + timestamp;
  const std::string sharedMemoryName = SHARED_MEMORY_BASE_NAME + timestamp;

  m_FlushInfo.SemName = semName;
  m_FlushInfo.ShmName = sharedMemoryName;

  const std::string pidStr = std::to_string(getpid());
  const std::string execStr = executablePath.string();

  pid_t pid = fork();
  if (pid == -1) {
    LOG_ERROR << "Failed to fork traceIpcLinux process: " << strerror(errno);
    return;
  }

  if (pid == 0) {
    // Child: detach from parent session and exec
    setsid();
    char* argv[] = {
        const_cast<char*>(execStr.c_str()),          const_cast<char*>(m_FilePath.c_str()),
        const_cast<char*>(pidStr.c_str()),           const_cast<char*>(semName.c_str()),
        const_cast<char*>(sharedMemoryName.c_str()), nullptr};
    execv(execStr.c_str(), argv);
    // If execv returns, it failed
    _exit(EXIT_FAILURE);
  }

  // Parent continues — child is now detached

  m_FlushInfo.Sem = SEM_FAILED;
  size_t numOfTries = 300;
  while (m_FlushInfo.Sem == SEM_FAILED) {
    m_FlushInfo.Sem = sem_open(semName.c_str(), 0);
    if (m_FlushInfo.Sem == SEM_FAILED) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (--numOfTries == 0) {
        LOG_ERROR << "Could not open semaphore: " << strerror(errno);
        LOG_ERROR << "Ipc process failed to run, check ipc_log.txt";
        return;
      }
    }
  }

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 3;
  if (sem_timedwait(m_FlushInfo.Sem, &ts) == -1) {
    LOG_ERROR << "sem_timedwait timed out: " << strerror(errno);
    LOG_ERROR << "Ipc process failed to run, check ipc_log.txt";
    sem_close(m_FlushInfo.Sem);
    m_FlushInfo.Sem = SEM_FAILED;
    return;
  }

  m_FlushInfo.ShmFd = shm_open(sharedMemoryName.c_str(), O_RDWR, 0666);
  if (m_FlushInfo.ShmFd == -1) {
    LOG_ERROR << "Could not open shared memory: " << strerror(errno);
    sem_close(m_FlushInfo.Sem);
    m_FlushInfo.Sem = SEM_FAILED;
    return;
  }

  m_FlushInfo.Buf =
      mmap(nullptr, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_FlushInfo.ShmFd, 0);
  if (m_FlushInfo.Buf == MAP_FAILED) {
    LOG_ERROR << "Could not mmap shared memory: " << strerror(errno);
    close(m_FlushInfo.ShmFd);
    m_FlushInfo.ShmFd = -1;
    sem_close(m_FlushInfo.Sem);
    m_FlushInfo.Sem = SEM_FAILED;
    return;
  }

  m_FlushInfo.Initialized = true;
}
#endif

void FastOFileStream::Flush() {
  fast_io::flush(m_FileStream);
}

} // namespace gits
