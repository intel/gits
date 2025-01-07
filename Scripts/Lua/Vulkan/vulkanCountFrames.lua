-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- ***************************************************************
-- ******************* !!!! DESCRIPTION !!! **********************
-- ***************************************************************

-- Script counts number of frames displayed in a stream.

-- ***************************************************************
-- ********************* !!!! GLOBALS !!!!  **********************
-- ***************************************************************

frameNr = 0

-- ***************************************************************
-- ********************* !!!! ACTIONS !!!! ***********************
-- ***************************************************************

function frameCountUp()
  frameNr = frameNr + 1
end

function frameCountDown()
  frameNr = frameNr - 1
end

-- ***************************************************************
-- ****************** !!!! VULKAN METHODS !!!! *******************
-- ***************************************************************

function vkQueuePresentKHR(queue, presentInfo)
  frameCountUp()
  drvVk.vkQueuePresentKHR(queue, presentInfo)
end

function vkGetDeviceProcAddr(device, name)
  if gits.udtToStr(name) == "vkUnwindQueuePresentGITS" then
    frameCountDown()
  end
end

-- ***************************************************************
-- ******************* !!!! GITS EVENTS !!!! *********************
-- ***************************************************************

function gitsProgramStart()
end

function gitsProgramExit()
  print(' -- ')
  print('Frame count: ' .. frameNr)
  print(' -- ')
end

function gitsStateRestoreBegin()
end

function gitsStateRestoreEnd()
end
