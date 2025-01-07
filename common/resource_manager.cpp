// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resource_manager.h"
#include "exception.h"
#include "key_value.h"
#include "tools.h"
#include "log.h"
#include "platform.h"
#include "pragmas.h"
#include "config.h"
#ifndef BUILD_FOR_CCODE
#include "gits.h"
#endif
#include <string>
#include <algorithm>
#include <memory>

namespace gits {
CLog& operator<<(CLog& log, TResourceType rt) {
  switch (rt) {
  case RESOURCE_INDEX:
    log << "RESOURCE_INDEX";
    break;
  case RESOURCE_TEXTURE:
    log << "RESOURCE_TEXTURE";
    break;
  case RESOURCE_BUFFER:
    log << "RESOURCE_BUFFER";
    break;
  case RESOURCE_CLIENT_SIZES:
    log << "RESOURCE_CLIENT_SIZES";
    break;
  case RESOURCE_DATA_RAW:
    log << "RESOURCE_DATA_RAW";
    break;
  default:
    log << "(Unknown resource type: " << static_cast<std::underlying_type<TResourceType>::type>(rt)
        << ")";
  }
  return log;
}

namespace {

const std::unordered_map<uint32_t, std::filesystem::path>& base_resource_filenames() {
  typedef std::unordered_map<uint32_t, std::filesystem::path> map_t;
  INIT_NEW_STATIC_OBJ(the_map, map_t)

  if (the_map.empty()) {
    the_map[RESOURCE_TEXTURE] = "gitsTextures.dat";
    the_map[RESOURCE_BUFFER] = "gitsBuffers.dat";
    the_map[RESOURCE_INDEX] = "gitsDataIndex.dat";
    the_map[RESOURCE_CLIENT_SIZES] = "gitsClientSizes.dat";
    the_map[RESOURCE_DATA_RAW] = "gitsData.raw";
  }
  return the_map;
}
} // namespace

std::unordered_map<uint32_t, std::filesystem::path> resource_filenames(
    const std::filesystem::path& prefix) {
  auto base_names = base_resource_filenames();
  auto iter = base_names.begin();
  decltype(base_names) the_map;

  for (; iter != base_names.end(); ++iter) {
    the_map[iter->first] = (prefix / iter->second).string();
  }

  return the_map;
}

CResourceManager::CResourceManager(
    const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping)
    : index_filename_(gits::get(filename_mapping, RESOURCE_INDEX)),
      filenames_map_(filename_mapping),
      fakeHash_(0) {

  if (std::filesystem::exists(index_filename_)) {
    typedef std::unordered_map<uint64_t, TResourceHandle> map64_t;
    auto index = read_map<map64_t>(index_filename_);
    index_.swap(index);
  }
}

CResourceManager::~CResourceManager() {
  for (auto& pair : _fileReader) {
    delete pair.second; // Delete each CBinIStream pointer
  }
  _fileReader.clear(); // Clear the map
}

std::vector<char> CResourceManager::get(hash_t hash) {
  if (hash == EmptyHash) {
    return std::vector<char>();
  }

  const TResourceHandle& r = gits::get(index_, hash);
  if (_fileReader[r.file_id] == nullptr) {
    const auto& file_name = gits::get(filenames_map_, r.file_id);
    _fileReader[r.file_id] = new CBinIStream(file_name);
  }
  _data.resize(r.size);
  _fileReader[r.file_id]->ReadWithOffset(_data.data(), r.size, r.offset);
  return std::move(_data);
}

TResourceHandle CResourceManager::get_resource_handle(hash_t toFind) {
  std::unordered_map<hash_t, TResourceHandle>::iterator it;
  it = index_.find(toFind);
  if (it == index_.end()) {
    throw std::runtime_error(
        "Error during ResourceManager::get_resource_handle - given hash value not found.");
  }
  return it->second;
}

CResourceManager2::CResourceManager2(
    const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping)
    : dirty_(false),
      index_filename_(gits::get(filename_mapping, RESOURCE_INDEX)),
      filenames_map_(filename_mapping),
      fakeHash_(0) {
  if (std::filesystem::exists(index_filename_)) {
    typedef std::unordered_map<uint64_t, TResourceHandle2> map64_t;
    auto index = read_map<map64_t>(index_filename_);
    index_.swap(index);
  }
}

CResourceManager2::~CResourceManager2() {
  try {
    //If we have put anything in the manager index needs to be rewritten.
    if (dirty_ && !Config::Get().common.recorder.nullIO) {
      write_map(index_filename_, index_);
    }
  } catch (...) {
    topmost_exception_handler("CResourceManager::~CResourceManager");
  }
  for (auto& elem : _fileWriter) {
    delete elem.second;
  }
  for (auto& elem : _fileReader) {
    delete elem.second;
  }
  _fileWriter.clear();
  _fileReader.clear();
}

hash_t CResourceManager2::getHash(uint32_t file_id, const void* data, size_t size) {
  if (data == nullptr || size == 0) {
    return EmptyHash;
  } else {
    return ++fakeHash_;
  }
}

hash_t CResourceManager2::put(uint32_t file_id, const void* data, size_t size) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (data == nullptr || size == 0) {
    return EmptyHash;
  }

  if (size > UINT32_MAX) {
    throw EOperationFailed("Cannot save resource due to size limitation, current size: " +
                           std::to_string(size));
  }
  return put(file_id, data, size, ++fakeHash_);
}

hash_t CResourceManager2::put(
    uint32_t file_id, const void* data, size_t size, hash_t hash, bool overwrite) {
  if (data == nullptr || size == 0 || hash == EmptyHash) {
    throw std::runtime_error("Error in ResourceManager::put - can't insert empty data / empty hash "
                             "when explicitly providing hash value.");
  }

  if (!overwrite) {
    auto it = index_.find(hash);

    // Already in the index, nothing else to do.
    if (it != index_.end()) {
      if (it->second.size == size) {
        return hash;
      } else {
        throw std::runtime_error("Error in ResourceManager::put - hash collision.");
      }
    }
  }

  dirty_ = true;

  const auto& file_name = gits::get(filenames_map_, file_id);
  if (Config::Get().IsPlayer()) {
    CALL_ONCE[] {
      Log(WARN)
          << "CResourceManager: Hash discrepancy. Modyfing stream files in gitsPlayer process.";
    };
  }

  //std::ofstream file(file_name, std::ios::binary | std::ios::app | std::ios::out);
  if (_fileWriter[file_id] == nullptr) {
    _fileWriter[file_id] = new CBinOStream(file_name);
    _fileWriter[file_id]->InitializeCompression();
  }
  uint64_t offsetInFile = 0;
  uint64_t offsetInChunk = 0;
  _fileWriter[file_id]->WriteCompressedAndGetOffset(static_cast<const char*>(data), size,
                                                    offsetInFile, offsetInChunk);

  TResourceHandle2 resource;
  resource.file_id = file_id;
  resource.offsetToStart = offsetInFile;
  resource.offsetInsideChunk = offsetInChunk;
  resource.size = size;

  // Remember where data was put.
  index_[hash] = resource;

  if (Config::Get().common.recorder.highIntegrity) {
    append_map(index_filename_, hash, resource);
    dirty_ = false;
  }

  return hash;
}

std::vector<char> CResourceManager2::get(hash_t hash) {
  if (hash == EmptyHash) {
    return std::vector<char>();
  }

  const TResourceHandle2& r = gits::get(index_, hash);
  if (_fileReader[r.file_id] == nullptr) {
    const auto& file_name = gits::get(filenames_map_, r.file_id);
    _fileReader[r.file_id] = new CBinIStream(file_name);
    _fileReader[r.file_id]->InitializeCompression();
  }
  _data.resize(r.size);
  _fileReader[r.file_id]->ReadWithOffset(_data.data(), r.size, r.offsetToStart,
                                         r.offsetInsideChunk);
  return std::move(_data);
}

TResourceHandle2 CResourceManager2::get_resource_handle(hash_t toFind) {
  std::unordered_map<hash_t, TResourceHandle2>::iterator it;
  it = index_.find(toFind);
  if (it == index_.end()) {
    throw std::runtime_error(
        "Error during ResourceManager2::get_resource_handle - given hash value not found.");
  }
  return it->second;
}

} // namespace gits
