// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directStorageQueueService.h"
#include "arguments.h"
#include "commandSerializersAuto.h"
#include "stateTrackingService.h"
#include "analyzerResults.h"
#include "subcaptureRecorder.h"

#include <vector>

namespace gits {
namespace DirectX {

DirectStorageQueueService::DirectStorageQueueService(StateTrackingService& stateService)
    : m_StateService(stateService) {}

void DirectStorageQueueService::RecordQueueSubmit(GITSKey queueKey) {
  IDStorageQueueSubmitCommand submit;
  submit.Key = m_StateService.GetUniqueCommandKey();
  submit.m_Object.Key = queueKey;
  m_StateService.GetRecorder().Record(IDStorageQueueSubmitSerializer(submit));
}

void DirectStorageQueueService::AddEnqueueStatus(IDStorageQueueEnqueueStatusCommand& c) {
  EnqueueStatus& enqueueStatus =
      m_EnqueueStatusByIndexByArray[c.m_statusArray.Key][c.m_index.Value];
  enqueueStatus.QueueKey = c.m_Object.Key;
  enqueueStatus.Serializer = std::make_unique<IDStorageQueueEnqueueStatusSerializer>(c);
}

void DirectStorageQueueService::DestroyObject(GITSKey objectKey) {
  m_EnqueueStatusByIndexByArray.erase(objectKey);
}

void DirectStorageQueueService::RestoreDirectStorageQueues() {
  AnalyzerResults& analyzerResults = m_StateService.GetAnalyzerResults();
  std::map<GITSKey, std::vector<IDStorageQueueEnqueueStatusSerializer*>> enqueueStatusByQueue;

  for (const auto& [statusArrayKey, enqueueStatusByIndex] : m_EnqueueStatusByIndexByArray) {
    if (!analyzerResults.RestoreObject(statusArrayKey)) {
      continue;
    }

    for (const auto& [index, enqueueStatus] : enqueueStatusByIndex) {
      m_StateService.RestoreState(enqueueStatus.QueueKey);
      enqueueStatusByQueue[enqueueStatus.QueueKey].push_back(enqueueStatus.Serializer.get());
    }
  }

  for (const auto& [queueKey, enqueueStatusCommands] : enqueueStatusByQueue) {
    for (IDStorageQueueEnqueueStatusSerializer* serializer : enqueueStatusCommands) {
      m_StateService.GetRecorder().Record(*serializer);
    }
    RecordQueueSubmit(queueKey);
  }
  m_EnqueueStatusByIndexByArray.clear();
}

} // namespace DirectX
} // namespace gits
