// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "shaderModuleTracker.h"

#include "rtsr/shared/rtsrCommon.h"

#include <algorithm>
#include <cstring>

namespace gits::vulkan {
namespace {

template <typename T>
const T* FindInChain(const void* chain, VkStructureType type) {
  auto* current = static_cast<const VkBaseInStructure*>(chain);
  while (current != nullptr) {
    if (current->sType == type) {
      return reinterpret_cast<const T*>(current);
    }
    current = current->pNext;
  }
  return nullptr;
}

} // namespace

size_t ShaderModuleTracker::IdentifierHash::operator()(const Identifier& identifier) const {
  const size_t bytecode =
      static_cast<size_t>(rtsr::ComputeBytecodeHash(identifier.Data.data(), identifier.Size));
  return bytecode ^ (std::hash<GITSKey>{}(identifier.DeviceKey) + 0x9e3779b9u + (bytecode << 6u) +
                     (bytecode >> 2u));
}

std::optional<std::vector<uint8_t>> ShaderModuleTracker::CopyBytecode(
    const VkShaderModuleCreateInfo* createInfo) {
  if (createInfo == nullptr || createInfo->pCode == nullptr || createInfo->codeSize < 20 ||
      createInfo->codeSize % sizeof(uint32_t) != 0 || createInfo->pCode[0] != 0x07230203u) {
    return std::nullopt;
  }
  const auto* bytes = reinterpret_cast<const uint8_t*>(createInfo->pCode);
  return std::vector<uint8_t>(bytes, bytes + createInfo->codeSize);
}

std::optional<ShaderModuleTracker::Identifier> ShaderModuleTracker::MakeIdentifier(
    GITSKey deviceKey, const VkShaderModuleIdentifierEXT* identifier) {
  if (identifier == nullptr || identifier->identifierSize == 0 ||
      identifier->identifierSize > VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT) {
    return std::nullopt;
  }
  Identifier result;
  result.DeviceKey = deviceKey;
  result.Size = identifier->identifierSize;
  std::copy_n(identifier->identifier, result.Size, result.Data.begin());
  return result;
}

std::optional<ShaderModuleTracker::Identifier> ShaderModuleTracker::MakeIdentifier(
    GITSKey deviceKey, const VkPipelineShaderStageModuleIdentifierCreateInfoEXT* identifier) {
  if (identifier == nullptr || identifier->identifierSize == 0 ||
      identifier->identifierSize > VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT ||
      identifier->pIdentifier == nullptr) {
    return std::nullopt;
  }
  Identifier result;
  result.DeviceKey = deviceKey;
  result.Size = identifier->identifierSize;
  std::copy_n(identifier->pIdentifier, result.Size, result.Data.begin());
  return result;
}

rtsr::RTSR_SHADER_STAGE ShaderModuleTracker::ToRtsrStage(VkShaderStageFlagBits stage) {
  switch (stage) {
  case VK_SHADER_STAGE_VERTEX_BIT:
    return rtsr::RTSR_SHADER_STAGE_VERTEX;
  case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
    return rtsr::RTSR_SHADER_STAGE_TESSELLATION_CONTROL;
  case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
    return rtsr::RTSR_SHADER_STAGE_TESSELLATION_EVALUATION;
  case VK_SHADER_STAGE_GEOMETRY_BIT:
    return rtsr::RTSR_SHADER_STAGE_GEOMETRY;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    return rtsr::RTSR_SHADER_STAGE_FRAGMENT;
  case VK_SHADER_STAGE_COMPUTE_BIT:
    return rtsr::RTSR_SHADER_STAGE_COMPUTE;
  case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
    return rtsr::RTSR_SHADER_STAGE_RAY_TRACING_RAYGEN;
  case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
    return rtsr::RTSR_SHADER_STAGE_RAY_TRACING_ANY_HIT;
  case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
    return rtsr::RTSR_SHADER_STAGE_RAY_TRACING_CLOSEST_HIT;
  case VK_SHADER_STAGE_MISS_BIT_KHR:
    return rtsr::RTSR_SHADER_STAGE_RAY_TRACING_MISS;
  case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
    return rtsr::RTSR_SHADER_STAGE_RAY_TRACING_INTERSECTION;
  default:
    return rtsr::RTSR_SHADER_STAGE_UNKNOWN;
  }
}

void ShaderModuleTracker::OnCreate(GITSKey deviceKey,
                                   GITSKey moduleKey,
                                   VkShaderModule module,
                                   const VkShaderModuleCreateInfo* createInfo,
                                   VkResult result) {
  const auto bytecode = CopyBytecode(createInfo);
  if (result != VK_SUCCESS || moduleKey == 0 || module == VK_NULL_HANDLE || !bytecode.has_value()) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto old = m_Modules.find(moduleKey);
  if (old != m_Modules.end()) {
    m_ModuleKeys[old->second.DeviceKey].erase(old->second.Handle);
  }
  m_Modules[moduleKey] = {deviceKey, module, std::move(*bytecode)};
  m_ModuleKeys[deviceKey][module] = moduleKey;
}

void ShaderModuleTracker::OnDestroy(GITSKey deviceKey, GITSKey moduleKey, VkShaderModule module) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (moduleKey == 0) {
    const auto deviceModules = m_ModuleKeys.find(deviceKey);
    if (deviceModules == m_ModuleKeys.end()) {
      return;
    }
    const auto key = deviceModules->second.find(module);
    if (key == deviceModules->second.end()) {
      return;
    }
    moduleKey = key->second;
  }
  const auto found = m_Modules.find(moduleKey);
  if (found != m_Modules.end()) {
    m_ModuleKeys[found->second.DeviceKey].erase(found->second.Handle);
    m_Modules.erase(found);
  }
}

void ShaderModuleTracker::OnDestroyDevice(GITSKey deviceKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (auto module = m_Modules.begin(); module != m_Modules.end();) {
    if (module->second.DeviceKey == deviceKey) {
      module = m_Modules.erase(module);
    } else {
      ++module;
    }
  }
  m_ModuleKeys.erase(deviceKey);
  for (auto identifier = m_Identifiers.begin(); identifier != m_Identifiers.end();) {
    if (identifier->first.DeviceKey == deviceKey) {
      identifier = m_Identifiers.erase(identifier);
    } else {
      ++identifier;
    }
  }
}

void ShaderModuleTracker::OnIdentifier(GITSKey moduleKey,
                                       const VkShaderModuleIdentifierEXT* identifier) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto module = m_Modules.find(moduleKey);
  if (module != m_Modules.end()) {
    const auto key = MakeIdentifier(module->second.DeviceKey, identifier);
    if (!key.has_value()) {
      return;
    }
    m_Identifiers[*key] = module->second.Bytecode;
  }
}

void ShaderModuleTracker::OnCreateInfoIdentifier(GITSKey deviceKey,
                                                 const VkShaderModuleCreateInfo* createInfo,
                                                 const VkShaderModuleIdentifierEXT* identifier) {
  const auto key = MakeIdentifier(deviceKey, identifier);
  const auto bytecode = CopyBytecode(createInfo);
  if (!key.has_value() || !bytecode.has_value()) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Identifiers[*key] = std::move(*bytecode);
}

std::optional<ShaderModuleTracker::ResolvedShader> ShaderModuleTracker::Resolve(
    GITSKey deviceKey, const VkPipelineShaderStageCreateInfo& stage) const {
  const rtsr::RTSR_SHADER_STAGE rtsrStage = ToRtsrStage(stage.stage);
  if (rtsrStage == rtsr::RTSR_SHADER_STAGE_UNKNOWN) {
    return std::nullopt;
  }

  std::vector<uint8_t> bytecode;
  if (stage.module != VK_NULL_HANDLE) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto deviceModules = m_ModuleKeys.find(deviceKey);
    if (deviceModules == m_ModuleKeys.end()) {
      return std::nullopt;
    }
    const auto key = deviceModules->second.find(stage.module);
    if (key == deviceModules->second.end()) {
      return std::nullopt;
    }
    const auto module = m_Modules.find(key->second);
    if (module == m_Modules.end() || module->second.DeviceKey != deviceKey) {
      return std::nullopt;
    }
    bytecode = module->second.Bytecode;
  } else if (const auto* inlineModule = FindInChain<VkShaderModuleCreateInfo>(
                 stage.pNext, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)) {
    const auto copied = CopyBytecode(inlineModule);
    if (!copied.has_value()) {
      return std::nullopt;
    }
    bytecode = std::move(*copied);
  } else {
    const auto* moduleIdentifier = FindInChain<VkPipelineShaderStageModuleIdentifierCreateInfoEXT>(
        stage.pNext, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT);
    const auto identifier = MakeIdentifier(deviceKey, moduleIdentifier);
    if (!identifier.has_value()) {
      return std::nullopt;
    }
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto found = m_Identifiers.find(*identifier);
    if (found == m_Identifiers.end()) {
      return std::nullopt;
    }
    bytecode = found->second;
  }

  ResolvedShader result;
  result.Wire.ShaderStage = rtsrStage;
  result.Wire.Bytecode = std::move(bytecode);
  result.Wire.BytecodeHash =
      rtsr::ComputeBytecodeHash(result.Wire.Bytecode.data(), result.Wire.Bytecode.size());
  result.Wire.EntrypointName =
      stage.pName != nullptr ? stage.pName : rtsr::RTSR_DEFAULT_ENTRYPOINT_NAME;
  result.Wire.BytecodeType = rtsr::RTSR_BYTECODE_TYPE_SPIRV;
  result.Wire.D3DShaderVersion = 0;
  return result;
}

} // namespace gits::vulkan
