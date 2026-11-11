// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "moduleRegistry.h"

#include "baseContext.h"
#include "baseEventBus.h"
#include "configurationAuto.h"
#include "log.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <map>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace gits::gui {

namespace {

// Case-insensitive, as PluginService matches plugin names; module ids follow the same rule.
bool SameName(const std::string& lhs, const std::string& rhs) {
  return std::ranges::equal(lhs, rhs, [](unsigned char a, unsigned char b) {
    return std::tolower(a) == std::tolower(b);
  });
}

bool ActivateModule(ModuleBasePanel& module) {
  try {
    return module.Activate();
  } catch (const std::exception& exception) {
    LOG_ERROR << "ModuleRegistry: Activating " << module.GetName()
              << " failed: " << exception.what();
    return false;
  }
}

void DeactivateModule(ModuleBasePanel& module) {
  try {
    module.Deactivate();
  } catch (const std::exception& exception) {
    LOG_ERROR << "ModuleRegistry: Deactivating " << module.GetName()
              << " failed: " << exception.what();
  }
}

std::vector<std::string> ModulePlugins(const ModuleBasePanel& module, Api api) {
  try {
    return module.Plugins(api);
  } catch (const std::exception& exception) {
    LOG_ERROR << "ModuleRegistry: Listing the plugins of " << module.GetName()
              << " failed: " << exception.what();
    return {};
  }
}

#ifdef _WIN32
std::filesystem::path ExecutableDirectory() {
  std::wstring path(MAX_PATH, L'\0');
  while (true) {
    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0) {
      return {};
    }
    if (length < path.size()) {
      path.resize(length);
      return std::filesystem::path(path).parent_path();
    }
    path.resize(path.size() * 2);
  }
}

gits::LogLevel StudioLogLevel() {
  for (const auto level :
       {gits::LogLevel::TRACEV, gits::LogLevel::TRACE, gits::LogLevel::INFOV, gits::LogLevel::INFO,
        gits::LogLevel::WARN, gits::LogLevel::ERR, gits::LogLevel::OFF}) {
    if (gits::log::GetSeverity(level) == plog::get()->getMaxSeverity()) {
      return level;
    }
  }
  return gits::LogLevel::INFO;
}

ModuleContext StudioModuleContext() {
  ModuleContext context;
  context.ImGui = ImGui::GetCurrentContext();
  ImGui::GetAllocatorFunctions(&context.ImGuiAlloc, &context.ImGuiFree,
                               &context.ImGuiAllocUserData);
  context.LogAppender = plog::get();
  context.LogLevel = StudioLogLevel();
  context.Events = &EventBus::GetInstance();
  context.Base = &BaseContext::GetBaseInstance();
  return context;
}
#endif

} // namespace

ModuleRegistry::ModuleRegistry() {
  m_EntryMap = std::map<ModuleId, ImGuiHelper::ButtonGroupItem>();
}

ModuleRegistry& ModuleRegistry::GetInstance() {
  // Function local, so it is built by the first registration rather than
  // depending on the order static objects across translation units run in.
  static ModuleRegistry instance;
  return instance;
}

bool ModuleRegistry::Register(ModuleId id,
                              Factory factory,
                              const char* name,
                              const char* nameHint) {
  auto& instance = GetInstance();
  instance.m_Entries.push_back(Entry{id, factory, nullptr});
  instance.m_EntryMap.emplace(
      id,
      ImGuiHelper::ButtonGroupItem{name != nullptr ? name : "", nameHint != nullptr ? nameHint : "",
                                   ImGuiHelper::ButtonStatus::BUTTON_STATUS_DEFAULT, "", true, true,
                                   ImGuiHelper::ItemType::CheckableButton});
  if (instance.BtnsModules != nullptr) {
    delete instance.BtnsModules;
  }
  instance.BtnsModules =
      new ImGuiHelper::TabGroup<ModuleId>(instance.m_EntryMap, false, true, true);
  return true;
}

// Module DLLs live in Modules/<id>/module.dll next to Studio, or in the GITS installation when
// Studio has no Modules folder. They are never unloaded: Studio keeps calling their code, for
// example the handlers they subscribe on its EventBus.
void ModuleRegistry::LoadModules() {
  m_ModulesLoaded = true;
#ifdef _WIN32
  try {
    std::error_code status;
    auto root = ExecutableDirectory() / "Modules";
    if (!std::filesystem::is_directory(root, status)) {
      root = BaseContext::GetBaseInstance().GetPathSafe(Path::GITS_BASE) / "Modules";
    }
    if (!std::filesystem::is_directory(root, status)) {
      LOG_INFO << "ModuleRegistry: No Modules directory found";
      return;
    }
    const auto context = StudioModuleContext();
    for (const auto& directory : std::filesystem::directory_iterator(root)) {
      const auto path = directory.path() / "module.dll";
      if (!std::filesystem::is_regular_file(path, status)) {
        continue;
      }
      const auto library =
          LoadLibraryExW(path.c_str(), nullptr,
                         LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
      if (library == nullptr) {
        LOG_ERROR << "ModuleRegistry: Loading " << path << " failed with Windows error "
                  << GetLastError();
        continue;
      }
      const auto entryPoint =
          reinterpret_cast<ModuleEntryPoint>(GetProcAddress(library, "GitsGuiModule"));
      if (entryPoint == nullptr) {
        LOG_ERROR << "ModuleRegistry: " << path << " does not export GitsGuiModule";
        continue;
      }
      try {
        ModuleRegistration registration;
        entryPoint(&context, &registration);
        if (registration.Id == nullptr || *registration.Id == '\0') {
          LOG_ERROR << "ModuleRegistry: " << path << " has no module id";
          continue;
        }
        const auto sameId = [&registration](const Entry& entry) {
          return SameName(entry.Id, registration.Id);
        };
        if (std::ranges::any_of(m_Entries, sameId)) {
          LOG_ERROR << "ModuleRegistry: " << path << " uses the module id " << registration.Id
                    << " of another module";
          continue;
        }
        Register(registration.Id, registration.Create, registration.Name, registration.NameHint);
      } catch (const std::exception& exception) {
        LOG_ERROR << "ModuleRegistry: Registering " << path << " failed: " << exception.what();
      }
    }
  } catch (const std::exception& exception) {
    LOG_ERROR << "ModuleRegistry: Loading modules failed: " << exception.what();
  }
#endif
}

void ModuleRegistry::CreateModules() {
  if (!m_ModulesLoaded) {
    LoadModules();
  }
  m_Panels.clear();
  m_Panels.reserve(m_Entries.size());
  for (auto it = m_Entries.begin(); it != m_Entries.end();) {
    if (it->Panel == nullptr) {
      try {
        it->Panel = it->Create();
      } catch (const std::exception& exception) {
        LOG_ERROR << "ModuleRegistry: Creating " << m_EntryMap.at(it->Id).label
                  << " failed: " << exception.what();
      }
    }
    if (it->Panel == nullptr) {
      m_EntryMap.erase(it->Id);
      it = m_Entries.erase(it);
      delete BtnsModules;
      BtnsModules = m_EntryMap.empty()
                        ? nullptr
                        : new ImGuiHelper::TabGroup<ModuleId>(m_EntryMap, false, true, true);
      continue;
    }
    m_Panels.push_back(it->Panel.get());
    ++it;
  }
  LOG_INFO << "ModuleRegistry: Created " << m_Panels.size() << " modules";
}

void ModuleRegistry::DestroyModules() {
  m_Panels.clear();
  // Reverse registration order, mirroring how members of a panel would go.
  for (auto it = m_Entries.rbegin(); it != m_Entries.rend(); ++it) {
    it->Panel.reset();
  }
}

std::vector<ModuleId> ModuleRegistry::Ids() const {
  std::vector<ModuleId> ids;
  for (const auto& entry : m_Entries) {
    if (entry.Panel != nullptr) {
      ids.push_back(entry.Id);
    }
  }
  return ids;
}

ModuleBasePanel* ModuleRegistry::Find(const ModuleId& id) const {
  auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                         [&id](const Entry& entry) { return entry.Id == id; });
  return it != m_Entries.end() ? it->Panel.get() : nullptr;
}

bool ModuleRegistry::EnableModule(const ModuleId& id) {
  if (BtnsModules == nullptr) {
    return false;
  }

  auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                         [&id](const Entry& entry) { return entry.Id == id; });
  if (it == m_Entries.end()) {
    return false;
  }
  auto enabled = ActivateModule(*it->Panel);
  BtnsModules->SetChecked(id, enabled);

  return enabled;
}

bool ModuleRegistry::DisableModule(const ModuleId& id) {
  if (BtnsModules == nullptr) {
    return false;
  }

  auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                         [&id](const Entry& entry) { return entry.Id == id; });
  if (it == m_Entries.end()) {
    return false;
  }
  DeactivateModule(*it->Panel);
  BtnsModules->SetChecked(id, false);

  return true;
}

void ModuleRegistry::AddActivePlugins(GitsAction action, Api api, Configuration& config) const {
  auto& plugins =
      action == GitsAction::CAPTURE ? config.common.recorder.plugins : config.common.player.plugins;
  for (const auto& entry : m_Entries) {
    if (entry.Panel == nullptr || !entry.Panel->IsActive()) {
      continue;
    }
    for (const auto& plugin : ModulePlugins(*entry.Panel, api)) {
      const bool listed = std::ranges::any_of(
          plugins, [&plugin](const std::string& name) { return SameName(name, plugin); });
      if (!listed) {
        LOG_INFO << "ModuleRegistry: Adding plugin " << plugin << " to this launch";
        plugins.push_back(plugin);
      }
    }
  }
}

bool ModuleRegistry::RenderModuleButtons() {
  auto result = false;
  if (BtnsModules != nullptr) {
    ImGui::Text("Modules");
    result = BtnsModules->Render();
    if (result) {
      for (auto& entry : m_Entries) {
        auto isChecked = BtnsModules->IsChecked(entry.Id);
        if (isChecked != entry.Panel->IsActive()) {
          if (isChecked) {
            BtnsModules->SetChecked(entry.Id, ActivateModule(*entry.Panel));
          } else {
            DeactivateModule(*entry.Panel);
          }
        }
      }
    }
  }
  return result;
}
} // namespace gits::gui
