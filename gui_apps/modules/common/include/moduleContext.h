// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "baseContext.h"
#include "baseEventBus.h"
#include "imgui.h"
#include "log.h"

namespace gits::gui {

// Studio's objects a module DLL binds to when Studio loads it. They outlive the module.
struct ModuleContext {
  ImGuiContext* ImGui = nullptr;
  ImGuiMemAllocFunc ImGuiAlloc = nullptr;
  ImGuiMemFreeFunc ImGuiFree = nullptr;
  void* ImGuiAllocUserData = nullptr;
  plog::IAppender* LogAppender = nullptr;
  gits::LogLevel LogLevel{};
  EventBus* Events = nullptr;
  BaseContext* Base = nullptr;
};

// Points the module's own ImGui, logger, EventBus and BaseContext at Studio's. Runs inside the
// module, before it creates its panel.
void BindModule(const ModuleContext& context);

} // namespace gits::gui
