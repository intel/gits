// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "directx.h"

#include <mutex>
#include <filesystem>
#include <set>
#include <fstream>
#include <unordered_map>

namespace gits {
namespace DirectX {

class DirectStorageService {
public:
  DirectStorageService();

  void openFile(IDStorageFactoryOpenFileCommand& c);
  void enqueueRequest(IDStorageQueueEnqueueRequestCommand& c);

private:
  struct FileRange {
    uint64_t newOffset{};
    uint64_t oldOffset{};
    uint64_t size{};
    FileRange(uint64_t newOffset, uint64_t oldOffset, uint64_t size)
        : newOffset(newOffset), oldOffset(oldOffset), size(size) {}
  };
  struct CompareFileRange {
    bool operator()(const FileRange& lhs, const FileRange& rhs) const {
      return lhs.oldOffset < rhs.oldOffset;
    }
  };
  using Buffer = std::vector<char>;
  using Ranges = std::set<FileRange, CompareFileRange>;

  bool captureDirectStorage_{};
  std::filesystem::path outFilePath_{};
  std::ofstream outFile_{};
  std::mutex mapMutex_{};
  std::unordered_map<unsigned, std::filesystem::path> files_{};
  std::unordered_map<std::filesystem::path, Ranges> fileReads_{};
};

} // namespace DirectX
} // namespace gits
