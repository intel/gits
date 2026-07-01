// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "IPlugin.h"

#include <filesystem>
#include <vector>

namespace gits {
namespace vulkan {

struct PluginInfo {
  std::filesystem::path DllPath = {};
  void* Dll = nullptr;
  IPlugin* Impl = nullptr;
  std::vector<void*> Dependencies = {};
  DestroyPluginPtr DestroyPlugin = nullptr;

  void Free();
};

class PluginService {
public:
  PluginService() = default;
  ~PluginService();
  PluginService(const PluginService&) = delete;
  PluginService& operator=(const PluginService&) = delete;

  void LoadPlugins();
  const std::vector<PluginInfo>& GetPlugins() const;

private:
  std::vector<PluginInfo> m_Plugins;
};

} // namespace vulkan
} // namespace gits
