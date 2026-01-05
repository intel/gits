// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <vector>
#include <stdexcept>
#include <cstdint>

#include "enumsAuto.h"
#include "bit_range.h"

namespace gits {
struct MemorySizeRequirementOverride {
  uint32_t fixedAmount;
  uint32_t percent;
};

struct ObjectRange {
  std::vector<uint32_t> objVector;
  BitRange range;
  bool empty() const;
};

struct VulkanObjectRange : ObjectRange {
  VulkanObjectMode objMode = VulkanObjectMode::MODE_VK_NONE;

  bool operator[](uint64_t queueSubmitNumber) const;
  void SetFromString(const std::string& str);
};
} // namespace gits
