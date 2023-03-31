#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_vulkan_base import *

Function(name='vkPassPhysicalDeviceMemoryPropertiesGITS', enabled=True, type=Param, stateTrack=True, runWrap=True, recExecWrap=True, recWrap=True, customDriver=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pMemoryProperties', type='VkPhysicalDeviceMemoryProperties*')
)

Function(name='vkGetBufferDeviceAddressUnifiedGITS', enabled=True, type=Param, stateTrack=True, recWrap=True, runWrap=True,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferOpaqueCaptureAddressUnifiedGITS', enabled=False, type=None,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkCmdPipelineBarrier2UnifiedGITS', enabled=True, type=Param, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer", ccodeWriteWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pDependencyInfo', type='const VkDependencyInfo*')
)

Function(name='vkTagMemoryContentsUpdateGITS', enabled=False, type=Param, recWrap=True, customDriver=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory'),
arg3=ArgDef(name='regionCount', type='uint32_t'),
arg4=ArgDef(name='pRegions', type='const VkBufferCopy*', count='regionCount')
)

Function(name='vkIAmGITS', enabled=False, type=Param, recExecWrap=True, stateTrack=True, level=GlobalLevel, pluginWrap=True,
retV=RetDef(type='void')
)

###############################################

Struct(name='VkInitializeImageGITS_', enabled=True, declareArray=True,
var1=VarDef(name='bufferOffset', type='VkDeviceSize'),
var2=VarDef(name='stencilBufferOffset', type='VkDeviceSize'),
var3=VarDef(name='imageExtent', type='VkExtent3D'),
var4=VarDef(name='imageMipLevel', type='uint32_t'),
var5=VarDef(name='imageArrayLayer', type='uint32_t'),
var6=VarDef(name='imageLayout', type='VkImageLayout')
)

Struct(name='VkInitializeImageDataGITS_', enabled=True, declareArray=True,
var1=VarDef(name='image', type='VkImage'),
var2=VarDef(name='layout', type='VkImageLayout'),
var3=VarDef(name='copyRegionsCount', type='uint32_t'),
var4=VarDef(name='pCopyRegions', type='const VkBufferImageCopy*', wrapType='CVkBufferImageCopyArray', wrapParams='initializeimagedatagits->copyRegionsCount, initializeimagedatagits->pCopyRegions', count='copyRegionsCount'),
var5=VarDef(name='initializeRegionsCount', type='uint32_t'),
var6=VarDef(name='pInitializeRegions', type='const VkInitializeImageGITS*', wrapType='CVkInitializeImageGITSArray', wrapParams='initializeimagedatagits->initializeRegionsCount, initializeimagedatagits->pInitializeRegions', count='initializeRegionsCount')
)

Struct(name='VkInitializeBufferDataGITS_', enabled=True, declareArray=True,
var1=VarDef(name='buffer', type='VkBuffer'),
var2=VarDef(name='bufferCopy', type='VkBufferCopy')
)
