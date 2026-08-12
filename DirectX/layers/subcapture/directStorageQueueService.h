// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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
  void DestroyObject(GITSKey objectKey);
  void RestoreDirectStorageQueues();

private:
  struct EnqueueStatus {
    GITSKey QueueKey{};
    std::unique_ptr<IDStorageQueueEnqueueStatusSerializer> Serializer;
  };

  void RecordQueueSubmit(GITSKey queueKey);

  StateTrackingService& m_StateService;
  std::map<GITSKey, std::map<GITSKey, EnqueueStatus>> m_EnqueueStatusByIndexByArray;
};

} // namespace DirectX
} // namespace gits
