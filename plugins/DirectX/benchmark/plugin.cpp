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

class Benchmark : public IPlugin {
public:
  Benchmark(CGits& gits, const char* pluginPath)
      : IPlugin(gits, pluginPath), gits_(gits), pluginPath_(pluginPath) {}

  ~Benchmark() = default;

  const char* getName() override {
    return "Benchmark";
  }

  void* getImpl() override {
    if (!pluginLayer_) {
      auto cfgPath = pluginPath_.parent_path() / "config.yml";
      auto cfgYaml = YAML::LoadFile(cfgPath.string());
      if (!cfgYaml) {
        throw std::runtime_error("Config file did not load correctly");
      }

      BenchmarkConfig cfg = {};
      cfg.cpuFrameBenchmarkConfig.enabled = cfgYaml["Config"]["Enabled"].as<bool>();
      cfg.cpuFrameBenchmarkConfig.output = cfgYaml["Config"]["Output"].as<std::string>();

      pluginLayer_ = std::make_unique<BenchmarkLayer>(gits_, cfg);
    }
    return pluginLayer_.get();
  }

private:
  CGits& gits_;
  std::filesystem::path pluginPath_;
  std::unique_ptr<BenchmarkLayer> pluginLayer_;
};

} // namespace DirectX
} // namespace gits

static std::unique_ptr<gits::DirectX::Benchmark> g_benchmark = nullptr;

GITS_PLUGIN_API IPlugin* createPlugin(gits::CGits& gits, const char* pluginPath) {
  if (!g_benchmark) {
    g_benchmark = std::make_unique<gits::DirectX::Benchmark>(gits, pluginPath);
  }
  return g_benchmark.get();
}

GITS_PLUGIN_API void destroyPlugin() {
  g_benchmark.reset();
}
