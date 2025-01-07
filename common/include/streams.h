// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   streams.h
 *
 * @brief Declaration of file streams used in GITS project
 *
 */

#pragma once

#include "pragmas.h"
#include "config.h"

#include <fstream>
#include <map>
#include <deque>
#include <cstdio>
#include <filesystem>

namespace gits {
template <int Value>
int identity() {
  return Value;
}
// This is to make sure that a 32-bit pointer read from 64-bit backing store
// won't cause a problem. If lower 32 bits do not uniquely identify a value,
// we need to do some kind of a mapping to make things work correctly.
// Until then, simpler solution is to just detect those cases.
void ensure_unique_ptr(uint64_t value);
void check_uint_conversion_possibility(uint64_t value);
template <typename T>
void write_to_stream(std::ostream& o, const T& value) {
  if (!Config::Get().common.recorder.nullIO) {
    o.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }
}

template <typename T>
void read_from_stream(std::istream& i, T& value) {
  i.read(reinterpret_cast<char*>(&value), sizeof(value));
}

template <class T>
void read_name_from_stream(std::istream& i, T& value) {
  read_from_stream(i, value);
}
template <class T>
void read_name_from_stream(std::istream& i, T*& value) {
  uint64_t v;
  read_from_stream(i, v);
  if (identity<sizeof(T*) != sizeof(uint64_t)>()) {
    ensure_unique_ptr(v);
  }
  value = reinterpret_cast<T*>((uintptr_t)v);
}

/**
   * @brief Binary file output stream.
   *
   * gits::CBinOStream class is a binary file output stream.
   */
enum WriteType : uint8_t {
  STANDALONE,
  PACKAGE,
  LARGE_STANDALONE
};
class CBinOStream : public std::ostream {
  std::streambuf* _buf;
  CompressionType _compressionType;
  std::vector<char> _dataToCompress;
  std::vector<char> _compressedDataToStore;
  uint64_t _offset;
  bool _initializedCompression;
  uint64_t _chunkSize;
  uint64_t _standaloneMaxSize;
  std::mutex mutex_;

public:
  bool InitializeCompression();
  bool WriteCompressed(const char* data, uint64_t dataSize);
  bool WriteCompressedAndGetOffset(const char* data,
                                   uint64_t dataSize,
                                   uint64_t& offsetInFile,
                                   uint64_t& offsetInChunk);
  std::ostream& WriteToOstream(const char* data, uint64_t dataSize);
  void write(const char* s, std::streamsize n);
  CBinOStream(const CBinOStream&) = delete;
  CBinOStream& operator=(const CBinOStream&) = delete;
  CBinOStream(CBinOStream&&) = delete;
  CBinOStream& operator=(CBinOStream&&) = delete;
  CBinOStream(const std::filesystem::path& fileName);
  ~CBinOStream();

private:
  void HelperWriteCompressed(const char* dataToWrite, uint64_t size, WriteType writeType);
  void HelperWriteCompressedLarge(const char* dataToWrite, uint64_t size, WriteType writeType);
};

template <typename T>
void write_to_stream(CBinOStream& o, const T& value) {
  if (!Config::Get().common.recorder.nullIO) {
    o.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }
}

template <class T>
void write_name_to_stream(CBinOStream& o, T value) {
  write_to_stream(o, value);
}
template <class T>
void write_name_to_stream(CBinOStream& o, T* value) {
  uint64_t v = reinterpret_cast<uintptr_t>(value);
  write_to_stream(o, v);
}

inline void write_size_t_to_stream(CBinOStream& o, size_t value) {
  uint64_t v = (uint64_t)value;
  write_to_stream(o, v);
}

CBinOStream& operator<<(CBinOStream& o, const char* value);
CBinOStream& operator<<(CBinOStream& o, const char& value);
CBinOStream& operator<<(CBinOStream& o, const std::string& value);

/**
   * @brief Binary file input stream.
   *
   * gits::CBinIStream class is a binary file input stream.
   */
class CBinIStream /*: public std::istream*/ {
  FILE* _file;
  std::filesystem::path _path;
  std::vector<char> _decompressedData;
  std::vector<char> _compressedData;
  uint64_t _offset;
  uint64_t _size;
  uint64_t _actualOffsetInFile;
  CompressionType _compressionType;
  bool _initializedCompression;
  uint64_t _chunkSize;
  uint64_t _standaloneMaxSize;

public:
  bool ReadHelper(char*, size_t);
  bool read(char*, size_t);
  int tellg() const;
  void get_delimited_string(std::string& s, char d);
  bool eof() const;
  int fileseek(FILE* stream, uint64_t offset, int origin);
  int getc();

  bool InitializeCompression();
  bool LoadChunk();
  bool ReadCompressed(char* data, uint64_t dataSize);
  void* ReadWithOffset(char* data,
                       uint64_t dataSize,
                       uint64_t offsetInFile,
                       uint64_t offsetInChunk = 0);
  CBinIStream(const std::filesystem::path& fileName);
  CBinIStream(const CBinIStream&) = delete;
  CBinIStream& operator=(const CBinIStream&) = delete;
  CBinIStream(CBinIStream&&) = delete;
  CBinIStream& operator=(CBinIStream&&) = delete;
  ~CBinIStream();
  const std::filesystem::path& Path() const {
    return _path;
  }
};

template <typename T>
void read_from_stream(CBinIStream& i, T& value) {
  i.read(reinterpret_cast<char*>(&value), sizeof(value));
}

template <class T>
void read_name_from_stream(CBinIStream& i, T& value) {
  read_from_stream(i, value);
}
template <class T>
void read_name_from_stream(CBinIStream& i, T*& value) {
  uint64_t v = 0U;
  read_from_stream(i, v);
  if (v <= UINT64_MAX) {
    if (identity<sizeof(T*) != sizeof(uint64_t)>()) {
      ensure_unique_ptr(v);
    }
    value = reinterpret_cast<T*>(static_cast<uintptr_t>(v));
  }
}

inline void read_size_t_from_stream(CBinIStream& i, size_t& value) {
  uint64_t v;
  read_from_stream(i, v);
  if (identity<sizeof(size_t) != sizeof(uint64_t)>()) {
    check_uint_conversion_possibility(v);
  }
  value = (size_t)v;
}

/**
   * @brief C code file output stream.
   *
   * gits::CCodeOStream class is a C code file output stream.
   */

#ifndef BUILD_FOR_CCODE
class CCodeOStream : public std::ostream {
public:
  enum TGitsSourceFile {
    GITS_MAIN_CPP = 0,
    GITS_FRAMES_CPP,
    GITS_EXTERNS_CPP,
    GITS_EXTERNS_H,
    GITS_STATE_RESTORE_CPP,
    GITS_PRE_RECORDER_CPP,
    GITS_FILES_COUNT
  };

  explicit CCodeOStream(const std::string& base_filename);
  CCodeOStream(const CCodeOStream&) = delete;
  CCodeOStream& operator=(const CCodeOStream&) = delete;
  CCodeOStream(CCodeOStream&&) = delete;
  CCodeOStream& operator=(CCodeOStream&&) = delete;
  virtual ~CCodeOStream();
  std::filebuf* rdbuf() const;
  bool is_open() const;

  //makes selected file active
  CCodeOStream& select(TGitsSourceFile file);

  TGitsSourceFile selectCCodeFile();

  void ScopeBegin();
  void ScopeEnd();
  CCodeOStream& Indent();
  void Register(intptr_t key, const std::string& prefix, bool addId, bool globalVal = false);
  std::string VariableName(intptr_t key) const;

private:
  //these functions output necessary data to files, before dump begins/ends
  void OnFilesOpen();
  void OnFilesClose();

  struct TFileInfo {
    TGitsSourceFile file;
    const char* fileExt;
  };

  typedef std::map<intptr_t, std::string> CVariableNameMap;
  struct TScopeData {
    int lastId;
    CVariableNameMap variables;
  };
  typedef std::deque<TScopeData> CScopeList;

  static const unsigned BUFFER_COUNT = GITS_FILES_COUNT;
  static const int INDENT_STEP = 2;
  static const TFileInfo FILES_INFO[BUFFER_COUNT];

  int _currBuffer;
  std::filebuf _fileBuffers[BUFFER_COUNT];
  int _indentLevels[BUFFER_COUNT];
  std::string _baseName;
  CScopeList _scopeList;
};

/**
    * @brief The helper class generates opening and closing brackests with required indentation
    * for the new scope in generated CCode.
    * To use this class, only its object needs to be declared locally in the required scope.
  */
class CScopeGenerator {
  CCodeOStream& _stream;
  bool _active;
  void operator=(CScopeGenerator& val);
  CScopeGenerator(CScopeGenerator& val);

public:
  CScopeGenerator(CCodeOStream& stream, bool active = true) : _stream(stream), _active(active) {
    if (_active) {
      _stream.Indent() << "{" << std::endl;
      _stream.ScopeBegin();
    }
  }

  ~CScopeGenerator() {
    if (_active) {
      _stream.ScopeEnd();
      _stream.Indent() << "}" << std::endl;
    }
  }
};
#endif
} // namespace gits
