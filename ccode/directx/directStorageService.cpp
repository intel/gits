// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageService.h"

#include "dataService.h"

#include <plog/Log.h>

#include <filesystem>
#include <mutex>

namespace directx {

DirectStorageService& DirectStorageService::Get() {
  static DirectStorageService s_Instance;
  return s_Instance;
}

LPCWSTR DirectStorageService::ResourcesFilePath() {
  if (m_ResourcesFilePath.empty()) {
    const auto path = DataService::Get().GetPath().parent_path() / "DirectStorageResources.bin";
    m_ResourcesFilePath = path.wstring();
    if (!std::filesystem::exists(path)) {
      LOG_ERROR << "CCode - DirectStorageResources.bin not found at " << path.string();
    } else {
      LOG_INFO << "CCode - DirectStorage resources file: " << path.string();
    }
  }
  return m_ResourcesFilePath.c_str();
}

void DirectStorageService::PrepareEnqueueRequest(unsigned queueKey,
                                                 DSTORAGE_REQUEST* request,
                                                 uint64_t newOffset) {
  if (!request) {
    return;
  }

  request->Source.File.Offset = newOffset;

  if (request->Options.DestinationType != DSTORAGE_REQUEST_DESTINATION_MEMORY) {
    return;
  }

  const uint64_t dstSize = request->Destination.Memory.Size;
  if (!dstSize) {
    return;
  }

  auto& buffers = m_Buffers[queueKey];
  buffers.emplace_back(dstSize);
  request->Destination.Memory.Buffer = buffers.back().data();
}

void DirectStorageService::ClearCompletedBatches(unsigned queueKey) {
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

void DirectStorageService::CompleteAllBatches() {
  DWORD timeoutMs = 60000; // 60 sec

  for (auto it = m_InflightBatches.begin(); it != m_InflightBatches.end();) {
    auto& dq = it->second;
    while (!dq.empty()) {
      HANDLE h = dq.front()->CompletionEvent.Get();
      if (h) {
        DWORD ret = WaitForSingleObject(h, timeoutMs);
        if (ret == WAIT_TIMEOUT) {
          LOG_ERROR << "DirectStorageService - Timeout while waiting for queue " << it->first
                    << " completion";
        }
      }
      dq.pop_front();
    }
    it = m_InflightBatches.erase(it);
  }
}

void DirectStorageService::OnSubmit(unsigned queueKey, IDStorageQueue1* queue) {
  if (!queue) {
    return;
  }

  ClearCompletedBatches(queueKey);

  auto batch = std::make_unique<Batch>();
  batch->CompletionEvent.Attach(CreateEventA(nullptr, FALSE, FALSE, nullptr));
  auto itBuffers = m_Buffers.find(queueKey);
  if (itBuffers != m_Buffers.end()) {
    batch->Buffers = std::move(itBuffers->second);
    m_Buffers.erase(itBuffers);
  }

  queue->EnqueueSetEvent(batch->CompletionEvent.Get());
  m_InflightBatches[queueKey].push_back(std::move(batch));
}

} // namespace directx
