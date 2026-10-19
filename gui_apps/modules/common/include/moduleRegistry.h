// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "moduleBasePanel.h"
#include "tabGroup.h"

#include <memory>
#include <vector>

namespace gits {
struct Configuration;
} // namespace gits

namespace gits::gui {

enum class ModuleId {
  Rtsr
};

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

  ModuleBasePanel* Find(ModuleId id) const;

  bool EnableModule(ModuleId id);
  bool DisableModule(ModuleId id);

  void AddActivePlugins(GitsAction action, Api api, Configuration& config) const;

  bool RenderModuleButtons();
  ImGuiHelper::TabGroup<ModuleId>* BtnsModules{nullptr};

private:
  ModuleRegistry();

  struct Entry {
    ModuleId Id;
    Factory Create;
    std::unique_ptr<ModuleBasePanel> Panel;
  };

  std::map<ModuleId, ImGuiHelper::ButtonGroupItem> m_EntryMap;

  std::vector<Entry> m_Entries;
  std::vector<ModuleBasePanel*> m_Panels;
};

} // namespace gits::gui

#define GITS_GUI_REGISTER_MODULE(id, PanelType, name, nameHint)                                    \
  extern "C" {                                                                                     \
  bool gitsGuiModule_##id = ::gits::gui::ModuleRegistry::Register(                                 \
      ::gits::gui::ModuleId::id,                                                                   \
      []() -> std::unique_ptr<::gits::gui::ModuleBasePanel> {                                      \
        return std::make_unique<PanelType>();                                                      \
      },                                                                                           \
      name,                                                                                        \
      nameHint);                                                                                   \
  }
