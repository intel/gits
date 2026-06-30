// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "fastOStream.h"
#include "vulkanHeader2.h"

namespace gits {
namespace vulkan {

FastOStream& operator<<(FastOStream& stream, const VkGraphicsPipelineCreateInfo& value);
FastOStream& operator<<(FastOStream& stream, const VkGraphicsPipelineCreateInfo* value);

FastOStream& operator<<(FastOStream& stream, const VkWriteDescriptorSet& value);
FastOStream& operator<<(FastOStream& stream, const VkWriteDescriptorSet* value);

FastOStream& operator<<(FastOStream& stream, const VkSubmitInfo& value);
FastOStream& operator<<(FastOStream& stream, const VkSubmitInfo* value);

} // namespace vulkan
} // namespace gits
