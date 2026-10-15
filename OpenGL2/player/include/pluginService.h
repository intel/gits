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

namespace gits::opengl2 {

struct PluginInfo {
  std::filesystem::path DllPath;
  void* Dll{};
  IPlugin* Impl{};
  std::vector<void*> Dependencies;
  DestroyPluginPtr DestroyPlugin{};

  void Free();
};

// Loads the Common.Player.Plugins plugins from Plugins/OpenGL2 next to the player.
class PluginService {
public:
  PluginService() = default;
  ~PluginService();
  PluginService(const PluginService&) = delete;
  PluginService& operator=(const PluginService&) = delete;

  void LoadPlugins();
  const std::vector<PluginInfo>& GetPlugins() const {
    return m_Plugins;
  }

private:
  std::vector<PluginInfo> m_Plugins;
};

} // namespace gits::opengl2
