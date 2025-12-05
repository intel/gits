#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_vulkan_base import *

Function(name='vkPassPhysicalDeviceMemoryPropertiesGITS', enabled=True, type=FuncType.PARAM, stateTrack=True, runWrap=True, recWrap=True, customDriver=True, level=FuncLevel.INSTANCE,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pMemoryProperties', type='const VkPhysicalDeviceMemoryProperties*')
)

Function(name='vkGetBufferDeviceAddressUnifiedGITS', enabled=True, type=FuncType.PARAM, stateTrack=True, recWrap=True, runWrap=True,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferOpaqueCaptureAddressUnifiedGITS', enabled=False, type=FuncType.NONE,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetDeviceMemoryOpaqueCaptureAddressUnifiedGITS', enabled=False, type=FuncType.PARAM,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceMemoryOpaqueCaptureAddressInfo*')
)

Function(name='vkCmdPipelineBarrier2UnifiedGITS', enabled=True, type=FuncType.PARAM, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pDependencyInfo', type='const VkDependencyInfo*')
)

Function(name='vkTagMemoryContentsUpdateGITS', enabled=False, type=FuncType.PARAM, recWrap=True, customDriver=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory'),
arg3=ArgDef(name='regionCount', type='uint32_t'),
arg4=ArgDef(name='pRegions', type='const VkBufferCopy*', count='regionCount')
)

Function(name='vkIAmGITS', enabled=False, type=FuncType.PARAM, recExecWrap=True, stateTrack=True, level=FuncLevel.GLOBAL, pluginWrap=True,
retV=RetDef(type='void')
)

Function(name='vkGetAccelerationStructureDeviceAddressUnifiedGITS', enabled=True, type=FuncType.PARAM, stateTrack=True, recWrap=False, runWrap=True,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkAccelerationStructureDeviceAddressInfoKHR*')
)

Function(name='vkPauseRecordingGITS', enabled=False, type=FuncType.PARAM, recExecWrap=True, level=FuncLevel.GLOBAL, customDriver=True,
retV=RetDef(type='void')
)

Function(name='vkContinueRecordingGITS', enabled=False, type=FuncType.PARAM, recExecWrap=True, level=FuncLevel.GLOBAL, customDriver=True,
retV=RetDef(type='void')
)

Function(name='vkIAmRecorderGITS', enabled=False, type=FuncType.PARAM, level=FuncLevel.GLOBAL, customDriver=True, disableInPlugin=True,
retV=RetDef(type='void')
)

Function(name='vkWaitSemaphoresUnifiedGITS', enabled=False, type=FuncType.PARAM, waitOperation=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pWaitInfo', type='const VkSemaphoreWaitInfo*'),
arg3=ArgDef(name='timeout', type='uint64_t')
)

Function(name='vkGetSemaphoreCounterValueUnifiedGITS', enabled=False, type=FuncType.PARAM,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='semaphore', type='VkSemaphore'),
arg3=ArgDef(name='pValue', type='uint64_t*', wrapParams='1, pValue')
)

###############################################

Enum(name='VkFormat', enumerators = [
VarDef(name='VK_FORMAT_CUSTOM_A8_UNORM_GITS', value='2000000025'),
])

Enum(name='VkCommandExecutionSideGITS', enumerators = [
VarDef(name='VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS', value='0'),
VarDef(name='VK_COMMAND_EXECUTION_SIDE_HOST_GITS', value='1')
])

Enum(name='VkStructureType', enumerators = [
VarDef(name='VK_STRUCTURE_TYPE_ORIGINAL_SHADER_GROUP_HANDLES_GITS', value='808620001'),
VarDef(name='VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS', value='808600002'),
])

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

Struct(name='VkBufferDeviceAddressGITS_', enabled=False,
var1=VarDef(name='originalDeviceAddress', type='uint64_t'),
var2=VarDef(name='buffer', type='VkBuffer'),
var3=VarDef(name='offset', type='int64_t')
)

Struct(name='VkBufferDeviceAddressPatchGITS_', enabled=False,
var1=VarDef(name='location', type='VkBufferDeviceAddressGITS'),
var2=VarDef(name='patchedValue', type='VkBufferDeviceAddressGITS')
)

Struct(name='VkAccelerationStructureBuildControlDataGITS_', enabled=False,
var1=VarDef(name='commandBuffer', type='VkCommandBuffer'),
var2=VarDef(name='executionSide', type='VkCommandExecutionSideGITS')
)

Struct(name='VkOpacityMicromapCustomDataGITS_', enabled=False,
var1=VarDef(name='primitiveCount', type='uint32_t'),
var2=VarDef(name='controlData', type='const VkAccelerationStructureBuildControlDataGITS&')
)

Struct(name='VkStructStoragePointerGITS_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sStructStorageType', type='VkStructureType'),
var4=VarDef(name='pStructStorage', type='const void*')
)

Struct(name='VkOriginalShaderGroupHandlesGITS_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=ArgDef(name='dataSize', type='uint32_t'),
var4=ArgDef(name='pData', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='(size_t)(originalshadergrouphandlesgits->dataSize), (const uint8_t *)originalshadergrouphandlesgits->pData')
)