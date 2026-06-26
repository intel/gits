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
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}

  ~PlatformPortabilityPlugin() = default;

  const char* getName() override {
    return "PlatformPortability";
  }

  void* getImpl() override {
    if (!m_PluginLayer) {
      m_PluginLayer = std::make_unique<PlatformPortabilityLayer>();
    }
    return m_PluginLayer.get();
  }

private:
  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<PlatformPortabilityLayer> m_PluginLayer;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::PlatformPortabilityPlugin> g_Plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::DirectX::PlatformPortabilityPlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
