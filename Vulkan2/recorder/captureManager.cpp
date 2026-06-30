// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"
#include "gits.h"
#include "log.h"
#include "wrappersAuto.h"

namespace gits {
namespace vulkan {

CaptureManager* CaptureManager::m_Instance = nullptr;

thread_local unsigned CaptureManager::m_RecursionDepth = 0;

CaptureManager& CaptureManager::Get() {
  if (!m_Instance) {
    m_Instance = new CaptureManager();
  }
  return *m_Instance;
}

void CaptureManager::Cleanup() {
  if (m_Instance) {
    delete m_Instance;
    m_Instance = nullptr;
  }
}

CaptureManager::CaptureManager() {
  m_Recorder.reset(new stream::OrderingRecorder());

  m_MapTrackingService.reset(new MapTrackingService(*m_Recorder));

  m_DispatchTablesHolder = std::make_unique<DispatchTablesHolder>(
      m_InstanceDispatchTable, m_DeviceDispatchTable, m_DispatchTablesMutex);

  m_PluginService.LoadPlugins();
  m_LayerManager.LoadLayers(*this, *m_Recorder.get(), m_PluginService);
}

CaptureManager::~CaptureManager() {}

void CaptureManager::LoadGlobalFunctions(PFN_vkGetInstanceProcAddr getProcAddr) {
  LoadGlobalLevelFunctions(getProcAddr, m_GlobalDispatchTable);
}

void CaptureManager::LoadInstanceFunctions(PFN_vkGetInstanceProcAddr getProcAddr,
                                           VkInstance instance) {
  void* dispatchKey = *reinterpret_cast<void**>(instance);
  std::unique_lock lock(m_DispatchTablesMutex);
  auto& dispatchTable = m_InstanceDispatchTable[dispatchKey];
  LoadInstanceLevelFunctions(getProcAddr, instance, dispatchTable);
}

void CaptureManager::LoadDeviceFunctions(PFN_vkGetDeviceProcAddr getProcAddr, VkDevice device) {
  void* dispatchKey = *reinterpret_cast<void**>(device);
  std::unique_lock lock(m_DispatchTablesMutex);
  auto& dispatchTable = m_DeviceDispatchTable[dispatchKey];
  LoadDeviceLevelFunctions(getProcAddr, device, dispatchTable);
}

void CaptureManager::LoadDeviceFunctions(void* dispatchKey, VkDevice device) {
  std::unique_lock lock(m_DispatchTablesMutex);
  auto& instanceTable = m_InstanceDispatchTable[dispatchKey];
  PFN_vkGetDeviceProcAddr getDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      instanceTable.vkGetInstanceProcAddr(instanceTable.instance, "vkGetDeviceProcAddr"));
  void* deviceDispatchKey = *reinterpret_cast<void**>(device);
  auto& dispatchTable = m_DeviceDispatchTable[deviceDispatchKey];
  LoadDeviceLevelFunctions(getDeviceProcAddr, device, dispatchTable);
}

PFN_vkVoidFunction CaptureManager::GetFunctionWrapper(const char* name) {
  auto it = g_FunctionWrappers.find(name);
  if (it != g_FunctionWrappers.end()) {
    return it->second;
  }
  return nullptr;
}

} // namespace vulkan
} // namespace gits
