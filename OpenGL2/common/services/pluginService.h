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

// Loads the Common.Player.Plugins or Common.Recorder.Plugins plugins from Plugins/OpenGL2 in
// the GITS installation. `getRealFunction` resolves the real driver functions that plugins call
// directly; the recorder and the player each have their own.
class PluginService {
public:
  explicit PluginService(void* (*getRealFunction)(const char* name))
      : m_GetRealFunction(getRealFunction) {}
  ~PluginService();
  PluginService(const PluginService&) = delete;
  PluginService& operator=(const PluginService&) = delete;

  void LoadPlugins();
  const std::vector<PluginInfo>& GetPlugins() const {
    return m_Plugins;
  }

private:
  void* (*m_GetRealFunction)(const char* name);
  std::vector<PluginInfo> m_Plugins;
};

} // namespace gits::opengl2
