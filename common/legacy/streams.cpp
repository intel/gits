// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   streams.cpp
 *
 * @brief Definition of file streams used in GITS project
 *
 */

#include "streams.h"
#include "tools.h"
#include "exception.h"
#include "gits.h"
#include "log.h"

#include <memory>
#include <stdexcept>
#include <algorithm>
#include <iomanip>

namespace {
std::streambuf* initialize_gits_streambuf(const std::filesystem::path& fileName,
                                          std::ios::openmode mode) {
  if (std::filesystem::is_directory(fileName)) {
    LOG_ERROR << "Expected stream file path, got a directory.";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  std::filebuf* fileBuf = new std::filebuf;
  bool opened = fileBuf->open(fileName, mode);
  if (!opened) {
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  return fileBuf;
}
} // namespace

void gits::ensure_unique_ptr(uint64_t value) {
  static std::unordered_map<uint32_t, uint64_t> values;
  auto old = values[static_cast<uint32_t>(value)];
  if (old != 0 && old != value) {
    throw std::runtime_error("Collision detected for pointer value held in the stream.");
  }
  values[static_cast<uint32_t>(value)] = value;
}

void gits::check_uint_conversion_possibility(uint64_t value) {
  uint32_t uint32_value = static_cast<uint32_t>(value);
  if (uint32_value != value) {
    throw std::runtime_error("cannot convert uint64_t to unit32_t");
  }
}

gits::CBinOStream& gits::operator<<(gits::CBinOStream& o, const char* value) {
  o.write(value, strlen(value));
  return o;
}

gits::CBinOStream& gits::operator<<(gits::CBinOStream& o, const char& value) {
  o.write(reinterpret_cast<const char*>(&value), sizeof(value));
  return o;
}

gits::CBinOStream& gits::operator<<(gits::CBinOStream& o, const std::string& value) {
  o.write(reinterpret_cast<const char*>(value.c_str()), value.size());
  return o;
}

void HelperCopy(const char* dataToCopy,
                uint64_t dataToCopySize,
                std::vector<char>& dataToCompress,
                uint64_t& offset) {
  memcpy(dataToCompress.data() + offset, dataToCopy, dataToCopySize);
  offset += dataToCopySize;
}

void gits::CBinOStream::HelperWriteCompressed(const char* dataToWrite,
                                              uint64_t size,
                                              WriteType writeType) {
  WriteToOstream(reinterpret_cast<char*>(&size), sizeof(size));
  WriteToOstream(reinterpret_cast<char*>(&writeType), sizeof(writeType));
  uint64_t outputSize =
      CGits::Instance().GitsStreamCompressor().Compress(dataToWrite, size, &_compressedDataToStore);
  WriteToOstream(reinterpret_cast<char*>(&outputSize), sizeof(outputSize));
  WriteToOstream(_compressedDataToStore.data(), outputSize);
}

void gits::CBinOStream::HelperWriteCompressedLarge(const char* dataToWrite,
                                                   uint64_t size,
                                                   WriteType writeType) {
  // Write the size and writeType to the stream upfront
  WriteToOstream(reinterpret_cast<char*>(&size), sizeof(size));
  WriteToOstream(reinterpret_cast<char*>(&writeType), sizeof(writeType));

  // Calculate the number of chunks needed to compress the data in segments of _standaloneMaxSize
  uint64_t chunksNumber = (size + _standaloneMaxSize - 1) / _standaloneMaxSize;
  WriteToOstream(reinterpret_cast<char*>(&chunksNumber), sizeof(chunksNumber));

  // Iterate over each chunk, compress, and write it to the stream
  for (uint64_t i = 0; i < size; i += _standaloneMaxSize) {
    uint64_t currentChunkSize = std::min(_standaloneMaxSize, size - i);
    uint64_t compressedSize = CGits::Instance().GitsStreamCompressor().Compress(
        dataToWrite + i, currentChunkSize, &_compressedDataToStore);

    // Write the current chunk size and its compressed size before the actual compressed data
    WriteToOstream(reinterpret_cast<char*>(&currentChunkSize), sizeof(currentChunkSize));
    WriteToOstream(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
    WriteToOstream(_compressedDataToStore.data(), compressedSize);
  }
}

bool gits::CBinOStream::InitializeCompression() {
  if (!_initializedCompression) {
    WriteToOstream(reinterpret_cast<char*>(&_compressionType), sizeof(_compressionType));
    WriteToOstream(reinterpret_cast<char*>(&_chunkSize), sizeof(_chunkSize));
    if (_compressionType != CompressionType::NONE) {
      // Allocate buffer for PACKAGE type compression based on configured chunk size
      _dataToCompress.resize(_chunkSize);

      // Determine the larger of _chunkSize and _standaloneMaxSize to ensure sufficient buffer size for compression
      uint64_t max_chunk_size = std::max(_chunkSize, _standaloneMaxSize);

      // Resize _compressedDataToStore to accommodate the maximum possible compressed size of the determined chunk
      _compressedDataToStore.resize(
          CGits::Instance().GitsStreamCompressor().MaxCompressedSize(max_chunk_size));
    }
    _initializedCompression = true;
  }
  return _initializedCompression;
}

bool gits::CBinOStream::WriteCompressed(const char* data, uint64_t dataSize) {
  std::unique_lock<std::mutex> lock(mutex_);
  InitializeCompression();
  if (_compressionType == CompressionType::NONE) {
    WriteToOstream(data, dataSize);
  } else {
    if (dataSize < _chunkSize) {
      //small package
      if (dataSize + _offset < _chunkSize) {
        HelperCopy(data, dataSize, _dataToCompress, _offset);
      } else {
        HelperWriteCompressed(_dataToCompress.data(), _offset, WriteType::PACKAGE);
        _offset = 0;
        HelperCopy(data, dataSize, _dataToCompress, _offset);
      }
    } else {
      //big package
      if (_offset > 0) {
        //write old packages if available
        HelperWriteCompressed(_dataToCompress.data(), _offset, WriteType::PACKAGE);
        _offset = 0;
      }
      if (dataSize > _standaloneMaxSize) {
        // Handle data sizes larger than 256MB due to LZ4's 2GB compression size limit.
        HelperWriteCompressedLarge(data, dataSize, WriteType::LARGE_STANDALONE);
      } else {
        HelperWriteCompressed(data, dataSize, WriteType::STANDALONE);
      }
    }
  }
  return true;
}

bool gits::CBinOStream::WriteCompressedAndGetOffset(const char* data,
                                                    uint64_t dataSize,
                                                    uint64_t& offsetInFile,
                                                    uint64_t& offsetInChunk) {
  std::unique_lock<std::mutex> lock(mutex_);
  InitializeCompression();
  if (_compressionType == CompressionType::NONE) {
    offsetInFile = tellp();
    _offset = 0;
    WriteToOstream(data, dataSize);
  } else {
    if (dataSize < _chunkSize) {
      //small package
      if (dataSize + _offset < _chunkSize) {
        offsetInChunk = _offset;
        HelperCopy(data, dataSize, _dataToCompress, _offset);
      } else {
        HelperWriteCompressed(_dataToCompress.data(), _offset, WriteType::PACKAGE);
        _offset = 0;
        offsetInChunk = _offset;
        HelperCopy(data, dataSize, _dataToCompress, _offset);
      }
      offsetInFile = tellp();
    } else {
      //big package
      if (_offset > 0) {
        //write old packages if available
        HelperWriteCompressed(_dataToCompress.data(), _offset, WriteType::PACKAGE);
        _offset = 0;
      }
      offsetInChunk = _offset;
      offsetInFile = tellp();
      if (dataSize > _standaloneMaxSize) {
        // Handle data sizes larger than 256MB due to LZ4's 2GB compression size limit.
        HelperWriteCompressedLarge(data, dataSize, WriteType::LARGE_STANDALONE);
      } else {
        HelperWriteCompressed(data, dataSize, WriteType::STANDALONE);
      }
    }
  }
  return true;
}

std::ostream& gits::CBinOStream::WriteToOstream(const char* data, uint64_t dataSize) {
  try {
    auto& stream = std::ostream::write(data, dataSize);
    if (Configurator::Get().common.recorder.highIntegrity) {
      return std::ostream::flush();
    }
    return stream;
  } catch (std::ostream::failure& e) {
    LOG_ERROR << "Failed to write to the stream. Probably not enough space on the disk.";
    LOG_ERROR << "Err code: " << e.code() << " Err msg: " << e.what();
    throw std::runtime_error("Failed to write to the stream.");
  }
}

void gits::CBinOStream::write(const char* s, std::streamsize n) {
  WriteCompressed(s, n);
}

gits::CBinOStream::CBinOStream(const std::filesystem::path& fileName)
    : std::ostream(nullptr),
      _buf(nullptr),
      _compressionType(Configurator::Get().common.recorder.compression.type),
      _offset(0),
      _initializedCompression(false),
      _chunkSize(0),
      _standaloneMaxSize(268435456) {
  CheckMinimumAvailableDiskSize();
  std::ios::openmode mode = std::ios::binary | std::ios::trunc | std::ios::out;
  _buf = initialize_gits_streambuf(fileName, mode);
  init(_buf);
  exceptions(std::ostream::badbit | std::ostream::failbit);
  if (_compressionType != CompressionType::NONE) {
    _chunkSize = Configurator::Get().common.recorder.compression.chunkSize;
  }
}

gits::CBinOStream::~CBinOStream() {
  try {
    if (_offset > 0) {
      HelperWriteCompressed(_dataToCompress.data(), _offset, WriteType::PACKAGE);
      _offset = 0;
    }
    delete _buf;
  } catch (...) {
    topmost_exception_handler("CBinOStream::~CBinOStream");
  }
}

/**
 * @brief Constructor
 *
 * Constructor of gits::CBinIStream class that creates binary
 * file input stream with specified file name.
 *
 * @param fileName Name of a file to create.
 */
gits::CBinIStream::CBinIStream(const std::filesystem::path& fileName)
    : _file(nullptr),
      _path(fileName),
      _offset(0),
      _size(0),
      _actualOffsetInFile(0),
      _compressionType(CompressionType::NONE),
      _initializedCompression(false),
      _chunkSize(0),
      _standaloneMaxSize(268435456) {
  _file = fopen(fileName.string().c_str(), "rb"
#ifdef GITS_PLATFORM_WINDOWS
                                           "S"
#endif
  );
  if (_file == nullptr) {
    LOG_ERROR << "Couldn't open file: " << fileName;
    throw std::runtime_error("failed to opeprn file");
  }
}

bool gits::CBinIStream::InitializeCompression() {
  if (!_initializedCompression) {
    ReadHelper(reinterpret_cast<char*>(&_compressionType), sizeof(_compressionType));
    ReadHelper(reinterpret_cast<char*>(&_chunkSize), sizeof(_chunkSize));
    CGits::Instance().CompressorInit(_compressionType);
    if (_compressionType != CompressionType::NONE) {
      uint64_t max_size = std::min(_decompressedData.max_size(), _compressedData.max_size());
      uint64_t max_chunk_size = std::max(_chunkSize, _standaloneMaxSize);
      if (max_chunk_size > 0 && max_chunk_size <= max_size) {
        // Allocate buffer for PACKAGE type compression based on configured chunk size
        _decompressedData.resize(_chunkSize);

        // Resize _compressedData to accommodate the maximum possible compressed size of the determined chunk
        _compressedData.resize(
            CGits::Instance().GitsStreamCompressor().MaxCompressedSize(max_chunk_size));
      }
    }
    _initializedCompression = true;
  }
  return _initializedCompression;
}

bool gits::CBinIStream::LoadChunk() {
  uint64_t size = 0;
  ReadHelper(reinterpret_cast<char*>(&size), sizeof(size));
  WriteType writeType = WriteType::STANDALONE;
  ReadHelper(reinterpret_cast<char*>(&writeType), sizeof(writeType));
  uint64_t compressedSize = 0;
  ReadHelper(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
  if (eof()) {
    return false;
  }
  if (compressedSize > _compressedData.max_size()) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (compressedSize > _compressedData.size()) {
    _compressedData.resize(compressedSize);
  }
  ReadHelper(_compressedData.data(), compressedSize);
  if (writeType == WriteType::PACKAGE) {
    _size = size;
    _offset = 0;
    CGits::Instance().GitsStreamCompressor().Decompress(_compressedData, compressedSize, _size,
                                                        _decompressedData.data());
    return true;
  } else {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
}

bool gits::CBinIStream::ReadCompressed(char* data, uint64_t dataSize) {
  InitializeCompression();
  uint64_t internalOffset = 0;
  if (_compressionType == CompressionType::NONE) {
    ReadHelper(data, dataSize);
  } else {
    if (_offset < _size) {
      if (_offset + dataSize <= _size) {
        //we have data loaded, just copy it and return
        memcpy(data, _decompressedData.data() + _offset, dataSize);
        _offset += dataSize;
        return true;
      } else {
        //It is possible that sometimes we have different logic for Read and Write functions (e.g. storing elem by elem, loading whole vector). In this case we need to load partly from old chunk and partly from new.
        internalOffset = _size - _offset;
        memcpy(data, _decompressedData.data() + _offset, internalOffset);
        _offset += internalOffset;
        dataSize -= internalOffset;
      }
    }
    if (eof()) {
      return false;
    }
    uint64_t size = 0;
    ReadHelper(reinterpret_cast<char*>(&size), sizeof(size));
    WriteType writeType = WriteType::STANDALONE;
    ReadHelper(reinterpret_cast<char*>(&writeType), sizeof(writeType));
    if (writeType == WriteType::LARGE_STANDALONE) {
      uint64_t chunksNumber = 0;
      ReadHelper(reinterpret_cast<char*>(&chunksNumber), sizeof(chunksNumber));

      if (chunksNumber > size) {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }

      for (uint64_t i = 0; i < chunksNumber; ++i) {
        uint64_t currentChunkSize = 0;
        uint64_t currentCompressedChunkSize = 0;

        // Read the uncompressed and compressed sizes for the current chunk
        ReadHelper(reinterpret_cast<char*>(&currentChunkSize), sizeof(currentChunkSize));
        ReadHelper(reinterpret_cast<char*>(&currentCompressedChunkSize),
                   sizeof(currentCompressedChunkSize));

        if (currentCompressedChunkSize > _compressedData.size()) {
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }

        // Read the compressed data for the current chunk
        ReadHelper(_compressedData.data(), currentCompressedChunkSize);

        // Decompress the current chunk
        CGits::Instance().GitsStreamCompressor().Decompress(
            _compressedData, currentCompressedChunkSize, currentChunkSize,
            (char*)data + i * _standaloneMaxSize);
      }

      // Reset offset and size after processing all chunks
      _offset = 0;
      _size = 0;
    } else {
      uint64_t compressedSize = 0;
      ReadHelper(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
      if (eof()) {
        return false;
      }
      if (compressedSize > _compressedData.max_size()) {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      if (compressedSize > _compressedData.size()) {
        _compressedData.resize(compressedSize);
      }
      ReadHelper(_compressedData.data(), compressedSize);
      if (writeType == WriteType::PACKAGE) {
        _size = size;
        _offset = 0;
        CGits::Instance().GitsStreamCompressor().Decompress(_compressedData, compressedSize, _size,
                                                            _decompressedData.data());

        memcpy((char*)data + internalOffset, _decompressedData.data() + _offset, dataSize);
        _offset += dataSize;
      } else {
        CGits::Instance().GitsStreamCompressor().Decompress(_compressedData, compressedSize, size,
                                                            data);
        _offset = 0;
        _size = 0;
      }
    }
  }
  return true;
}

void* gits::CBinIStream::ReadWithOffset(char* data,
                                        uint64_t dataSize,
                                        uint64_t offsetInFile,
                                        uint64_t offsetInChunk) {
  if (offsetInFile != _actualOffsetInFile) {
    int seekResult = fileseek(_file, offsetInFile, SEEK_SET);
    if (seekResult != 0) {
      throw std::runtime_error("Failed to seek the specified position in the file.");
    }
    _actualOffsetInFile = offsetInFile;
    _size = 0;
    if (offsetInChunk != 0) {
      LoadChunk();
    }
  }
  _offset = offsetInChunk;
  read(data, dataSize);
  if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
    _actualOffsetInFile = offsetInFile + dataSize;
  }
  return nullptr;
}

bool gits::CBinIStream::ReadHelper(char* buf, size_t size) {
#ifdef GITS_PLATFORM_WINDOWS
  auto ret = _fread_nolock_s(buf, size, 1, size, _file);
#else
  auto ret = fread(buf, 1, size, _file);
#endif
  return ret != 0;
}

bool gits::CBinIStream::read(char* buf, size_t size) {
  if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
    return ReadHelper(buf, size);
  } else {
    return ReadCompressed(buf, size);
  }
}

int gits::CBinIStream::tellg() const {
  return ftell(_file);
}

int gits::CBinIStream::getc() {
  if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
    return fgetc(_file);
  } else {
    int buf = 0;
    ReadCompressed(reinterpret_cast<char*>(&buf), 1);
    return buf;
  }
}

void gits::CBinIStream::get_delimited_string(std::string& s, char t) {
  s.resize(1024);
  int delim = getc();
  if (delim != t) {
    throw std::runtime_error("unexpected delimiter of string read");
  }

  for (int i = 0; i < 1024; ++i) {
    char byte = (char)getc();
    if (byte == t) {
      s.resize(i);
      return;
    }
    s[i] = byte;
  }

  throw std::runtime_error("error reading delimited string from file");
}

bool gits::CBinIStream::eof() const {
  return feof(_file);
}

int gits::CBinIStream::fileseek(FILE* stream, uint64_t offset, int origin) {
#ifdef GITS_PLATFORM_WINDOWS
  return _fseeki64(_file, offset, origin);
#else
  return fseeko64(_file, offset, origin);
#endif
}

gits::CBinIStream::~CBinIStream() {
  fclose(_file);
}
