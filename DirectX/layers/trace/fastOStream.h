// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <fast_io.h>
#include <fast_io_device.h>
#include "exception.h"

#include <memory>
#include <string>
#include <fstream>

namespace gits {
namespace DirectX {

namespace detail {

template <typename StreamT, typename T>
void print(StreamT& stream, const T& arg) {
  if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, const char*>) {
    fast_io::io::print(stream, fast_io::mnp::os_c_str(arg));
  } else if constexpr (std::is_same_v<T, char>) {
    fast_io::io::print(stream, fast_io::mnp::chvw(arg));
  } else if constexpr (std::is_pointer_v<T>) {
    fast_io::io::print(stream, fast_io::mnp::uhexupperfull(reinterpret_cast<std::uintptr_t>(arg)));
  } else if constexpr (std::is_enum_v<T>) {
    fast_io::io::print(stream, fast_io::mnp::enum_int_view(arg));
  } else {
    fast_io::io::print(stream, arg);
  }
}

template <typename StreamT, typename T>
void printHex(StreamT& stream, const T& arg) {
  fast_io::io::print(stream, fast_io::mnp::hex(arg));
}

template <typename StreamT, typename T>
void printHexFull(StreamT& stream, const T& arg) {
  fast_io::io::print(stream, fast_io::mnp::uhexfull(arg));
}

} // namespace detail

class FastOStream {
public:
  enum class Type {
    FileStream,
    StringStream
  };
  virtual ~FastOStream() = default;
  FastOStream(const Type type, const std::string& filePath = "")
      : type_{type}, filePath_{filePath} {}
  const std::string& getFilePath() {
    return filePath_;
  }
  virtual bool isOpen() = 0;
  virtual void close() = 0;
  virtual void flush() = 0;
  const Type type_;

protected:
  bool isOpen_{false};
  std::string filePath_;
};

class FastOStringStream : public FastOStream {
public:
  enum class FlushMethod {
    Ipc,
    File
  };

  ~FastOStringStream() override {
    try {
      close();
    } catch (...) {
      topmost_exception_handler("FastOStringStream::~FastOStringStream");
    }
  }

  FastOStringStream(const std::string& filePath = "",
                    const FlushMethod flushMethod = FlushMethod::Ipc,
                    const size_t bufferCapacity = 1000000)
      : FastOStream(Type::StringStream, filePath),
        flushMethod_(flushMethod),
        bufferCapacity_(bufferCapacity),
        forcedFlushThreshold_(bufferCapacity_ * forcedFlushCapacityRatio) {
    if (!filePath_.empty()) {
      open(filePath, flushMethod, bufferCapacity);
    }
  }

  void open(const std::string& filePath,
            const FlushMethod flushMethod,
            const size_t bufferCapacity);

  void close();

  bool isOpen() {
    return isOpen_;
  }

  void flush();

  fast_io::obuffer_view& getUnderlying() {
    return bufferStream_;
  }

  std::string extractString() {
    std::string str(bufferStream_.cbegin(), bufferStream_.size());
    bufferStream_.clear();
    return str;
  }

  void checkForcedFlush() {
    if (bufferStream_.size() > forcedFlushThreshold_) {
      flush();
    }
  }

private:
  void initializeIpcFlush();
  static constexpr const char* eventBaseName = "Local\\GitsDirectXTraceEvent";
  static constexpr const char* sharedMemoryBaseName = "Local\\GitsDirectXTraceSharedMemory";
  static constexpr double forcedFlushCapacityRatio = 0.9;
  struct FlushInfo {
    bool initialized;
    void* pBuf;
    void* hMapFile;
    void* hEvent;
  } flushInfo_{};
  FlushMethod flushMethod_;
  size_t bufferCapacity_;
  size_t forcedFlushThreshold_;

  std::string bufferStreamBuffer_;
  std::ofstream bufferStreamFile_;
  fast_io::obuffer_view bufferStream_;
};

class FastOFileStream : public FastOStream {
public:
  ~FastOFileStream() override {}

  FastOFileStream(const std::string& filePath = "") : FastOStream(Type::FileStream, filePath) {
    if (!filePath_.empty()) {
      open(filePath);
    }
  }

  void open(const std::string& filePath) {
    if (isOpen_) {
      close();
    }

    filePath_ = filePath;
    fileStream_ = fast_io::obuf_file(filePath_);

    isOpen_ = true;
  }

  void close() {
    isOpen_ = false;

    fileStream_.close();
    filePath_.clear();
  }

  bool isOpen() {
    return isOpen_;
  }

  void flush();

  fast_io::obuf_file& getUnderlying() {
    return fileStream_;
  }

private:
  fast_io::obuf_file fileStream_;
};

template <typename T>
FastOStream& operator<<(FastOStream& stream, const T& arg) {
  if (stream.type_ == FastOStream::Type::StringStream) {
    auto& sstream = static_cast<FastOStringStream&>(stream);
    sstream.checkForcedFlush();
    detail::print(sstream.getUnderlying(), arg);
  } else if (stream.type_ == FastOStream::Type::FileStream) {
    detail::print(static_cast<FastOFileStream&>(stream).getUnderlying(), arg);
  }

  return stream;
}

template <typename T>
FastOStream& printHex(FastOStream& stream, const T& arg) {
  if (stream.type_ == FastOStream::Type::StringStream) {
    auto& sstream = static_cast<FastOStringStream&>(stream);
    sstream.checkForcedFlush();
    detail::printHex(sstream.getUnderlying(), arg);
  } else if (stream.type_ == FastOStream::Type::FileStream) {
    detail::printHex(static_cast<FastOFileStream&>(stream).getUnderlying(), arg);
  }

  return stream;
}

template <typename T>
FastOStream& printHexFull(FastOStream& stream, const T& arg) {
  if (stream.type_ == FastOStream::Type::StringStream) {
    auto& sstream = static_cast<FastOStringStream&>(stream);
    sstream.checkForcedFlush();
    detail::printHexFull(sstream.getUnderlying(), arg);
  } else if (stream.type_ == FastOStream::Type::FileStream) {
    detail::printHexFull(static_cast<FastOFileStream&>(stream).getUnderlying(), arg);
  }

  return stream;
}

} // namespace DirectX
} // namespace gits
