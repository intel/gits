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

class BenchmarkPlugin : public IPlugin {
public:
  BenchmarkPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}

  ~BenchmarkPlugin() = default;

  const char* getName() override {
    return "Benchmark";
  }

  void* getImpl() override {
    if (!m_PluginLayer) {
      std::filesystem::path cfgPath = m_PluginPath.parent_path() / "config.yml";
      YAML::Node cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      BenchmarkConfig cfg{};
      cfg.Output = cfgYaml["Config"]["Output"].as<std::string>();

      if (m_Context.config->common.mode == GITSMode::MODE_RECORDER) {
        std::filesystem::path outputPath = m_Context.config->common.recorder.dumpPath / cfg.Output;
        cfg.Output = outputPath.string();
        cfg.IsCapture = true;
      }
      GITS_ASSERT(m_Context.msgBus);
      m_PluginLayer = std::make_unique<BenchmarkLayer>(cfg, *m_Context.msgBus);
    }
    return m_PluginLayer.get();
  }

private:
  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<BenchmarkLayer> m_PluginLayer;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::BenchmarkPlugin> g_Plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::DirectX::BenchmarkPlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
