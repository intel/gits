// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

#include <string>
#include <unordered_set>

namespace gits {
namespace vulkan {

// Parses a comma separated list of keys written as the trace prints them: decimal, with an
// optional O prefix of object keys and an S prefix of state restore keys. Throws
// std::invalid_argument for anything else.
std::unordered_set<GITSKey> ParseDumpKeys(const std::string& keys);

std::string DumpKeyToString(GITSKey key);

} // namespace vulkan
} // namespace gits
