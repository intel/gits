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

class PlatformPortabilityPlugin : public IPlugin {
public:
  PlatformPortabilityPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), context_(context), pluginPath_(pluginPath) {}

  ~PlatformPortabilityPlugin() = default;

  const char* getName() override {
    return "PlatformPortability";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      pluginLayer_ = std::make_unique<PlatformPortabilityLayer>();
    }
    return pluginLayer_.get();
  }

private:
  IPluginContext context_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<PlatformPortabilityLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::PlatformPortabilityPlugin> g_plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  // Initialize Plog for the plugin DLL
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_plugin) {
    g_plugin = std::make_unique<gits::DirectX::PlatformPortabilityPlugin>(context, pluginPath);
  }
  return g_plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_plugin.reset();
}
