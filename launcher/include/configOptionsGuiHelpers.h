// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "common.h"
#include "configMetaDataAuto.h"

namespace gits::gui::config_options_gui_helpers {

void ConfigOptionHelpButton(const ConfigFieldMetadata& option);

bool Trace(Mode mode, float indentation = 0.0f);

} // namespace gits::gui::config_options_gui_helpers
