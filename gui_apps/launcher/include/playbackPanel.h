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

namespace gits::gui {

class PlaybackPanel : public BasePanel {
public:
  PlaybackPanel();

  void Render() override;
  void RenderSimple();

private:
  void RowStreamPath();
  void RowConfigPath();
  void RowArguments();

  std::optional<std::filesystem::path> DroppedFilePath;

  // Event callbacks
  void PathCallback(const Event& e);
  void FileDropCallback(const Event& e);
  void ContextCallback(const Event& e);
};

} // namespace gits::gui
