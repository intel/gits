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

#include <filesystem>

namespace gits {
namespace DirectX {

class CpuPatchPlugin : public IPlugin {
public:
  CpuPatchPlugin(IPluginContext context, const char* pluginPath)
      : IPlugin(context, pluginPath), m_Context(context), m_PluginPath(pluginPath) {}

  ~CpuPatchPlugin() = default;

  const char* getName() override {
    return "CpuPatch";
  }

  void* getImpl() override {
    if (!m_PluginLayer && m_Context.config->common.player.execute) {
      m_PluginLayer = std::make_unique<CpuPatchLayer>(m_Context);
    }
    return m_PluginLayer.get();
  }

private:
  IPluginContext m_Context;
  std::filesystem::path m_PluginPath;
  std::unique_ptr<CpuPatchLayer> m_PluginLayer;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::CpuPatchPlugin> g_Plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_Plugin) {
    g_Plugin = std::make_unique<gits::DirectX::CpuPatchPlugin>(context, pluginPath);
  }
  return g_Plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_Plugin.reset();
}
