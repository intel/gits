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

-- Ignores initial pipeline cache data


-- ***************************************************************
-- ****************** !!!! VULKAN METHODS !!!! *******************
-- ***************************************************************

function vkCreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache)
  return drvVk.vkCreatePipelineCache(device, gits.luaStrToCStr("\x11\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"), pAllocator, pPipelineCache)
end
