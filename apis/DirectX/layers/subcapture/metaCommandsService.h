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

#include <unordered_map>
#include <vector>

namespace gits {
namespace DirectX {

class StateTrackingService;

class MetaCommandsService {
public:
  MetaCommandsService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void RestoreState();
  void InitializeMetaCommand(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command);
  void SetDeviceKey(ObjectKey deviceKey);
  void DestroyMetaCommand(ObjectKey key);

private:
  void RestoreStateInitialize();
  void RestoreStateFinalize();

private:
  StateTrackingService& m_StateService;
  ObjectKey m_DeviceKey{};
  ObjectKey m_CommandQueueKey{};
  ObjectKey m_CommandAllocatorKey{};
  ObjectKey m_CommandListKey{};
  ObjectKey m_FenceKey{};
  std::unordered_map<ObjectKey, std::vector<uint8_t>> m_MetaCommandData;
};

} // namespace DirectX
} // namespace gits
