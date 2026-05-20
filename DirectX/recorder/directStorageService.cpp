// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageService.h"
#include "log.h"
#include "configurationLib.h"

#include <fstream>
#include <iostream>

namespace gits {
namespace DirectX {

DirectStorageService::DirectStorageService() {
  const auto& cfg = Configurator::Get();
  m_CaptureDirectStorage = cfg.directx.recorder.captureDirectStorage;
  if (m_CaptureDirectStorage) {
    const std::filesystem::path& outFileDir =
        Configurator::IsPlayer() ? cfg.common.player.subcapturePath : cfg.common.recorder.dumpPath;
    m_OutFilePath = outFileDir / "DirectStorageResources.bin";
    m_OutFile.open(m_OutFilePath, std::ios::binary);
  }
}

void DirectStorageService::OpenFile(IDStorageFactoryOpenFileCommand& c) {
  if (m_CaptureDirectStorage) {
    std::lock_guard<std::mutex> lock(m_MapMutex);
    m_StorageFiles[c.m_ppv.Key] = c.m_path.Value;
  }
}

void DirectStorageService::EnqueueRequest(IDStorageQueueEnqueueRequestCommand& c) {
  if (!m_CaptureDirectStorage) {
    return;
  }
  GITS_ASSERT(c.m_request.Value);
  DSTORAGE_REQUEST& request = *c.m_request.Value;
  if ((request.Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    return;
  }
  GITS_ASSERT(c.m_request.FileKey);

  std::lock_guard<std::mutex> lock(m_MapMutex);
  const std::filesystem::path& filePath = m_StorageFiles[c.m_request.FileKey];
  std::unordered_map<uint64_t, DataBlock>& dataBlocks = m_FileBlockByPathByOffset[filePath];

  uint64_t newOffset = m_OutFile.tellp();
  bool store = false;
  auto itDataBlock = dataBlocks.find(request.Source.File.Offset);
  if (itDataBlock == dataBlocks.end()) {
    itDataBlock = dataBlocks
                      .insert(std::make_pair(request.Source.File.Offset,
                                             DataBlock(newOffset, request.Source.File.Size)))
                      .first;
    store = true;
  } else if (request.Source.File.Size > itDataBlock->second.Size) {
    itDataBlock->second.NewOffset = newOffset;
    itDataBlock->second.Size = request.Source.File.Size;
    store = true;
  }

  if (store) {
    std::ifstream inputFile(filePath, std::ios::binary);
    inputFile.seekg(itDataBlock->first);
    std::vector<char> buffer(itDataBlock->second.Size);
    inputFile.read(buffer.data(), itDataBlock->second.Size);
    m_OutFile.write(buffer.data(), itDataBlock->second.Size);
  }
  c.m_request.NewOffset = itDataBlock->second.NewOffset;
}

} // namespace DirectX
} // namespace gits
