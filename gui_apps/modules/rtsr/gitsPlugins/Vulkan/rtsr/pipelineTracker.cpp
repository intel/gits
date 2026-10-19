// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineTracker.h"

#include "log.h"
#include "vulkanStateDocument.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <map>
#include <sstream>
#include <type_traits>
#include <utility>

namespace gits::vulkan {
namespace {

constexpr size_t MAX_RETAINED_REPLACEMENTS_PER_DEVICE = 64;

template <typename Handle>
uint64_t HandleBits(Handle handle) {
  if constexpr (std::is_pointer_v<Handle>) {
    return reinterpret_cast<uint64_t>(handle);
  } else {
    return static_cast<uint64_t>(handle);
  }
}

template <typename Handle>
Handle HandleFromBits(uint64_t bits) {
  if constexpr (std::is_pointer_v<Handle>) {
    return reinterpret_cast<Handle>(bits);
  } else {
    return static_cast<Handle>(bits);
  }
}

std::string PipelineName(rtsr::RTSR_HASH hash, const char* type) {
  std::ostringstream name;
  name << "RTSR pipeline #" << std::uppercase << std::hex << hash << " (Vulkan " << type << ")";
  return name.str();
}

class DeviceOperation {
public:
  explicit DeviceOperation(std::shared_ptr<RtsrDeviceContext> device)
      : m_Device(std::move(device)), m_Active(m_Device && m_Device->BeginOperation()) {}
  ~DeviceOperation() {
    if (m_Active) {
      m_Device->EndOperation();
    }
  }
  explicit operator bool() const {
    return m_Active;
  }

private:
  std::shared_ptr<RtsrDeviceContext> m_Device;
  bool m_Active{};
};

class TemporaryShaderModules {
public:
  explicit TemporaryShaderModules(std::shared_ptr<RtsrDeviceContext> device)
      : m_Device(std::move(device)) {}
  ~TemporaryShaderModules() {
    if (m_Device && m_Device->Dispatch.vkDestroyShaderModule != nullptr) {
      for (const VkShaderModule module : m_Modules) {
        if (module != VK_NULL_HANDLE) {
          m_Device->Dispatch.vkDestroyShaderModule(m_Device->Handle, module, nullptr);
        }
      }
    }
  }

  bool Create(const rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO& shader, VkShaderModule& module) {
    module = VK_NULL_HANDLE;
    if (!m_Device || m_Device->Dispatch.vkCreateShaderModule == nullptr ||
        shader.Bytecode.empty() || shader.Bytecode.size() % sizeof(uint32_t) != 0) {
      return false;
    }
    std::vector<uint32_t> words(shader.Bytecode.size() / sizeof(uint32_t));
    std::memcpy(words.data(), shader.Bytecode.data(), shader.Bytecode.size());
    if (words[0] != 0x07230203u) {
      return false;
    }
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shader.Bytecode.size();
    createInfo.pCode = words.data();
    if (m_Device->Dispatch.vkCreateShaderModule(m_Device->Handle, &createInfo, nullptr, &module) !=
            VK_SUCCESS ||
        module == VK_NULL_HANDLE) {
      return false;
    }
    m_Modules.push_back(module);
    return true;
  }

private:
  std::shared_ptr<RtsrDeviceContext> m_Device;
  std::vector<VkShaderModule> m_Modules;
};

void StripShaderModulePNext(VkPipelineShaderStageCreateInfo& stage) {
  auto* head = reinterpret_cast<VkBaseOutStructure*>(const_cast<void*>(stage.pNext));
  VkBaseOutStructure** link = &head;
  while (*link != nullptr) {
    auto* node = *link;
    if (node->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO ||
        node->sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT) {
      *link = node->pNext;
    } else {
      link = &node->pNext;
    }
  }
  stage.pNext = head;
}

} // namespace

PipelineTracker::PipelineTracker(rtsr::RtsrClient& client,
                                 RtsrContextRegistry& contexts,
                                 ShaderModuleTracker& shaders)
    : m_Client(client), m_Contexts(contexts), m_Shaders(shaders) {}

void PipelineTracker::OnCreatePipelineLayout(GITSKey deviceKey,
                                             GITSKey key,
                                             VkPipelineLayout layout,
                                             VkResult result) {
  if (result == VK_SUCCESS && key != 0 && layout != VK_NULL_HANDLE) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_PipelineLayoutKeys[deviceKey][layout] = key;
  }
}

void PipelineTracker::OnCreateRenderPass(GITSKey deviceKey,
                                         GITSKey key,
                                         VkRenderPass renderPass,
                                         VkResult result) {
  if (result == VK_SUCCESS && key != 0 && renderPass != VK_NULL_HANDLE) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RenderPassKeys[deviceKey][renderPass] = key;
  }
}

PipelineTracker::~PipelineTracker() {
  std::vector<GITSKey> devices;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& entry : m_Pipelines) {
      const auto& pipeline = entry.second;
      if (std::find(devices.begin(), devices.end(), pipeline->Device->Key) == devices.end()) {
        devices.push_back(pipeline->Device->Key);
      }
    }
    for (const auto& retired : m_Retired) {
      const GITSKey deviceKey = retired.first;
      if (std::find(devices.begin(), devices.end(), deviceKey) == devices.end()) {
        devices.push_back(deviceKey);
      }
    }
  }
  for (const GITSKey device : devices) {
    OnDestroyDevice(device);
  }
}

void PipelineTracker::Publish(std::unique_ptr<TrackedPipeline> pipeline,
                              std::string state,
                              std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages) {
  const GITSKey key = pipeline->Key;
  const GITSKey deviceKey = pipeline->Device->Key;
  const VkPipeline handle = pipeline->Original;
  const auto type = pipeline->Type;
  const auto hash = pipeline->StateHash;
  const std::string name = pipeline->Name;
  const bool replacementEligible = pipeline->ReplacementEligible;
  const std::string replacementDiagnostic = pipeline->Diagnostic;
  rtsr::RTSR_PIPELINE_INCARNATION incarnation{};
  bool replaced = false;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    pipeline->Incarnation = m_NextIncarnation++;
    incarnation = pipeline->Incarnation;
    const auto old = m_Pipelines.find(key);
    if (old != m_Pipelines.end()) {
      DestroyReplacement(*old->second);
      m_PipelineKeys[old->second->Device->Key].erase(old->second->Original);
      m_Pipelines.erase(old);
      replaced = true;
    }
    m_PipelineKeys[deviceKey][handle] = key;
    m_Pipelines.emplace(key, std::move(pipeline));
    if (replaced) {
      m_Client.PipelineDeleted(key);
    }
    m_Client.PipelineCreated(key, type, state, hash, incarnation, replacementEligible,
                             replacementDiagnostic, std::move(stages));
    m_Client.SetPipelineName(key, name);
  }
}

void PipelineTracker::HarvestGraphics(vkCreateGraphicsPipelinesCommand& command) {
  auto device = m_Contexts.Device(command.m_device.Key);
  if (!device || command.m_pCreateInfos.Value == nullptr || command.m_pPipelines.Value == nullptr) {
    return;
  }
  const uint32_t count = std::min({command.m_createInfoCount.Value, command.m_pCreateInfos.Size,
                                   command.m_pPipelines.Size,
                                   static_cast<uint32_t>(command.m_pPipelines.Keys.size())});
  for (uint32_t i = 0; i < count; ++i) {
    const GITSKey key = command.m_pPipelines.Keys[i];
    const VkPipeline handle = command.m_pPipelines.Value[i];
    if (key == 0 || handle == VK_NULL_HANDLE) {
      continue;
    }

    const auto& createInfo = command.m_pCreateInfos.Value[i];
    std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages;
    stages.reserve(createInfo.stageCount);
    bool allShadersResolved = createInfo.pStages != nullptr;
    if (createInfo.pStages != nullptr) {
      for (uint32_t stageIndex = 0; stageIndex < createInfo.stageCount; ++stageIndex) {
        auto shader = m_Shaders.Resolve(command.m_device.Key, createInfo.pStages[stageIndex]);
        if (!shader.has_value()) {
          allShadersResolved = false;
          continue;
        }
        stages.push_back(std::move(shader->Wire));
      }
    }

    auto snapshot = PipelineStateSnapshot::Encode(createInfo);
    GITSKey layoutKey{};
    GITSKey renderPassKey{};
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      const auto layouts = m_PipelineLayoutKeys.find(command.m_device.Key);
      if (layouts != m_PipelineLayoutKeys.end()) {
        const auto layout = layouts->second.find(createInfo.layout);
        if (layout != layouts->second.end()) {
          layoutKey = layout->second;
        }
      }
      const auto renderPasses = m_RenderPassKeys.find(command.m_device.Key);
      if (renderPasses != m_RenderPassKeys.end()) {
        const auto renderPass = renderPasses->second.find(createInfo.renderPass);
        if (renderPass != renderPasses->second.end()) {
          renderPassKey = renderPass->second;
        }
      }
    }
    VulkanStateDocument document = MakeGraphicsStateDocument(
        createInfo, stages, allShadersResolved, snapshot != nullptr, layoutKey, renderPassKey);
    const bool eligible = snapshot && allShadersResolved && document.ReplacementEligible;
    auto pipeline = std::make_unique<TrackedPipeline>(
        key, device, handle, createInfo.layout, createInfo.renderPass,
        rtsr::RTSR_PIPELINE_TYPE_GRAPHICS, document.Hash, PipelineName(document.Hash, "graphics"),
        std::move(snapshot), eligible, document.Diagnostic);
    Publish(std::move(pipeline), std::move(document.Json), std::move(stages));
  }
}

void PipelineTracker::HarvestCompute(vkCreateComputePipelinesCommand& command) {
  auto device = m_Contexts.Device(command.m_device.Key);
  if (!device || command.m_pCreateInfos.Value == nullptr || command.m_pPipelines.Value == nullptr) {
    return;
  }
  const uint32_t count = std::min({command.m_createInfoCount.Value, command.m_pCreateInfos.Size,
                                   command.m_pPipelines.Size,
                                   static_cast<uint32_t>(command.m_pPipelines.Keys.size())});
  for (uint32_t i = 0; i < count; ++i) {
    const GITSKey key = command.m_pPipelines.Keys[i];
    const VkPipeline handle = command.m_pPipelines.Value[i];
    if (key == 0 || handle == VK_NULL_HANDLE) {
      continue;
    }

    const auto& createInfo = command.m_pCreateInfos.Value[i];
    std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages;
    auto shader = m_Shaders.Resolve(command.m_device.Key, createInfo.stage);
    const bool allShadersResolved = shader.has_value();
    if (shader.has_value()) {
      stages.push_back(std::move(shader->Wire));
    }

    auto snapshot = PipelineStateSnapshot::Encode(createInfo);
    GITSKey layoutKey{};
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      const auto layouts = m_PipelineLayoutKeys.find(command.m_device.Key);
      if (layouts != m_PipelineLayoutKeys.end()) {
        const auto layout = layouts->second.find(createInfo.layout);
        if (layout != layouts->second.end()) {
          layoutKey = layout->second;
        }
      }
    }
    VulkanStateDocument document = MakeComputeStateDocument(createInfo, stages, allShadersResolved,
                                                            snapshot != nullptr, layoutKey);
    const bool eligible = snapshot && allShadersResolved && document.ReplacementEligible;
    auto pipeline = std::make_unique<TrackedPipeline>(
        key, device, handle, createInfo.layout, VK_NULL_HANDLE, rtsr::RTSR_PIPELINE_TYPE_COMPUTE,
        document.Hash, PipelineName(document.Hash, "compute"), std::move(snapshot), eligible,
        document.Diagnostic);
    Publish(std::move(pipeline), std::move(document.Json), std::move(stages));
  }
}

void PipelineTracker::OnSetName(GITSKey deviceKey, const VkDebugUtilsObjectNameInfoEXT* nameInfo) {
  if (nameInfo == nullptr || nameInfo->objectType != VK_OBJECT_TYPE_PIPELINE ||
      nameInfo->objectHandle == 0 || nameInfo->pObjectName == nullptr) {
    return;
  }
  GITSKey key{};
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto devicePipelines = m_PipelineKeys.find(deviceKey);
    if (devicePipelines == m_PipelineKeys.end()) {
      return;
    }
    const auto found =
        devicePipelines->second.find(HandleFromBits<VkPipeline>(nameInfo->objectHandle));
    if (found == devicePipelines->second.end()) {
      return;
    }
    key = found->second;
    m_Pipelines.at(key)->Name = nameInfo->pObjectName;
  }
  m_Client.SetPipelineName(key, nameInfo->pObjectName);
}

void PipelineTracker::DestroyReplacement(TrackedPipeline& pipeline) {
  if (!pipeline.Replacement) {
    return;
  }
  auto& retired = m_Retired[pipeline.Device->Key];
  retired.Device = pipeline.Device;
  retired.Pipelines.push_back(std::move(pipeline.Replacement));
  m_ActiveReplacements.fetch_sub(1, std::memory_order_relaxed);
  CollectRetiredLocked(pipeline.Device->Key);
}

void PipelineTracker::CollectRetiredLocked(GITSKey deviceKey) {
  const auto retired = m_Retired.find(deviceKey);
  if (retired == m_Retired.end()) {
    return;
  }
  const auto& device = retired->second.Device;
  DeviceOperation operation(device);
  if (!operation || device->Dispatch.vkDestroyPipeline == nullptr) {
    return;
  }
  auto& pipelines = retired->second.Pipelines;
  for (auto pipeline = pipelines.begin(); pipeline != pipelines.end();) {
    if (!(*pipeline)->UntrackedUse && pipeline->use_count() == 1) {
      device->Dispatch.vkDestroyPipeline(device->Handle, (*pipeline)->Handle, nullptr);
      pipeline = pipelines.erase(pipeline);
    } else {
      ++pipeline;
    }
  }
  if (pipelines.empty()) {
    m_Retired.erase(retired);
  }
}

size_t PipelineTracker::ReplacementCountLocked(GITSKey deviceKey) const {
  size_t count = 0;
  const auto retired = m_Retired.find(deviceKey);
  if (retired != m_Retired.end()) {
    count = retired->second.Pipelines.size();
  }
  for (const auto& entry : m_Pipelines) {
    if (entry.second->Device->Key == deviceKey && entry.second->Replacement) {
      ++count;
    }
  }
  return count;
}

void PipelineTracker::OnDestroy(GITSKey deviceKey, GITSKey key, VkPipeline pipeline) {
  bool removed = false;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (key == 0) {
      const auto devicePipelines = m_PipelineKeys.find(deviceKey);
      if (devicePipelines != m_PipelineKeys.end()) {
        const auto found = devicePipelines->second.find(pipeline);
        if (found != devicePipelines->second.end()) {
          key = found->second;
        }
      }
    }
    const auto found = m_Pipelines.find(key);
    if (found != m_Pipelines.end()) {
      DestroyReplacement(*found->second);
      m_PipelineKeys[found->second->Device->Key].erase(found->second->Original);
      m_Pipelines.erase(found);
      removed = true;
    }
  }
  if (removed) {
    m_Client.PipelineDeleted(key);
  }
}

void PipelineTracker::OnDestroyPipelineLayout(GITSKey deviceKey, VkPipelineLayout layout) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto layouts = m_PipelineLayoutKeys.find(deviceKey);
  if (layouts != m_PipelineLayoutKeys.end()) {
    layouts->second.erase(layout);
  }
  for (auto& entry : m_Pipelines) {
    auto& pipeline = entry.second;
    if (pipeline->Device->Key == deviceKey && pipeline->Layout == layout) {
      pipeline->ReplacementEligible = false;
      pipeline->Diagnostic = "the original pipeline layout was destroyed";
      m_Client.SetPipelineEligibility(pipeline->Key, pipeline->Incarnation, false,
                                      pipeline->Diagnostic);
    }
  }
}

void PipelineTracker::OnDestroyRenderPass(GITSKey deviceKey, VkRenderPass renderPass) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto renderPasses = m_RenderPassKeys.find(deviceKey);
  if (renderPasses != m_RenderPassKeys.end()) {
    renderPasses->second.erase(renderPass);
  }
  for (auto& entry : m_Pipelines) {
    auto& pipeline = entry.second;
    if (pipeline->Device->Key == deviceKey && pipeline->RenderPass == renderPass) {
      pipeline->ReplacementEligible = false;
      pipeline->Diagnostic = "the original render pass was destroyed";
      m_Client.SetPipelineEligibility(pipeline->Key, pipeline->Incarnation, false,
                                      pipeline->Diagnostic);
    }
  }
}

void PipelineTracker::OnDestroyDevice(GITSKey deviceKey) {
  std::shared_ptr<RtsrDeviceContext> device;
  std::vector<VkPipeline> destroy;
  std::vector<GITSKey> deleted;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto pipeline = m_Pipelines.begin(); pipeline != m_Pipelines.end();) {
      if (pipeline->second->Device->Key == deviceKey) {
        device = pipeline->second->Device;
        deleted.push_back(pipeline->first);
        if (pipeline->second->Replacement) {
          destroy.push_back(pipeline->second->Replacement->Handle);
          m_ActiveReplacements.fetch_sub(1, std::memory_order_relaxed);
        }
        m_PipelineKeys[deviceKey].erase(pipeline->second->Original);
        pipeline = m_Pipelines.erase(pipeline);
      } else {
        ++pipeline;
      }
    }
    const auto retired = m_Retired.find(deviceKey);
    if (retired != m_Retired.end()) {
      if (!device) {
        device = retired->second.Device;
      }
      for (const auto& pipeline : retired->second.Pipelines) {
        destroy.push_back(pipeline->Handle);
      }
      m_Retired.erase(retired);
    }
    for (auto buffer = m_CommandBuffers.begin(); buffer != m_CommandBuffers.end();) {
      if (buffer->second.DeviceKey == deviceKey) {
        buffer = m_CommandBuffers.erase(buffer);
      } else {
        ++buffer;
      }
    }
    m_PipelineLayoutKeys.erase(deviceKey);
    m_RenderPassKeys.erase(deviceKey);
    m_PipelineKeys.erase(deviceKey);
  }
  if (device && device->Dispatch.vkDestroyPipeline != nullptr) {
    for (const VkPipeline pipeline : destroy) {
      device->Dispatch.vkDestroyPipeline(device->Handle, pipeline, nullptr);
    }
  }
  for (const GITSKey key : deleted) {
    m_Client.PipelineDeleted(key);
  }
}

void PipelineTracker::OnAllocateCommandBuffers(vkAllocateCommandBuffersCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || command.m_pAllocateInfo.Value == nullptr ||
      command.m_pCommandBuffers.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint32_t count =
      std::min({command.m_pAllocateInfo.Value->commandBufferCount, command.m_pCommandBuffers.Size,
                static_cast<uint32_t>(command.m_pCommandBuffers.Keys.size())});
  for (uint32_t i = 0; i < count; ++i) {
    if (command.m_pCommandBuffers.Keys[i] != 0 &&
        command.m_pCommandBuffers.Value[i] != VK_NULL_HANDLE) {
      m_CommandBuffers[command.m_pCommandBuffers.Keys[i]] =
          CommandBuffer{command.m_device.Key, command.m_pAllocateInfo.Value->commandPool, {}};
    }
  }
  CollectRetiredLocked(command.m_device.Key);
}

void PipelineTracker::OnFreeCommandBuffers(vkFreeCommandBuffersCommand& command) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const size_t count =
      std::min<size_t>(command.m_commandBufferCount.Value, command.m_pCommandBuffers.Keys.size());
  for (size_t i = 0; i < count; ++i) {
    const auto buffer = m_CommandBuffers.find(command.m_pCommandBuffers.Keys[i]);
    if (buffer != m_CommandBuffers.end() && buffer->second.DeviceKey == command.m_device.Key) {
      m_CommandBuffers.erase(buffer);
    }
  }
  CollectRetiredLocked(command.m_device.Key);
}

void PipelineTracker::OnResetCommandBuffer(GITSKey key, VkResult result) {
  if (result != VK_SUCCESS) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto buffer = m_CommandBuffers.find(key);
  if (buffer != m_CommandBuffers.end()) {
    buffer->second.Replacements.clear();
    CollectRetiredLocked(buffer->second.DeviceKey);
  }
}

void PipelineTracker::OnResetCommandPool(GITSKey deviceKey, VkCommandPool pool, bool destroy) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (auto buffer = m_CommandBuffers.begin(); buffer != m_CommandBuffers.end();) {
    if (buffer->second.DeviceKey == deviceKey && buffer->second.Pool == pool) {
      buffer->second.Replacements.clear();
      if (destroy) {
        buffer = m_CommandBuffers.erase(buffer);
        continue;
      }
    }
    ++buffer;
  }
  CollectRetiredLocked(deviceKey);
}

VkPipeline PipelineTracker::OnBind(GITSKey key, VkPipeline pipeline, GITSKey commandBufferKey) {
  if (!HasReplacementWork()) {
    return pipeline;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto found = m_Pipelines.find(key);
  if (found == m_Pipelines.end() || !found->second->Replacement) {
    return pipeline;
  }
  const auto& replacement = found->second->Replacement;
  const auto buffer = m_CommandBuffers.find(commandBufferKey);
  if (buffer == m_CommandBuffers.end() || buffer->second.DeviceKey != found->second->Device->Key) {
    replacement->UntrackedUse = true;
  } else {
    auto& references = buffer->second.Replacements;
    if (std::find(references.begin(), references.end(), replacement) == references.end()) {
      references.push_back(replacement);
    }
  }
  return replacement->Handle;
}

bool PipelineTracker::IsTracked(GITSKey key) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Pipelines.find(key) != m_Pipelines.end();
}

PipelineTracker::ReplacementInputs PipelineTracker::SnapshotForReplacement(GITSKey key) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  ReplacementInputs inputs;
  const auto found = m_Pipelines.find(key);
  if (found == m_Pipelines.end()) {
    return inputs;
  }
  const auto& pipeline = *found->second;
  inputs.Key = key;
  inputs.Device = pipeline.Device;
  inputs.Type = pipeline.Type;
  inputs.StateHash = pipeline.StateHash;
  inputs.Incarnation = pipeline.Incarnation;
  inputs.Name = pipeline.Name;
  inputs.Snapshot = pipeline.Snapshot;
  inputs.Eligible = pipeline.ReplacementEligible;
  inputs.Diagnostic = pipeline.Diagnostic;
  return inputs;
}

bool PipelineTracker::InstallReplacement(GITSKey key,
                                         rtsr::RTSR_HASH stateHash,
                                         rtsr::RTSR_PIPELINE_INCARNATION incarnation,
                                         VkPipeline replacement) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto found = m_Pipelines.find(key);
  if (found == m_Pipelines.end() || found->second->StateHash != stateHash ||
      found->second->Incarnation != incarnation || !found->second->ReplacementEligible ||
      found->second->Device->Retiring.load()) {
    return false;
  }
  CollectRetiredLocked(found->second->Device->Key);
  if (ReplacementCountLocked(found->second->Device->Key) >= MAX_RETAINED_REPLACEMENTS_PER_DEVICE) {
    LOG_WARNING << "Rtsr - the safe Vulkan replacement retention limit was reached";
    return false;
  }
  auto installed = std::make_shared<ReplacementPipeline>();
  installed->Handle = replacement;
  if (found->second->Replacement) {
    auto& retired = m_Retired[found->second->Device->Key];
    retired.Device = found->second->Device;
    retired.Pipelines.push_back(std::move(found->second->Replacement));
  } else {
    m_ActiveReplacements.fetch_add(1, std::memory_order_relaxed);
  }
  found->second->Replacement = std::move(installed);
  CollectRetiredLocked(found->second->Device->Key);
  return true;
}

rtsr::RTSR_RESULT PipelineTracker::ApplyUpdates(
    const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request) {
  if (request.UpdatedPipelines.size() != 1) {
    LOG_WARNING << "Rtsr - replacement requests must contain exactly one pipeline";
    return rtsr::RTSR_RESULT_FAIL;
  }
  bool allSucceeded = true;
  for (const auto& update : request.UpdatedPipelines) {
    const GITSKey key = static_cast<GITSKey>(update.Id);
    ReplacementInputs inputs = SnapshotForReplacement(key);
    if (!inputs.Device || !inputs.Snapshot || !inputs.Eligible ||
        inputs.StateHash != update.StateHash || inputs.Incarnation != update.Incarnation) {
      LOG_WARNING << "Rtsr - cannot replace Vulkan pipeline " << update.Id << ": "
                  << (inputs.Diagnostic.empty() ? "state is unavailable or changed"
                                                : inputs.Diagnostic);
      allSucceeded = false;
      continue;
    }

    DeviceOperation operation(inputs.Device);
    if (!operation) {
      allSucceeded = false;
      continue;
    }
    std::shared_lock<std::shared_mutex> replacementLock(inputs.Device->ReplacementMutex);
    inputs = SnapshotForReplacement(key);
    if (!inputs.Device || !inputs.Snapshot || !inputs.Eligible ||
        inputs.StateHash != update.StateHash || inputs.Incarnation != update.Incarnation) {
      allSucceeded = false;
      continue;
    }

    std::map<rtsr::RTSR_SHADER_STAGE, uint32_t> references;
    bool referencesValid = true;
    for (const uint32_t reference : update.ShaderReferences) {
      if (reference >= request.ShaderStages.size()) {
        referencesValid = false;
        break;
      }
      const auto& shader = request.ShaderStages[reference];
      if (shader.BytecodeType != rtsr::RTSR_BYTECODE_TYPE_SPIRV || shader.EntrypointName.empty() ||
          !references.emplace(shader.ShaderStage, reference).second) {
        referencesValid = false;
        break;
      }
    }
    if (!referencesValid || references.empty()) {
      allSucceeded = false;
      continue;
    }

    TemporaryShaderModules modules(inputs.Device);
    std::unordered_map<uint32_t, VkShaderModule> createdModules;
    for (const auto& [stage, reference] : references) {
      VkShaderModule module = VK_NULL_HANDLE;
      if (!modules.Create(request.ShaderStages[reference], module)) {
        referencesValid = false;
        break;
      }
      createdModules[reference] = module;
    }
    if (!referencesValid) {
      allSucceeded = false;
      continue;
    }

    auto workingState = inputs.Snapshot->Clone();
    if (!workingState) {
      allSucceeded = false;
      continue;
    }
    VkPipeline replacement = VK_NULL_HANDLE;
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    if (inputs.Type == rtsr::RTSR_PIPELINE_TYPE_GRAPHICS) {
      VkGraphicsPipelineCreateInfo* createInfo = workingState->MutableGraphics();
      if (createInfo == nullptr || createInfo->pStages == nullptr ||
          references.size() != createInfo->stageCount) {
        allSucceeded = false;
        continue;
      }
      createInfo->flags &= ~(VK_PIPELINE_CREATE_DERIVATIVE_BIT |
                             VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT);
      createInfo->basePipelineHandle = VK_NULL_HANDLE;
      createInfo->basePipelineIndex = -1;
      bool patchedAll = true;
      for (uint32_t i = 0; i < createInfo->stageCount; ++i) {
        auto& createStage = const_cast<VkPipelineShaderStageCreateInfo&>(createInfo->pStages[i]);
        const auto stage = ShaderModuleTracker::ToRtsrStage(createStage.stage);
        const auto reference = references.find(stage);
        if (reference == references.end()) {
          patchedAll = false;
          break;
        }
        const auto& wire = request.ShaderStages[reference->second];
        createStage.module = createdModules.at(reference->second);
        createStage.pName = wire.EntrypointName.c_str();
        StripShaderModulePNext(createStage);
      }
      if (!patchedAll || inputs.Device->Dispatch.vkCreateGraphicsPipelines == nullptr) {
        allSucceeded = false;
        continue;
      }
      result = inputs.Device->Dispatch.vkCreateGraphicsPipelines(
          inputs.Device->Handle, VK_NULL_HANDLE, 1, createInfo, nullptr, &replacement);
    } else if (inputs.Type == rtsr::RTSR_PIPELINE_TYPE_COMPUTE) {
      VkComputePipelineCreateInfo* createInfo = workingState->MutableCompute();
      if (createInfo == nullptr) {
        allSucceeded = false;
        continue;
      }
      createInfo->flags &= ~(VK_PIPELINE_CREATE_DERIVATIVE_BIT |
                             VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT);
      createInfo->basePipelineHandle = VK_NULL_HANDLE;
      createInfo->basePipelineIndex = -1;
      const auto stage = ShaderModuleTracker::ToRtsrStage(createInfo->stage.stage);
      const auto reference = references.find(stage);
      if (references.size() != 1 || reference == references.end() ||
          inputs.Device->Dispatch.vkCreateComputePipelines == nullptr) {
        allSucceeded = false;
        continue;
      }
      const auto& wire = request.ShaderStages[reference->second];
      createInfo->stage.module = createdModules.at(reference->second);
      createInfo->stage.pName = wire.EntrypointName.c_str();
      StripShaderModulePNext(createInfo->stage);
      result = inputs.Device->Dispatch.vkCreateComputePipelines(
          inputs.Device->Handle, VK_NULL_HANDLE, 1, createInfo, nullptr, &replacement);
    }

    if (result != VK_SUCCESS || replacement == VK_NULL_HANDLE) {
      if (replacement != VK_NULL_HANDLE) {
        inputs.Device->Dispatch.vkDestroyPipeline(inputs.Device->Handle, replacement, nullptr);
      }
      LOG_WARNING << "Rtsr - creating Vulkan replacement " << update.Id << " failed with VkResult "
                  << result;
      allSucceeded = false;
      continue;
    }
    if (inputs.Device->Dispatch.vkSetDebugUtilsObjectNameEXT != nullptr && !inputs.Name.empty()) {
      const std::string name = inputs.Name + "-updated";
      VkDebugUtilsObjectNameInfoEXT nameInfo{};
      nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
      nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
      nameInfo.objectHandle = HandleBits(replacement);
      nameInfo.pObjectName = name.c_str();
      inputs.Device->Dispatch.vkSetDebugUtilsObjectNameEXT(inputs.Device->Handle, &nameInfo);
    }
    if (!InstallReplacement(key, update.StateHash, update.Incarnation, replacement)) {
      inputs.Device->Dispatch.vkDestroyPipeline(inputs.Device->Handle, replacement, nullptr);
      allSucceeded = false;
    }
  }
  return allSucceeded ? rtsr::RTSR_RESULT_SUCCESS : rtsr::RTSR_RESULT_FAIL;
}

rtsr::RTSR_RESULT PipelineTracker::ResetPipelines(
    const rtsr::RTSR_RESET_PIPELINES_REQUEST& request) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  bool allMatched = true;
  for (const auto& reset : request.Pipelines) {
    const auto found = m_Pipelines.find(static_cast<GITSKey>(reset.Id));
    if (found == m_Pipelines.end() || found->second->Incarnation != reset.Incarnation) {
      allMatched = false;
      continue;
    }
    DestroyReplacement(*found->second);
  }
  return allMatched ? rtsr::RTSR_RESULT_SUCCESS : rtsr::RTSR_RESULT_FAIL;
}

} // namespace gits::vulkan
