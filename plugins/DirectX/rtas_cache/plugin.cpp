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

#include "yaml-cpp/yaml.h"
#include <filesystem>

namespace gits {
namespace DirectX {

class RtasCachePlugin : public IPlugin {
public:
  RtasCachePlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}
  ~RtasCachePlugin() = default;

  const char* getName() override {
    return "RtasCache";
  }

  void* getImpl() override {
    if (!m_PluginLayer) {
      std::filesystem::path cfgPath = m_PluginPath.parent_path() / "config.yml";
      YAML::Node cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      RtasCacheConfig cfg{};
      cfg.CacheFile = cfgYaml["Config"]["CacheFile"].as<std::string>();
      cfg.Record = cfgYaml["Config"]["Record"].as<bool>();
      cfg.StateRestoreOnly = cfgYaml["Config"]["StateRestoreOnly"].as<bool>();
      cfg.DumpCacheInfoFile = cfgYaml["Config"]["DumpCacheInfoFile"].as<bool>();

      m_PluginLayer = std::make_unique<RtasCacheLayer>(cfg);
    }
    return m_PluginLayer.get();
  }

private:
  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<RtasCacheLayer> m_PluginLayer;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::RtasCachePlugin> g_Plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);
  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::DirectX::RtasCachePlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
