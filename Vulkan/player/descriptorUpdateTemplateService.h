// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "arguments.h"

#include <vector>
#include <unordered_map>
#include <mutex>

namespace gits {
namespace vulkan {

// Stores VkDescriptorUpdateTemplateEntry lists for each live template handle
// so that handles embedded in the raw pData buffer can be remapped from
// recorder-side values to player-side values before the Vulkan API call.
class DescriptorUpdateTemplateService {
public:
  void StoreTemplate(VkDescriptorUpdateTemplate tmpl,
                     const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo);
  void RemoveTemplate(VkDescriptorUpdateTemplate tmpl);

  // Patches Vulkan handles embedded in arg.Data into arg.PatchedData and
  // updates arg.Value to point there.
  void RemapHandles(VkDescriptorUpdateTemplate tmpl, DescriptorTemplateDataArgument& arg);

private:
  struct TemplateInfo {
    std::vector<VkDescriptorUpdateTemplateEntry> entries;
  };

  std::unordered_map<VkDescriptorUpdateTemplate, TemplateInfo> m_Templates;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
