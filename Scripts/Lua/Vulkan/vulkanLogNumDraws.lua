-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
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
-- ********************* !!!! GLOBALS !!!!  **********************
-- ***************************************************************

drawNr = 0

-- ***************************************************************
-- ********************* !!!! ACTIONS !!!! ***********************
-- ***************************************************************

function drawCountUp()
  drawNr = drawNr + 1
end

-- ***************************************************************
-- ****************** !!!! VULKAN METHODS !!!! *******************
-- ***************************************************************

function vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance)
  drawCountUp()
  drvVk.vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance)
  print('Draw call no: ' .. drawNr)
end

function vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)
  drawCountUp()
  drvVk.vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)
  print('Draw call no: ' .. drawNr)
end

function vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride)
  drawCountUp()
  drvVk.vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride)
  print('Draw call no: ' .. drawNr)
end

function vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride)
  drawCountUp()
  drvVk.vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride)
  print('Draw call no: ' .. drawNr)
end

function vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ)
  drawCountUp()
  drvVk.vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ)
  print('Draw call no: ' .. drawNr)
end

function vkCmdDispatchIndirect(commandBuffer, buffer, offset)
  drawCountUp()
  drvVk.vkCmdDispatchIndirect(commandBuffer, buffer, offset)
  print('Draw call no: ' .. drawNr)
end

function vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions)
  drawCountUp()
  drvVk.vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions)
  print('Draw call no: ' .. drawNr)
end

function vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  drawCountUp()
  drvVk.vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  print('Draw call no: ' .. drawNr)
end

function vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter)
  drawCountUp()
  drvVk.vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter)
  print('Draw call no: ' .. drawNr)
end

function vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions)
  drawCountUp()
  drvVk.vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions)
  print('Draw call no: ' .. drawNr)
end

function vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions)
  drawCountUp()
  drvVk.vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions)
  print('Draw call no: ' .. drawNr)
end

function vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData)
  drawCountUp()
  drvVk.vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData)
  print('Draw call no: ' .. drawNr)
end

function vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data)
  drawCountUp()
  drvVk.vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data)
  print('Draw call no: ' .. drawNr)
end

function vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges)
  drawCountUp()
  drvVk.vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges)
  print('Draw call no: ' .. drawNr)
end

function vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges)
  drawCountUp()
  drvVk.vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges)
  print('Draw call no: ' .. drawNr)
end

function vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects)
  drawCountUp()
  drvVk.vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects)
  print('Draw call no: ' .. drawNr)
end

function vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  drawCountUp()
  drvVk.vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions)
  print('Draw call no: ' .. drawNr)
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
