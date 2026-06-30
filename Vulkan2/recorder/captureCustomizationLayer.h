// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "gitsRecorder.h"

#include <unordered_map>

namespace gits {
namespace vulkan {

class CaptureManager;

class CaptureCustomizationLayer : public Layer {
public:
  CaptureCustomizationLayer(CaptureManager& manager, GitsRecorder& recorder)
      : Layer("CaptureCustomization"), m_Manager(manager), m_Recorder(recorder) {}
#ifdef VK_USE_PLATFORM_WIN32_KHR
  void Pre(vkCreateWin32SurfaceKHRCommand& command) override;
#endif

  void Post(vkAllocateMemoryCommand& command) override;
  void Post(vkMapMemoryCommand& command) override;
  void Pre(vkUnmapMemoryCommand& command) override;
  void Pre(vkQueueSubmitCommand& command) override;

private:
  CaptureManager& m_Manager;
  GitsRecorder& m_Recorder;
};

} // namespace vulkan
} // namespace gits
