// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"

#include <memory>

namespace gits {
namespace DirectX {

std::unique_ptr<Command> CreateCommandCopy(const Command* command);

} // namespace DirectX
} // namespace gits
