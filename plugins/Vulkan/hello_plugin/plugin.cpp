// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "IPlugin.h"
#include "configurationAuto.h"
#include "layer.h"
#include "log.h"

#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace gits {
namespace vulkan {

class HelloPlugin : public IPlugin {
public:
  HelloPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_PluginPath(pluginPath) {
    (void)context;
  }

  ~HelloPlugin() override = default;

  const char* getName() override {
    return "HelloPlugin";
  }

  void* getImpl() override {
    if (!m_PluginLayer) {
      auto cfgPath = m_PluginPath.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      HelloPluginConfig cfg = {};
      cfg.PrintFrames = cfgYaml["Config"]["PrintFrames"].as<bool>();
      cfg.PrintQueueSubmits = cfgYaml["Config"]["PrintQueueSubmits"].as<bool>();

      m_PluginLayer = std::make_unique<HelloPluginLayer>(cfg);
    }
    return m_PluginLayer.get();
  }

private:
  std::filesystem::path m_PluginPath;
  std::unique_ptr<HelloPluginLayer> m_PluginLayer;
};

} // namespace vulkan
} // namespace gits

namespace {

std::unique_ptr<gits::vulkan::HelloPlugin> g_Plugin;

} // namespace

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::vulkan::HelloPlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
