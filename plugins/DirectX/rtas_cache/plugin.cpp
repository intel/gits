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

class RtasCache : public IPlugin {
public:
  RtasCache(CGits& gits, const char* pluginPath)
      : IPlugin(gits, pluginPath), gits_(gits), pluginPath_(pluginPath) {}

  ~RtasCache() = default;

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
      cfg.record = cfgYaml["Config"]["Record"].as<bool>();
      cfg.stateRestoreOnly = cfgYaml["Config"]["StateRestoreOnly"].as<bool>();

      pluginLayer_ = std::make_unique<RtasCacheLayer>(gits_, cfg);
    }
    return pluginLayer_.get();
  }

private:
  CGits& gits_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<RtasCacheLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::RtasCache> g_rtasCache = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(gits::CGits& gits, const char* pluginPath) {
  if (!g_rtasCache) {
    g_rtasCache = std::make_unique<gits::DirectX::RtasCache>(gits, pluginPath);
  }
  return g_rtasCache.get();
}
