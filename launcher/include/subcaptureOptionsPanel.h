// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include <string>
#include <optional>
#include <filesystem>

namespace gits::gui {

struct Event;

class SubcaptureOptionsPanel : public BasePanel {
public:
  SubcaptureOptionsPanel();

  void Render() override;

private:
  void RowSubcapturePath();

  // Struct to store options
  struct SubcaptureConfig {
    int StartFrame = 1;
    int EndFrame = 1;
    int CommandListSubcaptureFrame = 1;
    int CommandListExecutionsStart = 1;
    int CommandListExecutionsEnd = 1;

    std::string FramesRange() const {
      return std::to_string(StartFrame) + "-" + std::to_string(EndFrame);
    }

    std::string CommandListExecutionsRange() const {
      return std::to_string(CommandListExecutionsStart) + "-" +
             std::to_string(CommandListExecutionsEnd);
    }
  } SubcaptureConfig;
  std::optional<std::filesystem::path> DroppedFilePath;

  // Callbacks
  void ContextCallback(const Event& e);
  void PathCallback(const Event& e);
  void FileDropCallback(const Event& e);
};

} // namespace gits::gui
