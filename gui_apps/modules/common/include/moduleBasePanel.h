// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "baseCommon.h"

#include <string>
#include <vector>

namespace gits::gui {

// Contract implemented by the root panel of every Studio module: the module
// names itself for the host application's navigation, starts and stops its own
// runtime, and reports which GITS plugins it needs for a given API. A module
// never edits a GITS configuration: the host adds those plugins to the
// configuration of every player or capture target it launches while the module
// is active.
//
// Studio and a module DLL each have their own C++ runtime, so what these methods
// return stays owned by the module; Studio only reads it.
class ModuleBasePanel : public BasePanel {
public:
  virtual const char* GetName() const = 0;
  virtual const char* GetNameHint() const = 0;

  virtual bool Activate() = 0;
  virtual bool Deactivate() = 0;
  virtual bool IsActive() = 0;

  // GITS plugins are optional for a module, hence the empty default.
  virtual const std::vector<std::string>& Plugins(Api /*api*/) const {
    static const std::vector<std::string> none;
    return none;
  }
};

} // namespace gits::gui
