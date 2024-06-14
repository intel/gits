// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gits.cpp
 *
 * @brief Definition of GITS project information class.
 *
 */

#include "gits.h"
#include "streams.h"
#include "hashing.h"
#include "exception.h"
#include "log.h"
#include "pragmas.h"
#include "config.h"
#include "scheduler.h"
#include "token.h"
#include "function.h"
#include "tools.h"
#include "automateCCode.h"

#include <string>
#include <iostream>

namespace gits {
// Developer builds are of the form: dd.dd.dd.999
#ifndef VERSION_4
#define VERSION_4 999
#else
#if !defined(VERSION_1) || !defined(VERSION_2) || !defined(VERSION_3)
#error Inconsistently defined build label
#endif
#endif

// Developer versions will build to be compatible with latest stream format
// (denoted by first three components) and build number of 999
const int version_num_1 = 2;
const int version_num_2 = 0;
const int version_num_3 = 10;
const int version_num_4 = VERSION_4;

static_assert(VERSION_4 != 0,
              "Zero on last version component is reserved for compatibility checks.");

// Make sure we don't build GITS with mismatched version numbers
// Build system should set VERSION_x for 0-3, and if first three
// components do not match file set version, compile error will
// occur
#if defined(VERSION_1) || defined(VERSION_2) || defined(VERSION_3)
static_assert(version_num_1 == VERSION_1, "Mismatched version numbers.");
static_assert(version_num_2 == VERSION_2, "Mismatched version numbers.");
static_assert(version_num_3 == VERSION_3, "Mismatched version numbers.");
#endif

CGits* CGits::_instance;

/**
 * @brief Disposes of singleton GITS object instance
 *
 * Explicit destruction of this singleton was required for
 * proper cleanup on GNU/Linux platform
 */
void CGits::Dispose() {
  delete _instance;
  _instance = nullptr;
}

CGits::CGits() : CGits(version_num_1, version_num_2, version_num_3, version_num_4) {}

/**
 * @brief Constructor
 *
 * Constructor of CGits class.
 *
 * @param version version data
 */
CGits::CGits(uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3)
    : _version(v0, v1, v2, v3),
      _currentThreadId(0),
      _multithreadedApp(false),
      _kernelCounter(0),
      _cmdListCounter(0),
      _cmdQueueExecCounter(0),
      _drawCounter(0),
      _drawInFrameCounter(0),
      _finished(false),
      _glProgramsZipFile(nullptr),
      _glProgramsUnZipFile(nullptr),
      _frameNo(1),
      _restoringState(false),
      _ccodePreRecord(false),
      _ccodeStateRestore(false),
      _sc(nullptr),
      _currentLocalMemoryUsage(0),
      _maxLocalMemoryUsage(0),
      traceGLAPIBypass(false) {
  _ptrToOrderedId[nullptr] = 0;
}

CGits::~CGits() {
  try {
    // Release all resources explicitly, before signature creation.
    _imageWriter.finish();
    _file.reset();
    _libraryList.clear();
    _resources.reset();
    _resources2.reset();
    _compressor.reset();

    // Only create signature when recording, player can opt in through option.
    if (Config::Get().IsRecorder() && Config::Get().common.recorder.enabled) {
      sign_directory(Config::Get().common.recorder.dumpPath);
    }

    // Create a buildable CCode project by automatically copying necessary files.
    // TODO: extract it to a separate function.
    if (Config::Get().IsRecorder() && Config::Get().dumpCCode()) {
      auto& dumpPath = Config::Get().common.recorder.dumpPath;
      auto& installPath = Config::Get().common.recorder.installPath;
      std::filesystem::path ccodePath = installPath.parent_path() / "CCode";
      std::filesystem::path ccodeFiles[] = {dumpPath / "stream_externs.cpp",
                                            dumpPath / "stream_externs.h",
                                            dumpPath / "stream_main.cpp"};

      std::filesystem::path streamFramesFilePath = dumpPath / "stream_frames.cpp";
      std::filesystem::path streamStateRestorePath = dumpPath / "stream_state_restore.cpp";
      std::filesystem::path streamPreRecorderPath = dumpPath / "stream_pre_recorder.cpp";
      std::filesystem::path ccodeFilesDest = dumpPath / "CCodeSource/StreamFiles";

      std::filesystem::copy(ccodePath, dumpPath, std::filesystem::copy_options::recursive);
      std::filesystem::create_directory(ccodeFilesDest);

      for (const auto& file : ccodeFiles) {
        std::filesystem::rename(file, ccodeFilesDest / file.filename());
      }

      int fileDividerInBytes = 1048576; // each file after splitting is approximately 1 MB
      std::filesystem::path filesForDivide[] = {std::move(streamFramesFilePath),
                                                std::move(streamStateRestorePath),
                                                std::move(streamPreRecorderPath)};
      for (const auto& file : filesForDivide) {
        int divideCount = std::filesystem::file_size(file) / fileDividerInBytes;
        if (divideCount > 1) {
          auto breakPoints = scopeAnalyze(file);
          divideFile(file, ccodeFilesDest, breakPoints);
        } else {
          std::filesystem::rename(file, ccodeFilesDest / file.filename());
        }
      }
    }
  } catch (...) {
    topmost_exception_handler("CGits::~CGits");
  }
}

void CGits::AddLocalMemoryUsage(const size_t& size) {
  _currentLocalMemoryUsage += size;
  if (_currentLocalMemoryUsage > _maxLocalMemoryUsage) {
    _maxLocalMemoryUsage = _currentLocalMemoryUsage;
  }
}

void CGits::SubtractLocalMemoryUsage(const size_t& size) {
  _currentLocalMemoryUsage -= size;
}

size_t CGits::GetMaxLocalMemoryUsage() const {
  return _maxLocalMemoryUsage;
}

zipFile CGits::OpenZipFileGLPrograms() {
  if (_glProgramsZipFile != nullptr) {
    return _glProgramsZipFile;
  }

  std::filesystem::path path = Config::Get().common.recorder.dumpPath / "gitsPrograms.zip";
  auto zipName = path.string();

  _glProgramsZipFile = zipOpen(zipName.c_str(), APPEND_STATUS_ADDINZIP);
  if (_glProgramsZipFile == nullptr) {
    _glProgramsZipFile = zipOpen(zipName.c_str(), APPEND_STATUS_CREATE);
  }
  if (_glProgramsZipFile == nullptr) {
    CheckMinimumAvailableDiskSize();
    throw std::runtime_error("couldn't create zip archive: " + zipName);
  }

  return _glProgramsZipFile;
}

void CGits::CloseZipFileGLPrograms() {
  if (_glProgramsZipFile == nullptr) {
    return;
  }
  int r = zipClose(_glProgramsZipFile, nullptr);

  if (r != ZIP_OK) {
    CheckMinimumAvailableDiskSize();
    throw std::runtime_error("failed closing zip archive");
  }
  _glProgramsZipFile = nullptr;
}

namespace {
void EraseFromFirstNullChar(std::string& str) {
  auto nullCharPos = str.find('\0');
  if (nullCharPos != std::string::npos) {
    str.erase(nullCharPos);
  }
}
CToken* CTokenCreate(CId id) {
  switch (*id) {
  case CToken::ID_INIT_START:
    return new CTokenFrameNumber(CToken::ID_INIT_START);
  case CToken::ID_INIT_END:
    return new CTokenFrameNumber(CToken::ID_INIT_END);
  case CToken::ID_FRAME_START:
    return new CTokenFrameNumber(CToken::ID_FRAME_START);
  case CToken::ID_FRAME_END:
    return new CTokenFrameNumber(CToken::ID_FRAME_END);
  case CToken::ID_PRE_RECORD_START:
    return new CTokenFrameNumber(CToken::ID_PRE_RECORD_START);
  case CToken::ID_PRE_RECORD_END:
    return new CTokenFrameNumber(CToken::ID_PRE_RECORD_END);
  case CToken::ID_CCODE_FINISH:
    return new CTokenFrameNumber(CToken::ID_CCODE_FINISH);
  case CToken::ID_PLAYER_RECORDER_SYNC:
    return new CTokenPlayerRecorderSync;
  case CToken::ID_MAKE_CURRENT_THREAD:
    return new CTokenMakeCurrentThread;
  case CToken::ID_MAKE_CURRENT_THREAD_NO_CTX_SWITCH:
    return new CTokenMakeCurrentThreadNoCtxSwitch;
  case CToken::ID_REC_SCREEN_RESOLUTION:
    return new CTokenScreenResolution;
  default:
    return nullptr;
  }
}
} // namespace
void CGits::OpenUnZipFileGLPrograms() {
  if (_glProgramsUnZipFile != nullptr) {
    return;
  }

  //Open zip file
  std::filesystem::path arch_path = Config::Get().common.player.streamDir / "gitsPrograms.zip";
  _glProgramsUnZipFile = unzOpen(arch_path.string().c_str());
  if (_glProgramsUnZipFile == nullptr) {
    throw std::runtime_error("failed to open zip archive");
  }

  //Scan zip file and create map file_name->location
  ZippedFileInfo fileInfo;
  unz_file_info unzInfo;
  std::string fileName;
  fileName.reserve(100);
  int stat;
  stat = unzGoToFirstFile(_glProgramsUnZipFile);
  while (stat == UNZ_OK) {
    unzGetFilePos(_glProgramsUnZipFile, &fileInfo.location);
    fileName.clear();
    fileName.resize(100, '\0');
    unzGetCurrentFileInfo(_glProgramsUnZipFile, &unzInfo, &fileName[0], (uLong)fileName.size(),
                          nullptr, 0, nullptr, 0);
    fileInfo.size = unzInfo.uncompressed_size;
    EraseFromFirstNullChar(fileName);
    _glProgramsLocationsInZipFile[fileName] = fileInfo;
    stat = unzGoToNextFile(_glProgramsUnZipFile);
  }
}

void CGits::ReadGlProgramFromUnZipFile(std::string progname, std::string& text) {
  //Get files params form a map
  EraseFromFirstNullChar(progname);
  auto iter = _glProgramsLocationsInZipFile.find(progname);
  if (iter == _glProgramsLocationsInZipFile.end()) {
    throw std::runtime_error("failed, shader file not found in a zip file");
  }
  unz_file_pos location = iter->second.location;
  unsigned size = iter->second.size;

  //Read files
  unzGoToFilePos(_glProgramsUnZipFile, &location);
  int stat = unzOpenCurrentFile(_glProgramsUnZipFile);
  if (stat != UNZ_OK) {
    throw std::runtime_error("failed to open zip archive file for writing");
  }
  text.resize(size);
  stat = unzReadCurrentFile(_glProgramsUnZipFile, &text[0], (unsigned)text.size());
  if (stat == UNZ_ERRNO) {
    throw std::runtime_error("failed reading shader file");
  }
}

void CGits::CloseUnZipFileGLPrograms() {
  if (_glProgramsUnZipFile == nullptr) {
    return;
  }
  unzClose(_glProgramsUnZipFile);
  _glProgramsUnZipFile = nullptr;
}

//This method maps passed pointers to ordered unsigned integers.
//Those values are being written to the stream instead of pointer and allows to optimize playback unmapping operations.
uint64_t CGits::GetOrderedIdFromPtr(void* ptr) {
  static uint64_t newId = 20; //left some space for special cases
  uint64_t id = 0;
  if (_ptrToOrderedId.find(ptr) == _ptrToOrderedId.end()) {
    id = newId++;
    _ptrToOrderedId[ptr] = id;
  } else {
    id = _ptrToOrderedId[ptr];
  }
  return id;
}

/**
 * @brief Registers new library class
 *
 * Method registers new library class.
 *
 * @note Ownership is passed to GITS class.
 *
 * @param library Library class to register
 */
void CGits::Register(std::shared_ptr<CLibrary> library) {
  _libraryList.push_back(std::move(library));
}

/**
* @brief Registers new file class
*
* Method registers new file class.
*
* @note Ownership is passed to GITS class.
*
* @param file File class to register
*/
void CGits::Register(std::unique_ptr<CFile> file) {
  _file = std::move(file);
}

/**
 * @brief Selects library for use
 *
 * Method selects library for use.
 *
 * @param id Identifier of library that should be selected.
 *
 * @exception EOperationFailed Bad Identifier or library not registered.
 *
 * @return Pointer to library if library is set, 0 otherwise.
 */
CLibrary& CGits::Library(CLibrary::TId id) {
  // check if 'id' is from allowed range
  for (auto& lib : _libraryList) {
    if (lib->Id() == id) {
      return *lib;
    }
  }

  Log(ERR) << "Invalid library '" << id << "' requested";
  throw ENotFound(EXCEPTION_MESSAGE);
}

CToken* CGits::TokenCreate(CId id) {
  CToken* token = CTokenCreate(id);
  if (token != nullptr) {
    return token;
  }

  for (auto& lib : _libraryList) {
    token = lib->FunctionCreate(*id);
    if (token != nullptr) {
      return token;
    }
  }

  Log(ERR) << "Unknown token id requested for creation: " << *id;
  throw std::runtime_error("failed to create token");
}

/**
* @brief Returns reference to GITS file data
*
* Method returns a reference to GITS file data.
*
* @exception ENotFound GITS file data not set.
*
* @return Reference to GITS file data
*/
CFile& CGits::File() const {
  if (_file) {
    return *_file;
  }

  Log(ERR) << "GITS file data was not set properly!!!";
  throw ENotFound(EXCEPTION_MESSAGE);
}

void CGits::SetSC(StreamingContext* sc) {
  _sc = sc;
}

const StreamingContext* CGits::GetSC() const {
  if (_sc == nullptr) {
    throw std::runtime_error("streaming context not set");
  }
  return _sc;
}

// Class contains unique_ptr's to incomplete (in header) types.
StreamingContext::~StreamingContext() {}

/**
 * @brief Text dumper of GITS project information
 *
 * Method that dumps project information in text form.
 *
 * @param stream Output stream to use
 * @param version GITS project class to use
 *
 * @return Output stream
 */
std::ostream& operator<<(std::ostream& stream, const CGits& g) {
  stream << "gitsPlayer version " << g._version;
#if defined GITS_ARCH_X86
  stream << " x86";
#elif defined GITS_ARCH_X64
  stream << " x64";
#elif defined GITS_ARCH_ARM
  stream << " ARM";
#elif defined GITS_ARCH_A64
  stream << " AARCH64";
#else
#error Unknown platform
#endif
  return stream;
}

/**
 * @brief Binary dumper of GITS project information
 *
 * Method that saves GITS project information to a binary file.
 *
 * @param stream Output stream to use
 * @param g GITS project class to use
 *
 * @return Output stream
 */
CBinOStream& operator<<(CBinOStream& stream, const CGits& g) {
  stream << g._version;
  stream << g.File();
  return stream;
}

/**
 * @brief Binary loader of GITS project information
 *
 * Method that loads GITS project information from a binary file.
 *
 * @param stream Input stream to use
 * @param g GITS project class to use
 *
 * @return Input stream
 */
CBinIStream& operator>>(CBinIStream& stream, CGits& g) {
  // obtain GITS software version that was used to save that file
  CVersion version;
  stream >> version;

  Log(INFO, NO_PREFIX) << "Sequence recorded with: " << version;
  // check if file was not written with a newer version of GITS software
  if (g._version < version) {
    Log(WARN) << "File is written with newer version: " << version << "!!!";
  }

  // load file data
  std::unique_ptr<CFile> file(new CFile(version));
  stream >> *file;
  g.Register(std::move(file));

  return stream;
}

void CGits::CCounter::operator++(int) {
  (countersTable.back())++;
}

CGits::CCounter::CCounter() {
  countersTable.push_back(1);
}

CGits::CCounter::CCounter(std::initializer_list<uint64_t> init) : countersTable(init) {
  countersTable.push_back(0); //initial counter
}

/* **************************** F I L E ********************************* */

CFile::CFile(const CVersion& version)
    : _version(version), _properties(new nlohmann::ordered_json) {}

const CVersion& CFile::Version() const {
  return _version;
}

void CFile::SkippedCallAdd(unsigned id) {
  _skippedCalls[id]++;
}

const CFile::CSkippedCalls& CFile::SkippedCalls() const {
  return _skippedCalls;
}

nlohmann::ordered_json& CFile::GetProperties() const {
  return *_properties;
}

std::string CFile::ReadProperties() const {
  if (!_formerProperties.empty()) {
    return _formerProperties;
  }
  return _properties->dump(2);
}

void CGits::ResourceManagerInit(const std::filesystem::path& dump_dir) {
  const auto& mappings = resource_filenames(dump_dir);
  if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
    _resources.reset(new CResourceManager(mappings));
  } else {
    _resources2.reset(new CResourceManager2(mappings));
  }
}

void CGits::CompressorInit(CompressionType compressionType) {
  if (_compressor != nullptr) {
    return;
  }
  if (compressionType == CompressionType::LZ4) {
    _compressor = std::make_unique<LZ4StreamCompressor>();
  } else if (compressionType == CompressionType::ZSTD) {
    _compressor = std::make_unique<ZSTDStreamCompressor>();
  } else {
    _compressor = nullptr;
  }
}

void CGits::CurrentThreadId(int threadId) {
  if (threadId != _currentThreadId) {
    _multithreadedApp = true;
  }
  _currentThreadId = threadId;
}

void CGits::WriteImage(const std::string& filename,
                       size_t width,
                       size_t height,
                       bool hasAlpha,
                       std::vector<uint8_t>& data,
                       bool flip,
                       bool isBGR,
                       bool isSRGB) {
  if (!_imageWriter.running()) {
    _imageWriter.start(ImageWriter());
  }

  Image img(filename, width, height, hasAlpha, data, flip, isBGR, isSRGB);
  _imageWriter.queue().produce(img);
}

CBinOStream& operator<<(CBinOStream& stream, const CFile& file) {
  // dump all skipped calls to a file
  unsigned skipNum = unsigned(file._skippedCalls.size());
  stream.WriteToOstream(reinterpret_cast<const char*>(&skipNum), sizeof(skipNum));

  for (auto& skipped : file._skippedCalls) {
    stream.WriteToOstream(reinterpret_cast<const char*>(&skipped.first), sizeof(skipped.first));
    stream.WriteToOstream(reinterpret_cast<const char*>(&skipped.second), sizeof(skipped.second));
  }

  //Write properties to the stream file.
  //First size in bytes, than the string
  auto properties = file._properties->dump();

  const uint32_t propTreeSize = ensure_unsigned32bit_representible<size_t>(properties.size());
  stream.WriteToOstream(reinterpret_cast<const char*>(&propTreeSize), sizeof(propTreeSize));
  if (!Config::Get().common.recorder.nullIO) {
    stream.WriteToOstream(properties.c_str(), properties.size());
  }

  return stream;
}

namespace {
bool parse_json_recorder_diags(const std::string& properties,
                               nlohmann::ordered_json& outProperties) {
  nlohmann::ordered_json j = nlohmann::ordered_json::parse(properties, nullptr, false);
  if (j.is_discarded()) {
    return false;
  }
  outProperties = std::move(j);
  return true;
}
} // namespace

CBinIStream& operator>>(CBinIStream& stream, CFile& file) {
  uint32_t skipNum = 0U;
  stream.ReadHelper(reinterpret_cast<char*>(&skipNum), sizeof(skipNum));
  if (skipNum <= UINT32_MAX) {
    for (uint32_t i = 0; i < skipNum; i++) {
      uint32_t id = 0U;
      stream.ReadHelper(reinterpret_cast<char*>(&id), sizeof(id));

      uint32_t num = 0U;
      stream.ReadHelper(reinterpret_cast<char*>(&num), sizeof(num));

      file._skippedCalls[id] = num;
    }
  }

  uint32_t propsLength = 0;
  stream.ReadHelper(reinterpret_cast<char*>(&propsLength), sizeof(propsLength));
  if (propsLength <= UINT32_MAX) {
    std::string props(propsLength, '\0');
    stream.ReadHelper(&props[0], propsLength);
    bool result = false;
    if (props.find("<?xml version") != std::string::npos) {
      file._formerProperties = std::move(props);
      result = true;
    } else {
      result = parse_json_recorder_diags(props, *file._properties);
    }
    if (!result) {
      Log(ERR) << "Exception thrown when parsing diagnostic information";
      Log(ERR) << "Disabling Extras.Utilities.ExtendedDiagnostic might help.";
    }
  }

  return stream;
}

bool stream_older_than(uint64_t version) {
  return CGits::Instance().File().Version().version() < version;
}

TimerSet::TimerSet() : playback(true), restoration(true) {}

} // namespace gits
