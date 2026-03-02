// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits::gui {

class BasePanel {
public:
  explicit BasePanel() {}
  virtual ~BasePanel() = default;

  virtual void Render() = 0;
};

} // namespace gits::gui
