// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "commandSerializersAuto.h"

#include <map>
#include <memory>

namespace gits {
namespace DirectX {

class StateTrackingService;

class DirectStorageQueueService {
public:
  explicit DirectStorageQueueService(StateTrackingService& stateService);
  DirectStorageQueueService(const DirectStorageQueueService&) = delete;
  DirectStorageQueueService& operator=(const DirectStorageQueueService&) = delete;

  void AddEnqueueStatus(IDStorageQueueEnqueueStatusCommand& c);
  void DestroyObject(unsigned objectKey);
  void RestoreDirectStorageQueues();

private:
  struct EnqueueStatus {
    unsigned QueueKey{};
    std::unique_ptr<IDStorageQueueEnqueueStatusSerializer> Serializer;
  };

  void RecordQueueSubmit(unsigned queueKey);

  StateTrackingService& m_StateService;
  std::map<unsigned, std::map<unsigned, EnqueueStatus>> m_EnqueueStatusByIndexByArray;
};

} // namespace DirectX
} // namespace gits
