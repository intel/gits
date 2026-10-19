// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gits::vulkan {

class ShaderModuleTracker {
public:
  struct ResolvedShader {
    rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO Wire;
  };

  void OnCreate(GITSKey deviceKey,
                GITSKey moduleKey,
                VkShaderModule module,
                const VkShaderModuleCreateInfo* createInfo,
                VkResult result);
  void OnDestroy(GITSKey deviceKey, GITSKey moduleKey, VkShaderModule module);
  void OnDestroyDevice(GITSKey deviceKey);
  void OnIdentifier(GITSKey moduleKey, const VkShaderModuleIdentifierEXT* identifier);
  void OnCreateInfoIdentifier(GITSKey deviceKey,
                              const VkShaderModuleCreateInfo* createInfo,
                              const VkShaderModuleIdentifierEXT* identifier);

  std::optional<ResolvedShader> Resolve(GITSKey deviceKey,
                                        const VkPipelineShaderStageCreateInfo& stage) const;
  static rtsr::RTSR_SHADER_STAGE ToRtsrStage(VkShaderStageFlagBits stage);

private:
  struct Module {
    GITSKey DeviceKey{};
    VkShaderModule Handle{VK_NULL_HANDLE};
    std::vector<uint8_t> Bytecode;
  };

  struct Identifier {
    GITSKey DeviceKey{};
    uint32_t Size{};
    std::array<uint8_t, VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT> Data{};

    bool operator==(const Identifier& other) const {
      return DeviceKey == other.DeviceKey && Size == other.Size && Data == other.Data;
    }
  };

  struct IdentifierHash {
    size_t operator()(const Identifier& identifier) const;
  };

  static std::optional<std::vector<uint8_t>> CopyBytecode(
      const VkShaderModuleCreateInfo* createInfo);
  static std::optional<Identifier> MakeIdentifier(GITSKey deviceKey,
                                                  const VkShaderModuleIdentifierEXT* identifier);
  static std::optional<Identifier> MakeIdentifier(
      GITSKey deviceKey, const VkPipelineShaderStageModuleIdentifierCreateInfoEXT* identifier);
  mutable std::mutex m_Mutex;
  std::unordered_map<GITSKey, Module> m_Modules;
  std::unordered_map<GITSKey, std::unordered_map<VkShaderModule, GITSKey>> m_ModuleKeys;
  std::unordered_map<Identifier, std::vector<uint8_t>, IdentifierHash> m_Identifiers;
};

} // namespace gits::vulkan
