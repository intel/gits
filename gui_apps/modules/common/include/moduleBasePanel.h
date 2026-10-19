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

enum class ModuleId;

// Contract implemented by the root panel of every Studio module: the module
// names itself for the host application's navigation, starts and stops its own
// runtime, and reports which GITS plugins it needs for a given API. A module
// never edits a GITS configuration: the host adds those plugins to the
// configuration of every player or capture target it launches while the module
// is active.
class ModuleBasePanel : public BasePanel {
public:
  virtual ModuleId GetID() const = 0;
  virtual std::string GetName() const = 0;
  virtual std::string GetNameHint() const = 0;

  virtual bool Activate() = 0;
  virtual bool Deactivate() = 0;
  virtual bool IsActive() = 0;

  // GITS plugins are optional for a module, hence the empty default.
  virtual std::vector<std::string> Plugins(Api /*api*/) const {
    return {};
  }
};

} // namespace gits::gui
