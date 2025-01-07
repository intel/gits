// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

namespace gits {
namespace Vulkan {
% for name in sorted(vulkan_mapped_types) + sorted(vulkan_mapped_types_nondisp):
    typedef CVulkanObj<${name}> C${name};
% endfor

} // namespace Vulkan
} // namespace gits
