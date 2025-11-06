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

class AdapterSpoofPlugin : public IPlugin {
public:
  AdapterSpoofPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), context_(context), pluginPath_(pluginPath) {}

  ~AdapterSpoofPlugin() = default;

  const char* getName() override {
    return "AdapterSpoof";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      auto cfgPath = pluginPath_.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      AdapterSpoofConfig cfg = {};
      cfg.description = cfgYaml["Config"]["Description"].as<std::string>();
      cfg.vendorId = cfgYaml["Config"]["VendorId"].as<unsigned>();
      cfg.deviceId = cfgYaml["Config"]["DeviceId"].as<unsigned>();

      pluginLayer_ = std::make_unique<AdapterSpoofLayer>(*context_.gits, cfg);
    }
    return pluginLayer_.get();
  }

private:
  IPluginContext context_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<AdapterSpoofLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::AdapterSpoofPlugin> g_plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  if (!g_plugin) {
    g_plugin = std::make_unique<gits::DirectX::AdapterSpoofPlugin>(context, pluginPath);
  }
  return g_plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_plugin.reset();
}
