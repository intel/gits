// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "library.h"

namespace gits {
namespace vulkan {

class VulkanLibrary2 : public gits::CLibrary {
public:
  static VulkanLibrary2& Get();
  VulkanLibrary2(gits::CLibrary::state_creator_t stc = gits::CLibrary::state_creator_t())
      : gits::CLibrary(ID_VULKAN2, std::move(stc)) {}
  ~VulkanLibrary2() {}

  gits::CFunction* FunctionCreate(unsigned type) const override;

  const char* Name() const override {
    return "Vulkan2";
  }
};

} // namespace vulkan
} // namespace gits
