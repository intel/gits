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

class HelloPlugin : public IPlugin {
public:
  HelloPlugin(CGits& gits, const char* pluginPath)
      : IPlugin(gits, pluginPath), gits_(gits), pluginPath_(pluginPath) {}

  ~HelloPlugin() = default;

  const char* getName() override {
    return "HelloPlugin";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      auto cfgPath = pluginPath_.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      HelloPluginConfig cfg = {};
      cfg.printFrames = cfgYaml["Config"]["PrintFrames"].as<bool>();
      cfg.printGPUSubmissions = cfgYaml["Config"]["PrintGPUSubmissions"].as<bool>();

      pluginLayer_ = std::make_unique<HelloPluginLayer>(gits_, cfg);
    }
    return pluginLayer_.get();
  }

private:
  CGits& gits_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<HelloPluginLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::HelloPlugin> g_helloPlugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(gits::CGits& gits, const char* pluginPath) {
  if (!g_helloPlugin) {
    g_helloPlugin = std::make_unique<gits::DirectX::HelloPlugin>(gits, pluginPath);
  }
  return g_helloPlugin.get();
}
