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
  struct DataBlock {
    uint64_t NewOffset{};
    uint64_t Size{};
  };

  bool m_CaptureDirectStorage{};
  std::filesystem::path m_OutFilePath{};
  std::ofstream m_OutFile{};
  std::mutex m_MapMutex{};
  std::unordered_map<unsigned, std::filesystem::path> m_StorageFiles{};
  std::unordered_map<std::filesystem::path, std::unordered_map<uint64_t, DataBlock>>
      m_FileBlockByPathByOffset;
};

} // namespace DirectX
} // namespace gits
