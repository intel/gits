// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pluginService.h"
#include "gits.h"
#include "log.h"

#include <unordered_set>

namespace gits {
namespace DirectX {

void PluginInfo::free() {
  if (destroyPlugin) {
    destroyPlugin();
  }
  FreeLibrary(dll);
  for (auto dep : dependencies) {
    FreeLibrary(dep);
  }
}

PluginService::~PluginService() {
  for (auto& plugin : plugins_) {
    plugin.free();
  }
}

void PluginService::loadPlugins() {
  const auto isValidPluginsPath = [](const std::filesystem::path& path) -> bool {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
  };
  const auto toLowerCase = [](const std::string& str) -> std::string {
    auto lowerCaseStr = str;
    std::transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowerCaseStr;
  };

  auto& cfg = Configurator::Get();
  auto pluginNames =
      Configurator::IsPlayer() ? cfg.directx.player.plugins : cfg.directx.recorder.plugins;
  if (pluginNames.empty()) {
    return;
  }

  std::vector<char> moduleFilename(MAX_PATH + 1, 0);
  GetModuleFileNameA(nullptr, moduleFilename.data(), moduleFilename.size());

  // Try to load plugins from the Plugins/DirectX/ directory next to the binary
  auto exePath = std::filesystem::absolute(moduleFilename.data());
  auto pluginsPath = std::filesystem::absolute(exePath.parent_path() / "Plugins" / "DirectX");
  if (!isValidPluginsPath(pluginsPath)) {
    // Try to load plugins from the install path
    pluginsPath = std::filesystem::absolute(cfg.common.recorder.installPath.parent_path() /
                                            "Plugins" / "DirectX");
  }
  if (!isValidPluginsPath(pluginsPath)) {
    // Try to load plugins from the Plugins/DirectX/ directory from the binary's parent directory
    pluginsPath =
        std::filesystem::absolute(exePath.parent_path().parent_path() / "Plugins" / "DirectX");
  }

  if (isValidPluginsPath(pluginsPath)) {
    LOG_INFO << "PluginService - Loading plugins from: " << pluginsPath.string();
  } else {
    LOG_ERROR << "PluginService - Plugin directory does not exist! Expected:"
              << pluginsPath.string();
    LOG_ERROR << "PluginService - Will not load any plugin...";
    return;
  }

  std::unordered_set<std::string> pluginsToEnable;
  for (const auto& pluginName : pluginNames) {
    pluginsToEnable.insert(toLowerCase(pluginName));
  }

  for (const auto& entry : std::filesystem::directory_iterator(pluginsPath)) {
    if (!entry.is_directory()) {
      continue;
    }

    auto pluginPath = entry.path() / "plugin.dll";
    if (!std::filesystem::exists(pluginPath) || !std::filesystem::is_regular_file(pluginPath)) {
      LOG_ERROR << "PluginService - Can't find: " << pluginPath << ". The plugin will not load";
      continue;
    }

    PluginInfo plugin = {};
    plugin.dllPath = std::move(pluginPath);

    // Preload all the plugin dependencies (DLLs)
    auto dependenciesPath = entry.path() / "dependencies";
    if (std::filesystem::exists(dependenciesPath)) {
      for (const auto& dependency : std::filesystem::directory_iterator(dependenciesPath)) {
        if (!dependency.is_regular_file() || dependency.path().extension() != ".dll") {
          continue;
        }
        auto dll = LoadLibrary(dependency.path().string().c_str());
        if (!dll) {
          LOG_ERROR << "PluginService - Failed to load dependency DLL: " << dependency.path();
          continue;
        }
        plugin.dependencies.push_back(dll);
      }
    }

    // Load the main plugin DLL
    plugin.dll = LoadLibrary(plugin.dllPath.string().c_str());
    if (!plugin.dll) {
      LOG_ERROR << "PluginService - Failed to load Plugin DLL: " << plugin.dllPath;
      continue;
    }

    auto createPlugin =
        reinterpret_cast<CreatePluginPtr>(GetProcAddress(plugin.dll, "createPlugin"));
    if (!createPlugin) {
      LOG_ERROR << "PluginService - Failed to locate the 'createPlugin' function in DLL: "
                << plugin.dllPath;
      plugin.free();
      continue;
    }

    IPluginContext pluginContext;
    pluginContext.gits = &CGits::Instance();
    pluginContext.msgBus = &CGits::Instance().GetMessageBus();
    pluginContext.config = &cfg;
    pluginContext.logAppender = plog::get();

    plugin.destroyPlugin =
        reinterpret_cast<DestroyPluginPtr>(GetProcAddress(plugin.dll, "destroyPlugin"));
    plugin.impl = createPlugin(pluginContext, plugin.dllPath.string().c_str());
    if (!plugin.impl) {
      LOG_ERROR << "PluginService - Could not create the plugin instance for DLL: "
                << plugin.dllPath;
      plugin.free();
      continue;
    }

    auto pluginName = plugin.impl->getName();
    if (pluginsToEnable.count(toLowerCase(pluginName)) == 0) {
      LOG_DEBUG << "PluginService - Plugin '" << pluginName
                << "' found but not enabled in the GITS config file";
      plugin.free();
      continue;
    }

    LOG_INFO << "PluginService - Loaded '" << pluginName << "' plugin";
    plugins_.emplace_back(std::move(plugin));
  }

  if (plugins_.size() != pluginNames.size()) {
    LOG_ERROR << "PluginService - Loaded " << plugins_.size() << " plugins out of "
              << pluginNames.size() << " requested";
    LOG_ERROR << "PluginService - Check the plugin names in the GITS config file";
  }
}

const std::vector<PluginInfo>& PluginService::getPlugins() {
  return plugins_;
}

} // namespace DirectX
} // namespace gits
