// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowGeometryMetaRunner.h"

#include "hdcVirtualizationService.h"

namespace gits::opengl2 {

void WindowGeometryMetaRunner::Run() {
  HdcVirtualizationService::Get().SetGeometry(command.m_X.Value, command.m_Y.Value,
                                              command.m_Width.Value, command.m_Height.Value,
                                              command.m_Visible.Value);
}

} // namespace gits::opengl2
