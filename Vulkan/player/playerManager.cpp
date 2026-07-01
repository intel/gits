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
    // Tear the manager down at end of playback, while the Vulkan device and
    // driver library are still valid, so the asynchronous screenshot dumper
    // worker threads owned by the layers get flushed/joined before teardown
    // (otherwise the final present's screenshot of a one-frame substream is
    // lost when the process exits).  The new StreamReader playback path used by
    // Vulkan publishes {PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT} on the
    // MessageBus::get() singleton right after the playback loop (see
    // PlayStream); subscribe on that same bus so the synchronous publish()
    // invokes this handler at that safe point.  Mirrors the DirectX player.
    // Destroy() is idempotent (deletes and nulls the singleton), so it is safe
    // even if invoked again or alongside normal static destruction.
    MessageBus::get().subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
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
