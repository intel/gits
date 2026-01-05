// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits::gui {

class ISharedContext {
public:
  virtual ~ISharedContext() = default;
};

class BasePanel {
public:
  explicit BasePanel(ISharedContext& sharedContext) : sharedContext(sharedContext) {}
  virtual ~BasePanel() = default;

  virtual void Render() = 0;

  // Helper to cast to specific shared context type
  template <typename T>
  T& getSharedContext() {
    return static_cast<T&>(sharedContext);
  }

  template <typename T>
  const T& getSharedContext() const {
    return static_cast<const T&>(sharedContext);
  }

protected:
  ISharedContext& sharedContext;
};

} // namespace gits::gui
