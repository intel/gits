// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "baseEventBus.h"
#include "common.h"

namespace gits::gui {

struct NavigateEvent : Event {
  enum class Type {
    NavigateToPanel
  };

  SideBarItem Panel = SideBarItem::COUNT;

  NavigateEvent(SideBarItem panel) : EventType(Type::NavigateToPanel) {
    Panel = panel;
  }

  Type EventType;
};

struct StudioEvent : Event {
  enum class Type {
    LiveStreamModeChanged
  };

  StudioEvent(Type type) : EventType(type) {}

  Type EventType;
};

template <>
struct EventType<StudioEvent> {
  using Type = StudioEvent::Type;
};

} // namespace gits::gui
