// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "pragmas.h"

DISABLE_WARNINGS
#include <boost/property_tree/ptree_fwd.hpp>
ENABLE_WARNINGS

namespace pt = boost::property_tree;

namespace gits {
void gather_diagnostic_info(nlohmann::ordered_json& node);
}
