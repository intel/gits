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

#include "log.h"

namespace gits {
namespace vulkan {

void StateRestoreBeginRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void StateRestoreEndRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void FrameEndRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void MarkerUInt64Runner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void CreateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    uint64_t hWnd = manager.GetWindowService().SetWindow(
        m_Command.m_DisplayProtocol.Value, m_Command.m_Hwnd.Value, m_Command.m_Hinstance.Value,
        m_Command.m_X.Value, m_Command.m_Y.Value, m_Command.m_Width.Value, m_Command.m_Height.Value,
        m_Command.m_Visible.Value);

    m_Command.m_Hwnd.Value = hWnd;
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

void UpdateWindowMetaRunner::Run() {
  auto& manager = PlayerManager::Get();

  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(m_Command);
  }

  if (manager.ExecuteCommands() && !m_Command.m_Skip) {
    manager.GetWindowService().UpdateWindow(m_Command.m_Hwnd.Value, m_Command.m_X.Value,
                                            m_Command.m_Y.Value, m_Command.m_Width.Value,
                                            m_Command.m_Height.Value, m_Command.m_Visible.Value);
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
    // If the memory isn't currently mapped, skip the update and warn instead of
    // dereferencing a null/stale pointer.
    if (mappedEntry == nullptr || mappedEntry->Ptr == nullptr) {
      LOG_WARNING << "MappedDataMetaRunner: no live host mapping for device key "
                  << m_Command.m_Device.Key << ", memory key " << m_Command.m_Memory.Key
                  << " - skipping memory update.";
    } else {
      for (const auto& region : m_Command.m_Regions.Regions) {
        if (region.Offset + region.Size > mappedEntry->Size) {
          LOG_WARNING << "MappedDataMetaRunner: region [offset " << region.Offset << ", size "
                      << region.Size << "] exceeds mapped range (" << mappedEntry->Size
                      << ") for device key " << m_Command.m_Device.Key << ", memory key "
                      << m_Command.m_Memory.Key << " - skipping region.";
          continue;
        }
        char* dstPtr = static_cast<char*>(mappedEntry->Ptr) + region.Offset;
        std::memcpy(dstPtr, region.Data, region.Size);
      }
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(m_Command);
  }
}

} // namespace vulkan
} // namespace gits
