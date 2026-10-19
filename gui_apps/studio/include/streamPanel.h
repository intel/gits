// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <optional>
#include <filesystem>

#include "basePanel.h"
#include "baseEventBus.h"
#include "navBar.h"

namespace gits::gui {

class StreamPanel : public BasePanel {
public:
  StreamPanel();

  void Render() override;

  float WidthLeftColumn = 0.0f;

private:
  NavBar m_NavBar;

  void RowStreamPath();

  std::optional<std::filesystem::path> DroppedFilePath;

  // Event callbacks
  void PathCallback(const Event& e);
  void FileDropCallback(const Event& e);
  void ContextCallback(const Event& e);
  void ActionCallback(const Event& e);
};

} // namespace gits::gui
