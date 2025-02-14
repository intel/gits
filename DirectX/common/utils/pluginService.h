// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "IPlugin.h"
#include "tools_lite.h"

#include <wrl/client.h>
#include <vector>
#include <filesystem>

namespace gits {
namespace DirectX {

struct PluginInfo {
  std::filesystem::path dllPath = {};
  HMODULE dll = 0;
  IPlugin* impl = nullptr;
  std::vector<HMODULE> dependencies = {};
  DestroyPluginPtr destroyPlugin = nullptr;

  void free();
};

class PluginService : public gits::noncopyable {
public:
  PluginService() = default;
  ~PluginService();

  void loadPlugins();
  const std::vector<PluginInfo>& getPlugins();

private:
  std::vector<PluginInfo> plugins_;
};

} // namespace DirectX
} // namespace gits
