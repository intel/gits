// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace gits::common {
class IPCPipe {
public:
  enum Mode {
    READ_MODE,
    WRITE_MODE
  };

  IPCPipe(std::string pipeName, Mode mode);
  ~IPCPipe();

  bool create();
  bool connect();
  bool write(const std::string& message);
  bool read(std::string& message, int timeoutMs = -1);
  void close();
  bool isValid() const;
  bool isConnected() const;

private:
  std::string m_pipeName;
  Mode m_mode;
  bool m_isValid;
  bool m_connected = false;
  std::atomic<bool> m_closing{false};
  std::string m_readBuffer;
  mutable std::mutex m_mutex;

#ifdef _WIN32
  HANDLE m_hPipe;
  std::string getWindowsPipeName() const;
#else
  int m_fd;
  std::string getUnixPipeName() const;
  bool createFifo();
#endif
};

// Bidirectional communication manager
class IPCManager {
public:
  using StatusCallback = std::function<void(const std::string&)>;

  IPCManager(const std::string& baseName, bool isServer);
  ~IPCManager();

  bool initialize();
  bool sendCommand(const std::string& command);
  bool sendStatus(const std::string& status);
  void setStatusCallback(StatusCallback callback);
  void shutdown();

  // Allow access to pipes for direct reading (needed for player command processing)
  std::unique_ptr<IPCPipe> m_commandPipe;
  std::unique_ptr<IPCPipe> m_statusPipe;

private:
  std::string m_baseName;
  bool m_isServer;

  std::thread m_statusThread;
  std::atomic<bool> m_running{false};
  StatusCallback m_statusCallback;
  std::mutex m_callbackMutex;

  void statusReaderLoop();
};
} // namespace gits::common
