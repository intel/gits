// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"

#include "dispatchTablesHolder.h"
#include "log.h"

#include <windows.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gits::vulkan {
namespace {

rtsr::RTSR_API ApiFromVersion(uint32_t version) {
  if (version == 0 || VK_API_VERSION_MAJOR(version) == 0) {
    return rtsr::RTSR_API_VULKAN_1_0;
  }
  if (VK_API_VERSION_MAJOR(version) != 1) {
    return rtsr::RTSR_API_VULKAN_1_3;
  }
  switch (VK_API_VERSION_MINOR(version)) {
  case 0:
    return rtsr::RTSR_API_VULKAN_1_0;
  case 1:
    return rtsr::RTSR_API_VULKAN_1_1;
  case 2:
    return rtsr::RTSR_API_VULKAN_1_2;
  default:
    return rtsr::RTSR_API_VULKAN_1_3;
  }
}

std::filesystem::path ProcessPath() {
  std::vector<char> path(MAX_PATH + 1, '\0');
  GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size()));
  return path.data();
}

} // namespace

RtsrLayer::RtsrLayer(const rtsr::PluginConfig& config,
                     CGits* gits,
                     bool playback,
                     bool execute,
                     DispatchTablesHolder* dispatchTables)
    : Layer("Rtsr"),
      m_Config(config),
      m_Playback(playback),
      m_Available(execute && dispatchTables != nullptr),
      m_DispatchTables(dispatchTables) {
  if (!m_Available) {
    LOG_WARNING << "Rtsr - Vulkan client is disabled because execution or dispatch tables are "
                   "unavailable";
    return;
  }
  m_Client = std::make_unique<rtsr::RtsrClient>(
      config.Endpoints, [](std::string_view message) { LOG_INFO << message; });
  m_ShaderModules = std::make_unique<ShaderModuleTracker>();
  m_Pipelines = std::make_unique<PipelineTracker>(*m_Client, m_Contexts, *m_ShaderModules);
  m_Performance =
      std::make_unique<PerformanceTracker>(*m_Client, *m_Pipelines, m_Contexts, gits, playback);
}

RtsrLayer::~RtsrLayer() {
  m_Enabled.store(false);
  if (m_Client) {
    m_Client->Disconnect();
  }
  m_Performance.reset();
  m_Pipelines.reset();
  m_ShaderModules.reset();
  m_Contexts.Clear();
  m_Client.reset();
}

void RtsrLayer::FlushPendingContexts() {
  std::lock_guard<std::mutex> lock(m_LifecycleMutex);
  for (auto pending = m_PendingInstances.begin(); pending != m_PendingInstances.end();) {
    VkInstanceLevelDispatchTable dispatch{};
    if (!m_DispatchTables->CopyInstanceDispatchTable(pending->second.Handle, dispatch)) {
      ++pending;
      continue;
    }
    m_Contexts.AddInstance(pending->second.Key, pending->second.Handle, dispatch);
    pending = m_PendingInstances.erase(pending);
  }

  for (auto pending = m_PendingDevices.begin(); pending != m_PendingDevices.end();) {
    VkInstanceLevelDispatchTable instanceDispatch{};
    VkDeviceLevelDispatchTable dispatch{};
    if (!m_DispatchTables->CopyInstanceDispatchTable(pending->second.PhysicalDevice,
                                                     instanceDispatch) ||
        !m_DispatchTables->CopyDeviceDispatchTable(pending->second.Handle, dispatch)) {
      ++pending;
      continue;
    }
    m_Contexts.AddDevice(pending->second.Key, pending->second.PhysicalDeviceKey,
                         pending->second.PhysicalDevice, pending->second.Handle, instanceDispatch,
                         dispatch);
    pending = m_PendingDevices.erase(pending);
  }
}

void RtsrLayer::Connect(const vkCreateInstanceCommand& command) {
  if (!m_Available || m_Client == nullptr) {
    return;
  }
  {
    std::unique_lock<std::mutex> lock(m_LifecycleMutex);
    if (m_ConnectAttempted) {
      m_ConnectFinished.wait(lock, [this] { return !m_ConnectInProgress; });
      return;
    }
    m_ConnectAttempted = true;
    m_ConnectInProgress = true;
  }

  const auto processPath = ProcessPath();
  rtsr::ClientInfo info;
  info.ApplicationPath = processPath.string();
  info.ApplicationName = processPath.stem().string();
  uint32_t apiVersion = VK_API_VERSION_1_0;
  if (command.m_pCreateInfo.Value != nullptr &&
      command.m_pCreateInfo.Value->pApplicationInfo != nullptr) {
    const auto* application = command.m_pCreateInfo.Value->pApplicationInfo;
    apiVersion = application->apiVersion;
    if (application->pApplicationName != nullptr && *application->pApplicationName != '\0') {
      info.ApplicationName = application->pApplicationName;
    }
  }
  info.UsedApi = ApiFromVersion(apiVersion);

  rtsr::ClientCallbacks callbacks;
  callbacks.UpdatePipelines = [this](const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request) {
    return m_Pipelines->ApplyUpdates(request);
  };
  callbacks.ResetPipelines = [this](const rtsr::RTSR_RESET_PIPELINES_REQUEST& request) {
    return m_Pipelines->ResetPipelines(request);
  };
  callbacks.EnablePerformanceMeasurements = [this](bool enable) {
    return m_Performance->SetEnabled(enable);
  };
  callbacks.CloseConnection = [this]() {
    m_Performance->SetEnabled(false);
    {
      std::lock_guard<std::mutex> lock(m_LifecycleMutex);
      m_Enabled.store(false);
      if (!m_ConnectInProgress) {
        m_ConnectAttempted = false;
      }
    }
    m_ConnectFinished.notify_all();
    return rtsr::RTSR_RESULT_SUCCESS;
  };

  const bool connected = m_Client->Connect(info, std::move(callbacks),
                                           std::chrono::milliseconds(m_Config.ConnectTimeoutMs));
  bool stillConnected = false;
  {
    std::lock_guard<std::mutex> lock(m_LifecycleMutex);
    stillConnected = connected && m_Client->IsConnected() && !m_LiveInstances.empty();
    m_Enabled.store(stillConnected);
    m_ConnectInProgress = false;
    if (!stillConnected) {
      m_ConnectAttempted = false;
    }
  }
  if (connected && !stillConnected) {
    m_Client->Disconnect();
  }
  m_ConnectFinished.notify_all();
  if (!stillConnected) {
    LOG_WARNING << "Rtsr - no RTSR server answered on " << m_Config.Endpoints.Address
                << "; the Vulkan plugin is disabled";
  } else {
    LOG_INFO << "Rtsr - connected to the RTSR server for Vulkan "
             << VK_API_VERSION_MAJOR(apiVersion) << "." << VK_API_VERSION_MINOR(apiVersion);
  }
}

void RtsrLayer::Pre(vkCreateInstanceCommand&) {
  if (m_Enabled.load()) {
    FlushPendingContexts();
  }
}

void RtsrLayer::Post(vkCreateInstanceCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || command.m_pInstance.Value == nullptr ||
      *command.m_pInstance.Value == VK_NULL_HANDLE) {
    return;
  }
  if (!m_Available) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(m_LifecycleMutex);
    m_LiveInstances.insert(command.m_pInstance.Key);
    m_PendingInstances[command.m_pInstance.Key] =
        PendingInstance{command.m_pInstance.Key, *command.m_pInstance.Value};
  }
  Connect(command);
  if (!m_Enabled.load()) {
    return;
  }

  FlushPendingContexts();
}

void RtsrLayer::Pre(vkDestroyInstanceCommand& command) {
  std::lock_guard<std::mutex> lock(m_LifecycleMutex);
  m_PendingInstances.erase(command.m_instance.Key);
}

void RtsrLayer::Post(vkDestroyInstanceCommand& command) {
  if (!m_Available) {
    return;
  }
  bool lastInstance = false;
  {
    std::lock_guard<std::mutex> lock(m_LifecycleMutex);
    const bool removed = m_LiveInstances.erase(command.m_instance.Key) != 0;
    lastInstance = removed && m_LiveInstances.empty();
    if (lastInstance) {
      m_PendingDevices.clear();
      m_ConnectAttempted = false;
    }
  }
  m_Contexts.RemoveInstance(command.m_instance.Key);
  if (lastInstance) {
    m_Enabled.store(false);
    m_Contexts.Clear();
    m_Client->Disconnect();
    m_ConnectFinished.notify_all();
  }
}

void RtsrLayer::Pre(vkCreateDeviceCommand&) {
  if (m_Enabled.load()) {
    FlushPendingContexts();
  }
}

void RtsrLayer::Post(vkCreateDeviceCommand& command) {
  if (!m_Enabled.load() || command.m_Return.Value != VK_SUCCESS ||
      command.m_pDevice.Value == nullptr || *command.m_pDevice.Value == VK_NULL_HANDLE) {
    return;
  }
  FlushPendingContexts();
  {
    std::lock_guard<std::mutex> lock(m_LifecycleMutex);
    m_PendingDevices[command.m_pDevice.Key] =
        PendingDevice{command.m_pDevice.Key, command.m_physicalDevice.Key,
                      command.m_physicalDevice.Value, *command.m_pDevice.Value};
  }
  FlushPendingContexts();
}

void RtsrLayer::Pre(vkDestroyDeviceCommand& command) {
  if (command.m_Skip || !m_Available || !m_Pipelines || !m_ShaderModules) {
    return;
  }
  if (m_Enabled.load()) {
    FlushPendingContexts();
  }
  {
    std::lock_guard<std::mutex> lock(m_LifecycleMutex);
    m_PendingDevices.erase(command.m_device.Key);
  }
  m_Performance->PrepareDeviceDestroy(command.m_device.Key);
  m_Contexts.RetireDevice(command.m_device.Key);
  m_Performance->OnDestroyDevice(command.m_device.Key);
  m_Pipelines->OnDestroyDevice(command.m_device.Key);
  m_ShaderModules->OnDestroyDevice(command.m_device.Key);
}

void RtsrLayer::Post(vkSetDebugUtilsObjectNameEXTCommand& command) {
  if (m_Enabled.load() && command.m_Return.Value == VK_SUCCESS) {
    FlushPendingContexts();
    m_Pipelines->OnSetName(command.m_device.Key, command.m_pNameInfo.Value);
  }
}

void RtsrLayer::Post(vkCreateShaderModuleCommand& command) {
  if (!m_Enabled.load() || command.m_pShaderModule.Value == nullptr) {
    return;
  }
  FlushPendingContexts();
  m_ShaderModules->OnCreate(command.m_device.Key, command.m_pShaderModule.Key,
                            *command.m_pShaderModule.Value, command.m_pCreateInfo.Value,
                            command.m_Return.Value);
}

void RtsrLayer::Post(vkDestroyShaderModuleCommand& command) {
  if (m_ShaderModules) {
    m_ShaderModules->OnDestroy(command.m_device.Key, command.m_shaderModule.Key,
                               command.m_shaderModule.Value);
  }
}

void RtsrLayer::Pre(vkGetShaderModuleIdentifierEXTCommand& command) {
  if (m_Playback && m_Enabled.load()) {
    m_ShaderModules->OnIdentifier(command.m_shaderModule.Key, command.m_pIdentifier.Value);
  }
}

void RtsrLayer::Post(vkGetShaderModuleIdentifierEXTCommand& command) {
  if (!m_Enabled.load()) {
    return;
  }
  m_ShaderModules->OnIdentifier(command.m_shaderModule.Key, command.m_pIdentifier.Value);
}

void RtsrLayer::Pre(vkGetShaderModuleCreateInfoIdentifierEXTCommand& command) {
  if (m_Playback && m_Enabled.load()) {
    m_ShaderModules->OnCreateInfoIdentifier(command.m_device.Key, command.m_pCreateInfo.Value,
                                            command.m_pIdentifier.Value);
  }
}

void RtsrLayer::Post(vkGetShaderModuleCreateInfoIdentifierEXTCommand& command) {
  if (!m_Enabled.load()) {
    return;
  }
  m_ShaderModules->OnCreateInfoIdentifier(command.m_device.Key, command.m_pCreateInfo.Value,
                                          command.m_pIdentifier.Value);
}

void RtsrLayer::Post(vkCreateGraphicsPipelinesCommand& command) {
  if (!m_Enabled.load()) {
    return;
  }
  FlushPendingContexts();
  m_Pipelines->HarvestGraphics(command);
}

void RtsrLayer::Post(vkCreateComputePipelinesCommand& command) {
  if (!m_Enabled.load()) {
    return;
  }
  FlushPendingContexts();
  m_Pipelines->HarvestCompute(command);
}

void RtsrLayer::Post(vkCreatePipelineLayoutCommand& command) {
  if (m_Enabled.load() && command.m_pPipelineLayout.Value != nullptr) {
    m_Pipelines->OnCreatePipelineLayout(command.m_device.Key, command.m_pPipelineLayout.Key,
                                        *command.m_pPipelineLayout.Value, command.m_Return.Value);
  }
}

void RtsrLayer::Post(vkCreateRenderPassCommand& command) {
  if (m_Enabled.load() && command.m_pRenderPass.Value != nullptr) {
    m_Pipelines->OnCreateRenderPass(command.m_device.Key, command.m_pRenderPass.Key,
                                    *command.m_pRenderPass.Value, command.m_Return.Value);
    if (!command.m_Skip && command.m_Return.Value == VK_SUCCESS && command.m_pCreateInfo.Value) {
      m_Performance->TrackRenderPass(command.m_device.Key, *command.m_pRenderPass.Value,
                                     *command.m_pCreateInfo.Value);
    }
  }
}

void RtsrLayer::Post(vkCreateRenderPass2Command& command) {
  if (m_Enabled.load() && command.m_pRenderPass.Value != nullptr) {
    m_Pipelines->OnCreateRenderPass(command.m_device.Key, command.m_pRenderPass.Key,
                                    *command.m_pRenderPass.Value, command.m_Return.Value);
    if (!command.m_Skip && command.m_Return.Value == VK_SUCCESS && command.m_pCreateInfo.Value) {
      m_Performance->TrackRenderPass(command.m_device.Key, *command.m_pRenderPass.Value,
                                     *command.m_pCreateInfo.Value);
    }
  }
}

void RtsrLayer::Post(vkCreateRenderPass2KHRCommand& command) {
  if (m_Enabled.load() && command.m_pRenderPass.Value != nullptr) {
    m_Pipelines->OnCreateRenderPass(command.m_device.Key, command.m_pRenderPass.Key,
                                    *command.m_pRenderPass.Value, command.m_Return.Value);
    if (!command.m_Skip && command.m_Return.Value == VK_SUCCESS && command.m_pCreateInfo.Value) {
      m_Performance->TrackRenderPass(command.m_device.Key, *command.m_pRenderPass.Value,
                                     *command.m_pCreateInfo.Value);
    }
  }
}

void RtsrLayer::Post(vkDestroyPipelineCommand& command) {
  if (m_Pipelines) {
    m_Pipelines->OnDestroy(command.m_device.Key, command.m_pipeline.Key, command.m_pipeline.Value);
  }
}

void RtsrLayer::Pre(vkDestroyPipelineLayoutCommand& command) {
  if (m_Pipelines) {
    auto device = m_Contexts.Device(command.m_device.Key);
    std::unique_lock<std::shared_mutex> replacementLock;
    if (device) {
      replacementLock = std::unique_lock(device->ReplacementMutex);
    }
    m_Pipelines->OnDestroyPipelineLayout(command.m_device.Key, command.m_pipelineLayout.Value);
  }
}

void RtsrLayer::Pre(vkDestroyRenderPassCommand& command) {
  if (command.m_Skip) {
    return;
  }
  if (m_Performance) {
    m_Performance->ForgetRenderPass(command.m_device.Key, command.m_renderPass.Value);
  }
  if (m_Pipelines) {
    auto device = m_Contexts.Device(command.m_device.Key);
    std::unique_lock<std::shared_mutex> replacementLock;
    if (device) {
      replacementLock = std::unique_lock(device->ReplacementMutex);
    }
    m_Pipelines->OnDestroyRenderPass(command.m_device.Key, command.m_renderPass.Value);
  }
}

void RtsrLayer::Pre(vkCmdBindPipelineCommand& command) {
  if (!m_Enabled.load() || m_StateRestore.load() || command.m_Skip) {
    return;
  }
  if (m_Performance->IsActive()) {
    m_Performance->OnBindPipeline(command);
  }
  if (m_Pipelines->HasReplacementWork()) {
    command.m_pipeline.Value = m_Pipelines->OnBind(command.m_pipeline.Key, command.m_pipeline.Value,
                                                   command.m_commandBuffer.Key);
  }
}

#define RTSR_RENDER_SCOPE_HOOK(commandType, expression)                                            \
  void RtsrLayer::Post(commandType& command) {                                                     \
    if (!command.m_Skip && m_Performance && m_Performance->IsActive()) {                           \
      expression;                                                                                  \
    }                                                                                              \
  }

#define RTSR_BEGIN_RENDER_PASS(commandType)                                                        \
  RTSR_RENDER_SCOPE_HOOK(                                                                          \
      commandType, if (command.m_pRenderPassBegin.Value) {                                         \
        m_Performance->BeginRenderPass(command.m_commandBuffer.Key,                                \
                                       command.m_pRenderPassBegin.Value->renderPass);              \
      })
RTSR_BEGIN_RENDER_PASS(vkCmdBeginRenderPassCommand)
RTSR_BEGIN_RENDER_PASS(vkCmdBeginRenderPass2Command)
RTSR_BEGIN_RENDER_PASS(vkCmdBeginRenderPass2KHRCommand)
#undef RTSR_BEGIN_RENDER_PASS

RTSR_RENDER_SCOPE_HOOK(vkCmdNextSubpassCommand,
                       m_Performance->NextSubpass(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(vkCmdNextSubpass2Command,
                       m_Performance->NextSubpass(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(vkCmdNextSubpass2KHRCommand,
                       m_Performance->NextSubpass(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(vkCmdEndRenderPassCommand,
                       m_Performance->EndRendering(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(vkCmdEndRenderPass2Command,
                       m_Performance->EndRendering(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(vkCmdEndRenderPass2KHRCommand,
                       m_Performance->EndRendering(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(
    vkCmdBeginRenderingCommand, if (command.m_pRenderingInfo.Value) {
      m_Performance->BeginRendering(command.m_commandBuffer.Key,
                                    command.m_pRenderingInfo.Value->viewMask);
    })
RTSR_RENDER_SCOPE_HOOK(
    vkCmdBeginRenderingKHRCommand, if (command.m_pRenderingInfo.Value) {
      m_Performance->BeginRendering(command.m_commandBuffer.Key,
                                    command.m_pRenderingInfo.Value->viewMask);
    })
RTSR_RENDER_SCOPE_HOOK(vkCmdEndRenderingCommand,
                       m_Performance->EndRendering(command.m_commandBuffer.Key))
RTSR_RENDER_SCOPE_HOOK(vkCmdEndRenderingKHRCommand,
                       m_Performance->EndRendering(command.m_commandBuffer.Key))
#undef RTSR_RENDER_SCOPE_HOOK

#define RTSR_DEFINE_WORK_HOOKS(commandType, bindPoint)                                             \
  void RtsrLayer::Pre(commandType& command) {                                                      \
    if (!command.m_Skip && m_Enabled.load() && !m_StateRestore.load() &&                           \
        m_Performance->IsActive()) {                                                               \
      m_Performance->OnWorkPre(command.m_commandBuffer.Key, bindPoint);                            \
    }                                                                                              \
  }                                                                                                \
  void RtsrLayer::Post(commandType& command) {                                                     \
    if (!command.m_Skip && m_Performance && m_Performance->IsActive()) {                           \
      m_Performance->OnWorkPost(command.m_commandBuffer.Key);                                      \
    }                                                                                              \
  }

RTSR_DEFINE_WORK_HOOKS(vkCmdDrawCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndexedCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawMultiEXTCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawMultiIndexedEXTCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndirectCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndexedIndirectCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndirectCountCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndirectCountKHRCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndirectCountAMDCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndexedIndirectCountCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndexedIndirectCountKHRCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndexedIndirectCountAMDCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDrawIndirectByteCountEXTCommand, VK_PIPELINE_BIND_POINT_GRAPHICS)
RTSR_DEFINE_WORK_HOOKS(vkCmdDispatchCommand, VK_PIPELINE_BIND_POINT_COMPUTE)
RTSR_DEFINE_WORK_HOOKS(vkCmdDispatchIndirectCommand, VK_PIPELINE_BIND_POINT_COMPUTE)
RTSR_DEFINE_WORK_HOOKS(vkCmdDispatchBaseCommand, VK_PIPELINE_BIND_POINT_COMPUTE)
RTSR_DEFINE_WORK_HOOKS(vkCmdDispatchBaseKHRCommand, VK_PIPELINE_BIND_POINT_COMPUTE)

#undef RTSR_DEFINE_WORK_HOOKS

void RtsrLayer::Post(vkGetDeviceQueueCommand& command) {
  if (!command.m_Skip && m_Enabled.load()) {
    FlushPendingContexts();
    m_Performance->OnGetDeviceQueue(command);
  }
}

void RtsrLayer::Post(vkGetDeviceQueue2Command& command) {
  if (!command.m_Skip && m_Enabled.load()) {
    FlushPendingContexts();
    m_Performance->OnGetDeviceQueue2(command);
  }
}

void RtsrLayer::Post(vkCreateCommandPoolCommand& command) {
  if (!command.m_Skip && m_Enabled.load()) {
    FlushPendingContexts();
    m_Performance->OnCreateCommandPool(command);
  }
}

void RtsrLayer::Post(vkDestroyCommandPoolCommand& command) {
  if (!command.m_Skip && m_Pipelines) {
    m_Pipelines->OnResetCommandPool(command.m_device.Key, command.m_commandPool.Value, true);
  }
  if (!command.m_Skip && m_Performance) {
    m_Performance->OnDestroyCommandPool(command);
  }
}

void RtsrLayer::Post(vkResetCommandPoolCommand& command) {
  if (!command.m_Skip && command.m_Return.Value == VK_SUCCESS && m_Pipelines) {
    m_Pipelines->OnResetCommandPool(command.m_device.Key, command.m_commandPool.Value, false);
  }
  if (!command.m_Skip && m_Performance) {
    m_Performance->OnResetCommandPool(command);
  }
}

void RtsrLayer::Post(vkAllocateCommandBuffersCommand& command) {
  if (!command.m_Skip && m_Pipelines) {
    m_Pipelines->OnAllocateCommandBuffers(command);
  }
  if (!command.m_Skip && m_Enabled.load()) {
    FlushPendingContexts();
    m_Performance->OnAllocateCommandBuffers(command);
  }
}

void RtsrLayer::Post(vkFreeCommandBuffersCommand& command) {
  if (!command.m_Skip && m_Pipelines) {
    m_Pipelines->OnFreeCommandBuffers(command);
  }
  if (!command.m_Skip && m_Performance) {
    m_Performance->OnFreeCommandBuffers(command);
  }
}

void RtsrLayer::Post(vkResetCommandBufferCommand& command) {
  if (!command.m_Skip && m_Pipelines) {
    m_Pipelines->OnResetCommandBuffer(command.m_commandBuffer.Key, command.m_Return.Value);
  }
  if (!command.m_Skip && m_Performance) {
    m_Performance->OnResetCommandBuffer(command);
  }
}

void RtsrLayer::Post(vkBeginCommandBufferCommand& command) {
  if (!command.m_Skip && m_Pipelines) {
    m_Pipelines->OnResetCommandBuffer(command.m_commandBuffer.Key, command.m_Return.Value);
  }
  if (!command.m_Skip && m_Enabled.load() && !m_StateRestore.load() && m_Performance->IsActive()) {
    m_Performance->OnBeginCommandBuffer(command);
  }
}

void RtsrLayer::Pre(vkEndCommandBufferCommand& command) {
  if (!command.m_Skip && !m_StateRestore.load() && m_Performance && m_Performance->IsActive()) {
    m_Performance->OnEndCommandBufferPre(command);
  }
}

void RtsrLayer::Post(vkEndCommandBufferCommand& command) {
  if (!command.m_Skip && m_Performance && m_Performance->IsActive()) {
    m_Performance->OnEndCommandBufferPost(command);
  }
}

void RtsrLayer::Pre(vkCmdExecuteCommandsCommand& command) {
  if (!command.m_Skip && !m_StateRestore.load() && m_Performance && m_Performance->IsActive()) {
    m_Performance->OnExecuteCommands(command);
  }
}

void RtsrLayer::Pre(vkQueueSubmitCommand& command) {
  if (!command.m_Skip && !m_StateRestore.load() && m_Performance && m_Performance->IsActive()) {
    m_Performance->OnQueueSubmit(command);
  }
}

void RtsrLayer::Pre(vkQueueSubmit2Command& command) {
  if (!command.m_Skip && !m_StateRestore.load() && m_Performance && m_Performance->IsActive()) {
    m_Performance->OnQueueSubmit2(command);
  }
}

void RtsrLayer::Pre(vkQueueSubmit2KHRCommand& command) {
  if (!command.m_Skip && !m_StateRestore.load() && m_Performance && m_Performance->IsActive()) {
    m_Performance->OnQueueSubmit2(command);
  }
}

void RtsrLayer::Post(vkQueueSubmitCommand& command) {
  if (m_Performance) {
    m_Performance->OnQueueSubmitPost(command.m_queue.Value,
                                     command.m_Skip ? VK_ERROR_UNKNOWN : command.m_Return.Value);
  }
}

void RtsrLayer::Post(vkQueueSubmit2Command& command) {
  if (m_Performance) {
    m_Performance->OnQueueSubmitPost(command.m_queue.Value,
                                     command.m_Skip ? VK_ERROR_UNKNOWN : command.m_Return.Value);
  }
}

void RtsrLayer::Post(vkQueueSubmit2KHRCommand& command) {
  if (m_Performance) {
    m_Performance->OnQueueSubmitPost(command.m_queue.Value,
                                     command.m_Skip ? VK_ERROR_UNKNOWN : command.m_Return.Value);
  }
}

void RtsrLayer::Post(vkQueuePresentKHRCommand& command) {
  if (m_Enabled.load() && !m_StateRestore.load() && m_Performance->IsActive() &&
      (command.m_Return.Value == VK_SUCCESS || command.m_Return.Value == VK_SUBOPTIMAL_KHR)) {
    m_Performance->OnPresent();
  }
}

void RtsrLayer::Post(StateRestoreBeginCommand&) {
  m_StateRestore.store(true);
}

void RtsrLayer::Post(StateRestoreEndCommand&) {
  m_StateRestore.store(false);
  if (m_Enabled.load()) {
    FlushPendingContexts();
  }
}

} // namespace gits::vulkan
