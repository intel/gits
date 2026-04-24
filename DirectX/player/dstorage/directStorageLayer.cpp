// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageLayer.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

DirectStorageLayer::DirectStorageLayer() : Layer("DirectStoragePlayer") {
  m_ResourcesFilePath = Configurator::Get().common.player.streamDir / "DirectStorageResources.bin";
}

DirectStorageLayer::~DirectStorageLayer() {}

void DirectStorageLayer::Pre(IDStorageFactoryOpenFileCommand& c) {
  c.m_path.Value = const_cast<LPWSTR>(m_ResourcesFilePath.c_str());
}

void DirectStorageLayer::Post(IDStorageFactoryCreateQueueCommand& c) {
  auto queueKey = c.m_ppv.Key;
  auto eventName = "GITSPlayerDStorageEvent_" + std::to_string(queueKey);
  m_Events[queueKey].Attach(CreateEventA(nullptr, FALSE, FALSE, eventName.c_str()));
}

void DirectStorageLayer::Pre(IDStorageQueueEnqueueRequestCommand& c) {
  GITS_ASSERT(c.m_request.Value);
  auto& request = *c.m_request.Value;
  // Custom compression is not supported
  // Memory source is not supported
  if ((request.Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    c.Skip = true;
    return;
  }

  if (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_FILE) {
    // Update the source file offset to the new offset
    request.Source.File.Offset = c.m_request.NewOffset;
  }
  if (request.Options.DestinationType == DSTORAGE_REQUEST_DESTINATION_MEMORY) {
    auto queueKey = c.m_Object.Key;
    // Allocate a buffer for the request and set it as the destination buffer
    m_Buffers[queueKey].emplace_back(request.Destination.Memory.Size);
    request.Destination.Memory.Buffer = m_Buffers[queueKey].back().data();
  }
}

void DirectStorageLayer::Pre(IDStorageQueue1EnqueueSetEventCommand& c) {
  // Synchronization is handled by this layer, not the original stream
  c.Skip = true;
}

void DirectStorageLayer::Pre(IDStorageCustomDecompressionQueueSetRequestResultsCommand& c) {
  c.Skip = true;
}

void DirectStorageLayer::Pre(IDStorageQueueSubmitCommand& c) {
  auto queueKey = c.m_Object.Key;
  auto& queueEvent = m_Events[queueKey];
  auto* queue = static_cast<IDStorageQueue1*>(c.m_Object.Value);
  queue->EnqueueSetEvent(queueEvent.Get());
}

void DirectStorageLayer::Post(IDStorageQueueSubmitCommand& c) {
  auto queueKey = c.m_Object.Key;
  auto& queueEvent = m_Events[queueKey];
  WaitForSingleObject(queueEvent.Get(), INFINITE);
  // Deallocate any buffers associated with the queue
  m_Buffers.erase(queueKey);
}

} // namespace DirectX
} // namespace gits
