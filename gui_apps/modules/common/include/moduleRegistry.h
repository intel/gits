// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "moduleBasePanel.h"
#include "moduleContext.h"
#include "tabGroup.h"

#include <memory>
#include <string>
#include <vector>

namespace gits {
struct Configuration;
} // namespace gits

namespace gits::gui {

// A module's id: the first argument of its GITS_GUI_REGISTER_MODULE line, unique among modules.
using ModuleId = std::string;

class ModuleRegistry {
public:
  using Factory = std::unique_ptr<ModuleBasePanel> (*)();

  static ModuleRegistry& GetInstance();

  static bool Register(ModuleId id, Factory factory, const char* name, const char* nameHint);

  void CreateModules();
  void DestroyModules();

  const std::vector<ModuleBasePanel*>& Modules() const {
    return m_Panels;
  }
  // The ids of the created modules, in the order of Modules().
  std::vector<ModuleId> Ids() const;

  ModuleBasePanel* Find(const ModuleId& id) const;

  bool EnableModule(const ModuleId& id);
  bool DisableModule(const ModuleId& id);

  void AddActivePlugins(GitsAction action, Api api, Configuration& config) const;

  bool RenderModuleButtons();
  ImGuiHelper::TabGroup<ModuleId>* BtnsModules{nullptr};

private:
  ModuleRegistry();

  void LoadModules();

  struct Entry {
    ModuleId Id;
    Factory Create;
    std::unique_ptr<ModuleBasePanel> Panel;
  };

  std::map<ModuleId, ImGuiHelper::ButtonGroupItem> m_EntryMap;

  std::vector<Entry> m_Entries;
  std::vector<ModuleBasePanel*> m_Panels;
  bool m_ModulesLoaded{false};
};

// What a module DLL hands to Studio. The strings live in the DLL, which Studio never unloads.
struct ModuleRegistration {
  const char* Id{};
  ModuleRegistry::Factory Create{};
  const char* Name{};
  const char* NameHint{};
};

using ModuleEntryPoint = void (*)(const ModuleContext* context, ModuleRegistration* registration);

} // namespace gits::gui

// Defines GitsGuiModule, the one function a module DLL exports. Studio calls it right after loading
// the DLL; it binds the module to Studio and returns what Studio registers.
#define GITS_GUI_REGISTER_MODULE(id, PanelType, name, nameHint)                                    \
  extern "C" __declspec(dllexport) void GitsGuiModule(                                             \
      const ::gits::gui::ModuleContext* context, ::gits::gui::ModuleRegistration* registration) {  \
    ::gits::gui::BindModule(*context);                                                             \
    *registration = {id,                                                                           \
                     []() -> std::unique_ptr<::gits::gui::ModuleBasePanel> {                       \
                       return std::make_unique<PanelType>();                                       \
                     },                                                                            \
                     name, nameHint};                                                              \
  }
