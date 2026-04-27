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

  void OpenFile(IDStorageFactoryOpenFileCommand& c);
  void EnqueueRequest(IDStorageQueueEnqueueRequestCommand& c);

private:
  struct FileRange {
    uint64_t NewOffset{};
    uint64_t OldOffset{};
    uint64_t Size{};
    FileRange(uint64_t NewOffset, uint64_t oldOffset, uint64_t size)
        : NewOffset(NewOffset), OldOffset(oldOffset), Size(size) {}
  };
  struct CompareFileRange {
    bool operator()(const FileRange& lhs, const FileRange& rhs) const {
      return lhs.OldOffset < rhs.OldOffset;
    }
  };
  using Buffer = std::vector<char>;
  using Ranges = std::set<FileRange, CompareFileRange>;

  bool m_CaptureDirectStorage{};
  std::filesystem::path m_OutFilePath{};
  std::ofstream m_OutFile{};
  std::mutex m_MapMutex{};
  std::unordered_map<unsigned, std::filesystem::path> m_Files{};
  std::unordered_map<std::filesystem::path, Ranges> m_FileReads{};
};

} // namespace DirectX
} // namespace gits
