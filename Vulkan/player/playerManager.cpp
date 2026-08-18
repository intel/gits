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
#include "messageBus.h"

namespace gits {
namespace vulkan {

PlayerManager* PlayerManager::m_Instance = nullptr;

PlayerManager& PlayerManager::Get() {
  if (!m_Instance) {
    m_Instance = new PlayerManager();
    // Tear down when playback finishes, same as DirectX PlayerManager.
    gits::MessageBus::get().subscribe(
        {PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
        [](Topic t, const MessagePtr& m) { PlayerManager::Destroy(); });
  }
  return *m_Instance;
}

PlayerManager::~PlayerManager() {
  try {
    LOG_INFO << "PlayerManager: Playback completed. Cleaning up...";
    // Tear down layers (and their async resource dumpers) before the dispatch
    // tables and driver library go away.  The member-destruction order would
    // otherwise destroy m_LayerManager last -- after dl::close_library below and
    // after the dispatch-table maps are gone -- so the ScreenshotsLayer's
    // worker-thread flush of the final present's screenshot would run against a
    // closed library and dangling dispatch table, dropping that screenshot
    // entirely for a single-frame stream.
    m_LayerManager.Shutdown();
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
  m_PluginService->SetVkDriverRewindPresentCountPtr(
      m_SwapchainImageSyncService.GetDriverRewindPresentCountPtr());
  m_PluginService->LoadPlugins();
  m_LayerManager.LoadLayers(*this, *m_PluginService);

  m_ExecuteCommands = Configurator::Get().common.player.execute;
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
  if (m_PluginService) {
    m_PluginService->SetVulkanDeviceDispatchTable(&dispatchTable);
    m_PluginService->SetVulkanInstanceDispatchTable(&instanceTable);
  }
}

} // namespace vulkan
} // namespace gits
