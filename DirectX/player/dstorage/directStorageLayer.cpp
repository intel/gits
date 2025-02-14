// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageLayer.h"
#include "config.h"

namespace gits {
namespace DirectX {

DirectStorageLayer::DirectStorageLayer() : Layer("DirectStoragePlayer") {
  resourcesFilePath_ = Config::Get().common.player.streamDir / "DirectStorageResources.bin";
}

DirectStorageLayer::~DirectStorageLayer() {}

void DirectStorageLayer::pre(IDStorageFactoryOpenFileCommand& c) {
  c.path_.value = const_cast<LPWSTR>(resourcesFilePath_.c_str());
}

void DirectStorageLayer::post(IDStorageFactoryCreateQueueCommand& c) {
  auto queueKey = c.ppv_.key;
  auto eventName = "GITSPlayerDStorageEvent_" + std::to_string(queueKey);
  events_[queueKey].Attach(CreateEventA(nullptr, FALSE, FALSE, eventName.c_str()));
}

void DirectStorageLayer::pre(IDStorageQueueEnqueueRequestCommand& c) {
  GITS_ASSERT(c.request_.value);
  auto& request = *c.request_.value;
  // Custom compression is not supported
  // Memory source is not supported
  if ((request.Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    c.skip = true;
    return;
  }

  if (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_FILE) {
    // Update the source file offset to the new offset
    request.Source.File.Offset = c.request_.newOffset;
  }
  if (request.Options.DestinationType == DSTORAGE_REQUEST_DESTINATION_MEMORY) {
    auto queueKey = c.object_.key;
    // Allocate a buffer for the request and set it as the destination buffer
    buffers_[queueKey].emplace_back(request.Destination.Memory.Size);
    request.Destination.Memory.Buffer = buffers_[queueKey].back().data();
  }
}

void DirectStorageLayer::pre(IDStorageQueue1EnqueueSetEventCommand& c) {
  // Synchronization is handled by this layer, not the original stream
  c.skip = true;
}

void DirectStorageLayer::pre(IDStorageCustomDecompressionQueueSetRequestResultsCommand& c) {
  c.skip = true;
}

void DirectStorageLayer::pre(IDStorageQueueSubmitCommand& c) {
  auto queueKey = c.object_.key;
  auto& queueEvent = events_[queueKey];
  auto* queue = static_cast<IDStorageQueue1*>(c.object_.value);
  queue->EnqueueSetEvent(queueEvent.Get());
}

void DirectStorageLayer::post(IDStorageQueueSubmitCommand& c) {
  auto queueKey = c.object_.key;
  auto& queueEvent = events_[queueKey];
  WaitForSingleObject(queueEvent.Get(), INFINITE);
  // Deallocate any buffers associated with the queue
  buffers_.erase(queueKey);
}

} // namespace DirectX
} // namespace gits
