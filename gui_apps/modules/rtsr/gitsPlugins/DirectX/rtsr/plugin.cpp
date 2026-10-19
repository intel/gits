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
#include "rtsrPluginConfig.h"
#include "rtsr/shared/rtsrCommon.h"

#include <filesystem>
#include <memory>
#include <string_view>

namespace gits {
namespace DirectX {

class RtsrPlugin : public IPlugin {
public:
  RtsrPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}

  ~RtsrPlugin() = default;

  const char* getName() override {
    return rtsr::PLUGIN_NAME;
  }

  void* getImpl() override {
    if (!m_Layer) {
      m_Layer = CreateLayer();
    }
    return m_Layer.get();
  }

private:
  std::unique_ptr<RtsrLayer> CreateLayer() {
    const auto configPath = m_PluginPath.parent_path() / "config.yml";
    const rtsr::PluginConfig config = rtsr::ReadPluginConfig(
        configPath, [](std::string_view message) { LOG_WARNING << message; });

    // Taken from the context rather than from Configurator::Get(), which inside
    // a plugin DLL resolves to a second, never-populated singleton.
    const bool playback =
        m_Context.config != nullptr && m_Context.config->common.mode == GITSMode::MODE_PLAYER;

    return std::make_unique<RtsrLayer>(config, m_Context.gits, playback);
  }

  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<RtsrLayer> m_Layer;
};

} // namespace DirectX
} // namespace gits

namespace {

std::unique_ptr<gits::DirectX::RtsrPlugin> g_plugin;

} // namespace

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  // Initialize Plog for the plugin DLL
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_plugin) {
    g_plugin = std::make_unique<gits::DirectX::RtsrPlugin>(context, pluginPath);
  }
  return g_plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  // Resetting the plugin destroys the layer, which releases the RTSR session
  // while the process and winsock are still alive.
  g_plugin.reset();
}
