// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandRunnersCustom.h"
#include "layerAuto.h"
#include "playerManager.h"

namespace gits {
namespace vulkan {

void CreateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    uint64_t hWnd = manager.GetWindowService().SetWindow(
        m_Command.m_Hwnd.Value, m_Command.m_Hinstance.Value, m_Command.m_X.Value,
        m_Command.m_Y.Value, m_Command.m_Width.Value, m_Command.m_Height.Value,
        m_Command.m_Visible.Value);

    m_Command.m_Hwnd.Value = hWnd;
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void MappedDataMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    auto mappedEntry =
        manager.GetMapTrackingService().GetData(m_Command.m_Device.Key, m_Command.m_Memory.Key);
    for (const auto& region : m_Command.m_Regions.Regions) {
      char* dstPtr = static_cast<char*>(mappedEntry->Ptr) + region.Offset;
      std::memcpy(dstPtr, region.Data, region.Size);
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

} // namespace vulkan
} // namespace gits
