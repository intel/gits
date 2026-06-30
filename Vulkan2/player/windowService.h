// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader.h"

#include <unordered_map>

namespace gits {
namespace vulkan {

class WindowService {
public:
  uint64_t SetWindow(uint64_t handle,
                     uint64_t instance,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height,
                     bool visible);
  uint64_t GetCurrentWindowHandle(uint64_t captureWindow);
  uint64_t GetCurrentInstance(uint64_t captureInstance);

private:
  std::unordered_map<uint64_t, uint64_t> m_WindowMap;
  std::unordered_map<uint64_t, uint64_t> m_InstanceMap;
};

} // namespace vulkan
} // namespace gits
