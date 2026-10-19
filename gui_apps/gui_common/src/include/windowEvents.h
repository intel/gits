// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"

#include <filesystem>

namespace gits::gui {
class WindowEvents {
public:
  virtual void OnResize(ImVec2 size){};
  virtual void OnPosition(ImVec2 position){};
  virtual void OnFileDrop(const std::filesystem::path& filePath){};
};

} // namespace gits::gui
