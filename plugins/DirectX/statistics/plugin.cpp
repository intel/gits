// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "IPlugin.h"
#include "statisticsLayerAuto.h"
#include "log.h"
#include "configurationAuto.h"

#include "yaml-cpp/yaml.h"
#include <filesystem>

namespace gits {
namespace DirectX {

class StatisticsPlugin : public IPlugin {
public:
  StatisticsPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}
  ~StatisticsPlugin() = default;

  const char* getName() override {
    return "Statistics";
  }

  void* getImpl() override {
    if (!m_PluginLayer) {
      std::filesystem::path cfgPath = m_PluginPath.parent_path() / "config.yml";
      YAML::Node cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      StatisticsConfig cfg{};
      cfg.Output = cfgYaml["Config"]["Output"].as<std::string>();

      if (m_Context.config->common.mode == GITSMode::MODE_RECORDER) {
        std::filesystem::path outputPath = m_Context.config->common.recorder.dumpPath / cfg.Output;
        cfg.Output = outputPath.string();
        cfg.IsCapture = true;
      }
      GITS_ASSERT(m_Context.msgBus);
      m_PluginLayer = std::make_unique<StatisticsLayer>(cfg, *m_Context.msgBus);
    }
    return m_PluginLayer.get();
  }

private:
  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<StatisticsLayer> m_PluginLayer;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::StatisticsPlugin> g_Plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);
  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::DirectX::StatisticsPlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
