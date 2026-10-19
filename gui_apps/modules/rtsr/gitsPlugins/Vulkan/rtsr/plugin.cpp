// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "IPlugin.h"
#include "layer.h"

#include "configurationAuto.h"
#include "log.h"
#include "rtsrPluginConfig.h"
#include "rtsr/shared/rtsrCommon.h"

#include <filesystem>
#include <memory>
#include <string_view>

namespace gits::vulkan {

class RtsrPlugin final : public IPlugin {
public:
  RtsrPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}

  const char* getName() override {
    return rtsr::PLUGIN_NAME;
  }

  void* getImpl() override {
    if (!m_Layer) {
      const auto configPath = m_PluginPath.parent_path() / "config.yml";
      const rtsr::PluginConfig config = rtsr::ReadPluginConfig(
          configPath, [](std::string_view message) { LOG_WARNING << message; });
      const bool playback =
          m_Context.config != nullptr && m_Context.config->common.mode == GITSMode::MODE_PLAYER;
      const bool execute =
          !playback || (m_Context.config != nullptr && m_Context.config->common.player.execute);
      m_Layer = std::make_unique<RtsrLayer>(config, m_Context.gits, playback, execute,
                                            m_Context.vkDispatchTables);
    }
    return m_Layer.get();
  }

private:
  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<RtsrLayer> m_Layer;
};

} // namespace gits::vulkan

namespace {
std::unique_ptr<gits::vulkan::RtsrPlugin> g_Plugin;
}

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);
  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::vulkan::RtsrPlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
