// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "handleMapService.h"
#include "multithreadedObjectCreationService.h"

#include <stdexcept>
#include <string>
#include <type_traits>

namespace gits {
namespace vulkan {

class HandleResolver {
public:
  explicit HandleResolver(MultithreadedObjectCreationService* service) : m_Service(service) {}

  template <typename HandleT>
  uint64_t Resolve(GITSKey key) const {
    if (!key) {
      return 0;
    }
    if constexpr (std::is_same_v<HandleT, VkPipeline>) {
      if (m_Service) {
        auto result = m_Service->Complete(key);
        if (result && !HandleMapService::Get().TryGetHandle(key)) {
          throw std::runtime_error("Pipeline creation produced no handle for key " +
                                   std::to_string(key) + ": VkResult " +
                                   std::to_string(static_cast<int>(*result)));
        }
      }
    }
    return HandleMapService::Get().GetHandle(key);
  }

  uint64_t ResolveObject(GITSKey key, VkObjectType type) const {
    return type == VK_OBJECT_TYPE_PIPELINE ? Resolve<VkPipeline>(key) : Resolve<uint64_t>(key);
  }

  uint64_t ResolveObject(GITSKey key, VkDebugReportObjectTypeEXT type) const {
    return type == VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT ? Resolve<VkPipeline>(key)
                                                            : Resolve<uint64_t>(key);
  }

private:
  MultithreadedObjectCreationService* m_Service;
};

} // namespace vulkan
} // namespace gits
