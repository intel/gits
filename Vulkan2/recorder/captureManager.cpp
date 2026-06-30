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
#include "vulkanApiIfaceRecorder.h"
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

CaptureManager::CaptureManager() {
  CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<gits::ApisIface::Api3d>(new VulkanApiIfaceRecorder()));

  m_Recorder.reset(new GitsRecorder());

  m_MapTrackingService.reset(new MapTrackingService(*m_Recorder));

  m_LayerManager.LoadLayers(*this, *m_Recorder.get());
}

void CaptureManager::LoadGlobalFunctions(PFN_vkGetInstanceProcAddr getProcAddr) {
  LoadGlobalLevelFunctions(getProcAddr, m_GlobalDispatchTable);
}

void CaptureManager::LoadInstanceFunctions(PFN_vkGetInstanceProcAddr getProcAddr,
                                           VkInstance instance) {
  void* dispatchKey = *reinterpret_cast<void**>(instance);
  auto& dispatchTable = m_InstanceDispatchTable[dispatchKey];
  LoadInstanceLevelFunctions(getProcAddr, instance, dispatchTable);
}

void CaptureManager::LoadDeviceFunctions(PFN_vkGetDeviceProcAddr getProcAddr, VkDevice device) {
  void* dispatchKey = *reinterpret_cast<void**>(device);
  auto& dispatchTable = m_DeviceDispatchTable[dispatchKey];
  LoadDeviceLevelFunctions(getProcAddr, device, dispatchTable);
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
