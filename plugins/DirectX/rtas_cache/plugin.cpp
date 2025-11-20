// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
      : IPlugin(context, pluginPath), context_(context), pluginPath_(pluginPath) {}

  ~RtasCachePlugin() = default;

  const char* getName() override {
    return "RtasCache";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      auto cfgPath = pluginPath_.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      RtasCacheConfig cfg = {};
      cfg.cacheFile = cfgYaml["Config"]["CacheFile"].as<std::string>();
      cfg.record = cfgYaml["Config"]["Record"].as<bool>();
      cfg.stateRestoreOnly = cfgYaml["Config"]["StateRestoreOnly"].as<bool>();
      cfg.dumpCacheInfoFile = cfgYaml["Config"]["DumpCacheInfoFile"].as<bool>();

      pluginLayer_ = std::make_unique<RtasCacheLayer>(cfg);
    }
    return pluginLayer_.get();
  }

private:
  IPluginContext context_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<RtasCacheLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::RtasCachePlugin> g_plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  // Initialize Plog for the plugin DLL
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_plugin) {
    g_plugin = std::make_unique<gits::DirectX::RtasCachePlugin>(context, pluginPath);
  }
  return g_plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_plugin.reset();
}
