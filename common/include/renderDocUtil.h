// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS

#include "config.h"
#include "gits.h"
#include "vulkan_basic.h"

#include <Windows.h>
#include "renderdoc_app.h"

#include <string>

namespace gits {
class RenderDocUtil : gits::noncopyable {
private:
  RENDERDOC_API_1_0_0* rdoc;
  HMODULE renderDocLibrary;
  static RenderDocUtil* instance;
  RENDERDOC_DevicePointer pRenderDocDevice;
  bool isValid;
  void SetOutputName();
  RenderDocUtil();

public:
  static std::string dllpath;
  static RenderDocUtil& GetInstance();
  ~RenderDocUtil();
  void StartRecording();
  void StopRecording();
  void LaunchRenderDocUI();
  void SetRenderDocDevice(VkInstance vkInstance);
};
} // namespace gits
#endif // GITS_PLATFORM_WINDOWS
