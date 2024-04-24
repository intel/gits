// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
    Log(ERR) << "Expected stream file path, got a directory.";
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
  uint64_t outputSize = _compressor->Compress(dataToWrite, size, &_compressedDataToStore);
  WriteToOstream(reinterpret_cast<char*>(&outputSize), sizeof(outputSize));
  WriteToOstream(_compressedDataToStore.data(), outputSize);
}

bool gits::CBinOStream::WriteCompressed(const char* data, uint64_t dataSize) {
  if (!_initializedCompression) {
    WriteToOstream(reinterpret_cast<char*>(&_compressionType), sizeof(_compressionType));
    WriteToOstream(reinterpret_cast<char*>(&_chunkSize), sizeof(_chunkSize));
    _initializedCompression = true;
  }
  if (_compressionType == CompressionType::NONE) {
    WriteToOstream(data, dataSize);
  } else {
    if (dataSize < COMPRESSED_PACKAGE_SIZE) {
      //small package
      if (dataSize + _offset < COMPRESSED_PACKAGE_SIZE) {
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
      HelperWriteCompressed(data, dataSize, WriteType::STANDALONE);
    }
  }
  return true;
}

std::ostream& gits::CBinOStream::WriteToOstream(const char* data, uint64_t dataSize) {
  return std::ostream::write(data, dataSize);
}

void gits::CBinOStream::write(const char* s, std::streamsize n) {
  WriteCompressed(s, n);
}

gits::CBinOStream::CBinOStream(const std::filesystem::path& fileName)
    : std::ostream(nullptr),
      _buf(nullptr),
      _compressor(new LZ4StreamCompressor()),
      _compressionType(CompressionType::LZ4),
      _offset(0),
      _initializedCompression(false) {
  CheckMinimumAvailableDiskSize();
  std::ios::openmode mode = std::ios::binary | std::ios::trunc | std::ios::out;
  _buf = initialize_gits_streambuf(fileName, mode);
  init(_buf);
  _dataToCompress.resize(COMPRESSED_PACKAGE_SIZE);
  _compressedDataToStore.resize(COMPRESSED_PACKAGE_SIZE);
}

gits::CBinOStream::~CBinOStream() {
  if (_offset > 0) {
    HelperWriteCompressed(_dataToCompress.data(), _offset, WriteType::PACKAGE);
    _offset = 0;
  }
  delete _buf;
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
    : _file(nullptr), _path(fileName), _offset(0), _size(0), _initializedCompression(false) {
  _file = fopen(fileName.string().c_str(), "rb"
#ifdef GITS_PLATFORM_WINDOWS
                                           "S"
#endif
  );
  if (_file == nullptr) {
    Log(ERR) << "Couldn't open file: " << fileName;
    throw std::runtime_error("failed to opeprn file");
  }
  _decompressedData.resize(COMPRESSED_PACKAGE_SIZE);
  _compressedData.resize(COMPRESSED_PACKAGE_SIZE);
}

bool gits::CBinIStream::ReadCompressed(char* data, uint64_t dataSize) {
  if (!_initializedCompression) {
    ReadHelper(reinterpret_cast<char*>(&_compressionType), sizeof(_compressionType));
    ReadHelper(reinterpret_cast<char*>(&_chunkSize), sizeof(_chunkSize));
    if (_compressionType == CompressionType::LZ4) {
      _compressor = new LZ4StreamCompressor;
    } else if (_compressionType == CompressionType::ZSTD) {
      _compressor = new ZSTDStreamCompressor;
    }
    _initializedCompression = true;
  }
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
        uint64_t oldChunkToCopySize = _size - _offset;
        memcpy(data, _decompressedData.data() + _offset, oldChunkToCopySize);
        _offset += oldChunkToCopySize;
        dataSize -= oldChunkToCopySize;
        data += oldChunkToCopySize;
      }
    }
    if (eof()) {
      return false;
    }
    uint64_t size = 0;
    ReadHelper(reinterpret_cast<char*>(&size), sizeof(size));
    WriteType writeType = WriteType::STANDALONE;
    ReadHelper(reinterpret_cast<char*>(&writeType), sizeof(writeType));
    uint64_t compressedSize = 0;
    ReadHelper(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
    if (eof()) {
      return false;
    }
    if (compressedSize > _compressedData.size()) {
      _compressedData.resize(compressedSize);
    }
    ReadHelper(_compressedData.data(), compressedSize);
    if (writeType == WriteType::PACKAGE) {
      _size = size;
      _offset = 0;
      _compressor->Decompress(_compressedData, compressedSize, _size, _decompressedData.data());

      memcpy(data, _decompressedData.data() + _offset, dataSize);
      _offset += dataSize;
    } else {
      _compressor->Decompress(_compressedData, compressedSize, size, data);
      _offset = 0;
      _size = 0;
    }
  }
  return true;
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

gits::CBinIStream::~CBinIStream() {
  fclose(_file);
}

/**
 * @brief Constructor
 *
 * Constructor of gits::CCodeOStream class that creates text
 * file output stream with specified file name. Output file
 * will have C code representation of captured function calls.
 *
 * @param fileName Name of a file to create.
 */

gits::CCodeOStream::CCodeOStream(const std::string& base_filename)
    : std::ostream(&_fileBuffers[0]), _currBuffer(GITS_MAIN_CPP), _baseName(base_filename) {
  CheckMinimumAvailableDiskSize();

  for (auto& bufferIndentLevel : _indentLevels) {
    bufferIndentLevel = 0;
  }

  TScopeData scope = {0};
  _scopeList.push_front(scope);

  //validate _filesData
  for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
    if (FILES_INFO[i].file != static_cast<TGitsSourceFile>(i)) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }

  //initialize other filebuffers
  for (size_t i = 1; i < BUFFER_COUNT; ++i) {
    std::ios::init(&_fileBuffers[i]);
  }

  //open buffers
  for (size_t i = 0; i < BUFFER_COUNT; ++i) {
    if (_fileBuffers[i].open((base_filename + FILES_INFO[i].fileExt).c_str(),
                             ios_base::out | std::ios::binary) == nullptr) {
      std::ios::setstate(ios_base::failbit);
    }
  }

  //output common file content
  OnFilesOpen();

  //set default buffer
  std::ostream::rdbuf(&_fileBuffers[GITS_MAIN_CPP]);
}

gits::CCodeOStream::~CCodeOStream() {
  try {
    //output some closing stuff
    OnFilesClose();

    for (auto& fileBuffer : _fileBuffers) {
      fileBuffer.close();
    }
  } catch (...) {
    topmost_exception_handler("CCodeOStream::~CCodeOStream");
  }
}

std::filebuf* gits::CCodeOStream::rdbuf() const {
  return const_cast<std::filebuf*>(&_fileBuffers[_currBuffer]);
}

bool gits::CCodeOStream::is_open() const {
  bool allOpen = true;
  for (const auto& fileBuffer : _fileBuffers) {
    allOpen = allOpen && fileBuffer.is_open();
  }
  return allOpen;
}

gits::CCodeOStream& gits::CCodeOStream::select(gits::CCodeOStream::TGitsSourceFile file) {
  flush();

  if (file < GITS_FILES_COUNT) {
    _currBuffer = file;
    std::ostream::rdbuf(&_fileBuffers[_currBuffer]);
  } else {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *this;
}

gits::CCodeOStream::TGitsSourceFile gits::CCodeOStream::selectCCodeFile() {
  auto& gits = CGits::Instance();
  CCodeOStream::TGitsSourceFile file = CCodeOStream::GITS_FRAMES_CPP;
  if (gits.IsCCodePreRecord()) {
    file = CCodeOStream::GITS_PRE_RECORDER_CPP;
  } else if (gits.IsCCodeStateRestore()) {
    file = CCodeOStream::GITS_STATE_RESTORE_CPP;
  }
  return file;
}

//assert(_filesInfo[i].file == i);
const gits::CCodeOStream::TFileInfo gits::CCodeOStream::FILES_INFO[] = {
    {GITS_MAIN_CPP, "_main.cpp"},
    {GITS_FRAMES_CPP, "_frames.cpp"},
    {GITS_EXTERNS_CPP, "_externs.cpp"},
    {GITS_EXTERNS_H, "_externs.h"},
    {GITS_STATE_RESTORE_CPP, "_state_restore.cpp"},
    {GITS_PRE_RECORDER_CPP, "_pre_recorder.cpp"}};

//g++ 4.3.3 doesn't seem to like it when std::toupper is used directly in
//std::transform, this wrapper is a workaround for that
namespace {
struct ToUpper {
  char operator()(char arg) const {
    return char(std::toupper(arg));
  }
};
} // namespace

void gits::CCodeOStream::OnFilesOpen() {
  CCodeOStream& stream = *this;

  for (auto& file_info : FILES_INFO) {
    stream.select(file_info.file) << std::showbase
                                  << "/***********************************************\\\n"
                                     " This file was generated with GITS version:      \n"
                                     "  "
                                  << CGits::Instance().Version()
                                  << "\n"
                                     "\\***********************************************/\n\n";
  }

  const std::string::size_type slash = _baseName.rfind('\\');
  const std::string::size_type bckSlash = _baseName.rfind('/');
  const std::string::size_type minmax =
      (slash == std::string::npos || bckSlash == std::string::npos) ? (std::min)(slash, bckSlash)
                                                                    : (std::max)(slash, bckSlash);
  const std::string::size_type fileNameBegin = 1 + minmax;
  std::string fileNameBase = _baseName.substr(fileNameBegin);

  TGitsSourceFile filesWithIncludes[] = {GITS_FRAMES_CPP, GITS_STATE_RESTORE_CPP,
                                         GITS_PRE_RECORDER_CPP};
  for (const auto& file : filesWithIncludes) {
    stream.select(file);
    stream << "#include \"gitsApi.h\"\n";
    stream << "#include \"" << fileNameBase << FILES_INFO[GITS_EXTERNS_H].fileExt << "\""
           << std::endl;

    stream << "#include \"helperGL.h\"" << std::endl;
    stream << "#include \"helperVk.h\"" << std::endl;
    stream << "#include \"helperCL.h\"" << std::endl;
#ifdef WITH_LEVELZERO
    stream << "#ifdef WITH_LEVELZERO" << std::endl;
    stream << "#include \"helperL0.h\"" << std::endl;
    stream << "#endif" << std::endl;
#endif
  }

  //gl includes for main
  stream.select(GITS_MAIN_CPP);

  //include our dump declarations
  stream << "#include \"gitsApi.h\"\n";

  stream << "\n\nvoid RunFrames()\n{" << std::endl;

  stream.select(GITS_EXTERNS_CPP);
  stream << "#include \"" << fileNameBase << FILES_INFO[GITS_EXTERNS_H].fileExt << "\"\n"
         << std::endl;

  {
    stream.select(GITS_EXTERNS_H) << "#pragma once" << std::endl;
    stream << "#include \"gitsApi.h\"\n";
    stream << "#include <memory>\n";
    stream << "#include <vector>\n\n";
  }
}

void gits::CCodeOStream::OnFilesClose() {
  CCodeOStream& stream = *this;
  //files need to be completed here
  //close drawing function
  stream.select(GITS_MAIN_CPP) << "}" << std::endl;
}

void gits::CCodeOStream::ScopeBegin() {
  _indentLevels[_currBuffer] += INDENT_STEP;
  TScopeData scope = {0};
  _scopeList.push_front(scope);
}

void gits::CCodeOStream::ScopeEnd() {
  _indentLevels[_currBuffer] = std::max(0, _indentLevels[_currBuffer] - INDENT_STEP);
  _scopeList.pop_front();
}

gits::CCodeOStream& gits::CCodeOStream::Indent() {
  if (_indentLevels[_currBuffer]) {
    CCodeOStream& stream = *this;
    stream << std::setfill(' ') << std::setw(_indentLevels[_currBuffer]) << " ";
  }
  return *this;
}

void gits::CCodeOStream::Register(intptr_t key,
                                  const std::string& prefix,
                                  bool addId,
                                  bool globalVar) {
  std::stringstream str;
  str << prefix;

  if (globalVar) {
    if (addId) {
      str << "_" << ++_scopeList.back().lastId;
    }
    _scopeList.back().variables[key] = str.str();
  } else {
    if (addId) {
      str << "_" << ++_scopeList.front().lastId;
    }
    _scopeList.front().variables[key] = str.str();
  }
}

std::string gits::CCodeOStream::VariableName(intptr_t key) const {
  for (const auto& scope : _scopeList) {
    auto it2 = scope.variables.find(key);
    if (it2 != scope.variables.end()) {
      return it2->second;
    }
  }
  Log(ERR) << "Key '" << std::hex << key << std::dec << "' not found!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
