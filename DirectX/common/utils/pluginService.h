// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "IPlugin.h"

#include <wrl/client.h>
#include <vector>
#include <filesystem>

namespace gits {
namespace DirectX {

struct PluginInfo {
  std::filesystem::path DllPath = {};
  HMODULE Dll = 0;
  IPlugin* Impl = nullptr;
  std::vector<HMODULE> Dependencies = {};
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

} // namespace DirectX
} // namespace gits
