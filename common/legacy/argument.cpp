// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   argument.cpp
 *
 * @brief Definitions of library function call argument wrappers.
 *
 */

#include "argument.h"
#include "gits.h"
#include "exception.h"
#include "log2.h"
#include "buffer.h"
#include "config.h"
#include "streams.h"
#include "pragmas.h"
#include "tools.h"

#include <string>
#include <filesystem>

/* ******************************* A R G U M E N T ***************************** */

void gits::CArgument::Fill(void* ptr) const {
  LOG_ERROR << "Argument fill not implemented";
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

/* ************************ B U F F E R   A R G U M E N T ********************** */

/**
 * @brief Saves argument data to a binary file
 *
 * Method saves argument data to a binary file.
 *
 * @note Sequential bytes are ordered in oposite byte order
 *       than the network byte order.
 *
 * @param stream Output stream to use.
 */
void gits::CArgumentBuffer::Write(CBinOStream& stream) const {
  CBuffer buffer(Buffer(), Length());
  stream << buffer;
}

/**
 * @brief Loads argument data from a binary file
 *
 * Method loads argument data from a binary file.
 *
 * @note Sequential bytes are ordered in oposite byte order
 *       than the network byte order.
 *
 * @param stream Input stream to use.
 */
void gits::CArgumentBuffer::Read(CBinIStream& stream) {
  CBuffer buffer(Buffer(), Length());
  stream >> buffer;
}

gits::CArgumentFileText::CArgumentFileText(const std::string& fileName, const std::string& text)
    : _fileName(fileName), _text(text) {
  init(fileName.c_str(), text.c_str(), static_cast<unsigned>(text.size()));
}

gits::CArgumentFileText::CArgumentFileText(const char* fileName, const char* text, unsigned length)
    : _fileName(fileName), _text(text, length) {
  init(fileName, text, length);
}

void gits::CArgumentFileText::init(const char* fileName, const char* text, unsigned length) {
  if (_fileName.find('/') == std::string::npos && _fileName.find('\\') == std::string::npos) {
    throw std::runtime_error("CArgumentFileText file has to be contained in direcotry, given:" +
                             _fileName);
  }

  if (Configurator::Get().common.recorder.nullIO) {
    return;
  }
  std::filesystem::path path = Configurator::Get().common.recorder.dumpPath / _fileName;
  if (Configurator::Get().common.recorder.zipTextFiles) {
    auto f = CGits::Instance().OpenZipFileGLPrograms();
    int r = zipOpenNewFileInZip(f, fileName, nullptr, nullptr, 0, nullptr, 0, nullptr, 0, 0);
    if (r != ZIP_OK) {
      throw std::runtime_error("failed to open zip archive file for writing");
    }
    r = zipWriteInFileInZip(f, text, length);
    if (r != ZIP_OK) {
      throw std::runtime_error("failed writing to zip archive file");
    }
    r = zipCloseFileInZip(f);
    if (r != ZIP_OK) {
      CheckMinimumAvailableDiskSize();
      throw std::runtime_error("failed closing zip archive file");
    }
  } else {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream textStream(path, std::ios::binary);
    if (!textStream) {
      CheckMinimumAvailableDiskSize();
      LOG_ERROR << "Cannot create file '" << path << "'!!!";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    // save text to a file
    textStream.write(_text.data(), _text.size());
  }
}

void gits::CArgumentFileText::Write(CBinOStream& stream) const {
  // save filename
  stream << '"' << _fileName << '"';
}

void gits::CArgumentFileText::Read(CBinIStream& stream) {
  // load filename
  stream.get_delimited_string(_fileName, '"');
  LoadTextFromFile();
}

void gits::CArgumentFileText::LoadTextFromFile() {
  // load text from a file
  std::filesystem::path path = Configurator::Get().common.player.streamDir / _fileName;
  std::ifstream textStream(path, std::ios::binary);

  // check if file was opened
  if (textStream) {
    _text.assign(std::istreambuf_iterator<char>(textStream), std::istreambuf_iterator<char>());
  } else {
    // Try with zip archive if the file is not found.
    // We will search in zip archive named the same as top level direcotry of file we search
    // with appended .zip to it.
    auto slash = _fileName.find("/");
    if (slash != 0 && slash != std::string::npos) {
      CGits::Instance().OpenUnZipFileGLPrograms();
      CGits::Instance().ReadGlProgramFromUnZipFile(_fileName, _text);
    }
  }
}

gits::CBinaryResource::CBinaryResource() {
  _resource_hash = CResourceManager::EmptyHash;
}

gits::CBinaryResource::CBinaryResource(hash_t hash) : _resource_hash(hash) {}

gits::CBinaryResource::CBinaryResource(TResourceType type, const void* data, size_t size) {
  reset(type, data, size);
}

void gits::CBinaryResource::reset(TResourceType type, const void* data, size_t size) {
  _resource_hash = CGits::Instance().ResourceManager2().put(type, data, size);
}

const char* gits::CBinaryResource::Name() const {
  return "const void*";
}

bool gits::CBinaryResource::Array() const {
  return false;
}

void gits::CBinaryResource::Write(CBinOStream& stream) const {
  stream << CBuffer(_resource_hash);
}

void gits::CBinaryResource::Read(CBinIStream& stream) {
  CBuffer buffer(_resource_hash);
  stream >> buffer;
  if (stream_older_than(GITS_TOKEN_COMPRESSION)) {
    _data = CGits::Instance().ResourceManager().get(_resource_hash);
  } else {
    _data = CGits::Instance().ResourceManager2().get(_resource_hash);
  }
}

gits::CBinaryResource::PointerProxy gits::CBinaryResource::Data() const {
  if (Configurator::IsPlayer()) {
    return PointerProxy(_data.data(), _data.size());
  } else {
    LOG_ERROR << "CBinaryResource: Getting Data not available in Recorder";
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

gits::hash_t gits::CBinaryResource::GetResourceHash() const {
  return _resource_hash;
}

void gits::CBinaryResource::Deallocate() {
  DeallocateVector(_data);
}
/* ******************************** C H A R ****************************** */

const char* gits::Cchar::NAME = "char";

gits::Cchar::Cchar() : _value() {}

gits::Cchar::Cchar(char value) : _value(value) {}

void gits::Cchar::Write(CBinOStream& stream) const {
  write_to_stream(stream, _value);
}

void gits::Cchar::Read(CBinIStream& stream) {
  read_from_stream(stream, _value);
}

/* ************************* CONST CHAR ** ****************************** */

template <typename T>
gits::CByteStringArray<T>::CByteStringArray() {}

template <typename T>
gits::CByteStringArray<T>::~CByteStringArray() {
  for (Cchar::CSArray* elem : _cStringTable) {
    delete elem;
  }
}

template <typename T>
gits::CByteStringArray<T>::CByteStringArray(uint32_t count, const T** value) {
  for (uint32_t i = 0; i < count; i++) {
    Cchar::CSArray* elem = new Cchar::CSArray(reinterpret_cast<const char*>(value[i]), '\0', 1);
    _cStringTable.push_back(elem);
    _lengthsArray.push_back(elem->Vector().size());
  }
}

template <typename T>
gits::CByteStringArray<T>::CByteStringArray(uint32_t count, T** value)
    : CByteStringArray(count, const_cast<const T**>(value)) {}

template <typename T>
gits::CByteStringArray<T>::CByteStringArray(uint32_t count, const void** value)
    : CByteStringArray(count, reinterpret_cast<const T**>(value)) {}

template <typename T>
gits::CByteStringArray<T>::CByteStringArray(uint32_t count,
                                            const T** value,
                                            const size_t* lengths) {
  for (uint32_t i = 0; i < count; i++) {
    Cchar::CSArray* elem = new Cchar::CSArray(lengths[i], reinterpret_cast<const char*>(value[i]));
    _cStringTable.push_back(elem);
    _lengthsArray.push_back(elem->Vector().size());
  }
}

template <typename T>
const T** gits::CByteStringArray<T>::Original() {
  for (Cchar::CSArray* elem : _cStringTable) {
    _constCharArray.push_back(reinterpret_cast<const T*>(*elem[0]));
  }
  if (_cStringTable.size() > 0) {
    return reinterpret_cast<const T**>(&_constCharArray[0]);
  } else {
    return nullptr;
  }
}

template <typename T>
void gits::CByteStringArray<T>::Write(CBinOStream& stream) const {
  write_name_to_stream(stream, (unsigned)_cStringTable.size());
  for (auto& arg : _cStringTable) {
    arg->Write(stream);
  }
}

template <typename T>
void gits::CByteStringArray<T>::Read(CBinIStream& stream) {
  unsigned tableSize;
  read_name_from_stream(stream, tableSize);
  uint64_t max_size = std::min(_cStringTable.max_size(), _lengthsArray.max_size());
  if (tableSize <= max_size) {
    for (unsigned i = 0; i < tableSize; i++) {
      Cchar::CSArray* argPtr(new Cchar::CSArray());
      argPtr->Read(stream);
      _cStringTable.push_back(argPtr);
      _lengthsArray.push_back(argPtr->Vector().size());
    }
  } else {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
}

template <>
const char* gits::CStringArray::NAME = "const char**";
template <>
const char* gits::CBufferArray::NAME = "const unsigned char**";

template class gits::CByteStringArray<char>;
template class gits::CByteStringArray<unsigned char>;

/* ******************************** I N T ****************************** */

const char* gits::Cint::NAME = "int";

void gits::Cint::Write(CBinOStream& stream) const {
  write_to_stream(stream, _value);
}

void gits::Cint::Read(CBinIStream& stream) {
  read_from_stream(stream, _value);
}

const char* gits::Cuint8_t::NAME = "uint8_t";
const char* gits::Cuint16_t::NAME = "uint16_t";
const char* gits::Cuint32_t::NAME = "uint32_t";
const char* gits::Cuint64_t::NAME = "uint64_t";
const char* gits::Cint8_t::NAME = "int8_t";
const char* gits::Cint16_t::NAME = "int16_t";
const char* gits::Cint32_t::NAME = "int32_t";
const char* gits::Cint64_t::NAME = "int64_t";
const char* gits::Cfloat::NAME = "float";
const char* gits::Cdouble::NAME = "double";
const char* gits::Csize_t::NAME = "size_t";

gits::CMappedHandle::CMappedHandle() : version_(0), handle_(nullptr) {}

gits::CMappedHandle::CMappedHandle(void* arg) : version_(currentVersion_), handle_(arg) {}

const char* gits::CMappedHandle::NAME = "void*"; // Most if not all handle types are void* typedefs.

const char* gits::CMappedHandle::Name() const {
  return NAME;
}

const char* gits::CMappedHandle::TypeNameStr() {
  return NAME;
}

const char* gits::CMappedHandle::WrapTypeNameStr() {
  return "CMappedHandle";
}

void gits::CMappedHandle::Write(CBinOStream& stream) const {
  assert(*version_ == currentVersion_);
  version_.Write(stream);
  write_name_to_stream(stream, handle_);
}

void gits::CMappedHandle::Read(CBinIStream& stream) {
  version_.Read(stream);
  switch (*version_) {
  case 0:
    read_name_from_stream(stream, handle_);
    return;
  default:
    if (*version_ > currentVersion_) {
      LOG_ERROR << "This stream uses version " << *version_
                << " of the CMappedHandle argument, "
                   "but the highest version this GITS build supports is "
                << currentVersion_ << ". Get the latest GITS and try again.";
      throw ENotSupported(EXCEPTION_MESSAGE);
    } else {
      LOG_ERROR << "Unknown CMappedHandle argument version: " << *version_;
      throw ENotSupported(EXCEPTION_MESSAGE);
    }
  }
}

void* gits::CMappedHandle::Original() const {
  return handle_;
}

void* gits::CMappedHandle::Value() const {
  return GetMapping(handle_);
}

void* gits::CMappedHandle::operator*() const {
  return Value();
}

std::set<uint64_t> gits::CMappedHandle::GetMappedPointers() {
  LOG_ERROR << "CMappedHandle::GetMappedPointers() is not implemented yet.";
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

void gits::CMappedHandle::AddMapping(void* key, void* value) {
  get_map()[key] = value;
}

void gits::CMappedHandle::RemoveMapping(void* key) {
  if (CheckMapping(key)) {
    get_map().erase(key);
  }
}

void* gits::CMappedHandle::GetMapping(void* key) {
  const auto& the_map = get_map();
  const auto iter = the_map.find(key);
  if (iter == the_map.end()) {
    LOG_ERROR << "Couldn't map handle: " << key;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  return iter->second;
}

bool gits::CMappedHandle::CheckMapping(void* key) {
  const auto& the_map = get_map();
  return the_map.find(key) != the_map.end();
}

gits::CMappedHandle::handle_map_t& gits::CMappedHandle::get_map() {
  INIT_NEW_STATIC_OBJ(handles_map, handle_map_t)
  static bool initialized = false;
  if (!initialized) {
    handles_map[nullptr] = nullptr;
    initialized = true;
  }
  return handles_map;
}
