// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools.h"
#include "pragmas.h"

#include <unordered_map>
#include <string>
#include <limits>
#include <memory>
#include <filesystem>

namespace gits {
enum TResourceType {
  RESOURCE_INDEX,
  RESOURCE_TEXTURE,
  RESOURCE_BUFFER,
  RESOURCE_CLIENT_SIZES,
  RESOURCE_DATA_RAW
};
CLog& operator<<(CLog& log, TResourceType rt);
std::unordered_map<uint32_t, std::filesystem::path> resource_filenames(
    const std::filesystem::path& prefix);

struct TResourceHandle {
  uint64_t offset;
  uint32_t file_id;
  uint32_t size;
};

static_assert(sizeof(TResourceHandle) == 16, "Padding detected in TResourceHandle");

struct FileData { // Unit of work that ResourceManager schedules in separate thread.
  uint64_t size;
  char* name;
  void* ptr;

  void swap(FileData& fd) {
    std::swap(name, fd.name);
    std::swap(size, fd.size);
    std::swap(ptr, fd.ptr);
  }
};

template <>
inline int get_product_cost(const gits::FileData& d) {
  return (int)std::min<uint64_t>(d.size, std::numeric_limits<int>::max());
}

typedef uint64_t hash_t;
class CBinOStream;
class CBinIStream;

class CResourceManager {
public:
  CResourceManager(const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping);
  CResourceManager(const CResourceManager& other) = delete;
  CResourceManager& operator=(const CResourceManager& other) = delete;
  ~CResourceManager();
  std::vector<char> get(hash_t hash);

  TResourceHandle get_resource_handle(hash_t);

  static const hash_t EmptyHash = 0;

private:
  std::filesystem::path index_filename_;
  std::unordered_map<hash_t, TResourceHandle> index_;
  std::unordered_map<uint32_t, std::filesystem::path> filenames_map_;
  std::unordered_map<uint32_t, uint64_t> file_sizes_;

  hash_t fakeHash_;
  std::map<uint32_t, CBinIStream*> _fileReader;
  std::vector<char> _data;
};

struct TResourceHandle2 {
  uint64_t offsetToStart;
  uint64_t offsetInsideChunk;
  uint32_t file_id;
  uint64_t size;
};
class CResourceManager2 {
public:
  CResourceManager2(const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping);
  CResourceManager2(const CResourceManager2& other) = delete;
  CResourceManager2& operator=(const CResourceManager2& other) = delete;
  ~CResourceManager2();

  hash_t getHash(uint32_t file_id, const void* data, size_t size);
  hash_t put(uint32_t file_id, const void* data, size_t size);
  hash_t put(uint32_t file_id, const void* data, size_t size, hash_t hash, bool overwrite = false);
  std::vector<char> get(hash_t hash);

  TResourceHandle2 get_resource_handle(hash_t);

  static const hash_t EmptyHash = 0;

private:
  bool dirty_;
  std::filesystem::path index_filename_;
  std::unordered_map<hash_t, TResourceHandle2> index_;
  std::unordered_map<uint32_t, std::filesystem::path> filenames_map_;
  std::unordered_map<uint32_t, uint64_t> file_sizes_;
  std::mutex mutex_;

  hash_t fakeHash_;
  std::map<uint32_t, CBinOStream*> _fileWriter;
  std::map<uint32_t, CBinIStream*> _fileReader;
  std::vector<char> _data;
};
} // namespace gits
