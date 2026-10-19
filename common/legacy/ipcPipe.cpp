// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ipcPipe.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>

#ifdef _WIN32
#include <limits>
#else
#include <sys/select.h>
#include <sys/file.h>
#include <poll.h>
#include <cstring>
#include <csignal>
#include <pthread.h>
#endif

namespace {
constexpr size_t MaxMessageSize = 511;
#ifdef _WIN32
bool CompleteIo(
    HANDLE pipe, OVERLAPPED& operation, BOOL immediate, DWORD& transferred, DWORD timeout) {
  if (immediate) {
    return true;
  }
  if (GetLastError() != ERROR_IO_PENDING) {
    return false;
  }
  if (WaitForSingleObject(operation.hEvent, timeout) != WAIT_OBJECT_0) {
    CancelIoEx(pipe, &operation);
    GetOverlappedResult(pipe, &operation, &transferred, TRUE);
    return false;
  }
  return GetOverlappedResult(pipe, &operation, &transferred, FALSE) != FALSE;
}
#endif
} // namespace

// ============================================================================
// IPCPipe Implementation
// ============================================================================

namespace gits::common {
IPCPipe::IPCPipe(std::string pipeName, Mode mode)
    : m_pipeName(std::move(pipeName)), m_mode(mode), m_isValid(false) {
#ifdef _WIN32
  m_hPipe = INVALID_HANDLE_VALUE;
#else
  m_fd = -1;
#endif
}

IPCPipe::~IPCPipe() {
  close();
}

#ifdef _WIN32
// Windows Implementation
std::string IPCPipe::getWindowsPipeName() const {
  return "\\\\.\\pipe\\" + m_pipeName;
}

bool IPCPipe::create() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_mode != READ_MODE) {
    return false;
  }
  if (m_isValid) {
    return true;
  }
  m_closing = false;

  std::string fullName = getWindowsPipeName();

  m_hPipe = CreateNamedPipeA(
      fullName.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
      1,    // Max instances
      1024, // Out buffer size
      1024, // In buffer size
      0,    // Default timeout
      NULL  // Default security
  );

  m_isValid = (m_hPipe != INVALID_HANDLE_VALUE);
  return m_isValid;
}

bool IPCPipe::connect() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_connected) {
    return true;
  }
  m_closing = false;
  if (m_mode == READ_MODE) {
    // Server side - wait for client connection with timeout to avoid indefinite blocking
    if (!m_isValid) {
      return false;
    }

    // Use overlapped I/O for non-blocking connect with timeout
    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
      return false;
    }

    BOOL connectResult = ConnectNamedPipe(m_hPipe, &overlapped);
    DWORD error = GetLastError();

    // If already connected or connection succeeded immediately
    if (connectResult || error == ERROR_PIPE_CONNECTED) {
      CloseHandle(overlapped.hEvent);
      m_connected = true;
      return true;
    }

    // If connection pending, wait with 5 second timeout
    if (error == ERROR_IO_PENDING) {
      DWORD transferred = 0;
      SetLastError(ERROR_IO_PENDING);
      m_connected = CompleteIo(m_hPipe, overlapped, FALSE, transferred, 100);
      CloseHandle(overlapped.hEvent);
      if (!m_connected) {
        DisconnectNamedPipe(m_hPipe);
      }
      return m_connected;
    }

    CloseHandle(overlapped.hEvent);
    return false;
  } else {
    // Client side - connect to existing pipe
    std::string fullName = getWindowsPipeName();

    m_hPipe = CreateFileA(fullName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_OVERLAPPED, NULL);

    m_isValid = (m_hPipe != INVALID_HANDLE_VALUE);
    m_connected = m_isValid;
    return m_isValid;
  }
}

bool IPCPipe::write(const std::string& message) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isValid || !m_connected || m_closing || m_mode != WRITE_MODE || message.empty() ||
      message.size() > MaxMessageSize || message.find('\n') != std::string::npos) {
    return false;
  }

  const auto framed = message + '\n';
  OVERLAPPED operation = {};
  operation.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
  if (!operation.hEvent) {
    return false;
  }
  DWORD bytesWritten = 0;
  const auto result = WriteFile(m_hPipe, framed.data(), static_cast<DWORD>(framed.size()),
                                &bytesWritten, &operation);
  const bool success = CompleteIo(m_hPipe, operation, result, bytesWritten, 100);
  CloseHandle(operation.hEvent);
  m_connected = success && bytesWritten == framed.size();
  return m_connected;
}

bool IPCPipe::read(std::string& message, int timeoutMs) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isValid || !m_connected || m_mode != READ_MODE) {
    return false;
  }
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(timeoutMs, 0));
  do {
    const auto end = m_readBuffer.find('\n');
    if (end != std::string::npos) {
      message = m_readBuffer.substr(0, end);
      m_readBuffer.erase(0, end + 1);
      return true;
    }
    if (m_readBuffer.size() > MaxMessageSize) {
      m_connected = false;
      return false;
    }
    DWORD available = 0;
    if (!PeekNamedPipe(m_hPipe, NULL, 0, NULL, &available, NULL)) {
      m_connected = false;
      return false;
    }
    if (available) {
      char buffer[512];
      DWORD bytesRead = 0;
      OVERLAPPED operation = {};
      operation.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
      if (!operation.hEvent) {
        return false;
      }
      const auto result = ReadFile(m_hPipe, buffer, std::min<DWORD>(available, sizeof(buffer)),
                                   &bytesRead, &operation);
      const auto success = CompleteIo(m_hPipe, operation, result, bytesRead, 100);
      CloseHandle(operation.hEvent);
      if (!success || !bytesRead) {
        m_connected = false;
        return false;
      }
      m_readBuffer.append(buffer, bytesRead);
      continue;
    }
    if (timeoutMs == 0) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } while (!m_closing && (timeoutMs < 0 || std::chrono::steady_clock::now() < deadline ||
                          m_readBuffer.find('\n') != std::string::npos));
  return false;
}

void IPCPipe::close() {
  m_closing = true;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_hPipe != INVALID_HANDLE_VALUE) {
    if (m_mode == READ_MODE) {
      DisconnectNamedPipe(m_hPipe);
    }
    CloseHandle(m_hPipe);
    m_hPipe = INVALID_HANDLE_VALUE;
  }
  m_isValid = false;
  m_connected = false;
  m_readBuffer.clear();
}

#else
// Unix Implementation (Linux/macOS)
std::string IPCPipe::getUnixPipeName() const {
  return "/tmp/" + m_pipeName + "_pipe";
}

bool IPCPipe::createFifo() {
  std::string fullName = getUnixPipeName();

  if (mkfifo(fullName.c_str(), 0600) == -1) {
    if (errno != EEXIST) {
      return false;
    }
  }

  struct stat info = {};
  return lstat(fullName.c_str(), &info) == 0 && S_ISFIFO(info.st_mode) && info.st_uid == geteuid();
}

bool IPCPipe::create() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_mode != READ_MODE) {
    return false;
  }
  if (m_isValid) {
    return true;
  }
  m_closing = false;

  if (!createFifo()) {
    return false;
  }

  // Open for reading (this will block until a writer connects)
  std::string fullName = getUnixPipeName();
  m_fd = open(fullName.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);

  if (m_fd != -1 && flock(m_fd, LOCK_EX | LOCK_NB) == 0) {
    struct stat info = {};
    m_isValid = fstat(m_fd, &info) == 0 && S_ISFIFO(info.st_mode) && info.st_uid == geteuid();
  }
  if (!m_isValid && m_fd != -1) {
    ::close(m_fd);
    m_fd = -1;
  }

  return m_isValid;
}

bool IPCPipe::connect() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_connected) {
    return true;
  }
  m_closing = false;
  if (m_mode == READ_MODE) {
    if (!m_isValid) {
      return false;
    }
    char byte;
    const auto result = ::read(m_fd, &byte, 1);
    if (result == 1) {
      m_readBuffer += byte;
    }
    m_connected = result == 1 || (result < 0 && errno == EAGAIN);
    return m_connected;
  } else {
    // Client side - open existing FIFO for writing
    std::string fullName = getUnixPipeName();

    // Try to open the FIFO
    m_fd = open(fullName.c_str(), O_WRONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);

    if (m_fd != -1) {
      struct stat info = {};
      m_isValid = fstat(m_fd, &info) == 0 && S_ISFIFO(info.st_mode) && info.st_uid == geteuid();
      if (!m_isValid) {
        ::close(m_fd);
        m_fd = -1;
      }
    }
    m_connected = m_isValid;
    return m_isValid;
  }
}

bool IPCPipe::write(const std::string& message) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isValid || !m_connected || m_closing || m_mode != WRITE_MODE || message.empty() ||
      message.size() > MaxMessageSize || message.find('\n') != std::string::npos) {
    return false;
  }
  // Block SIGPIPE only on this thread and consume only a signal generated by this write.
  sigset_t blocked, previous, pending;
  sigemptyset(&blocked);
  sigaddset(&blocked, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &blocked, &previous) != 0) {
    return false;
  }
  sigpending(&pending);
  const bool alreadyPending = sigismember(&pending, SIGPIPE) == 1;
  const auto framed = message + '\n';
  ssize_t bytesWritten;
  do {
    bytesWritten = ::write(m_fd, framed.data(), framed.size());
  } while (bytesWritten < 0 && errno == EINTR && !m_closing);
  const int error = errno;
  if (bytesWritten < 0 && error == EPIPE && !alreadyPending) {
    timespec timeout = {};
    sigtimedwait(&blocked, nullptr, &timeout);
  }
  pthread_sigmask(SIG_SETMASK, &previous, nullptr);
  const bool success = bytesWritten == static_cast<ssize_t>(framed.size());
  if (!success) {
    m_connected = false;
  }
  return success;
}

bool IPCPipe::read(std::string& message, int timeoutMs) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isValid || !m_connected || m_mode != READ_MODE) {
    return false;
  }
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(timeoutMs, 0));
  do {
    const auto end = m_readBuffer.find('\n');
    if (end != std::string::npos) {
      if (end > MaxMessageSize) {
        m_connected = false;
        return false;
      }
      message = m_readBuffer.substr(0, end);
      m_readBuffer.erase(0, end + 1);
      return true;
    }
    if (m_readBuffer.size() > MaxMessageSize) {
      m_connected = false;
      return false;
    }
    char buffer[512];
    const auto count = ::read(m_fd, buffer, sizeof(buffer));
    if (count > 0) {
      m_readBuffer.append(buffer, static_cast<size_t>(count));
      continue;
    }
    if (count == 0 || (errno != EAGAIN && errno != EINTR)) {
      m_connected = false;
      return false;
    }
    if (timeoutMs == 0) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } while (!m_closing && (timeoutMs < 0 || std::chrono::steady_clock::now() < deadline ||
                          m_readBuffer.find('\n') != std::string::npos));
  return false;
}

void IPCPipe::close() {
  m_closing = true;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_fd != -1) {
    // Clean up FIFO file if we're the server
    if (m_mode == READ_MODE) {
      std::string fullName = getUnixPipeName();
      struct stat opened = {}, named = {};
      if (fstat(m_fd, &opened) == 0 && lstat(fullName.c_str(), &named) == 0 &&
          opened.st_dev == named.st_dev && opened.st_ino == named.st_ino) {
        unlink(fullName.c_str());
      }
    }
    ::close(m_fd);
    m_fd = -1;
  }
  m_isValid = false;
  m_connected = false;
  m_readBuffer.clear();
}

#endif

bool IPCPipe::isValid() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_isValid;
}

bool IPCPipe::isConnected() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isValid || !m_connected) {
    return false;
  }

#ifdef _WIN32
  if (m_mode == WRITE_MODE) {
    return m_connected;
  }
  if (PeekNamedPipe(m_hPipe, NULL, 0, NULL, NULL, NULL)) {
    return true;
  }

  const DWORD error = GetLastError();
  return error != ERROR_BROKEN_PIPE && error != ERROR_PIPE_NOT_CONNECTED &&
         error != ERROR_INVALID_HANDLE;
#else
  struct pollfd descriptor;
  descriptor.fd = m_fd;
  descriptor.events = (m_mode == WRITE_MODE) ? POLLOUT : POLLIN;
  descriptor.revents = 0;

  const int result = poll(&descriptor, 1, 0);
  if (result < 0) {
    return errno != EBADF;
  }

  if (result == 0) {
    return true;
  }

  return (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0;
#endif
}

// ============================================================================
// IPCManager Implementation
// ============================================================================

IPCManager::IPCManager(const std::string& baseName, bool isServer)
    : m_baseName(baseName), m_isServer(isServer) {

  if (isServer) {
    // Server: reads commands, writes status
    m_commandPipe = std::make_unique<IPCPipe>(baseName + "_cmd", IPCPipe::READ_MODE);
    m_statusPipe = std::make_unique<IPCPipe>(baseName + "_status", IPCPipe::WRITE_MODE);
  } else {
    // Client: writes commands, reads status
    m_commandPipe = std::make_unique<IPCPipe>(baseName + "_cmd", IPCPipe::WRITE_MODE);
    m_statusPipe = std::make_unique<IPCPipe>(baseName + "_status", IPCPipe::READ_MODE);
  }
}

IPCManager::~IPCManager() {
  shutdown();
}

bool IPCManager::initialize() {
  auto& incoming = m_isServer ? m_commandPipe : m_statusPipe;
  if (!incoming->create()) {
    return false;
  }

  // Connect to pipes
  if (!m_commandPipe->connect()) {
    std::cerr << "Failed to connect to command pipe" << std::endl;
    return false;
  }

  if (!m_statusPipe->connect()) {
    std::cerr << "Failed to connect to status pipe" << std::endl;
    return false;
  }

  // Start status monitoring thread for client
  if (!m_isServer && !m_running.exchange(true)) {
    m_running = true;
    m_statusThread = std::thread(&IPCManager::statusReaderLoop, this);
  }

  return true;
}

bool IPCManager::sendCommand(const std::string& command) {
  if (!m_isServer && m_commandPipe) {
    return m_commandPipe->write(command);
  }
  return false;
}

bool IPCManager::sendStatus(const std::string& status) {
  if (m_isServer && m_statusPipe) {
    return m_statusPipe->write(status);
  }
  return false;
}

void IPCManager::setStatusCallback(StatusCallback callback) {
  std::lock_guard<std::mutex> lock(m_callbackMutex);
  m_statusCallback = callback;
}

void IPCManager::statusReaderLoop() {
  while (m_running) {
    std::string status;
    if (m_statusPipe && m_statusPipe->read(status, 1000)) { // 1 second timeout
      StatusCallback callback;
      {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_statusCallback;
      }
      if (callback) {
        callback(status);
      }
    }
  }
}

void IPCManager::shutdown() {
  m_running = false;

  if (m_commandPipe) {
    m_commandPipe->close();
  }

  if (m_statusPipe) {
    m_statusPipe->close();
  }
  if (m_statusThread.joinable()) {
    m_statusThread.join();
  }
}
} // namespace gits::common
