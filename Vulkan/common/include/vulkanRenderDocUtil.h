// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS

#include "gits.h"
#include "vulkan_basic.h"

#include <Windows.h>
#include "renderdoc_app.h"

#include <string>
#include <unordered_map>

namespace gits::Vulkan {
class RenderDocUtil : gits::noncopyable {
private:
  class RenderDocCapturer {
  private:
    static uint32_t ID;
    uint32_t capturerID;
    VkInstance vkInstance;
    RENDERDOC_DevicePointer device;
    bool isRecording;
    void SetCapturePath();

  public:
    RenderDocCapturer(VkInstance instance);
    void Start();
    void Stop();
    void LaunchUI();
  };

  static RENDERDOC_API_1_0_0* rdoc;
  HMODULE renderDocLibrary;
  std::unordered_map<VkInstance, RenderDocCapturer> rdocCapturers;
  RenderDocUtil();

public:
  static std::string dllpath;
  static RenderDocUtil& GetInstance();
  RenderDocCapturer& GetCapturer(VkInstance instance);
  void AddCapturer(VkInstance instance);
  void DeleteCapturer(VkInstance instance);
  void StartRecording();
  void StopRecording();
  void LaunchRenderDocUI();
  ~RenderDocUtil();
};
} // namespace gits::Vulkan
#endif // GITS_PLATFORM_WINDOWS
