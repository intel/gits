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
    const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping,
    uint32_t asyncBufferMaxCost,
    THashType hashType,
    bool hashPartially,
    uint32_t partialHashCutoff,
    uint32_t partialHashChunks,
    uint32_t partialHashRatio)
    : dirty_(false),
      index_filename_(gits::get(filename_mapping, RESOURCE_INDEX)),
      filenames_map_(filename_mapping),
      fileWriter_(asyncBufferMaxCost),
      hashType_(hashType),
      hashPartially_(hashPartially),
      partialHashCutoff_(partialHashCutoff),
      partialHashChunks_(partialHashChunks),
      partialHashRatio_(partialHashRatio),
      asyncBufferMaxCost_(asyncBufferMaxCost),
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

CResourceManager::~CResourceManager() {
  try {
    //If we have put anything in the manager index needs to be rewritten.
    if (dirty_ && !Config::Get().recorder.extras.utilities.nullIO) {
      write_map(index_filename_, index_);
    }

    fileWriter_.finish();
  } catch (...) {
    topmost_exception_handler("CResourceManager::~CResourceManager");
  }
}

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

hash_t CResourceManager::getHash(uint32_t file_id, const void* data, size_t size) const {
  if (data == nullptr || size == 0) {
    return EmptyHash;
  }

  if (Config::Get().IsRecorder()) {
    return ComputeHash(data, size, hashType_, hashPartially_, partialHashCutoff_,
                       partialHashChunks_, partialHashRatio_);
  } else {
    return ComputeHash(data, size, THashType::CRC32ISH, false, 8192, 10, 20);
  }
}

hash_t CResourceManager::put(uint32_t file_id, const void* data, size_t size) {
  if (data == nullptr || size == 0) {
    return EmptyHash;
  }

  if (size > UINT32_MAX) {
    throw EOperationFailed("Cannot save resource due to size limitation, current size: " +
                           std::to_string(size));
  }

  if (Config::Get().recorder.extras.optimizations.removeResourceHash) {
    return put(file_id, data, size, ++fakeHash_);
  }

  return put(file_id, data, size, getHash(file_id, data, size));
}

hash_t CResourceManager::put(
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

  // Keep track of this file size, initialize to current size on disk.
  if (file_sizes_[file_id] == 0 && std::filesystem::exists(file_name)) {
    file_sizes_[file_id] = std::filesystem::file_size(file_name);
  }

  TResourceHandle resource;
  resource.file_id = file_id;
  resource.offset = file_sizes_[file_id];
  resource.size = ensure_unsigned32bit_representible<size_t>(size);

  file_sizes_[file_id] += size;

  // Push work to separate thread if configured such way.
  if (asyncBufferMaxCost_ != 0) {
    if (!fileWriter_.running()) {
      fileWriter_.start(FileWriter());
    }

    FileData prod{};
    const auto fileNameLength = file_name.string().length();
    prod.name = new char[fileNameLength + 1];
    std::strcpy(prod.name, file_name.string().c_str());
    prod.size = size;
    prod.ptr = operator new(size);
    memcpy(prod.ptr, data, size);

    fileWriter_.queue().produce(prod);
  } else {
    if (!Config::Get().recorder.extras.utilities.nullIO) {
      std::ofstream file(file_name, std::ios::binary | std::ios::app | std::ios::out);
      file.write(static_cast<const char*>(data), size);
    }
  }

  // Remember where data was put.
  index_[hash] = resource;

  if (Config::Get().recorder.extras.utilities.highIntegrity) {
    write_map(index_filename_, index_);
  }

  return hash;
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

} // namespace gits
