// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "moduleBasePanel.h"

namespace gits::gui {

// Root panel of the smallest Studio module. It uses what a module shares with Studio: ImGui,
// the EventBus, the BaseContext and the log.
class HelloPanel : public ModuleBasePanel {
public:
  HelloPanel();

  void Render() override;

  const char* GetName() const override;
  const char* GetNameHint() const override;

  bool Activate() override;
  bool Deactivate() override;
  bool IsActive() override;

private:
  bool m_Active{false};
  int m_ThemeChanges{0};
};

} // namespace gits::gui
