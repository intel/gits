// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pNextChainCopy.h"
#include "argumentCoders.h"
#include "log.h"
#include "vulkanHelpers.h"

#include <cstring>
#include <string>

namespace gits {
namespace vulkan {

const void* PNextChainCopy::Copy(const void* pNext, VkStructureType sType) {
  m_Nodes.clear();
  if (!getPNextStructure(pNext, sType)) {
    return pNext;
  }

  const void* head = nullptr;
  const void** link = &head;
  for (auto* node = static_cast<const VkBaseInStructure*>(pNext); node != nullptr;
       node = node->pNext) {
    const size_t size = GetPNextStructureSize(node->sType);
    if (size == 0) {
      GITS_ASSERT(size != 0, "pNext structure " +
                                 std::to_string(static_cast<uint32_t>(node->sType)) +
                                 " is unknown to GITS and precedes structure " +
                                 std::to_string(static_cast<uint32_t>(sType)) +
                                 ", so GITS cannot modify the latter as recording requires. "
                                 "Record with a GITS build that supports the structure.");
      return head;
    }
    auto& storage = m_Nodes.emplace_back((size + sizeof(uint64_t) - 1) / sizeof(uint64_t));
    std::memcpy(storage.data(), node, size);
    *link = storage.data();
    if (node->sType == sType) {
      break;
    }
    link = reinterpret_cast<const void**>(
        &reinterpret_cast<VkBaseInStructure*>(storage.data())->pNext);
  }
  return head;
}

} // namespace vulkan
} // namespace gits
