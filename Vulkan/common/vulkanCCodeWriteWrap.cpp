// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanCCodeWriteWrap.cpp
*
* @brief Manual overrides of CArgument::Write(CCodeOStream &stream) methods. Applies for Vulkan API.
*
* These overrides are used when the regular ccodeWrap mechanism is not enough.
* For example, when the *_wrap function has different parameters from the API
* call it wraps. The manual overrides allow us to write anything to the CCode.
*/

#include "vulkanCCodeWriteWrap.h"

#include <cstddef>
#include <iostream>

namespace gits {
namespace Vulkan {

void CvkMapMemory_CCODEWRITEWRAP(CCodeOStream& stream, const CvkMapMemory& function) {
  stream.Indent() << "vkMapMemory_CCODEWRAP(" << function._device << ", " << function._memory
                  << ", " << function._offset << ", " << function._size << ", " << function._flags
                  << ");" << std::endl;
}

void CvkGetFenceStatus_CCODEWRITEWRAP(CCodeOStream& stream, const CvkGetFenceStatus& function) {
  stream.Indent() << "vkGetFenceStatus_CCODEWRAP(" << function._return_value << ", "
                  << function._device << ", " << function._fence << ");" << std::endl;
}

// This wrapper is not meant to print a CvkUpdateDescriptorSets_CCODEWRAP,
// but to print multiple separate `CvkUpdateDescriptorSets` calls
// instead of one giant call.
void CvkUpdateDescriptorSets_CCODEWRITEWRAP(CCodeOStream& stream,
                                            const CvkUpdateDescriptorSets& function) {
  size_t chunkSize = Config::Get().recorder.vulkan.utilities.maxArraySizeForCCode;
  size_t itDescriptorWrite = 0, itDescriptorCopy = 0;

  while (itDescriptorWrite < *function._descriptorWriteCount ||
         itDescriptorCopy < *function._descriptorCopyCount) {
    size_t sizeDescriptorWrite =
        CalculateChunkSize(*function._descriptorWriteCount, chunkSize, itDescriptorWrite);
    size_t sizeDescriptorCopy =
        CalculateChunkSize(*function._descriptorCopyCount, chunkSize, itDescriptorCopy);

    StartScope(stream);

    function._pDescriptorWrites.VariableNameRegister(stream, false);
    function._pDescriptorWrites.Declare(stream, itDescriptorWrite,
                                        itDescriptorWrite + sizeDescriptorWrite);
    function._pDescriptorCopies.VariableNameRegister(stream, false);
    function._pDescriptorCopies.Declare(stream, itDescriptorCopy,
                                        itDescriptorCopy + sizeDescriptorCopy);
    stream.Indent() << function.Name() << "(" << function._device << ", " << sizeDescriptorWrite
                    << ", " << function._pDescriptorWrites << ", " << sizeDescriptorCopy << ", "
                    << function._pDescriptorCopies << ");" << std::endl;

    EndScope(stream);

    itDescriptorWrite += chunkSize;
    itDescriptorCopy += chunkSize;
  }
}

void CvkWaitForFences_CCODEWRITEWRAP(CCodeOStream& stream, const CvkWaitForFences& function) {
  StartScope(stream);
  RegisterAndDeclareIfNeeded(stream, function._return_value);
  RegisterAndDeclareIfNeeded(stream, function._device);
  RegisterAndDeclareIfNeeded(stream, function._fenceCount);
  RegisterAndDeclareIfNeeded(stream, function._pFences);
  RegisterAndDeclareIfNeeded(stream, function._waitAll);
  RegisterAndDeclareIfNeeded(stream, function._timeout);

  stream.Indent() << "vkWaitForFences_CCODEWRAP(" << function._return_value << ", "
                  << function._device << ", " << function._fenceCount << ", " << function._pFences
                  << ", " << function._waitAll << ", " << function._timeout << ");" << std::endl;

  EndScope(stream);
}

void CvkCreateGraphicsPipelines_CCODEWRITEWRAP(CCodeOStream& stream,
                                               const CvkCreateGraphicsPipelines& function) {
  size_t chunkSize = Config::Get().recorder.vulkan.utilities.maxArraySizeForCCode;
  auto createInfoCount = *function._createInfoCount;

  for (size_t i = 0; i < createInfoCount; i += chunkSize) {
    size_t size = chunkSize;
    if (i + chunkSize > createInfoCount) {
      size = createInfoCount - i;
    }

    StartScope(stream);

    function._pCreateInfos.VariableNameRegister(stream, false);
    function._pCreateInfos.Declare(stream, i, i + size);
    function._pPipelines.VariableNameRegister(stream, false);
    function._pPipelines.Declare(stream, i, i + size);

    auto returnVal = function.Return();
    returnVal->VariableNameRegister(stream, true);

    stream.Indent() << returnVal->Name() << " " << stream.VariableName(returnVal->ScopeKey())
                    << " = " << function.Name() << "(" << function._device << ", "
                    << function._pipelineCache << ", " << size << ", " << function._pCreateInfos
                    << ", " << function._pAllocator << ", " << function._pPipelines << ");"
                    << std::endl;

    function._pPipelines.PostAction(stream, i, i + size);

    EndScope(stream);
  }
}

void CvkCmdPipelineBarrier_CCODEWRITEWRAP(CCodeOStream& stream,
                                          const CvkCmdPipelineBarrier& function) {
  size_t chunkSize = Config::Get().recorder.vulkan.utilities.maxArraySizeForCCode;
  size_t itMemoryBarriers = 0, itBufferMemoryBarriers = 0, itImageMemoryBarriers = 0;
  size_t memoryBarrierCount = *function._memoryBarrierCount;
  size_t bufferMemoryBarrierCount = *function._bufferMemoryBarrierCount;
  size_t imageMemoryBarrierCount = *function._imageMemoryBarrierCount;

  while (itMemoryBarriers < memoryBarrierCount ||
         itBufferMemoryBarriers < bufferMemoryBarrierCount ||
         itImageMemoryBarriers < imageMemoryBarrierCount) {
    size_t sizeMemoryBarrier = CalculateChunkSize(memoryBarrierCount, chunkSize, itMemoryBarriers);
    size_t sizeBufferMemoryBarrier =
        CalculateChunkSize(bufferMemoryBarrierCount, chunkSize, itBufferMemoryBarriers);
    size_t sizeImageMemoryBarrier =
        CalculateChunkSize(imageMemoryBarrierCount, chunkSize, itImageMemoryBarriers);

    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
    function._pMemoryBarriers.VariableNameRegister(stream, false);
    function._pMemoryBarriers.Declare(stream, itMemoryBarriers,
                                      itMemoryBarriers + sizeMemoryBarrier);
    function._pBufferMemoryBarriers.VariableNameRegister(stream, false);
    function._pBufferMemoryBarriers.Declare(stream, itBufferMemoryBarriers,
                                            itBufferMemoryBarriers + sizeBufferMemoryBarrier);
    function._pImageMemoryBarriers.VariableNameRegister(stream, false);
    function._pImageMemoryBarriers.Declare(stream, itImageMemoryBarriers,
                                           itImageMemoryBarriers + sizeImageMemoryBarrier);
    stream.Indent() << "vkCmdPipelineBarrier(" << function._commandBuffer << ", "
                    << function._srcStageMask << ", " << function._dstStageMask << ", "
                    << function._dependencyFlags << ", " << sizeMemoryBarrier << ", "
                    << function._pMemoryBarriers << ", " << sizeBufferMemoryBarrier << ", "
                    << function._pBufferMemoryBarriers << ", " << sizeImageMemoryBarrier << ", "
                    << function._pImageMemoryBarriers << ");" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;

    itMemoryBarriers += chunkSize;
    itBufferMemoryBarriers += chunkSize;
    itImageMemoryBarriers += chunkSize;
  }
}

void CvkCmdPipelineBarrier2_CCODEWRITEWRAP(CCodeOStream& stream,
                                           const CvkCmdPipelineBarrier2& function) {
  CVkDependencyInfoCCodeWriter(stream, function.Name(), function._commandBuffer,
                               function._pDependencyInfo);
}

void CvkCmdPipelineBarrier2KHR_CCODEWRITEWRAP(CCodeOStream& stream,
                                              const CvkCmdPipelineBarrier2KHR& function) {
  CVkDependencyInfoCCodeWriter(stream, function.Name(), function._commandBuffer,
                               function._pDependencyInfo);
}

void CvkCmdPipelineBarrier2UnifiedGITS_CCODEWRITEWRAP(
    CCodeOStream& stream, const CvkCmdPipelineBarrier2UnifiedGITS& function) {
  CVkDependencyInfoCCodeWriter(stream, function.Name(), function._commandBuffer,
                               function._pDependencyInfo);
}

void CVkDependencyInfoCCodeWriter(CCodeOStream& stream,
                                  const std::string& name,
                                  const CVkCommandBuffer& commandBuffer,
                                  const CVkDependencyInfo& dependencyInfo) {
  size_t chunkSize = Config::Get().recorder.vulkan.utilities.maxArraySizeForCCode;
  size_t itMemoryBarriers = 0, itBufferMemoryBarriers = 0, itImageMemoryBarriers = 0;
  size_t memoryBarrierCount = dependencyInfo.GetMemoryBarrierCount();
  size_t bufferMemoryBarrierCount = dependencyInfo.GetBufferMemoryBarrierCount();
  size_t imageMemoryBarrierCount = dependencyInfo.GetImageMemoryBarrierCount();

  while (itMemoryBarriers < memoryBarrierCount ||
         itBufferMemoryBarriers < bufferMemoryBarrierCount ||
         itImageMemoryBarriers < imageMemoryBarrierCount) {
    size_t sizeMemoryBarrier = CalculateChunkSize(memoryBarrierCount, chunkSize, itMemoryBarriers);
    size_t sizeBufferMemoryBarrier =
        CalculateChunkSize(bufferMemoryBarrierCount, chunkSize, itBufferMemoryBarriers);
    size_t sizeImageMemoryBarrier =
        CalculateChunkSize(imageMemoryBarrierCount, chunkSize, itImageMemoryBarriers);

    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
    dependencyInfo.VariableNameRegister(stream, false);
    dependencyInfo.Declare(stream, itMemoryBarriers, itMemoryBarriers + sizeMemoryBarrier,
                           itBufferMemoryBarriers, itBufferMemoryBarriers + sizeBufferMemoryBarrier,
                           itImageMemoryBarriers, itImageMemoryBarriers + sizeImageMemoryBarrier);
    stream.Indent() << name << "(" << commandBuffer << ", "
                    << "&" << dependencyInfo << ");" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;

    itMemoryBarriers += chunkSize;
    itBufferMemoryBarriers += chunkSize;
    itImageMemoryBarriers += chunkSize;
  }
}
} // namespace Vulkan
} // namespace gits
