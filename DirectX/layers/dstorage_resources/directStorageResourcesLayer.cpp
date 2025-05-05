// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageResourcesLayer.h"
#include "config.h"
#include "configurationLib.h"
#include "gits.h"

#include <fstream>
#include <iostream>

namespace gits {
namespace DirectX {

DirectStorageResourcesLayer::DirectStorageResourcesLayer() : Layer("DirectStorageResources") {
  const auto& cfg = Configurator::Get();
  const auto& outFileDir =
      Configurator::IsPlayer() ? cfg.common.player.subcapturePath : cfg.common.recorder.dumpPath;
  outFilePath_ = outFileDir / "DirectStorageResources.bin";
  outFile_.open(outFilePath_, std::ios::binary);
}
DirectStorageResourcesLayer::~DirectStorageResourcesLayer() {}

void DirectStorageResourcesLayer::post(IDStorageFactoryOpenFileCommand& c) {
  std::lock_guard<std::mutex> lock(mapMutex_);
  files_[c.ppv_.key] = c.path_.value;
}

void DirectStorageResourcesLayer::pre(IDStorageQueueEnqueueRequestCommand& c) {
  GITS_ASSERT(c.request_.value);
  auto& request = *c.request_.value;
  // Custom compression is not supported
  // Memory source is not supported
  if ((request.Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    return;
  }

  GITS_ASSERT(c.request_.fileKey);

  std::lock_guard<std::mutex> lock(mapMutex_);
  const auto& filePath = files_[c.request_.fileKey];
  auto& fileReads = fileReads_[filePath];
  auto range = FileRange(outFile_.tellp(), request.Source.File.Offset, request.Source.File.Size);
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
