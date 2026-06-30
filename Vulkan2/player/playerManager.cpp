// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"
#include "pluginService.h"
#include "configurator.h"
#include "log.h"

namespace gits {
namespace vulkan {

PlayerManager* PlayerManager::m_Instance = nullptr;

PlayerManager& PlayerManager::Get() {
  if (!m_Instance) {
    m_Instance = new PlayerManager();
    CGits::Instance().GetMessageBus().subscribe(
        {PUBLISHER_PLAYER, TOPIC_END}, [](Topic t, const MessagePtr& m) {
          auto msg = std::dynamic_pointer_cast<PlaybackEndMessage>(m);
          if (msg) {
            PlayerManager::Destroy();
          }
        });
  }
  return *m_Instance;
}

PlayerManager::~PlayerManager() {
  try {
    LOG_INFO << "PlayerManager: Playback completed. Cleaning up...";
    m_PluginService.reset();
    dl::close_library(m_Lib);
    m_Lib = nullptr;
  } catch (...) {
    topmost_exception_handler("PlayerManager::~PlayerManager");
  }
}

PlayerManager::PlayerManager() : m_SwapchainImageSyncService(*this) {
  LoadGlobalFunctions();

  m_DispatchTablesHolder = std::make_unique<DispatchTablesHolder>(
      m_InstanceDispatchTable, m_DeviceDispatchTable, m_DispatchTablesMutex);

  m_PluginService = std::make_unique<PluginService>();
  m_PluginService->LoadPlugins();
  m_LayerManager.LoadLayers(*this, *m_PluginService);
}

void PlayerManager::LoadGlobalFunctions() {
  auto& cfg = Configurator::Get();
  m_Lib = dl::open_library(cfg.common.player.libVK.string().c_str());
  GITS_ASSERT(m_Lib);
  m_GetInstanceProcAddr =
      reinterpret_cast<PFN_vkGetInstanceProcAddr>(dl::load_symbol(m_Lib, "vkGetInstanceProcAddr"));
  LoadGlobalLevelFunctions(m_GetInstanceProcAddr, m_GlobalDispatchTable);
}

void PlayerManager::LoadInstanceFunctions(VkInstance instance) {
  void* dispatchKey = *reinterpret_cast<void**>(instance);
  std::unique_lock lock(m_DispatchTablesMutex);
  auto& dispatchTable = m_InstanceDispatchTable[dispatchKey];
  LoadInstanceLevelFunctions(m_GetInstanceProcAddr, instance, dispatchTable);
}

void PlayerManager::LoadDeviceFunctions(void* dispatchKey, VkDevice device) {
  std::unique_lock lock(m_DispatchTablesMutex);
  auto& instanceTable = m_InstanceDispatchTable[dispatchKey];
  PFN_vkGetDeviceProcAddr getDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      instanceTable.vkGetInstanceProcAddr(instanceTable.instance, "vkGetDeviceProcAddr"));
  void* deviceDispatchKey = *reinterpret_cast<void**>(device);
  auto& dispatchTable = m_DeviceDispatchTable[deviceDispatchKey];
  LoadDeviceLevelFunctions(getDeviceProcAddr, device, dispatchTable);
}

} // namespace vulkan
} // namespace gits
