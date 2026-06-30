// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pluginService.h"
#include "configurator.h"
#include "gits.h"
#include "log.h"

#include <algorithm>
#include <unordered_set>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace gits {
namespace vulkan {

void PluginInfo::Free() {
  if (DestroyPlugin) {
    DestroyPlugin();
    DestroyPlugin = nullptr;
  }
  Impl = nullptr;
#if defined(_WIN32)
  if (Dll) {
    FreeLibrary(static_cast<HMODULE>(Dll));
    Dll = nullptr;
  }
  for (void* dep : Dependencies) {
    if (dep) {
      FreeLibrary(static_cast<HMODULE>(dep));
    }
  }
#endif
  Dependencies.clear();
}

PluginService::~PluginService() {
  for (auto& plugin : m_Plugins) {
    plugin.Free();
  }
}

void PluginService::LoadPlugins() {
  auto& cfg = Configurator::Get();
  const auto& pluginNames = cfg.common.shared.plugins;
  if (pluginNames.empty()) {
    return;
  }

#if !defined(_WIN32)
  LOG_INFO << "PluginService - Vulkan plugins are not yet supported on Linux. Skipping load...";
  return;
#else
  const auto isValidPluginsPath = [](const std::filesystem::path& path) -> bool {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
  };
  const auto toLowerCase = [](const std::string& str) -> std::string {
    auto lowerCaseStr = str;
    std::transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowerCaseStr;
  };

  std::vector<char> moduleFilename(MAX_PATH + 1, 0);
  GetModuleFileNameA(nullptr, moduleFilename.data(), static_cast<DWORD>(moduleFilename.size()));

  auto exePath = std::filesystem::absolute(moduleFilename.data());
  auto pluginsPath = std::filesystem::absolute(exePath.parent_path() / "Plugins" / "Vulkan");
  if (!isValidPluginsPath(pluginsPath)) {
    pluginsPath = std::filesystem::absolute(cfg.common.recorder.installPath.parent_path() /
                                            "Plugins" / "Vulkan");
  }
  if (!isValidPluginsPath(pluginsPath)) {
    pluginsPath =
        std::filesystem::absolute(exePath.parent_path().parent_path() / "Plugins" / "Vulkan");
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

    auto pluginDllPath = entry.path() / "plugin.dll";
    if (!std::filesystem::exists(pluginDllPath) ||
        !std::filesystem::is_regular_file(pluginDllPath)) {
      LOG_ERROR << "PluginService - Can't find: " << pluginDllPath << ". The plugin will not load";
      continue;
    }

    PluginInfo plugin = {};
    plugin.DllPath = std::move(pluginDllPath);

    auto dependenciesPath = entry.path() / "dependencies";
    if (std::filesystem::exists(dependenciesPath)) {
      for (const auto& dependency : std::filesystem::directory_iterator(dependenciesPath)) {
        if (!dependency.is_regular_file() || dependency.path().extension() != ".dll") {
          continue;
        }
        auto depDll = LoadLibraryA(dependency.path().string().c_str());
        if (!depDll) {
          LOG_ERROR << "PluginService - Failed to load dependency DLL: " << dependency.path();
          continue;
        }
        plugin.Dependencies.push_back(depDll);
      }
    }

    auto mainDll = LoadLibraryA(plugin.DllPath.string().c_str());
    if (!mainDll) {
      LOG_ERROR << "PluginService - Failed to load Plugin DLL: " << plugin.DllPath;
      continue;
    }
    plugin.Dll = mainDll;

    auto createPlugin = reinterpret_cast<CreatePluginPtr>(GetProcAddress(mainDll, "createPlugin"));
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
        reinterpret_cast<DestroyPluginPtr>(GetProcAddress(mainDll, "destroyPlugin"));
    plugin.Impl = createPlugin(pluginContext, plugin.DllPath.string().c_str());
    if (!plugin.Impl) {
      LOG_ERROR << "PluginService - Could not create the plugin instance for DLL: "
                << plugin.DllPath;
      plugin.Free();
      continue;
    }

    auto implName = plugin.Impl->getName();
    if (pluginsToEnable.count(toLowerCase(implName)) == 0) {
      LOG_DEBUG << "PluginService - Plugin '" << implName
                << "' found but not enabled in the GITS config file";
      plugin.Free();
      continue;
    }

    LOG_INFO << "PluginService - Loaded '" << implName << "' plugin";
    m_Plugins.emplace_back(std::move(plugin));
  }

  if (m_Plugins.size() != pluginNames.size()) {
    LOG_ERROR << "PluginService - Loaded " << m_Plugins.size() << " plugins out of "
              << pluginNames.size() << " requested";
    LOG_ERROR << "PluginService - Check the plugin names in the GITS config file";
  }
#endif
}

const std::vector<PluginInfo>& PluginService::GetPlugins() const {
  return m_Plugins;
}

} // namespace vulkan
} // namespace gits
