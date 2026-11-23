// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "fastOStream.h"
#include "vulkanHeader2.h"

namespace gits {
namespace vulkan {

FastOStream& PrintPNext(FastOStream& stream, const void* pNext);

} // namespace vulkan
} // namespace gits
