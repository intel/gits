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
  void SetDeviceKey(GITSKey deviceKey);
  void DestroyMetaCommand(GITSKey key);

private:
  void RestoreStateInitialize();
  void RestoreStateFinalize();

private:
  StateTrackingService& m_StateService;
  GITSKey m_DeviceKey{};
  GITSKey m_CommandQueueKey{};
  GITSKey m_CommandAllocatorKey{};
  GITSKey m_CommandListKey{};
  GITSKey m_FenceKey{};
  std::unordered_map<GITSKey, std::vector<uint8_t>> m_MetaCommandData;
};

} // namespace DirectX
} // namespace gits
