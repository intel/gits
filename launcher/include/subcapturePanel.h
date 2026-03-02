// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "eventBus.h"

#include <string>

namespace gits::gui {

class SubcapturePanel : public BasePanel {
public:
  SubcapturePanel();

  void Render() override;

  const std::string GetCLIArguments() const;

private:
  void RowSubcapturePath();

  struct SubcaptureConfig {
    bool Enabled = true;
    bool Optimize = true;
    bool ExecutionSerialization = false;
    int StartFrame = 1;
    int EndFrame = 1;

    std::string Range() const {
      return std::to_string(StartFrame) + "-" + std::to_string(EndFrame);
    }
  } SubcaptureConfig;

  // Event callbacks
  void PathCallback(const Event& e);
};

} // namespace gits::gui
