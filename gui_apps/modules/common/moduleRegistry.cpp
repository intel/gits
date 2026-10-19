// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "moduleRegistry.h"

#include "configurationAuto.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>

namespace gits::gui {

namespace {

// PluginService matches the configured names case-insensitively.
bool SamePluginName(const std::string& lhs, const std::string& rhs) {
  return std::ranges::equal(lhs, rhs, [](unsigned char a, unsigned char b) {
    return std::tolower(a) == std::tolower(b);
  });
}

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

void ModuleRegistry::CreateModules() {
  m_Panels.clear();
  m_Panels.reserve(m_Entries.size());
  for (auto& entry : m_Entries) {
    if (entry.Panel == nullptr) {
      entry.Panel = entry.Create();
    }
    m_Panels.push_back(entry.Panel.get());
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

ModuleBasePanel* ModuleRegistry::Find(ModuleId id) const {
  auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                         [id](const Entry& entry) { return entry.Id == id; });
  return it != m_Entries.end() ? it->Panel.get() : nullptr;
}

bool ModuleRegistry::EnableModule(ModuleId id) {
  if (BtnsModules == nullptr) {
    return false;
  }

  auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                         [id](const Entry& entry) { return entry.Id == id; });
  if (it == m_Entries.end()) {
    return false;
  }
  auto enabled = it->Panel->Activate();
  BtnsModules->SetChecked(id, enabled);

  return enabled;
}

bool ModuleRegistry::DisableModule(ModuleId id) {
  if (BtnsModules == nullptr) {
    return false;
  }

  auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                         [id](const Entry& entry) { return entry.Id == id; });
  if (it == m_Entries.end()) {
    return false;
  }
  it->Panel->Deactivate();
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
    for (const auto& plugin : entry.Panel->Plugins(api)) {
      const bool listed = std::ranges::any_of(
          plugins, [&plugin](const std::string& name) { return SamePluginName(name, plugin); });
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
            BtnsModules->SetChecked(entry.Id, entry.Panel->Activate());
          } else {
            entry.Panel->Deactivate();
          }
        }
      }
    }
  }
  return result;
}
} // namespace gits::gui
