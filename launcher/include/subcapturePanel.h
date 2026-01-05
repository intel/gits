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

namespace gits::gui {

class SubcapturePanel : public BasePanel {
public:
  using BasePanel::BasePanel; // boiler-plate constructors be gone!

  void Render() override;

  const std::string GetCLIArguments() const;

private:
  void RowSubcapturePath();

  struct SubcaptureConfig {
    bool Enabled = true;
    int StartFrame = 0;
    int EndFrame = 0;
    bool Optimize = false;
    bool OptimizeRay = false;

    std::string Range() const {
      return std::to_string(StartFrame) + "-" + std::to_string(EndFrame);
    }
  } SubcaptureConfig;
};

} // namespace gits::gui
