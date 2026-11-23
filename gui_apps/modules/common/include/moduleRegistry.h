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
  static ModuleRegistry& GetInstance();

  // Loads the module DLLs and creates their panels, once.
  void CreateModules();
  void DestroyModules();

  size_t Count() const {
    return m_Entries.size();
  }
  std::vector<ModuleId> Ids() const;

  ModuleBasePanel* Find(const ModuleId& id) const;

  bool EnableModule(const ModuleId& id);
  bool DisableModule(const ModuleId& id);

  void AddActivePlugins(GitsAction action, Api api, Configuration& config) const;

  // Draws the sidebar buttons; true when the user changed a checkbox or the selection.
  bool RenderModuleButtons();
  // The sidebar buttons, one per module; null while no module is loaded.
  ImGuiHelper::TabGroup<ModuleId>* Buttons() const {
    return m_Buttons.get();
  }

private:
  ModuleRegistry() = default;

  void LoadModules();
  void CreateButtons();
  // Activates or deactivates the module and shows the result on its button. Returns whether the
  // module is in the requested state.
  bool SetActive(const ModuleId& id, bool active);

  // The panel's virtual destructor lives in the module, so the module's heap frees it.
  using ModulePanel = std::unique_ptr<ModuleBasePanel>;

  struct Entry {
    ModuleId Id;
    ModulePanel Panel;
  };

  std::vector<Entry> m_Entries;
  std::unique_ptr<ImGuiHelper::TabGroup<ModuleId>> m_Buttons;
  bool m_ModulesLoaded{false};
};

// What a module DLL hands to Studio. Its name and hint come from the panel.
struct ModuleRegistration {
  const char* Id{};
  std::unique_ptr<ModuleBasePanel> (*Create)(){};
};

} // namespace gits::gui

// Defines GitsGuiModule, the function a module DLL exports. Studio calls it right after loading
// the DLL; it binds the module to Studio and returns the module's id and a factory for its panel.
#define GITS_GUI_REGISTER_MODULE(id, PanelType)                                                    \
  extern "C" __declspec(dllexport) void GitsGuiModule(                                             \
      const ::gits::gui::ModuleContext* context, ::gits::gui::ModuleRegistration* registration) {  \
    ::gits::gui::BindModule(*context);                                                             \
    *registration = {id, []() -> std::unique_ptr<::gits::gui::ModuleBasePanel> {                   \
                       return std::make_unique<PanelType>();                                       \
                     }};                                                                           \
  }
