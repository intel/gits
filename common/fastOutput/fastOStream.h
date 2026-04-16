// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

namespace detail {

template <typename StreamT, typename T>
void Print(StreamT& stream, const T& arg) {
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
void PrintHex(StreamT& stream, const T& arg) {
  fast_io::io::print(stream, fast_io::mnp::hex(arg));
}

template <typename StreamT, typename T>
void PrintHexFull(StreamT& stream, const T& arg) {
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
      : m_Type{type}, m_FilePath{filePath} {}
  const std::string& GetFilePath() {
    return m_FilePath;
  }
  virtual bool IsOpen() = 0;
  virtual void Close() = 0;
  virtual void Flush() = 0;
  const Type m_Type;

protected:
  bool m_IsOpen{false};
  std::string m_FilePath;
};

class FastOStringStream : public FastOStream {
public:
  enum class FlushMethod {
    Ipc,
    File
  };

  ~FastOStringStream() override {
    try {
      Close();
    } catch (...) {
      topmost_exception_handler("FastOStringStream::~FastOStringStream");
    }
  }

  FastOStringStream(const std::string& filePath = "",
                    const FlushMethod flushMethod = FlushMethod::Ipc,
                    const size_t bufferCapacity = 1000000)
      : FastOStream(Type::StringStream, filePath),
        m_FlushMethod(flushMethod),
        m_BufferCapacity(bufferCapacity),
        m_ForcedFlushThreshold(m_BufferCapacity * FORCED_FLUSH_CAPACITY_RATIO) {
    if (!m_FilePath.empty()) {
      Open(filePath, flushMethod, bufferCapacity);
    }
  }

  void Open(const std::string& filePath,
            const FlushMethod flushMethod,
            const size_t bufferCapacity);

  void Close();

  bool IsOpen() {
    return m_IsOpen;
  }

  void Flush();

  fast_io::obuffer_view& GetUnderlying() {
    return m_BufferStream;
  }

  std::string ExtractString() {
    std::string str(m_BufferStream.cbegin(), m_BufferStream.size());
    m_BufferStream.clear();
    return str;
  }

  void CheckForcedFlush() {
    if (m_BufferStream.size() > m_ForcedFlushThreshold) {
      Flush();
    }
  }

private:
  static constexpr double FORCED_FLUSH_CAPACITY_RATIO = 0.9;
#ifdef WIN32
  void InitializeIpcFlush();
  static constexpr const char* EVENT_BASE_NAME = "Local\\GitsGraphicsAPITraceEvent";
  static constexpr const char* SHARED_MEMORY_BASE_NAME = "Local\\GitsGraphicsAPITraceSharedMemory";
  struct FlushInfo {
    bool Initialized;
    void* Buf;
    void* MapFile;
    void* Event;
  } m_FlushInfo{};
#endif
  FlushMethod m_FlushMethod;
  size_t m_BufferCapacity;
  size_t m_ForcedFlushThreshold;

  std::string m_BufferStreamBuffer;
  std::ofstream m_BufferStreamFile;
  fast_io::obuffer_view m_BufferStream;
};

class FastOFileStream : public FastOStream {
public:
  ~FastOFileStream() override {}

  FastOFileStream(const std::string& filePath = "") : FastOStream(Type::FileStream, filePath) {
    if (!m_FilePath.empty()) {
      Open(filePath);
    }
  }

  void Open(const std::string& filePath) {
    if (m_IsOpen) {
      Close();
    }

    m_FilePath = filePath;
    m_FileStream = fast_io::obuf_file(m_FilePath);

    m_IsOpen = true;
  }

  void Close() {
    m_IsOpen = false;

    m_FileStream.close();
    m_FilePath.clear();
  }

  bool IsOpen() {
    return m_IsOpen;
  }

  void Flush();

  fast_io::obuf_file& GetUnderlying() {
    return m_FileStream;
  }

private:
  fast_io::obuf_file m_FileStream;
};

template <typename T>
FastOStream& operator<<(FastOStream& stream, const T& arg) {
  if (stream.m_Type == FastOStream::Type::StringStream) {
    auto& sstream = static_cast<FastOStringStream&>(stream);
    sstream.CheckForcedFlush();
    detail::Print(sstream.GetUnderlying(), arg);
  } else if (stream.m_Type == FastOStream::Type::FileStream) {
    detail::Print(static_cast<FastOFileStream&>(stream).GetUnderlying(), arg);
  }

  return stream;
}

template <typename T>
FastOStream& PrintHex(FastOStream& stream, const T& arg) {
  if (stream.m_Type == FastOStream::Type::StringStream) {
    auto& sstream = static_cast<FastOStringStream&>(stream);
    sstream.CheckForcedFlush();
    detail::PrintHex(sstream.GetUnderlying(), arg);
  } else if (stream.m_Type == FastOStream::Type::FileStream) {
    detail::PrintHex(static_cast<FastOFileStream&>(stream).GetUnderlying(), arg);
  }

  return stream;
}

template <typename T>
FastOStream& PrintHexFull(FastOStream& stream, const T& arg) {
  if (stream.m_Type == FastOStream::Type::StringStream) {
    auto& sstream = static_cast<FastOStringStream&>(stream);
    sstream.CheckForcedFlush();
    detail::PrintHexFull(sstream.GetUnderlying(), arg);
  } else if (stream.m_Type == FastOStream::Type::FileStream) {
    detail::PrintHexFull(static_cast<FastOFileStream&>(stream).GetUnderlying(), arg);
  }

  return stream;
}

} // namespace gits
