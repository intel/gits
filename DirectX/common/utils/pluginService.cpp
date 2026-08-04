// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pluginService.h"
#include "log.h"
#include "configurator.h"
#include "gits.h"

#include <unordered_set>

namespace gits {
namespace DirectX {

void PluginInfo::Free() {
  if (DestroyPlugin) {
    DestroyPlugin();
  }
  FreeLibrary(Dll);
  for (auto dep : Dependencies) {
    FreeLibrary(dep);
  }
}

PluginService::~PluginService() {
  for (auto& plugin : m_Plugins) {
    plugin.Free();
  }
}

void PluginService::LoadPlugins() {
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
    plugin.DllPath = std::move(pluginPath);

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
        plugin.Dependencies.push_back(dll);
      }
    }

    // Load the main plugin DLL
    plugin.Dll = LoadLibrary(plugin.DllPath.string().c_str());
    if (!plugin.Dll) {
      LOG_ERROR << "PluginService - Failed to load Plugin DLL: " << plugin.DllPath;
      continue;
    }

    auto createPlugin =
        reinterpret_cast<CreatePluginPtr>(GetProcAddress(plugin.Dll, "createPlugin"));
    if (!createPlugin) {
      LOG_ERROR << "PluginService - Failed to locate the 'createPlugin' function in DLL: "
                << plugin.DllPath;
      plugin.Free();
      continue;
    }

    IPluginContext pluginContext;
    pluginContext.gits = &CGits::Instance();
    pluginContext.msgBus = &gits::MessageBus::get();
    pluginContext.config = &cfg;
    pluginContext.logAppender = plog::get();

    plugin.DestroyPlugin =
        reinterpret_cast<DestroyPluginPtr>(GetProcAddress(plugin.Dll, "destroyPlugin"));
    plugin.Impl = createPlugin(pluginContext, plugin.DllPath.string().c_str());
    if (!plugin.Impl) {
      LOG_ERROR << "PluginService - Could not create the plugin instance for DLL: "
                << plugin.DllPath;
      plugin.Free();
      continue;
    }

    auto pluginName = plugin.Impl->getName();
    if (pluginsToEnable.count(toLowerCase(pluginName)) == 0) {
      LOG_DEBUG << "PluginService - Plugin '" << pluginName
                << "' found but not enabled in the GITS config file";
      plugin.Free();
      continue;
    }

    // Initialize the plugin (this will call the plugin's layer constructor)
    if (!plugin.Impl->getImpl()) {
      LOG_ERROR << "PluginService - Failed to initialize plugin: " << pluginName;
      plugin.Free();
      continue;
    }

    LOG_INFO << "PluginService - Loaded '" << pluginName << "' plugin";
    m_Plugins.emplace_back(std::move(plugin));
  }

  if (m_Plugins.size() != pluginNames.size()) {
    LOG_ERROR << "PluginService - Loaded " << m_Plugins.size() << " plugins out of "
              << pluginNames.size() << " requested";
    LOG_ERROR << "PluginService - Check the plugin names in the GITS config file";
  }
}

const std::vector<PluginInfo>& PluginService::GetPlugins() const {
  return m_Plugins;
}

} // namespace DirectX
} // namespace gits
