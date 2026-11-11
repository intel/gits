// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "moduleContext.h"

#include "baseContext.h"
#include "baseEventBus.h"
#include "log.h"

namespace gits::gui {

void BindModule(const ModuleContext& context) {
  ImGui::SetAllocatorFunctions(context.ImGuiAlloc, context.ImGuiFree, context.ImGuiAllocUserData);
  ImGui::SetCurrentContext(context.ImGui);
  gits::log::Initialize(context.LogLevel, context.LogAppender);
  EventBus::SetInstance(*context.Events);
  BaseContext::SetBaseInstance(*context.Base);
}

} // namespace gits::gui
