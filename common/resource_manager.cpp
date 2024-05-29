// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

DISABLE_WARNINGS
#include <boost/interprocess/file_mapping.hpp>
ENABLE_WARNINGS

using boost::interprocess::file_mapping;

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

void precache_resources(const std::filesystem::path& dirname) {
  static const int FILE_WARMUP_CYCLES = 2;

  for (auto& res_file : resource_filenames(dirname)) {
    std::ifstream file(res_file.second, std::ios::binary | std::ios::in);

    for (int file_warmup_cnt = 0; file_warmup_cnt < FILE_WARMUP_CYCLES; file_warmup_cnt++) {
      if (file.is_open()) {
        file.clear(); //clear all flags set, if any.
        file.seekg(0, std::ios::beg);
        char byte;
        file.read(&byte, 1);
        // eof flag status should be checked before calling seekg. New version
        // of seekg unconditionally clears eof flag.
        while (!file.eof()) {
          file.seekg(1024 * 4, std::ios::cur);
          file.read(&byte, 1);
        }
      }
    }
  }
}

CResourceManager::CResourceManager(
    const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping)
    : index_filename_(gits::get(filename_mapping, RESOURCE_INDEX)),
      filenames_map_(filename_mapping),
      fakeHash_(0) {
  for (const auto& one_mapping : filename_mapping) {
    if (std::filesystem::exists(one_mapping.second)) {
      std::shared_ptr<file_mapping> mapping = std::make_shared<file_mapping>(
          one_mapping.second.string().c_str(), boost::interprocess::read_only);
      mappings_map_[one_mapping.first] = std::move(mapping);
    }
  }

  if (std::filesystem::exists(index_filename_)) {
    typedef std::unordered_map<uint64_t, TResourceHandle> map64_t;
    auto index = read_map<map64_t>(index_filename_);
    index_.swap(index);
  }
}

class OFBinStreamWrap {
  std::shared_ptr<std::ofstream> _ofstrPtr;

public:
  OFBinStreamWrap() {}
  OFBinStreamWrap(const char* name)
      : _ofstrPtr(new std::ofstream(name, std::ios::binary | std::ios::app | std::ios::out)) {}
  std::ofstream& Stream() {
    return *_ofstrPtr;
  }
};

struct FileWriter {
  typedef std::map<std::string, OFBinStreamWrap> FilesMap;

  static void consume_filedata(const FileData& data, FilesMap& files) {
    if (!Config::Get().recorder.extras.utilities.nullIO) {
      if (files.find(data.name) == files.end()) {
        files[data.name] = OFBinStreamWrap(data.name);
      }
      files[data.name].Stream().write(static_cast<const char*>(data.ptr), data.size);
    }
    operator delete(data.ptr);
    delete[] data.name;
  }
  void operator()(ProducerConsumer<FileData>& sync) {
    FilesMap files;
    try {
      for (;;) {
        FileData data = {};
        if (!sync.consume(data)) {
          break;
        }
        consume_filedata(data, files);
      }
    } catch (...) {
      Log(ERR) << "Writer thread failed: Exiting Gits!!!";
      fast_exit(1);
    }
    Log(INFO) << "Resource writer thread finished.";
  }

  FileWriter& operator=(const FileWriter& other) = delete;
};

mapped_file CResourceManager::get(hash_t hash) const {
  if (hash == EmptyHash) {
    return mapped_file();
  }

  const TResourceHandle& r = gits::get(index_, hash);
  std::shared_ptr<file_mapping> mapping = gits::get(mappings_map_, r.file_id);
  if (!mapping) {
    const auto msg = "Resource file mapping is null.";
    throw std::runtime_error(std::string(EXCEPTION_MESSAGE) + msg);
  }
  return mapped_file(*mapping, r.offset, r.size);
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
    if (dirty_ && !Config::Get().recorder.extras.utilities.nullIO) {
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

  if (Config::Get().recorder.extras.utilities.highIntegrity) {
    write_map(index_filename_, index_);
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
  _fileReader[r.file_id]->ReadCompressedWithOffset(_data.data(), r.size, r.offsetToStart,
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
