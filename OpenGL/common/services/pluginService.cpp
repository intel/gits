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
#include "messageBus.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace gits::opengl {

namespace {

#ifdef _WIN32
constexpr const char* PLUGIN_FILE_NAME = "plugin.dll";
constexpr const char* LIBRARY_EXTENSION = ".dll";

void* OpenLibrary(const std::filesystem::path& path) {
  return LoadLibraryA(path.string().c_str());
}

void* FindSymbol(void* module, const char* name) {
  return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), name));
}

void CloseLibrary(void* module) {
  FreeLibrary(static_cast<HMODULE>(module));
}

std::filesystem::path ExecutablePath() {
  std::vector<char> moduleFilename(MAX_PATH + 1, 0);
  GetModuleFileNameA(nullptr, moduleFilename.data(), static_cast<DWORD>(moduleFilename.size()));
  return std::filesystem::absolute(moduleFilename.data());
}
#else
constexpr const char* PLUGIN_FILE_NAME = "plugin.so";
constexpr const char* LIBRARY_EXTENSION = ".so";

void* OpenLibrary(const std::filesystem::path& path) {
  void* module = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (module == nullptr) {
    LOG_ERROR << "PluginService - " << dlerror();
  }
  return module;
}

void* FindSymbol(void* module, const char* name) {
  return dlsym(module, name);
}

void CloseLibrary(void* module) {
  dlclose(module);
}

std::filesystem::path ExecutablePath() {
  std::error_code error;
  return std::filesystem::read_symlink("/proc/self/exe", error);
}
#endif

std::string ToLowerCase(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return str;
}

bool IsDirectory(const std::filesystem::path& path) {
  return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

// The recorder runs inside the application's process, so its plugins are found relative to
// the recorder installation rather than to the executable.
std::filesystem::path FindPluginsPath(const Configuration& cfg) {
  if (Configurator::IsRecorder()) {
    return std::filesystem::absolute(cfg.common.recorder.installPath.parent_path() / "Plugins" /
                                     "OpenGL");
  }
  const auto exePath = ExecutablePath();
  auto pluginsPath = exePath.parent_path() / "Plugins" / "OpenGL";
  if (!IsDirectory(pluginsPath)) {
    pluginsPath = exePath.parent_path().parent_path() / "Plugins" / "OpenGL";
  }
  return pluginsPath;
}

} // namespace

void PluginInfo::Free() {
  if (DestroyPlugin) {
    DestroyPlugin();
    DestroyPlugin = nullptr;
  }
  Impl = nullptr;
  if (Dll) {
    CloseLibrary(Dll);
    Dll = nullptr;
  }
  for (void* dependency : Dependencies) {
    CloseLibrary(dependency);
  }
  Dependencies.clear();
}

PluginService::~PluginService() {
  for (auto& plugin : m_Plugins) {
    plugin.Free();
  }
}

void PluginService::LoadPlugins() {
  auto& cfg = Configurator::Get();
  const auto& pluginNames =
      Configurator::IsPlayer() ? cfg.common.player.plugins : cfg.common.recorder.plugins;
  if (pluginNames.empty()) {
    return;
  }

  const auto pluginsPath = FindPluginsPath(cfg);
  if (!IsDirectory(pluginsPath)) {
    LOG_ERROR << "PluginService - Plugin directory does not exist! Expected: "
              << pluginsPath.string();
    LOG_ERROR << "PluginService - Will not load any plugin...";
    return;
  }
  LOG_INFO << "PluginService - Loading plugins from: " << pluginsPath.string();

  std::unordered_set<std::string> pluginsToEnable;
  for (const auto& pluginName : pluginNames) {
    pluginsToEnable.insert(ToLowerCase(pluginName));
  }

  for (const auto& entry : std::filesystem::directory_iterator(pluginsPath)) {
    if (!entry.is_directory()) {
      continue;
    }

    PluginInfo plugin;
    plugin.DllPath = entry.path() / PLUGIN_FILE_NAME;
    if (!std::filesystem::is_regular_file(plugin.DllPath)) {
      LOG_ERROR << "PluginService - Can't find: " << plugin.DllPath << ". The plugin will not load";
      continue;
    }

    const auto dependenciesPath = entry.path() / "dependencies";
    if (IsDirectory(dependenciesPath)) {
      for (const auto& dependency : std::filesystem::directory_iterator(dependenciesPath)) {
        if (!dependency.is_regular_file() || dependency.path().extension() != LIBRARY_EXTENSION) {
          continue;
        }
        void* dependencyModule = OpenLibrary(dependency.path());
        if (!dependencyModule) {
          LOG_ERROR << "PluginService - Failed to load dependency library: " << dependency.path();
          continue;
        }
        plugin.Dependencies.push_back(dependencyModule);
      }
    }

    void* dll = OpenLibrary(plugin.DllPath);
    if (!dll) {
      LOG_ERROR << "PluginService - Failed to load plugin library: " << plugin.DllPath;
      plugin.Free();
      continue;
    }
    plugin.Dll = dll;

    auto createPlugin = reinterpret_cast<CreatePluginPtr>(FindSymbol(dll, "createPlugin"));
    if (!createPlugin) {
      LOG_ERROR << "PluginService - Failed to locate the 'createPlugin' function in: "
                << plugin.DllPath;
      plugin.Free();
      continue;
    }

    IPluginContext context{};
    context.config = &cfg;
    context.gits = &CGits::Instance();
    context.msgBus = &MessageBus::get();
    context.logAppender = plog::get();
    context.glGetRealFunction = m_GetRealFunction;

    plugin.DestroyPlugin = reinterpret_cast<DestroyPluginPtr>(FindSymbol(dll, "destroyPlugin"));
    plugin.Impl = createPlugin(context, plugin.DllPath.string().c_str());
    if (!plugin.Impl) {
      LOG_ERROR << "PluginService - Could not create the plugin instance for: " << plugin.DllPath;
      plugin.Free();
      continue;
    }

    const std::string name = plugin.Impl->getName();
    if (pluginsToEnable.count(ToLowerCase(name)) == 0) {
      LOG_DEBUG << "PluginService - Plugin '" << name
                << "' found but not enabled in the GITS config file";
      plugin.Free();
      continue;
    }

    LOG_INFO << "PluginService - Loaded '" << name << "' plugin";
    m_Plugins.push_back(std::move(plugin));
  }

  if (m_Plugins.size() != pluginNames.size()) {
    LOG_ERROR << "PluginService - Loaded " << m_Plugins.size() << " plugins out of "
              << pluginNames.size() << " requested";
    LOG_ERROR << "PluginService - Check the plugin names in the GITS config file";
  }
}

} // namespace gits::opengl
