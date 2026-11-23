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

// What a module DLL exports: see GITS_GUI_REGISTER_MODULE.
using ModuleEntryPoint = void (*)(const ModuleContext* context, ModuleRegistration* registration);

// Case-insensitive, as PluginService matches plugin names; module ids follow the same rule.
bool SameName(const std::string& lhs, const std::string& rhs) {
  return std::ranges::equal(lhs, rhs, [](unsigned char a, unsigned char b) {
    return std::tolower(a) == std::tolower(b);
  });
}

void SetModuleActive(ModuleBasePanel& module, bool active) {
  try {
    if (active) {
      module.Activate();
    } else {
      module.Deactivate();
    }
  } catch (const std::exception& exception) {
    LOG_ERROR << "ModuleRegistry: " << (active ? "Activating " : "Deactivating ")
              << module.GetName() << " failed: " << exception.what();
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

ModuleRegistry& ModuleRegistry::GetInstance() {
  static ModuleRegistry instance;
  return instance;
}

void ModuleRegistry::CreateButtons() {
  std::map<ModuleId, ImGuiHelper::ButtonGroupItem> items;
  for (const auto& entry : m_Entries) {
    const char* name = entry.Panel->GetName();
    const char* nameHint = entry.Panel->GetNameHint();
    items.emplace(entry.Id, ImGuiHelper::ButtonGroupItem{
                                name != nullptr ? name : "", nameHint != nullptr ? nameHint : "",
                                ImGuiHelper::ButtonStatus::BUTTON_STATUS_DEFAULT, "", true, true,
                                ImGuiHelper::ItemType::CheckableButton});
  }
  // A button group needs at least one item.
  if (!items.empty()) {
    m_Buttons = std::make_unique<ImGuiHelper::TabGroup<ModuleId>>(items, false, true, true);
  }
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
        const std::string id = registration.Id;
        const auto sameId = [&id](const Entry& entry) { return SameName(entry.Id, id); };
        if (std::ranges::any_of(m_Entries, sameId)) {
          LOG_ERROR << "ModuleRegistry: " << path << " uses the module id " << id
                    << " of another module";
          continue;
        }
        ModulePanel panel(registration.Create());
        if (panel == nullptr) {
          LOG_ERROR << "ModuleRegistry: " << path << " did not create a panel";
          continue;
        }
        m_Entries.push_back(Entry{id, std::move(panel)});
      } catch (const std::exception& exception) {
        LOG_ERROR << "ModuleRegistry: Creating " << path << " failed: " << exception.what();
      }
    }
  } catch (const std::exception& exception) {
    LOG_ERROR << "ModuleRegistry: Loading modules failed: " << exception.what();
  }
#endif
  CreateButtons();
}

void ModuleRegistry::CreateModules() {
  if (m_ModulesLoaded) {
    return;
  }
  LoadModules();
  LOG_INFO << "ModuleRegistry: Created " << m_Entries.size() << " modules";
}

void ModuleRegistry::DestroyModules() {
  m_Buttons.reset();
  // Reverse load order, mirroring how members of a panel would go.
  while (!m_Entries.empty()) {
    m_Entries.pop_back();
  }
}

std::vector<ModuleId> ModuleRegistry::Ids() const {
  std::vector<ModuleId> ids;
  for (const auto& entry : m_Entries) {
    ids.push_back(entry.Id);
  }
  return ids;
}

ModuleBasePanel* ModuleRegistry::Find(const ModuleId& id) const {
  const auto it = std::ranges::find(m_Entries, id, &Entry::Id);
  return it != m_Entries.end() ? it->Panel.get() : nullptr;
}

bool ModuleRegistry::SetActive(const ModuleId& id, bool active) {
  auto* panel = Find(id);
  if (panel == nullptr) {
    return false;
  }
  SetModuleActive(*panel, active);
  m_Buttons->SetChecked(id, panel->IsActive());
  return panel->IsActive() == active;
}

bool ModuleRegistry::EnableModule(const ModuleId& id) {
  return SetActive(id, true);
}

bool ModuleRegistry::DisableModule(const ModuleId& id) {
  return SetActive(id, false);
}

void ModuleRegistry::AddActivePlugins(GitsAction action, Api api, Configuration& config) const {
  auto& plugins =
      action == GitsAction::CAPTURE ? config.common.recorder.plugins : config.common.player.plugins;
  for (const auto& entry : m_Entries) {
    if (!entry.Panel->IsActive()) {
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
  if (m_Buttons == nullptr) {
    return false;
  }
  ImGui::Text("Modules");
  const bool changed = m_Buttons->Render();
  if (changed) {
    // A click is a request; the panel decides, and the button shows what it decided.
    for (const auto& entry : m_Entries) {
      SetActive(entry.Id, m_Buttons->IsChecked(entry.Id));
    }
  }
  return changed;
}
} // namespace gits::gui
