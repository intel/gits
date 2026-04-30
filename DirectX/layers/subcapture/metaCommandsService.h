// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  void SetDeviceKey(unsigned deviceKey);

private:
  void RestoreStateInitialize();
  void RestoreStateFinalize();

private:
  StateTrackingService& m_StateService;
  unsigned m_DeviceKey{};
  unsigned m_CommandQueueKey{};
  unsigned m_CommandAllocatorKey{};
  unsigned m_CommandListKey{};
  unsigned m_FenceKey{};
  std::unordered_map<unsigned, std::vector<uint8_t>> m_MetaCommandData;
};

} // namespace DirectX
} // namespace gits
