// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "IPlugin.h"
#include "layer.h"

#include "yaml-cpp/yaml.h"
#include <filesystem>

namespace gits {
namespace DirectX {

class RtasSizeCheckPlugin final : public IPlugin {
public:
  RtasSizeCheckPlugin(CGits& gits, const char* pluginPath)
      : IPlugin(gits, pluginPath), gits_(gits), pluginPath_(pluginPath) {}

  ~RtasSizeCheckPlugin() = default;

  const char* getName() override {
    return "RtasSizeCheck";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      auto cfgPath = pluginPath_.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      pluginLayer_ = std::make_unique<RtasSizeCheckLayer>(gits_);
    }
    return pluginLayer_.get();
  }

private:
  CGits& gits_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<RtasSizeCheckLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::RtasSizeCheckPlugin> g_RtasSizeCheckPlugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(gits::CGits& gits, const char* pluginPath) {
  if (!g_RtasSizeCheckPlugin) {
    g_RtasSizeCheckPlugin = std::make_unique<gits::DirectX::RtasSizeCheckPlugin>(gits, pluginPath);
  }
  return g_RtasSizeCheckPlugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_RtasSizeCheckPlugin.reset();
}
