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
#include "textEditorWidget.h"
#include "textTreeViewer.h"

namespace gits::gui {

class TracePanel : public BasePanel {
public:
  TracePanel();

  void Render() override;

private:
  bool m_DidLoadTrace = false;
  gits::ImGuiHelper::TextEditorWidget m_TraceEditor;
  gits::ImGuiHelper::TextTreeViewer m_TextTreeViewer;

  void LoadTraceFile();

  // Event callbacks
  void ActionCallback(const Event& e);
};

} // namespace gits::gui
