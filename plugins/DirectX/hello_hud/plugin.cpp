// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "IPlugin.h"
#include "layer.h"
#include "log.h"
#include "configurationAuto.h"
#include "gits.h"

#include "yaml-cpp/yaml.h"
#include <filesystem>

namespace gits {
namespace DirectX {

class HelloHUD : public IPlugin {
public:
  HelloHUD(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), context_(context), pluginPath_(pluginPath) {}

  ~HelloHUD() = default;

  const char* getName() override {
    return "HelloHUD";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      auto cfgPath = pluginPath_.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      HelloHUDConfig cfg = {};
      cfg.Text = cfgYaml["Config"]["Text"].as<std::string>();
      pluginLayer_ = std::make_unique<HelloHUDLayer>(cfg, context_.gits);
    }
    return pluginLayer_.get();
  }

private:
  IPluginContext context_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<HelloHUDLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::HelloHUD> g_plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  // Initialize Plog for the plugin DLL
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_plugin) {
    g_plugin = std::make_unique<gits::DirectX::HelloHUD>(context, pluginPath);
  }
  return g_plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_plugin.reset();
}
