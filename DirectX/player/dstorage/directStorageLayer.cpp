// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageLayer.h"
#include "configurationLib.h"
#include "log.h"

namespace gits {
namespace DirectX {

DirectStorageLayer::DirectStorageLayer() : Layer("DirectStoragePlayer") {
  m_ResourcesFilePath = Configurator::Get().common.player.streamDir / "DirectStorageResources.bin";
}

DirectStorageLayer::~DirectStorageLayer() {
  CompleteAllBatches();
  GITS_ASSERT(m_Buffers.empty());
}

void DirectStorageLayer::ClearCompletedBatches(unsigned queueKey) {
  auto itDeque = m_InflightBatches.find(queueKey);
  if (itDeque == m_InflightBatches.end()) {
    return;
  }
  auto& dq = itDeque->second;
  while (!dq.empty()) {
    HANDLE h = dq.front()->CompletionEvent.Get();
    if (WaitForSingleObject(h, 0) != WAIT_OBJECT_0) {
      break;
    }
    dq.pop_front();
  }
  if (dq.empty()) {
    m_InflightBatches.erase(itDeque);
  }
}

void DirectStorageLayer::CompleteAllBatches() {
  DWORD timeout = 60000; // 60 sec
  if (Configurator::Get().directx.player.infiniteWaitForFence) {
    timeout = INFINITE;
  }
  while (!m_InflightBatches.empty()) {
    auto itMap = m_InflightBatches.begin();
    auto& dq = itMap->second;
    while (!dq.empty()) {
      DWORD ret = WaitForSingleObject(dq.front()->CompletionEvent.Get(), timeout);
      if (ret == WAIT_TIMEOUT) {
        LOG_ERROR << "DirectStorageLayer - Timeout while waiting for queue " << itMap->first
                  << " completion";
      }
      dq.pop_front();
    }
    m_InflightBatches.erase(itMap);
  }
}

void DirectStorageLayer::Pre(IDStorageFactoryOpenFileCommand& c) {
  c.m_path.Value = const_cast<LPWSTR>(m_ResourcesFilePath.c_str());
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
  ClearCompletedBatches(queueKey);

  auto batch = std::make_unique<Batch>();
  batch->CompletionEvent.Attach(CreateEventA(nullptr, FALSE, FALSE, nullptr));
  auto itBuffers = m_Buffers.find(queueKey);
  if (itBuffers != m_Buffers.end()) {
    batch->Buffers = std::move(itBuffers->second);
    m_Buffers.erase(itBuffers);
  }

  auto* queue = static_cast<IDStorageQueue1*>(c.m_Object.Value);
  queue->EnqueueSetEvent(batch->CompletionEvent.Get());

  m_InflightBatches[queueKey].push_back(std::move(batch));
}

} // namespace DirectX
} // namespace gits
