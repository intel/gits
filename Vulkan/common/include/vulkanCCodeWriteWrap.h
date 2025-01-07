// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanCCodeWriteWrap.h
*
* @brief Manual overrides of CArgument::Write(CCodeOStream &stream) methods. Applies for Vulkan API.
*
* These overrides are used when the regular ccodeWrap mechanism is not enough.
* For example, when the *_wrap function has different parameters from the API
* call it wraps. The manual overrides allow us to write anything to the CCode.
*/

#pragma once

#include "ccodeWriteWrap.h"
#include "vulkanFunctions.h"

namespace gits {
namespace Vulkan {
//CCODEWRITEWRAP functions
void CvkMapMemory_CCODEWRITEWRAP(CCodeOStream& stream, const CvkMapMemory& function);
void CvkGetFenceStatus_CCODEWRITEWRAP(CCodeOStream& stream, const CvkGetFenceStatus& function);
void CvkUpdateDescriptorSets_CCODEWRITEWRAP(CCodeOStream& stream,
                                            const CvkUpdateDescriptorSets& function);
void CvkWaitForFences_CCODEWRITEWRAP(CCodeOStream& stream, const CvkWaitForFences& function);
void CvkCreateGraphicsPipelines_CCODEWRITEWRAP(CCodeOStream& stream,
                                               const CvkCreateGraphicsPipelines& function);
void CvkCmdPipelineBarrier_CCODEWRITEWRAP(CCodeOStream& stream,
                                          const CvkCmdPipelineBarrier& function);
void CvkCmdPipelineBarrier2_CCODEWRITEWRAP(CCodeOStream& stream,
                                           const CvkCmdPipelineBarrier2& function);
void CvkCmdPipelineBarrier2KHR_CCODEWRITEWRAP(CCodeOStream& stream,
                                              const CvkCmdPipelineBarrier2KHR& function);
void CvkCmdPipelineBarrier2UnifiedGITS_CCODEWRITEWRAP(
    CCodeOStream& stream, const CvkCmdPipelineBarrier2UnifiedGITS& function);

//Helpers
void CVkDependencyInfoCCodeWriter(CCodeOStream& stream,
                                  const std::string& name,
                                  const CVkCommandBuffer& commandBuffer,
                                  const CVkDependencyInfo& dependencyInfo);
} // namespace Vulkan
} // namespace gits
