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

class DescriptorUpdateTemplateService {
public:
  void StoreTemplate(VkDescriptorUpdateTemplate tmpl,
                     const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo);
  void RemoveTemplate(VkDescriptorUpdateTemplate tmpl);
  void SerializeData(VkDescriptorUpdateTemplate tmpl,
                     const void* pData,
                     DescriptorTemplateDataArgument& arg);

private:
  struct TemplateInfo {
    std::vector<VkDescriptorUpdateTemplateEntry> entries;
  };

  std::unordered_map<VkDescriptorUpdateTemplate, TemplateInfo> m_Templates;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
