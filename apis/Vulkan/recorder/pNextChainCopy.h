// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

// Owns a copy of the beginning of a pNext chain, so a structure in it can be modified without
// writing to application memory.
class PNextChainCopy {
public:
  // Copies the chain up to and including the first structure of the given type and returns the
  // head of the copy. The copied structures keep their original sTypes and their pNext links,
  // so the chain continues into the application's structures after the copied one.
  // When the chain has no structure of the given type, nothing is copied and the original chain
  // is returned. A structure unknown to GITS that precedes the requested one cannot be copied,
  // which fails the recording.
  const void* Copy(const void* pNext, VkStructureType sType);

private:
  std::vector<std::vector<uint64_t>> m_Nodes;
};

} // namespace vulkan
} // namespace gits
