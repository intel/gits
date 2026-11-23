// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandSerializer.h"
#include "command.h"

namespace gits {
namespace DirectX {

std::unique_ptr<stream::CommandSerializer> createCommandSerializer(const Command* command);

} // namespace DirectX
} // namespace gits
