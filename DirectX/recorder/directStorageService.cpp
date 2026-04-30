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
    m_Files[c.m_ppv.Key] = c.m_path.Value;
  }
}

void DirectStorageService::EnqueueRequest(IDStorageQueueEnqueueRequestCommand& c) {
  if (!m_CaptureDirectStorage) {
    return;
  }
  GITS_ASSERT(c.m_request.Value);
  DSTORAGE_REQUEST& request = *c.m_request.Value;
  // Custom compression is not supported
  // Memory source is not supported
  if ((request.Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    return;
  }

  GITS_ASSERT(c.m_request.FileKey);

  std::lock_guard<std::mutex> lock(m_MapMutex);
  const std::filesystem::path& filePath = m_Files[c.m_request.FileKey];
  Ranges& fileReads = m_FileReads[filePath];
  FileRange range =
      FileRange(m_OutFile.tellp(), request.Source.File.Offset, request.Source.File.Size);
  auto result = fileReads.insert(range);
  // Assumes that all the chunks accessed are the same size (or smaller) than when originally accessed
  GITS_ASSERT(result.first->Size >= request.Source.File.Size);
  if (result.second) {
    // Write file data into DirectStorageResources.bin
    std::ifstream inputFile(filePath, std::ios::binary);
    inputFile.seekg(range.OldOffset);
    Buffer buffer(range.Size);
    inputFile.read(buffer.data(), range.Size);
    m_OutFile.write(buffer.data(), range.Size);
  }
  // Save the DirectStorageResources.bin offset
  c.m_request.NewOffset = result.first->NewOffset;
}
} // namespace DirectX
} // namespace gits
