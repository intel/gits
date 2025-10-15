// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageService.h"
#include "configurationLib.h"
#include "gits.h"

#include <fstream>
#include <iostream>

namespace gits {
namespace DirectX {

DirectStorageService::DirectStorageService() {
  const auto& cfg = Configurator::Get();
  captureDirectStorage_ = cfg.directx.capture.captureDirectStorage;
  if (captureDirectStorage_) {
    const std::filesystem::path& outFileDir =
        Configurator::IsPlayer() ? cfg.common.player.subcapturePath : cfg.common.recorder.dumpPath;
    outFilePath_ = outFileDir / "DirectStorageResources.bin";
    outFile_.open(outFilePath_, std::ios::binary);
  }
}

void DirectStorageService::openFile(IDStorageFactoryOpenFileCommand& c) {
  if (captureDirectStorage_) {
    std::lock_guard<std::mutex> lock(mapMutex_);
    files_[c.ppv_.key] = c.path_.value;
  }
}

void DirectStorageService::enqueueRequest(IDStorageQueueEnqueueRequestCommand& c) {
  if (!captureDirectStorage_) {
    return;
  }
  GITS_ASSERT(c.request_.value);
  DSTORAGE_REQUEST& request = *c.request_.value;
  // Custom compression is not supported
  // Memory source is not supported
  if ((request.Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    return;
  }

  GITS_ASSERT(c.request_.fileKey);

  std::lock_guard<std::mutex> lock(mapMutex_);
  const std::filesystem::path& filePath = files_[c.request_.fileKey];
  Ranges& fileReads = fileReads_[filePath];
  FileRange range =
      FileRange(outFile_.tellp(), request.Source.File.Offset, request.Source.File.Size);
  auto result = fileReads.insert(range);
  // Assumes that all the chunks accessed are the same size (or smaller) than when originally accessed
  GITS_ASSERT(result.first->size >= request.Source.File.Size);
  if (result.second) {
    // Write file data into DirectStorageResources.bin
    std::ifstream inputFile(filePath, std::ios::binary);
    inputFile.seekg(range.oldOffset);
    Buffer buffer(range.size);
    inputFile.read(buffer.data(), range.size);
    outFile_.write(buffer.data(), range.size);
  }
  // Save the DirectStorageResources.bin offset
  c.request_.newOffset = result.first->newOffset;
}
} // namespace DirectX
} // namespace gits
