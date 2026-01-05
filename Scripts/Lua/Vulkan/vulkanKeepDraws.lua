-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- ***************************************************************
-- ******************* !!!! DESCRIPTION !!! **********************
-- ***************************************************************

-- Only specified in settings Vulkan drawcalls will be played back
-- by GITS. Other drawcalls will be suppressed.  

-- ***************************************************************
-- ******************** !!!! SETTINGS !!!! ***********************
-- ***************************************************************

keepDraws = "-"

-- ***************************************************************
-- ********************* !!!! GLOBALS !!!!  **********************
-- ***************************************************************

drawNr = 0
KeepDrawsRange = gits.newBitRange(keepDraws)

-- ***************************************************************
-- ********************* !!!! ACTIONS !!!! ***********************
-- ***************************************************************

function keepChecker()
  if (gits.inBitRange(KeepDrawsRange, drawNr)) then
    return true
  end
end

function drawCountUp()
  drawNr = drawNr + 1
end

-- ***************************************************************
-- ****************** !!!! VULKAN METHODS !!!! *******************
-- ***************************************************************

function vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance)
  end
end

function vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)
  end
end

function vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride)
  end
end

function vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride)
  end
end

function vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ)
  end
end

function vkCmdDispatchIndirect(commandBuffer, buffer, offset)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdDispatchIndirect(commandBuffer, buffer, offset)
  end
end

function vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions)
  end
end

function vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  end
end

function vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter)
  end
end

function vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions)
  end
end

function vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions)
  end
end

function vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData)
  end
end

function vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data)
  end
end

function vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges)
  end
end

function vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges)
  end
end

function vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects)
  end
end

function vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  drawCountUp()
  if keepChecker() then
    drvVk.vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  end
end

-- ***************************************************************
-- ******************* !!!! GITS EVENTS !!!! *********************
-- ***************************************************************

function gitsProgramStart()
end

function gitsProgramExit()
end

function gitsStateRestoreBegin()
end

function gitsStateRestoreEnd()
end
