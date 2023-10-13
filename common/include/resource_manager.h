// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "mapped_file.h"
#include "tools.h"
#include "pragmas.h"

#include <unordered_map>
#include <string>
#include <limits>
#include <memory>
#include <filesystem>

namespace boost {
namespace interprocess {
class file_mapping;
}
} // namespace boost

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
void precache_resources(const std::filesystem::path& prefix);

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

class CResourceManager {
public:
  CResourceManager(const std::unordered_map<uint32_t, std::filesystem::path>& filename_mapping,
                   uint32_t asyncBufferMaxCost,
                   THashType hashType,
                   bool hashPartially,
                   uint32_t partialHashCutoff,
                   uint32_t partialHashChunks,
                   uint32_t partialHashRatio);
  ~CResourceManager();

  mapped_file get(hash_t hash) const;
  hash_t getHash(uint32_t file_id, const void* data, size_t size) const;
  hash_t put(uint32_t file_id, const void* data, size_t size);
  hash_t put(uint32_t file_id, const void* data, size_t size, hash_t hash, bool overwrite = false);

  TResourceHandle get_resource_handle(hash_t);

  static const hash_t EmptyHash = 0;

private:
  bool dirty_;
  std::filesystem::path index_filename_;
  std::unordered_map<hash_t, TResourceHandle> index_;
  std::unordered_map<uint32_t, std::filesystem::path> filenames_map_;
  std::unordered_map<uint32_t, uint64_t> file_sizes_;
  std::unordered_map<uint32_t, std::shared_ptr<boost::interprocess::file_mapping>> mappings_map_;

  Task<FileData> fileWriter_;

  //data needed to entange Resource manager from Config.
  THashType hashType_;
  bool hashPartially_;
  uint32_t partialHashCutoff_;
  uint32_t partialHashChunks_;
  uint32_t partialHashRatio_;
  uint32_t asyncBufferMaxCost_;

  hash_t fakeHash_;
};
} // namespace gits
