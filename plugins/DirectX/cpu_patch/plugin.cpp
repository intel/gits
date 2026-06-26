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
      : IPlugin(context, pluginPath), context_(context), pluginPath_(pluginPath) {}

  ~CpuPatchPlugin() = default;

  const char* getName() override {
    return "CpuPatch";
  }

  void* getImpl() override {
    if (!pluginLayer_ && context_.config->directx.player.execute) {
      pluginLayer_ = std::make_unique<CpuPatchLayer>(context_);
    }
    return pluginLayer_.get();
  }

private:
  IPluginContext context_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<CpuPatchLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::CpuPatchPlugin> g_plugin = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath) {
  // Initialize Plog for the plugin DLL
  gits::log::Initialize(context.config->common.shared.thresholdLogLevel, context.logAppender);

  if (!g_plugin) {
    g_plugin = std::make_unique<gits::DirectX::CpuPatchPlugin>(context, pluginPath);
  }
  return g_plugin.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_plugin.reset();
}
