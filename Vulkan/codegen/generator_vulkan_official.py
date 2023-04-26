#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_vulkan_base import *

Function(name='vkAcquireDrmDisplayEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='drmFd', type='int32_t'),
arg3=ArgDef(name='display', type='VkDisplayKHR')
)

Function(name='vkAcquireFullScreenExclusiveModeEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR')
)

Function(name='vkAcquireImageANDROID', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='nativeFenceFd', type='int'),
arg4=ArgDef(name='semaphore', type='VkSemaphore'),
arg5=ArgDef(name='fence', type='VkFence')
)

Function(name='vkAcquireNextImage2KHR', enabled=True, type=Param, runWrap=True, ccodeWrap=True, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pAcquireInfo', type='const VkAcquireNextImageInfoKHR*'),
arg3=ArgDef(name='pImageIndex', type='uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='1,pImageIndex')
)

Function(name='vkAcquireNextImageKHR', enabled=True, type=Param, runWrap=True, ccodeWrap=True, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR'),
arg3=ArgDef(name='timeout', type='uint64_t'),
arg4=ArgDef(name='semaphore', type='VkSemaphore'),
arg5=ArgDef(name='fence', type='VkFence'),
arg6=ArgDef(name='pImageIndex', type='uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='1,pImageIndex')
)

Function(name='vkAcquirePerformanceConfigurationINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pAcquireInfo', type='const VkPerformanceConfigurationAcquireInfoINTEL*'),
arg3=ArgDef(name='pConfiguration', type='VkPerformanceConfigurationINTEL*')
)

Function(name='vkAcquireProfilingLockKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkAcquireProfilingLockInfoKHR*')
)

Function(name='vkAcquireWinrtDisplayNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR')
)

Function(name='vkAcquireXlibDisplayEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='dpy', type='Display*'),
arg3=ArgDef(name='display', type='VkDisplayKHR')
)

Function(name='vkAllocateCommandBuffers', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pAllocateInfo', type='const VkCommandBufferAllocateInfo*'),
arg3=ArgDef(name='pCommandBuffers', type='VkCommandBuffer*', wrapType='CVkCommandBuffer::CSMapArray', wrapParams='pAllocateInfo->commandBufferCount, pCommandBuffers', count='pAllocateInfo->commandBufferCount')
)

Function(name='vkAllocateDescriptorSets', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pAllocateInfo', type='const VkDescriptorSetAllocateInfo*'),
arg3=ArgDef(name='pDescriptorSets', type='VkDescriptorSet*', wrapType='CVkDescriptorSet::CSMapArray', wrapParams='pAllocateInfo->descriptorSetCount, pDescriptorSets', count='pAllocateInfo->descriptorSetCount')
)

Function(name='vkAllocateMemory', enabled=True, type=Param, stateTrack=True, recExecWrap=True, runWrap=True, recWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pAllocateInfo', type='const VkMemoryAllocateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pMemory', type='VkDeviceMemory*', wrapType='CVkDeviceMemory::CSMapArray', wrapParams = '1, pMemory')
)

Function(name='vkBeginCommandBuffer', enabled=True, type=Param, stateTrack=True, recWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pBeginInfo', type='const VkCommandBufferBeginInfo*')
)

Function(name='vkBindAccelerationStructureMemoryNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='bindInfoCount', type='uint32_t'),
arg3=ArgDef(name='pBindInfos', type='const VkBindAccelerationStructureMemoryInfoNV*')
)

Function(name='vkBindBufferMemory', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='memory', type='VkDeviceMemory'),
arg4=ArgDef(name='memoryOffset', type='VkDeviceSize')
)

Function(name='vkBindBufferMemory2', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='bindInfoCount', type='uint32_t'),
arg3=ArgDef(name='pBindInfos', type='const VkBindBufferMemoryInfo*', wrapType='CVkBindBufferMemoryInfoArray', wrapParams='bindInfoCount, pBindInfos', count='bindInfoCount')
)

Function(name='vkBindBufferMemory2KHR', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='bindInfoCount', type='uint32_t'),
arg3=ArgDef(name='pBindInfos', type='const VkBindBufferMemoryInfo*', wrapType='CVkBindBufferMemoryInfoArray', wrapParams='bindInfoCount, pBindInfos', count='bindInfoCount')
)

Function(name='vkBindImageMemory', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='memory', type='VkDeviceMemory'),
arg4=ArgDef(name='memoryOffset', type='VkDeviceSize')
)

Function(name='vkBindImageMemory2', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='bindInfoCount', type='uint32_t'),
arg3=ArgDef(name='pBindInfos', type='const VkBindImageMemoryInfo*', wrapType='CVkBindImageMemoryInfoArray', wrapParams='bindInfoCount, pBindInfos', count='bindInfoCount')
)

Function(name='vkBindImageMemory2KHR', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='bindInfoCount', type='uint32_t'),
arg3=ArgDef(name='pBindInfos', type='const VkBindImageMemoryInfo*', wrapType='CVkBindImageMemoryInfoArray', wrapParams='bindInfoCount, pBindInfos', count='bindInfoCount')
)

Function(name='vkBindOpticalFlowSessionImageNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='session', type='VkOpticalFlowSessionNV'),
arg3=ArgDef(name='bindingPoint', type='VkOpticalFlowSessionBindingPointNV'),
arg4=ArgDef(name='view', type='VkImageView'),
arg5=ArgDef(name='layout', type='VkImageLayout')
)

Function(name='vkBindVideoSessionMemoryKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='videoSession', type='VkVideoSessionKHR'),
arg3=ArgDef(name='bindSessionMemoryInfoCount', type='uint32_t'),
arg4=ArgDef(name='pBindSessionMemoryInfos', type='const VkBindVideoSessionMemoryInfoKHR*')
)

Function(name='vkBuildAccelerationStructuresKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='infoCount', type='uint32_t'),
arg4=ArgDef(name='pInfos', type='const VkAccelerationStructureBuildGeometryInfoKHR*'),
arg5=ArgDef(name='ppBuildRangeInfos', type='const VkAccelerationStructureBuildRangeInfoKHR* const*')
)

Function(name='vkBuildMicromapsEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='infoCount', type='uint32_t'),
arg4=ArgDef(name='pInfos', type='const VkMicromapBuildInfoEXT*')
)

Function(name='vkCmdBeginConditionalRenderingEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pConditionalRenderingBegin', type='const VkConditionalRenderingBeginInfoEXT*')
)

Function(name='vkCmdBeginDebugUtilsLabelEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pLabelInfo', type='const VkDebugUtilsLabelEXT*')
)

Function(name='vkCmdBeginQuery', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='query', type='uint32_t'),
arg4=ArgDef(name='flags', type='VkQueryControlFlags')
)

Function(name='vkCmdBeginQueryIndexedEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='query', type='uint32_t'),
arg4=ArgDef(name='flags', type='VkQueryControlFlags'),
arg5=ArgDef(name='index', type='uint32_t')
)

Function(name='vkCmdBeginRenderPass', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRenderPassBegin', type='const VkRenderPassBeginInfo*'),
arg3=ArgDef(name='contents', type='VkSubpassContents')
)

Function(name='vkCmdBeginRenderPass2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRenderPassBegin', type='const VkRenderPassBeginInfo*'),
arg3=ArgDef(name='pSubpassBeginInfo', type='const VkSubpassBeginInfo*')
)

Function(name='vkCmdBeginRenderPass2KHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRenderPassBegin', type='const VkRenderPassBeginInfo*'),
arg3=ArgDef(name='pSubpassBeginInfo', type='const VkSubpassBeginInfo*')
)

Function(name='vkCmdBeginRendering', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRenderingInfo', type='const VkRenderingInfo*')
)

Function(name='vkCmdBeginRenderingKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRenderingInfo', type='const VkRenderingInfo*')
)

Function(name='vkCmdBeginTransformFeedbackEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstCounterBuffer', type='uint32_t'),
arg3=ArgDef(name='counterBufferCount', type='uint32_t'),
arg4=ArgDef(name='pCounterBuffers', type='const VkBuffer*', wrapParams='counterBufferCount, pCounterBuffers', count='counterBufferCount'),
arg5=ArgDef(name='pCounterBufferOffsets', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='counterBufferCount, pCounterBufferOffsets', count='counterBufferCount')
)

Function(name='vkCmdBeginVideoCodingKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pBeginInfo', type='const VkVideoBeginCodingInfoKHR*')
)

Function(name='vkCmdBindDescriptorBufferEmbeddedSamplersEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
arg3=ArgDef(name='layout', type='VkPipelineLayout'),
arg4=ArgDef(name='set', type='uint32_t')
)

Function(name='vkCmdBindDescriptorBuffersEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='bufferCount', type='uint32_t'),
arg3=ArgDef(name='pBindingInfos', type='const VkDescriptorBufferBindingInfoEXT*')
)

Function(name='vkCmdBindDescriptorSets', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
arg3=ArgDef(name='layout', type='VkPipelineLayout'),
arg4=ArgDef(name='firstSet', type='uint32_t'),
arg5=ArgDef(name='descriptorSetCount', type='uint32_t'),
arg6=ArgDef(name='pDescriptorSets', type='const VkDescriptorSet*', wrapType='CVkDescriptorSet::CSArray', wrapParams='descriptorSetCount, pDescriptorSets', count='descriptorSetCount'),
arg7=ArgDef(name='dynamicOffsetCount', type='uint32_t'),
arg8=ArgDef(name='pDynamicOffsets', type='const uint32_t*', wrapParams='dynamicOffsetCount, pDynamicOffsets', count='dynamicOffsetCount')
)

Function(name='vkCmdBindIndexBuffer', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='indexType', type='VkIndexType')
)

Function(name='vkCmdBindPipeline', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
arg3=ArgDef(name='pipeline', type='VkPipeline')
)

Function(name='vkCmdBindPipelineShaderGroupNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
arg3=ArgDef(name='pipeline', type='VkPipeline'),
arg4=ArgDef(name='groupIndex', type='uint32_t')
)

Function(name='vkCmdBindShadersEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stageCount', type='uint32_t'),
arg3=ArgDef(name='pStages', type='const VkShaderStageFlagBits*'),
arg4=ArgDef(name='pShaders', type='const VkShaderEXT*')
)

Function(name='vkCmdBindShadingRateImageNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='imageView', type='VkImageView'),
arg3=ArgDef(name='imageLayout', type='VkImageLayout')
)

Function(name='vkCmdBindTransformFeedbackBuffersEXT', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstBinding', type='uint32_t'),
arg3=ArgDef(name='bindingCount', type='uint32_t'),
arg4=ArgDef(name='pBuffers', type='const VkBuffer*', wrapParams='bindingCount, pBuffers', count='bindingCount'),
arg5=ArgDef(name='pOffsets', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pOffsets', count='bindingCount'),
arg6=ArgDef(name='pSizes', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pSizes', count='bindingCount')
)

Function(name='vkCmdBindVertexBuffers', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstBinding', type='uint32_t'),
arg3=ArgDef(name='bindingCount', type='uint32_t'),
arg4=ArgDef(name='pBuffers', type='const VkBuffer*', wrapParams='bindingCount, pBuffers', count='bindingCount'),
arg5=ArgDef(name='pOffsets', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pOffsets', count='bindingCount')
)

Function(name='vkCmdBindVertexBuffers2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstBinding', type='uint32_t'),
arg3=ArgDef(name='bindingCount', type='uint32_t'),
arg4=ArgDef(name='pBuffers', type='const VkBuffer*', wrapParams='bindingCount, pBuffers', count='bindingCount'),
arg5=ArgDef(name='pOffsets', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pOffsets', count='bindingCount'),
arg6=ArgDef(name='pSizes', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pSizes', count='bindingCount'),
arg7=ArgDef(name='pStrides', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pStrides', count='bindingCount')
)

Function(name='vkCmdBindVertexBuffers2EXT', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstBinding', type='uint32_t'),
arg3=ArgDef(name='bindingCount', type='uint32_t'),
arg4=ArgDef(name='pBuffers', type='const VkBuffer*', wrapParams='bindingCount, pBuffers', count='bindingCount'),
arg5=ArgDef(name='pOffsets', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pOffsets', count='bindingCount'),
arg6=ArgDef(name='pSizes', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pSizes', count='bindingCount'),
arg7=ArgDef(name='pStrides', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='bindingCount, pStrides', count='bindingCount')
)

Function(name='vkCmdBlitImage', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcImage', type='VkImage'),
arg3=ArgDef(name='srcImageLayout', type='VkImageLayout'),
arg4=ArgDef(name='dstImage', type='VkImage'),
arg5=ArgDef(name='dstImageLayout', type='VkImageLayout'),
arg6=ArgDef(name='regionCount', type='uint32_t'),
arg7=ArgDef(name='pRegions', type='const VkImageBlit*', wrapType='CVkImageBlitArray', wrapParams='regionCount, pRegions', count='regionCount'),
arg8=ArgDef(name='filter', type='VkFilter')
)

Function(name='vkCmdBlitImage2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pBlitImageInfo', type='const VkBlitImageInfo2*')
)

Function(name='vkCmdBlitImage2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pBlitImageInfo', type='const VkBlitImageInfo2*')
)

Function(name='vkCmdBuildAccelerationStructureNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkAccelerationStructureInfoNV*'),
arg3=ArgDef(name='instanceData', type='VkBuffer'),
arg4=ArgDef(name='instanceOffset', type='VkDeviceSize'),
arg5=ArgDef(name='update', type='VkBool32'),
arg6=ArgDef(name='dst', type='VkAccelerationStructureNV'),
arg7=ArgDef(name='src', type='VkAccelerationStructureNV'),
arg8=ArgDef(name='scratch', type='VkBuffer'),
arg9=ArgDef(name='scratchOffset', type='VkDeviceSize')
)

Function(name='vkCmdBuildAccelerationStructuresIndirectKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='infoCount', type='uint32_t'),
arg3=ArgDef(name='pInfos', type='const VkAccelerationStructureBuildGeometryInfoKHR*'),
arg4=ArgDef(name='pIndirectDeviceAddresses', type='const VkDeviceAddress*'),
arg5=ArgDef(name='pIndirectStrides', type='const uint32_t*'),
arg6=ArgDef(name='ppMaxPrimitiveCounts', type='const uint32_t* const*')
)

Function(name='vkCmdBuildAccelerationStructuresKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='infoCount', type='uint32_t'),
arg3=ArgDef(name='pInfos', type='const VkAccelerationStructureBuildGeometryInfoKHR*'),
arg4=ArgDef(name='ppBuildRangeInfos', type='const VkAccelerationStructureBuildRangeInfoKHR* const*')
)

Function(name='vkCmdBuildMicromapsEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='infoCount', type='uint32_t'),
arg3=ArgDef(name='pInfos', type='const VkMicromapBuildInfoEXT*')
)

Function(name='vkCmdClearAttachments', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='attachmentCount', type='uint32_t'),
arg3=ArgDef(name='pAttachments', type='const VkClearAttachment*', wrapType='CVkClearAttachmentArray', wrapParams='attachmentCount, pAttachments', count='attachmentCount'),
arg4=ArgDef(name='rectCount', type='uint32_t'),
arg5=ArgDef(name='pRects', type='const VkClearRect*', wrapType='CVkClearRectArray', wrapParams='rectCount, pRects', count='rectCount')
)

Function(name='vkCmdClearColorImage', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='imageLayout', type='VkImageLayout'),
arg4=ArgDef(name='pColor', type='const VkClearColorValue*'),
arg5=ArgDef(name='rangeCount', type='uint32_t'),
arg6=ArgDef(name='pRanges', type='const VkImageSubresourceRange*', wrapType='CVkImageSubresourceRangeArray', wrapParams='rangeCount, pRanges', count='rangeCount')
)

Function(name='vkCmdClearDepthStencilImage', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='imageLayout', type='VkImageLayout'),
arg4=ArgDef(name='pDepthStencil', type='const VkClearDepthStencilValue*'),
arg5=ArgDef(name='rangeCount', type='uint32_t'),
arg6=ArgDef(name='pRanges', type='const VkImageSubresourceRange*', wrapType='CVkImageSubresourceRangeArray', wrapParams='rangeCount, pRanges', count='rangeCount')
)

Function(name='vkCmdControlVideoCodingKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCodingControlInfo', type='const VkVideoCodingControlInfoKHR*')
)

Function(name='vkCmdCopyAccelerationStructureKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkCopyAccelerationStructureInfoKHR*')
)

Function(name='vkCmdCopyAccelerationStructureNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='dst', type='VkAccelerationStructureNV'),
arg3=ArgDef(name='src', type='VkAccelerationStructureNV'),
arg4=ArgDef(name='mode', type='VkCopyAccelerationStructureModeKHR')
)

Function(name='vkCmdCopyAccelerationStructureToMemoryKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkCopyAccelerationStructureToMemoryInfoKHR*')
)

Function(name='vkCmdCopyBuffer', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcBuffer', type='VkBuffer'),
arg3=ArgDef(name='dstBuffer', type='VkBuffer'),
arg4=ArgDef(name='regionCount', type='uint32_t'),
arg5=ArgDef(name='pRegions', type='const VkBufferCopy*', wrapType='CVkBufferCopyArray', wrapParams='regionCount, pRegions', count='regionCount')
)

Function(name='vkCmdCopyBuffer2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyBufferInfo', type='const VkCopyBufferInfo2*')
)

Function(name='vkCmdCopyBuffer2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyBufferInfo', type='const VkCopyBufferInfo2*')
)

Function(name='vkCmdCopyBufferToImage', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcBuffer', type='VkBuffer'),
arg3=ArgDef(name='dstImage', type='VkImage'),
arg4=ArgDef(name='dstImageLayout', type='VkImageLayout'),
arg5=ArgDef(name='regionCount', type='uint32_t'),
arg6=ArgDef(name='pRegions', type='const VkBufferImageCopy*', wrapType='CVkBufferImageCopyArray', wrapParams='regionCount, pRegions', count='regionCount')
)

Function(name='vkCmdCopyBufferToImage2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyBufferToImageInfo', type='const VkCopyBufferToImageInfo2*')
)

Function(name='vkCmdCopyBufferToImage2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyBufferToImageInfo', type='const VkCopyBufferToImageInfo2*')
)

Function(name='vkCmdCopyImage', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcImage', type='VkImage'),
arg3=ArgDef(name='srcImageLayout', type='VkImageLayout'),
arg4=ArgDef(name='dstImage', type='VkImage'),
arg5=ArgDef(name='dstImageLayout', type='VkImageLayout'),
arg6=ArgDef(name='regionCount', type='uint32_t'),
arg7=ArgDef(name='pRegions', type='const VkImageCopy*', wrapType='CVkImageCopyArray', wrapParams='regionCount, pRegions', count='regionCount')
)

Function(name='vkCmdCopyImage2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyImageInfo', type='const VkCopyImageInfo2*')
)

Function(name='vkCmdCopyImage2KHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyImageInfo', type='const VkCopyImageInfo2*')
)

Function(name='vkCmdCopyImageToBuffer', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcImage', type='VkImage'),
arg3=ArgDef(name='srcImageLayout', type='VkImageLayout'),
arg4=ArgDef(name='dstBuffer', type='VkBuffer'),
arg5=ArgDef(name='regionCount', type='uint32_t'),
arg6=ArgDef(name='pRegions', type='const VkBufferImageCopy*', wrapType='CVkBufferImageCopyArray', wrapParams='regionCount, pRegions', count='regionCount')
)

Function(name='vkCmdCopyImageToBuffer2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyImageToBufferInfo', type='const VkCopyImageToBufferInfo2*')
)

Function(name='vkCmdCopyImageToBuffer2KHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCopyImageToBufferInfo', type='const VkCopyImageToBufferInfo2*')
)

Function(name='vkCmdCopyMemoryIndirectNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='copyBufferAddress', type='VkDeviceAddress'),
arg3=ArgDef(name='copyCount', type='uint32_t'),
arg4=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdCopyMemoryToAccelerationStructureKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkCopyMemoryToAccelerationStructureInfoKHR*')
)

Function(name='vkCmdCopyMemoryToImageIndirectNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='copyBufferAddress', type='VkDeviceAddress'),
arg3=ArgDef(name='copyCount', type='uint32_t'),
arg4=ArgDef(name='stride', type='uint32_t'),
arg5=ArgDef(name='dstImage', type='VkImage'),
arg6=ArgDef(name='dstImageLayout', type='VkImageLayout'),
arg7=ArgDef(name='pImageSubresources', type='const VkImageSubresourceLayers*')
)

Function(name='vkCmdCopyMemoryToMicromapEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkCopyMemoryToMicromapInfoEXT*')
)

Function(name='vkCmdCopyMicromapEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkCopyMicromapInfoEXT*')
)

Function(name='vkCmdCopyMicromapToMemoryEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pInfo', type='const VkCopyMicromapToMemoryInfoEXT*')
)

Function(name='vkCmdCopyQueryPoolResults', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='firstQuery', type='uint32_t'),
arg4=ArgDef(name='queryCount', type='uint32_t'),
arg5=ArgDef(name='dstBuffer', type='VkBuffer'),
arg6=ArgDef(name='dstOffset', type='VkDeviceSize'),
arg7=ArgDef(name='stride', type='VkDeviceSize'),
arg8=ArgDef(name='flags', type='VkQueryResultFlags')
)

Function(name='vkCmdDebugMarkerBeginEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pMarkerInfo', type='const VkDebugMarkerMarkerInfoEXT*')
)

Function(name='vkCmdDebugMarkerEndEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkCmdDebugMarkerInsertEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pMarkerInfo', type='const VkDebugMarkerMarkerInfoEXT*')
)

Function(name='vkCmdDecodeVideoKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pDecodeInfo', type='const VkVideoDecodeInfoKHR*')
)

Function(name='vkCmdDecompressMemoryIndirectCountNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='indirectCommandsAddress', type='VkDeviceAddress'),
arg3=ArgDef(name='indirectCommandsCountAddress', type='VkDeviceAddress'),
arg4=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDecompressMemoryNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='decompressRegionCount', type='uint32_t'),
arg3=ArgDef(name='pDecompressMemoryRegions', type='const VkDecompressMemoryRegionNV*')
)

Function(name='vkCmdDispatch', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='groupCountX', type='uint32_t'),
arg3=ArgDef(name='groupCountY', type='uint32_t'),
arg4=ArgDef(name='groupCountZ', type='uint32_t')
)

Function(name='vkCmdDispatchBase', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='baseGroupX', type='uint32_t'),
arg3=ArgDef(name='baseGroupY', type='uint32_t'),
arg4=ArgDef(name='baseGroupZ', type='uint32_t'),
arg5=ArgDef(name='groupCountX', type='uint32_t'),
arg6=ArgDef(name='groupCountY', type='uint32_t'),
arg7=ArgDef(name='groupCountZ', type='uint32_t')
)

Function(name='vkCmdDispatchBaseKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='baseGroupX', type='uint32_t'),
arg3=ArgDef(name='baseGroupY', type='uint32_t'),
arg4=ArgDef(name='baseGroupZ', type='uint32_t'),
arg5=ArgDef(name='groupCountX', type='uint32_t'),
arg6=ArgDef(name='groupCountY', type='uint32_t'),
arg7=ArgDef(name='groupCountZ', type='uint32_t')
)

Function(name='vkCmdDispatchIndirect', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize')
)

Function(name='vkCmdDraw', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='vertexCount', type='uint32_t'),
arg3=ArgDef(name='instanceCount', type='uint32_t'),
arg4=ArgDef(name='firstVertex', type='uint32_t'),
arg5=ArgDef(name='firstInstance', type='uint32_t')
)

Function(name='vkCmdDrawIndexed', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='indexCount', type='uint32_t'),
arg3=ArgDef(name='instanceCount', type='uint32_t'),
arg4=ArgDef(name='firstIndex', type='uint32_t'),
arg5=ArgDef(name='vertexOffset', type='int32_t'),
arg6=ArgDef(name='firstInstance', type='uint32_t')
)

Function(name='vkCmdDrawIndexedIndirect', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='drawCount', type='uint32_t'),
arg5=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndexedIndirectCount', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndexedIndirectCountAMD', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndexedIndirectCountKHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndirect', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='drawCount', type='uint32_t'),
arg5=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndirectByteCountEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='instanceCount', type='uint32_t'),
arg3=ArgDef(name='firstInstance', type='uint32_t'),
arg4=ArgDef(name='counterBuffer', type='VkBuffer'),
arg5=ArgDef(name='counterBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='counterOffset', type='uint32_t'),
arg7=ArgDef(name='vertexStride', type='uint32_t')
)

Function(name='vkCmdDrawIndirectCount', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndirectCountAMD', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawIndirectCountKHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawMeshTasksEXT', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='groupCountX', type='uint32_t'),
arg3=ArgDef(name='groupCountY', type='uint32_t'),
arg4=ArgDef(name='groupCountZ', type='uint32_t')
)

Function(name='vkCmdDrawMeshTasksIndirectCountEXT', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawMeshTasksIndirectCountNV', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='countBuffer', type='VkBuffer'),
arg5=ArgDef(name='countBufferOffset', type='VkDeviceSize'),
arg6=ArgDef(name='maxDrawCount', type='uint32_t'),
arg7=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawMeshTasksIndirectEXT', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='drawCount', type='uint32_t'),
arg5=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawMeshTasksIndirectNV', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='drawCount', type='uint32_t'),
arg5=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawMeshTasksNV', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='taskCount', type='uint32_t'),
arg3=ArgDef(name='firstTask', type='uint32_t')
)

Function(name='vkCmdDrawMultiEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='drawCount', type='uint32_t'),
arg3=ArgDef(name='pVertexInfo', type='const VkMultiDrawInfoEXT*'),
arg4=ArgDef(name='instanceCount', type='uint32_t'),
arg5=ArgDef(name='firstInstance', type='uint32_t'),
arg6=ArgDef(name='stride', type='uint32_t')
)

Function(name='vkCmdDrawMultiIndexedEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='drawCount', type='uint32_t'),
arg3=ArgDef(name='pIndexInfo', type='const VkMultiDrawIndexedInfoEXT*'),
arg4=ArgDef(name='instanceCount', type='uint32_t'),
arg5=ArgDef(name='firstInstance', type='uint32_t'),
arg6=ArgDef(name='stride', type='uint32_t'),
arg7=ArgDef(name='pVertexOffset', type='const int32_t*')
)

Function(name='vkCmdEncodeVideoKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pEncodeInfo', type='const VkVideoEncodeInfoKHR*')
)

Function(name='vkCmdEndConditionalRenderingEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkCmdEndDebugUtilsLabelEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkCmdEndQuery', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='query', type='uint32_t')
)

Function(name='vkCmdEndQueryIndexedEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='query', type='uint32_t'),
arg4=ArgDef(name='index', type='uint32_t')
)

Function(name='vkCmdEndRenderPass', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkCmdEndRenderPass2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pSubpassEndInfo', type='const VkSubpassEndInfo*')
)

Function(name='vkCmdEndRenderPass2KHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pSubpassEndInfo', type='const VkSubpassEndInfo*')
)

Function(name='vkCmdEndRendering', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkCmdEndRenderingKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkCmdEndTransformFeedbackEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstCounterBuffer', type='uint32_t'),
arg3=ArgDef(name='counterBufferCount', type='uint32_t'),
arg4=ArgDef(name='pCounterBuffers', type='const VkBuffer*', wrapParams='counterBufferCount, pCounterBuffers', count='counterBufferCount'),
arg5=ArgDef(name='pCounterBufferOffsets', type='const VkDeviceSize*', wrapType='Cuint64_t::CSArray', wrapParams='counterBufferCount, pCounterBufferOffsets', count='counterBufferCount')
)

Function(name='vkCmdEndVideoCodingKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pEndCodingInfo', type='const VkVideoEndCodingInfoKHR*')
)

Function(name='vkCmdExecuteCommands', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='commandBufferCount', type='uint32_t'),
arg3=ArgDef(name='pCommandBuffers', type='const VkCommandBuffer*', wrapParams='commandBufferCount, pCommandBuffers', count='commandBufferCount')
)

Function(name='vkCmdExecuteGeneratedCommandsNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='isPreprocessed', type='VkBool32'),
arg3=ArgDef(name='pGeneratedCommandsInfo', type='const VkGeneratedCommandsInfoNV*')
)

Function(name='vkCmdFillBuffer', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='dstBuffer', type='VkBuffer'),
arg3=ArgDef(name='dstOffset', type='VkDeviceSize'),
arg4=ArgDef(name='size', type='VkDeviceSize'),
arg5=ArgDef(name='data', type='uint32_t')
)

Function(name='vkCmdInsertDebugUtilsLabelEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pLabelInfo', type='const VkDebugUtilsLabelEXT*')
)

Function(name='vkCmdNextSubpass', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='contents', type='VkSubpassContents')
)

Function(name='vkCmdNextSubpass2', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pSubpassBeginInfo', type='const VkSubpassBeginInfo*'),
arg3=ArgDef(name='pSubpassEndInfo', type='const VkSubpassEndInfo*')
)

Function(name='vkCmdNextSubpass2KHR', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pSubpassBeginInfo', type='const VkSubpassBeginInfo*'),
arg3=ArgDef(name='pSubpassEndInfo', type='const VkSubpassEndInfo*')
)

Function(name='vkCmdOpticalFlowExecuteNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='session', type='VkOpticalFlowSessionNV'),
arg3=ArgDef(name='pExecuteInfo', type='const VkOpticalFlowExecuteInfoNV*')
)

Function(name='vkCmdPipelineBarrier', enabled=True, type=Param, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer", ccodeWriteWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcStageMask', type='VkPipelineStageFlags'),
arg3=ArgDef(name='dstStageMask', type='VkPipelineStageFlags'),
arg4=ArgDef(name='dependencyFlags', type='VkDependencyFlags'),
arg5=ArgDef(name='memoryBarrierCount', type='uint32_t'),
arg6=ArgDef(name='pMemoryBarriers', type='const VkMemoryBarrier*', wrapType='CVkMemoryBarrierArray', wrapParams='memoryBarrierCount, pMemoryBarriers', count='memoryBarrierCount'),
arg7=ArgDef(name='bufferMemoryBarrierCount', type='uint32_t'),
arg8=ArgDef(name='pBufferMemoryBarriers', type='const VkBufferMemoryBarrier*', wrapType='CVkBufferMemoryBarrierArray', wrapParams='bufferMemoryBarrierCount, pBufferMemoryBarriers', count='bufferMemoryBarrierCount'),
arg9=ArgDef(name='imageMemoryBarrierCount', type='uint32_t'),
arg10=ArgDef(name='pImageMemoryBarriers', type='const VkImageMemoryBarrier*', wrapType='CVkImageMemoryBarrierArray', wrapParams='imageMemoryBarrierCount, pImageMemoryBarriers', count='imageMemoryBarrierCount')
)

Function(name='vkCmdPipelineBarrier2', enabled=True, type=Param, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer", ccodeWriteWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pDependencyInfo', type='const VkDependencyInfo*')
)

Function(name='vkCmdPipelineBarrier2KHR', enabled=True, type=Param, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer", ccodeWriteWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pDependencyInfo', type='const VkDependencyInfo*')
)

Function(name='vkCmdPreprocessGeneratedCommandsNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pGeneratedCommandsInfo', type='const VkGeneratedCommandsInfoNV*')
)

Function(name='vkCmdPushConstants', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='layout', type='VkPipelineLayout'),
arg3=ArgDef(name='stageFlags', type='VkShaderStageFlags'),
arg4=ArgDef(name='offset', type='uint32_t'),
arg5=ArgDef(name='size', type='uint32_t'),
arg6=ArgDef(name='pValues', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='(size_t)(size), (const uint8_t *)pValues')
)

Function(name='vkCmdPushDescriptorSetKHR', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
arg3=ArgDef(name='layout', type='VkPipelineLayout'),
arg4=ArgDef(name='set', type='uint32_t'),
arg5=ArgDef(name='descriptorWriteCount', type='uint32_t'),
arg6=ArgDef(name='pDescriptorWrites', type='const VkWriteDescriptorSet*', wrapType='CVkWriteDescriptorSetArray', wrapParams='descriptorWriteCount, pDescriptorWrites', count='descriptorWriteCount')
)

Function(name='vkCmdPushDescriptorSetWithTemplateKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='descriptorUpdateTemplate', type='VkDescriptorUpdateTemplate'),
arg3=ArgDef(name='layout', type='VkPipelineLayout'),
arg4=ArgDef(name='set', type='uint32_t'),
arg5=ArgDef(name='pData', type='const void*', wrapType='CUpdateDescriptorSetWithTemplateArray', wrapParams='descriptorUpdateTemplate, pData')
)

Function(name='vkCmdRefreshObjectsKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRefreshObjects', type='const VkRefreshObjectListKHR*')
)

Function(name='vkCmdResetEvent', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='event', type='VkEvent'),
arg3=ArgDef(name='stageMask', type='VkPipelineStageFlags')
)

Function(name='vkCmdResetEvent2', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='event', type='VkEvent'),
arg3=ArgDef(name='stageMask', type='VkPipelineStageFlags2')
)

Function(name='vkCmdResetEvent2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='event', type='VkEvent'),
arg3=ArgDef(name='stageMask', type='VkPipelineStageFlags2')
)

Function(name='vkCmdResetQueryPool', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='firstQuery', type='uint32_t'),
arg4=ArgDef(name='queryCount', type='uint32_t')
)

Function(name='vkCmdResolveImage', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='srcImage', type='VkImage'),
arg3=ArgDef(name='srcImageLayout', type='VkImageLayout'),
arg4=ArgDef(name='dstImage', type='VkImage'),
arg5=ArgDef(name='dstImageLayout', type='VkImageLayout'),
arg6=ArgDef(name='regionCount', type='uint32_t'),
arg7=ArgDef(name='pRegions', type='const VkImageResolve*', wrapType='CVkImageResolveArray', wrapParams='regionCount, pRegions', count='regionCount')
)

Function(name='vkCmdResolveImage2', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pResolveImageInfo', type='const VkResolveImageInfo2*')
)

Function(name='vkCmdResolveImage2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pResolveImageInfo', type='const VkResolveImageInfo2*')
)

Function(name='vkCmdSetAlphaToCoverageEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='alphaToCoverageEnable', type='VkBool32')
)

Function(name='vkCmdSetAlphaToOneEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='alphaToOneEnable', type='VkBool32')
)

Function(name='vkCmdSetBlendConstants', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='blendConstants', type='const float*', wrapType='Cfloat::CSArray', wrapParams='4, blendConstants', count='4')
)

Function(name='vkCmdSetCheckpointNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pCheckpointMarker', type='const void*')
)

Function(name='vkCmdSetCoarseSampleOrderNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='sampleOrderType', type='VkCoarseSampleOrderTypeNV'),
arg3=ArgDef(name='customSampleOrderCount', type='uint32_t'),
arg4=ArgDef(name='pCustomSampleOrders', type='const VkCoarseSampleOrderCustomNV*')
)

Function(name='vkCmdSetColorBlendAdvancedEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstAttachment', type='uint32_t'),
arg3=ArgDef(name='attachmentCount', type='uint32_t'),
arg4=ArgDef(name='pColorBlendAdvanced', type='const VkColorBlendAdvancedEXT*')
)

Function(name='vkCmdSetColorBlendEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstAttachment', type='uint32_t'),
arg3=ArgDef(name='attachmentCount', type='uint32_t'),
arg4=ArgDef(name='pColorBlendEnables', type='const VkBool32*')
)

Function(name='vkCmdSetColorBlendEquationEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstAttachment', type='uint32_t'),
arg3=ArgDef(name='attachmentCount', type='uint32_t'),
arg4=ArgDef(name='pColorBlendEquations', type='const VkColorBlendEquationEXT*')
)

Function(name='vkCmdSetColorWriteEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='attachmentCount', type='uint32_t'),
arg3=ArgDef(name='pColorWriteEnables', type='const VkBool32*')
)

Function(name='vkCmdSetColorWriteMaskEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstAttachment', type='uint32_t'),
arg3=ArgDef(name='attachmentCount', type='uint32_t'),
arg4=ArgDef(name='pColorWriteMasks', type='const VkColorComponentFlags*')
)

Function(name='vkCmdSetConservativeRasterizationModeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='conservativeRasterizationMode', type='VkConservativeRasterizationModeEXT')
)

Function(name='vkCmdSetCoverageModulationModeNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='coverageModulationMode', type='VkCoverageModulationModeNV')
)

Function(name='vkCmdSetCoverageModulationTableEnableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='coverageModulationTableEnable', type='VkBool32')
)

Function(name='vkCmdSetCoverageModulationTableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='coverageModulationTableCount', type='uint32_t'),
arg3=ArgDef(name='pCoverageModulationTable', type='const float*')
)

Function(name='vkCmdSetCoverageReductionModeNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='coverageReductionMode', type='VkCoverageReductionModeNV')
)

Function(name='vkCmdSetCoverageToColorEnableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='coverageToColorEnable', type='VkBool32')
)

Function(name='vkCmdSetCoverageToColorLocationNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='coverageToColorLocation', type='uint32_t')
)

Function(name='vkCmdSetCullMode', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='cullMode', type='VkCullModeFlags')
)

Function(name='vkCmdSetCullModeEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='cullMode', type='VkCullModeFlags')
)

Function(name='vkCmdSetDepthBias', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthBiasConstantFactor', type='float'),
arg3=ArgDef(name='depthBiasClamp', type='float'),
arg4=ArgDef(name='depthBiasSlopeFactor', type='float')
)

Function(name='vkCmdSetDepthBiasEnable', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthBiasEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthBiasEnableEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthBiasEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthBounds', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='minDepthBounds', type='float'),
arg3=ArgDef(name='maxDepthBounds', type='float')
)

Function(name='vkCmdSetDepthBoundsTestEnable', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthBoundsTestEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthBoundsTestEnableEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthBoundsTestEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthClampEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthClampEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthClipEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthClipEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthClipNegativeOneToOneEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='negativeOneToOne', type='VkBool32')
)

Function(name='vkCmdSetDepthCompareOp', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthCompareOp', type='VkCompareOp')
)

Function(name='vkCmdSetDepthCompareOpEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthCompareOp', type='VkCompareOp')
)

Function(name='vkCmdSetDepthTestEnable', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthTestEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthTestEnableEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthTestEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthWriteEnable', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthWriteEnable', type='VkBool32')
)

Function(name='vkCmdSetDepthWriteEnableEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='depthWriteEnable', type='VkBool32')
)

Function(name='vkCmdSetDescriptorBufferOffsetsEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
arg3=ArgDef(name='layout', type='VkPipelineLayout'),
arg4=ArgDef(name='firstSet', type='uint32_t'),
arg5=ArgDef(name='setCount', type='uint32_t'),
arg6=ArgDef(name='pBufferIndices', type='const uint32_t*'),
arg7=ArgDef(name='pOffsets', type='const VkDeviceSize*')
)

Function(name='vkCmdSetDeviceMask', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='deviceMask', type='uint32_t')
)

Function(name='vkCmdSetDeviceMaskKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='deviceMask', type='uint32_t')
)

Function(name='vkCmdSetDiscardRectangleEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstDiscardRectangle', type='uint32_t'),
arg3=ArgDef(name='discardRectangleCount', type='uint32_t'),
arg4=ArgDef(name='pDiscardRectangles', type='const VkRect2D*', count='discardRectangleCount')
)

Function(name='vkCmdSetDiscardRectangleEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='discardRectangleEnable', type='VkBool32')
)

Function(name='vkCmdSetDiscardRectangleModeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='discardRectangleMode', type='VkDiscardRectangleModeEXT')
)

Function(name='vkCmdSetEvent', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='event', type='VkEvent'),
arg3=ArgDef(name='stageMask', type='VkPipelineStageFlags')
)

Function(name='vkCmdSetEvent2', enabled=True, type=Param, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='event', type='VkEvent'),
arg3=ArgDef(name='pDependencyInfo', type='const VkDependencyInfo*')
)

Function(name='vkCmdSetEvent2KHR', enabled=True, type=Param, stateTrack=True, recWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='event', type='VkEvent'),
arg3=ArgDef(name='pDependencyInfo', type='const VkDependencyInfo*')
)

Function(name='vkCmdSetExclusiveScissorEnableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstExclusiveScissor', type='uint32_t'),
arg3=ArgDef(name='exclusiveScissorCount', type='uint32_t'),
arg4=ArgDef(name='pExclusiveScissorEnables', type='const VkBool32*')
)

Function(name='vkCmdSetExclusiveScissorNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstExclusiveScissor', type='uint32_t'),
arg3=ArgDef(name='exclusiveScissorCount', type='uint32_t'),
arg4=ArgDef(name='pExclusiveScissors', type='const VkRect2D*')
)

Function(name='vkCmdSetExtraPrimitiveOverestimationSizeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='extraPrimitiveOverestimationSize', type='float')
)

Function(name='vkCmdSetFragmentShadingRateEnumNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='shadingRate', type='VkFragmentShadingRateNV'),
arg3=ArgDef(name='combinerOps', type='const VkFragmentShadingRateCombinerOpKHR[2]')
)

Function(name='vkCmdSetFragmentShadingRateKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pFragmentSize', type='const VkExtent2D*'),
arg3=ArgDef(name='combinerOps', type='const VkFragmentShadingRateCombinerOpKHR[2]')
)

Function(name='vkCmdSetFrontFace', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='frontFace', type='VkFrontFace')
)

Function(name='vkCmdSetFrontFaceEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='frontFace', type='VkFrontFace')
)

Function(name='vkCmdSetLineRasterizationModeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='lineRasterizationMode', type='VkLineRasterizationModeEXT')
)

Function(name='vkCmdSetLineStippleEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='lineStippleFactor', type='uint32_t'),
arg3=ArgDef(name='lineStipplePattern', type='uint16_t')
)

Function(name='vkCmdSetLineStippleEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stippledLineEnable', type='VkBool32')
)

Function(name='vkCmdSetLineWidth', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='lineWidth', type='float')
)

Function(name='vkCmdSetLogicOpEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='logicOp', type='VkLogicOp')
)

Function(name='vkCmdSetLogicOpEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='logicOpEnable', type='VkBool32')
)

Function(name='vkCmdSetPatchControlPointsEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='patchControlPoints', type='uint32_t')
)

Function(name='vkCmdSetPerformanceMarkerINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pMarkerInfo', type='const VkPerformanceMarkerInfoINTEL*')
)

Function(name='vkCmdSetPerformanceOverrideINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pOverrideInfo', type='const VkPerformanceOverrideInfoINTEL*')
)

Function(name='vkCmdSetPerformanceStreamMarkerINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pMarkerInfo', type='const VkPerformanceStreamMarkerInfoINTEL*')
)

Function(name='vkCmdSetPolygonModeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='polygonMode', type='VkPolygonMode')
)

Function(name='vkCmdSetPrimitiveRestartEnable', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='primitiveRestartEnable', type='VkBool32')
)

Function(name='vkCmdSetPrimitiveRestartEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='primitiveRestartEnable', type='VkBool32')
)

Function(name='vkCmdSetPrimitiveTopology', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='primitiveTopology', type='VkPrimitiveTopology')
)

Function(name='vkCmdSetPrimitiveTopologyEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='primitiveTopology', type='VkPrimitiveTopology')
)

Function(name='vkCmdSetProvokingVertexModeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='provokingVertexMode', type='VkProvokingVertexModeEXT')
)

Function(name='vkCmdSetRasterizationSamplesEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='rasterizationSamples', type='VkSampleCountFlagBits')
)

Function(name='vkCmdSetRasterizationStreamEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='rasterizationStream', type='uint32_t')
)

Function(name='vkCmdSetRasterizerDiscardEnable', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='rasterizerDiscardEnable', type='VkBool32')
)

Function(name='vkCmdSetRasterizerDiscardEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='rasterizerDiscardEnable', type='VkBool32')
)

Function(name='vkCmdSetRayTracingPipelineStackSizeKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineStackSize', type='uint32_t')
)

Function(name='vkCmdSetRepresentativeFragmentTestEnableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='representativeFragmentTestEnable', type='VkBool32')
)

Function(name='vkCmdSetSampleLocationsEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pSampleLocationsInfo', type='const VkSampleLocationsInfoEXT*')
)

Function(name='vkCmdSetSampleLocationsEnableEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='sampleLocationsEnable', type='VkBool32')
)

Function(name='vkCmdSetSampleMaskEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='samples', type='VkSampleCountFlagBits'),
arg3=ArgDef(name='pSampleMask', type='const VkSampleMask*')
)

Function(name='vkCmdSetScissor', enabled=True, type=Param, runWrap=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstScissor', type='uint32_t'),
arg3=ArgDef(name='scissorCount', type='uint32_t'),
arg4=ArgDef(name='pScissors', type='const VkRect2D*', wrapType='CVkRect2DArray', wrapParams='scissorCount, pScissors', count='scissorCount')
)

Function(name='vkCmdSetScissorWithCount', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='scissorCount', type='uint32_t'),
arg3=ArgDef(name='pScissors', type='const VkRect2D*', wrapType='CVkRect2DArray', wrapParams='scissorCount, pScissors', count='scissorCount')
)

Function(name='vkCmdSetScissorWithCountEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='scissorCount', type='uint32_t'),
arg3=ArgDef(name='pScissors', type='const VkRect2D*')
)

Function(name='vkCmdSetShadingRateImageEnableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='shadingRateImageEnable', type='VkBool32')
)

Function(name='vkCmdSetStencilCompareMask', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='faceMask', type='VkStencilFaceFlags'),
arg3=ArgDef(name='compareMask', type='uint32_t')
)

Function(name='vkCmdSetStencilOp', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='faceMask', type='VkStencilFaceFlags'),
arg3=ArgDef(name='failOp', type='VkStencilOp'),
arg4=ArgDef(name='passOp', type='VkStencilOp'),
arg5=ArgDef(name='depthFailOp', type='VkStencilOp'),
arg6=ArgDef(name='compareOp', type='VkCompareOp')
)

Function(name='vkCmdSetStencilOpEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='faceMask', type='VkStencilFaceFlags'),
arg3=ArgDef(name='failOp', type='VkStencilOp'),
arg4=ArgDef(name='passOp', type='VkStencilOp'),
arg5=ArgDef(name='depthFailOp', type='VkStencilOp'),
arg6=ArgDef(name='compareOp', type='VkCompareOp')
)

Function(name='vkCmdSetStencilReference', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='faceMask', type='VkStencilFaceFlags'),
arg3=ArgDef(name='reference', type='uint32_t')
)

Function(name='vkCmdSetStencilTestEnable', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stencilTestEnable', type='VkBool32')
)

Function(name='vkCmdSetStencilTestEnableEXT', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stencilTestEnable', type='VkBool32')
)

Function(name='vkCmdSetStencilWriteMask', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='faceMask', type='VkStencilFaceFlags'),
arg3=ArgDef(name='writeMask', type='uint32_t')
)

Function(name='vkCmdSetTessellationDomainOriginEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='domainOrigin', type='VkTessellationDomainOrigin')
)

Function(name='vkCmdSetVertexInputEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='vertexBindingDescriptionCount', type='uint32_t'),
arg3=ArgDef(name='pVertexBindingDescriptions', type='const VkVertexInputBindingDescription2EXT*'),
arg4=ArgDef(name='vertexAttributeDescriptionCount', type='uint32_t'),
arg5=ArgDef(name='pVertexAttributeDescriptions', type='const VkVertexInputAttributeDescription2EXT*')
)

Function(name='vkCmdSetViewport', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstViewport', type='uint32_t'),
arg3=ArgDef(name='viewportCount', type='uint32_t'),
arg4=ArgDef(name='pViewports', type='const VkViewport*', wrapType='CVkViewportArray', wrapParams='viewportCount, pViewports', count='viewportCount')
)

Function(name='vkCmdSetViewportShadingRatePaletteNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstViewport', type='uint32_t'),
arg3=ArgDef(name='viewportCount', type='uint32_t'),
arg4=ArgDef(name='pShadingRatePalettes', type='const VkShadingRatePaletteNV*')
)

Function(name='vkCmdSetViewportSwizzleNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstViewport', type='uint32_t'),
arg3=ArgDef(name='viewportCount', type='uint32_t'),
arg4=ArgDef(name='pViewportSwizzles', type='const VkViewportSwizzleNV*')
)

Function(name='vkCmdSetViewportWScalingEnableNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='viewportWScalingEnable', type='VkBool32')
)

Function(name='vkCmdSetViewportWScalingNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='firstViewport', type='uint32_t'),
arg3=ArgDef(name='viewportCount', type='uint32_t'),
arg4=ArgDef(name='pViewportWScalings', type='const VkViewportWScalingNV*', count='viewportCount')
)

Function(name='vkCmdSetViewportWithCount', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='viewportCount', type='uint32_t'),
arg3=ArgDef(name='pViewports', type='const VkViewport*', wrapType='CVkViewportArray', wrapParams='viewportCount, pViewports', count='viewportCount')
)

Function(name='vkCmdSetViewportWithCountEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='viewportCount', type='uint32_t'),
arg3=ArgDef(name='pViewports', type='const VkViewport*')
)

Function(name='vkCmdTraceRaysIndirect2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='indirectDeviceAddress', type='VkDeviceAddress')
)

Function(name='vkCmdTraceRaysIndirectKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRaygenShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg3=ArgDef(name='pMissShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg4=ArgDef(name='pHitShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg5=ArgDef(name='pCallableShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg6=ArgDef(name='indirectDeviceAddress', type='VkDeviceAddress')
)

Function(name='vkCmdTraceRaysKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pRaygenShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg3=ArgDef(name='pMissShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg4=ArgDef(name='pHitShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg5=ArgDef(name='pCallableShaderBindingTable', type='const VkStridedDeviceAddressRegionKHR*'),
arg6=ArgDef(name='width', type='uint32_t'),
arg7=ArgDef(name='height', type='uint32_t'),
arg8=ArgDef(name='depth', type='uint32_t')
)

Function(name='vkCmdTraceRaysNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='raygenShaderBindingTableBuffer', type='VkBuffer'),
arg3=ArgDef(name='raygenShaderBindingOffset', type='VkDeviceSize'),
arg4=ArgDef(name='missShaderBindingTableBuffer', type='VkBuffer'),
arg5=ArgDef(name='missShaderBindingOffset', type='VkDeviceSize'),
arg6=ArgDef(name='missShaderBindingStride', type='VkDeviceSize'),
arg7=ArgDef(name='hitShaderBindingTableBuffer', type='VkBuffer'),
arg8=ArgDef(name='hitShaderBindingOffset', type='VkDeviceSize'),
arg9=ArgDef(name='hitShaderBindingStride', type='VkDeviceSize'),
arg10=ArgDef(name='callableShaderBindingTableBuffer', type='VkBuffer'),
arg11=ArgDef(name='callableShaderBindingOffset', type='VkDeviceSize'),
arg12=ArgDef(name='callableShaderBindingStride', type='VkDeviceSize'),
arg13=ArgDef(name='width', type='uint32_t'),
arg14=ArgDef(name='height', type='uint32_t'),
arg15=ArgDef(name='depth', type='uint32_t')
)

Function(name='vkCmdUpdateBuffer', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='dstBuffer', type='VkBuffer'),
arg3=ArgDef(name='dstOffset', type='VkDeviceSize'),
arg4=ArgDef(name='dataSize', type='VkDeviceSize'),
arg5=ArgDef(name='pData', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='(size_t)(dataSize), (const uint8_t *)pData')
)

Function(name='vkCmdWaitEvents', enabled=True, type=Param, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='eventCount', type='uint32_t'),
arg3=ArgDef(name='pEvents', type='const VkEvent*', wrapParams='eventCount, pEvents', count='eventCount'),
arg4=ArgDef(name='srcStageMask', type='VkPipelineStageFlags'),
arg5=ArgDef(name='dstStageMask', type='VkPipelineStageFlags'),
arg6=ArgDef(name='memoryBarrierCount', type='uint32_t'),
arg7=ArgDef(name='pMemoryBarriers', type='const VkMemoryBarrier*', wrapType='CVkMemoryBarrierArray', wrapParams='memoryBarrierCount, pMemoryBarriers', count='memoryBarrierCount'),
arg8=ArgDef(name='bufferMemoryBarrierCount', type='uint32_t'),
arg9=ArgDef(name='pBufferMemoryBarriers', type='const VkBufferMemoryBarrier*', wrapType='CVkBufferMemoryBarrierArray', wrapParams='bufferMemoryBarrierCount, pBufferMemoryBarriers', count='bufferMemoryBarrierCount'),
arg10=ArgDef(name='imageMemoryBarrierCount', type='uint32_t'),
arg11=ArgDef(name='pImageMemoryBarriers', type='const VkImageMemoryBarrier*', wrapType='CVkImageMemoryBarrierArray', wrapParams='imageMemoryBarrierCount, pImageMemoryBarriers', count='imageMemoryBarrierCount')
)

Function(name='vkCmdWaitEvents2', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='eventCount', type='uint32_t'),
arg3=ArgDef(name='pEvents', type='const VkEvent*'),
arg4=ArgDef(name='pDependencyInfos', type='const VkDependencyInfo*')
)

Function(name='vkCmdWaitEvents2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='eventCount', type='uint32_t'),
arg3=ArgDef(name='pEvents', type='const VkEvent*'),
arg4=ArgDef(name='pDependencyInfos', type='const VkDependencyInfo*')
)

Function(name='vkCmdWriteAccelerationStructuresPropertiesKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='accelerationStructureCount', type='uint32_t'),
arg3=ArgDef(name='pAccelerationStructures', type='const VkAccelerationStructureKHR*'),
arg4=ArgDef(name='queryType', type='VkQueryType'),
arg5=ArgDef(name='queryPool', type='VkQueryPool'),
arg6=ArgDef(name='firstQuery', type='uint32_t')
)

Function(name='vkCmdWriteAccelerationStructuresPropertiesNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='accelerationStructureCount', type='uint32_t'),
arg3=ArgDef(name='pAccelerationStructures', type='const VkAccelerationStructureNV*'),
arg4=ArgDef(name='queryType', type='VkQueryType'),
arg5=ArgDef(name='queryPool', type='VkQueryPool'),
arg6=ArgDef(name='firstQuery', type='uint32_t')
)

Function(name='vkCmdWriteBufferMarker2AMD', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stage', type='VkPipelineStageFlags2'),
arg3=ArgDef(name='dstBuffer', type='VkBuffer'),
arg4=ArgDef(name='dstOffset', type='VkDeviceSize'),
arg5=ArgDef(name='marker', type='uint32_t')
)

Function(name='vkCmdWriteBufferMarkerAMD', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineStage', type='VkPipelineStageFlagBits'),
arg3=ArgDef(name='dstBuffer', type='VkBuffer'),
arg4=ArgDef(name='dstOffset', type='VkDeviceSize'),
arg5=ArgDef(name='marker', type='uint32_t')
)

Function(name='vkCmdWriteMicromapsPropertiesEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='micromapCount', type='uint32_t'),
arg3=ArgDef(name='pMicromaps', type='const VkMicromapEXT*'),
arg4=ArgDef(name='queryType', type='VkQueryType'),
arg5=ArgDef(name='queryPool', type='VkQueryPool'),
arg6=ArgDef(name='firstQuery', type='uint32_t')
)

Function(name='vkCmdWriteTimestamp', enabled=True, type=Param, stateTrack=True, tokenCache="SD()._commandbufferstates[commandBuffer]->tokensBuffer",
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='pipelineStage', type='VkPipelineStageFlagBits'),
arg3=ArgDef(name='queryPool', type='VkQueryPool'),
arg4=ArgDef(name='query', type='uint32_t')
)

Function(name='vkCmdWriteTimestamp2', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stage', type='VkPipelineStageFlags2'),
arg3=ArgDef(name='queryPool', type='VkQueryPool'),
arg4=ArgDef(name='query', type='uint32_t')
)

Function(name='vkCmdWriteTimestamp2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='stage', type='VkPipelineStageFlags2'),
arg3=ArgDef(name='queryPool', type='VkQueryPool'),
arg4=ArgDef(name='query', type='uint32_t')
)

Function(name='vkCompileDeferredNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline'),
arg3=ArgDef(name='shader', type='uint32_t')
)

Function(name='vkCopyAccelerationStructureKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pInfo', type='const VkCopyAccelerationStructureInfoKHR*')
)

Function(name='vkCopyAccelerationStructureToMemoryKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pInfo', type='const VkCopyAccelerationStructureToMemoryInfoKHR*')
)

Function(name='vkCopyMemoryToAccelerationStructureKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pInfo', type='const VkCopyMemoryToAccelerationStructureInfoKHR*')
)

Function(name='vkCopyMemoryToMicromapEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pInfo', type='const VkCopyMemoryToMicromapInfoEXT*')
)

Function(name='vkCopyMicromapEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pInfo', type='const VkCopyMicromapInfoEXT*')
)

Function(name='vkCopyMicromapToMemoryEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pInfo', type='const VkCopyMicromapToMemoryInfoEXT*')
)

Function(name='vkCreateAccelerationStructureKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkAccelerationStructureCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pAccelerationStructure', type='VkAccelerationStructureKHR*')
)

Function(name='vkCreateAccelerationStructureNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkAccelerationStructureCreateInfoNV*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pAccelerationStructure', type='VkAccelerationStructureNV*')
)

Function(name='vkCreateBuffer', enabled=True, type=CreateBuffer, stateTrack=True, runWrap=True, recWrap=True, recExecWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkBufferCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pBuffer', type='VkBuffer*', wrapType='CVkBuffer::CSMapArray', wrapParams='1, pBuffer')
)

Function(name='vkCreateBufferView', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkBufferViewCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pView', type='VkBufferView*', wrapType='CVkBufferView::CSMapArray', wrapParams='1, pView')
)

Function(name='vkCreateCommandPool', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkCommandPoolCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pCommandPool', type='VkCommandPool*', wrapType='CVkCommandPool::CSMapArray', wrapParams='1, pCommandPool')
)

Function(name='vkCreateComputePipelines', enabled=True, type=Param, stateTrack=True, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipelineCache', type='VkPipelineCache'),
arg3=ArgDef(name='createInfoCount', type='uint32_t'),
arg4=ArgDef(name='pCreateInfos', type='const VkComputePipelineCreateInfo*', wrapType='CVkComputePipelineCreateInfoArray', wrapParams='createInfoCount, pCreateInfos', count='createInfoCount'),
arg5=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg6=ArgDef(name='pPipelines', type='VkPipeline*', wrapType='CVkPipeline::CSMapArray', wrapParams='createInfoCount, pPipelines', count='createInfoCount')
)

Function(name='vkCreateDebugReportCallbackEXT', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkDebugReportCallbackCreateInfoEXT*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pCallback', type='VkDebugReportCallbackEXT*')
)

Function(name='vkCreateDebugUtilsMessengerEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkDebugUtilsMessengerCreateInfoEXT*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pMessenger', type='VkDebugUtilsMessengerEXT*')
)

Function(name='vkCreateDeferredOperationKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg3=ArgDef(name='pDeferredOperation', type='VkDeferredOperationKHR*')
)

Function(name='vkCreateDescriptorPool', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDescriptorPoolCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pDescriptorPool', type='VkDescriptorPool*', wrapType='CVkDescriptorPool::CSMapArray', wrapParams='1, pDescriptorPool')
)

Function(name='vkCreateDescriptorSetLayout', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDescriptorSetLayoutCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSetLayout', type='VkDescriptorSetLayout*', wrapType='CVkDescriptorSetLayout::CSMapArray', wrapParams='1, pSetLayout')
)

Function(name='vkCreateDescriptorUpdateTemplate', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDescriptorUpdateTemplateCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pDescriptorUpdateTemplate', type='VkDescriptorUpdateTemplate*', wrapType='CVkDescriptorUpdateTemplate::CSMapArray', wrapParams='1, pDescriptorUpdateTemplate')
)

Function(name='vkCreateDescriptorUpdateTemplateKHR', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDescriptorUpdateTemplateCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pDescriptorUpdateTemplate', type='VkDescriptorUpdateTemplate*', wrapType='CVkDescriptorUpdateTemplate::CSMapArray', wrapParams='1, pDescriptorUpdateTemplate')
)

Function(name='vkCreateDevice', enabled=True, type=Param, stateTrack=True, recWrap=True, recExecWrap=True, runWrap=True, ccodeWrap=True, customDriver=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDeviceCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pDevice', type='VkDevice*', wrapType='CVkDevice::CSMapArray', wrapParams='1, pDevice')
)

#Function(name='vkCreateDirectFBSurfaceEXT', enabled=False, type=Param,
#retV=RetDef(type='VkResult'),
#arg1=ArgDef(name='instance', type='VkInstance'),
#arg2=ArgDef(name='pCreateInfo', type='const VkDirectFBSurfaceCreateInfoEXT*'),
#arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
#arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*')
#)

Function(name='vkCreateDisplayModeKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR'),
arg3=ArgDef(name='pCreateInfo', type='const VkDisplayModeCreateInfoKHR*'),
arg4=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg5=ArgDef(name='pMode', type='VkDisplayModeKHR*')
)

Function(name='vkCreateDisplayPlaneSurfaceKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkDisplaySurfaceCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*')
)

Function(name='vkCreateEvent', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkEventCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pEvent', type='VkEvent*', wrapType='CVkEvent::CSMapArray', wrapParams='1, pEvent')
)

Function(name='vkCreateFence', enabled=True, type=Param, stateTrack=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkFenceCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pFence', type='VkFence*', wrapType='CVkFence::CSMapArray', wrapParams='1, pFence')
)

Function(name='vkCreateFramebuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkFramebufferCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pFramebuffer', type='VkFramebuffer*', wrapType='CVkFramebuffer::CSMapArray', wrapParams='1, pFramebuffer')
)

Function(name='vkCreateGraphicsPipelines', enabled=True, type=Param, stateTrack=True, runWrap=True, ccodeWriteWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipelineCache', type='VkPipelineCache'),
arg3=ArgDef(name='createInfoCount', type='uint32_t'),
arg4=ArgDef(name='pCreateInfos', type='const VkGraphicsPipelineCreateInfo*', wrapType='CVkGraphicsPipelineCreateInfoArray', wrapParams='createInfoCount, pCreateInfos', count='createInfoCount'),
arg5=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg6=ArgDef(name='pPipelines', type='VkPipeline*', wrapType='CVkPipeline::CSMapArray', wrapParams='createInfoCount, pPipelines', count='createInfoCount')
)

Function(name='vkCreateHeadlessSurfaceEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkHeadlessSurfaceCreateInfoEXT*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*')
)

Function(name='vkCreateIOSSurfaceMVK', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkIOSSurfaceCreateInfoMVK*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*')
)

Function(name='vkCreateImage', enabled=True, type=CreateImage, stateTrack=True, recExecWrap=True, runWrap=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkImageCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pImage', type='VkImage*', wrapType='CVkImage::CSMapArray', wrapParams='1, pImage')
)

Function(name='vkCreateImageView', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkImageViewCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pView', type='VkImageView*', wrapType='CVkImageView::CSMapArray', wrapParams='1, pView')
)

Function(name='vkCreateIndirectCommandsLayoutNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkIndirectCommandsLayoutCreateInfoNV*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pIndirectCommandsLayout', type='VkIndirectCommandsLayoutNV*')
)

Function(name='vkCreateInstance', enabled=True, type=Param, recExecWrap=True, runWrap=True, stateTrack=True, customDriver=True, level=GlobalLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='pCreateInfo', type='const VkInstanceCreateInfo*'),
arg2=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg3=ArgDef(name='pInstance', type='VkInstance*', wrapType='CVkInstance::CSMapArray', wrapParams='1, pInstance')
)

Function(name='vkCreateMacOSSurfaceMVK', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkMacOSSurfaceCreateInfoMVK*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*')
)

Function(name='vkCreateMicromapEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkMicromapCreateInfoEXT*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pMicromap', type='VkMicromapEXT*')
)

Function(name='vkCreateOpticalFlowSessionNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkOpticalFlowSessionCreateInfoNV*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pSession', type='VkOpticalFlowSessionNV*')
)

Function(name='vkCreatePipelineCache', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkPipelineCacheCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pPipelineCache', type='VkPipelineCache*', wrapType='CVkPipelineCache::CSMapArray', wrapParams='1, pPipelineCache')
)

Function(name='vkCreatePipelineCache', enabled=True, type=Param, stateTrack=True, version=1,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkPipelineCacheCreateInfo*', wrapType='CVkPipelineCacheCreateInfo_V1'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pPipelineCache', type='VkPipelineCache*', wrapType='CVkPipelineCache::CSMapArray', wrapParams='1, pPipelineCache')
)

Function(name='vkCreatePipelineLayout', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkPipelineLayoutCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pPipelineLayout', type='VkPipelineLayout*', wrapType='CVkPipelineLayout::CSMapArray', wrapParams='1, pPipelineLayout')
)

Function(name='vkCreatePrivateDataSlot', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkPrivateDataSlotCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pPrivateDataSlot', type='VkPrivateDataSlot*')
)

Function(name='vkCreatePrivateDataSlotEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkPrivateDataSlotCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pPrivateDataSlot', type='VkPrivateDataSlot*')
)

Function(name='vkCreateQueryPool', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkQueryPoolCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pQueryPool', type='VkQueryPool*', wrapType='CVkQueryPool::CSMapArray', wrapParams='1, pQueryPool')
)

Function(name='vkCreateRayTracingPipelinesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='deferredOperation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pipelineCache', type='VkPipelineCache'),
arg4=ArgDef(name='createInfoCount', type='uint32_t'),
arg5=ArgDef(name='pCreateInfos', type='const VkRayTracingPipelineCreateInfoKHR*'),
arg6=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg7=ArgDef(name='pPipelines', type='VkPipeline*')
)

Function(name='vkCreateRayTracingPipelinesNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipelineCache', type='VkPipelineCache'),
arg3=ArgDef(name='createInfoCount', type='uint32_t'),
arg4=ArgDef(name='pCreateInfos', type='const VkRayTracingPipelineCreateInfoNV*'),
arg5=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg6=ArgDef(name='pPipelines', type='VkPipeline*')
)

Function(name='vkCreateRenderPass', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkRenderPassCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pRenderPass', type='VkRenderPass*', wrapType='CVkRenderPass::CSMapArray', wrapParams='1, pRenderPass')
)

Function(name='vkCreateRenderPass2', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkRenderPassCreateInfo2*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pRenderPass', type='VkRenderPass*', wrapType='CVkRenderPass::CSMapArray', wrapParams='1, pRenderPass')
)

Function(name='vkCreateRenderPass2KHR', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkRenderPassCreateInfo2*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pRenderPass', type='VkRenderPass*', wrapType='CVkRenderPass::CSMapArray', wrapParams='1, pRenderPass')
)

Function(name='vkCreateSampler', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkSamplerCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSampler', type='VkSampler*', wrapType='CVkSampler::CSMapArray', wrapParams='1, pSampler')
)

Function(name='vkCreateSamplerYcbcrConversion', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkSamplerYcbcrConversionCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pYcbcrConversion', type='VkSamplerYcbcrConversion*')
)

Function(name='vkCreateSamplerYCbCrConversionKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkSamplerYcbcrConversionCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pYcbcrConversion', type='VkSamplerYcbcrConversion*')
)

Function(name='vkCreateSemaphore', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkSemaphoreCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSemaphore', type='VkSemaphore*', wrapType='CVkSemaphore::CSMapArray', wrapParams='1, pSemaphore')
)

Function(name='vkCreateShaderModule', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkShaderModuleCreateInfo*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pShaderModule', type='VkShaderModule*', wrapType='CVkShaderModule::CSMapArray', wrapParams='1, pShaderModule')
)

Function(name='vkCreateShadersEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='createInfoCount', type='uint32_t'),
arg3=ArgDef(name='pCreateInfos', type='const VkShaderCreateInfoEXT*'),
arg4=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg5=ArgDef(name='pShaders', type='VkShaderEXT*')
)

Function(name='vkCreateSharedSwapchainsKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchainCount', type='uint32_t'),
arg3=ArgDef(name='pCreateInfos', type='const VkSwapchainCreateInfoKHR*', count='swapchainCount'),
arg4=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg5=ArgDef(name='pSwapchains', type='VkSwapchainKHR*', count='swapchainCount')
)

Function(name='vkCreateSwapchainKHR', enabled=True, type=Param, stateTrack=True, runWrap=True, recExecWrap=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkSwapchainCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSwapchain', type='VkSwapchainKHR*', wrapType='CVkSwapchainKHR::CSMapArray', wrapParams='1, pSwapchain')
)

Function(name='vkCreateValidationCacheEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkValidationCacheCreateInfoEXT*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pValidationCache', type='VkValidationCacheEXT*', wrapType='CVkValidationCacheEXT::CSMapArray', wrapParams='1, pValidationCache')
)

Function(name='vkCreateVideoSessionKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkVideoSessionCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pVideoSession', type='VkVideoSessionKHR*')
)

Function(name='vkCreateVideoSessionParametersKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkVideoSessionParametersCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pVideoSessionParameters', type='VkVideoSessionParametersKHR*')
)

Function(name='vkCreateWaylandSurfaceKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkWaylandSurfaceCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*')
)

Function(name='vkCreateWin32SurfaceKHR', enabled=True, type=Param, preToken='CGitsVkCreateNativeWindow(pCreateInfo->hinstance, pCreateInfo->hwnd)', postToken='CGitsVkEnumerateDisplayMonitors(true)', stateTrack=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkWin32SurfaceCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*', wrapType='CVkSurfaceKHR::CSMapArray', wrapParams='1, pSurface')
)

Function(name='vkCreateXcbSurfaceKHR', enabled=True, type=Param, preToken='CGitsVkCreateNativeWindow(pCreateInfo->connection, pCreateInfo->window)', stateTrack=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkXcbSurfaceCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*', wrapType='CVkSurfaceKHR::CSMapArray', wrapParams='1, pSurface')
)

Function(name='vkCreateXlibSurfaceKHR', enabled=True, type=Param, preToken='CGitsVkCreateXlibWindow(pCreateInfo->dpy, pCreateInfo->window)', stateTrack=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pCreateInfo', type='const VkXlibSurfaceCreateInfoKHR*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper'),
arg4=ArgDef(name='pSurface', type='VkSurfaceKHR*', wrapType='CVkSurfaceKHR::CSMapArray', wrapParams='1, pSurface')
)

Function(name='vkDebugMarkerSetObjectNameEXT', enabled=True, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pNameInfo', type='const VkDebugMarkerObjectNameInfoEXT*')
)

Function(name='vkDebugMarkerSetObjectTagEXT', enabled=True, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pTagInfo', type='const VkDebugMarkerObjectTagInfoEXT*')
)

Function(name='vkDebugReportMessageEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='flags', type='VkDebugReportFlagsEXT'),
arg3=ArgDef(name='objectType', type='VkDebugReportObjectTypeEXT'),
arg4=ArgDef(name='object', type='uint64_t'),
arg5=ArgDef(name='location', type='size_t'),
arg6=ArgDef(name='messageCode', type='int32_t'),
arg7=ArgDef(name='pLayerPrefix', type='const char*'),
arg8=ArgDef(name='pMessage', type='const char*')
)

Function(name='vkDeferredOperationJoinKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='operation', type='VkDeferredOperationKHR')
)

Function(name='vkDestroyAccelerationStructureKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='accelerationStructure', type='VkAccelerationStructureKHR'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyAccelerationStructureNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='accelerationStructure', type='VkAccelerationStructureNV'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyBuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='buffer', type='VkBuffer', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyBufferView', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='bufferView', type='VkBufferView', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyCommandPool', enabled=True, type=Param, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='commandPool', type='VkCommandPool', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyDebugReportCallbackEXT', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='callback', type='VkDebugReportCallbackEXT'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyDebugUtilsMessengerEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='messenger', type='VkDebugUtilsMessengerEXT'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyDeferredOperationKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='operation', type='VkDeferredOperationKHR'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyDescriptorPool', enabled=True, type=Param, stateTrack=True, recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorPool', type='VkDescriptorPool', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyDescriptorSetLayout', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorSetLayout', type='VkDescriptorSetLayout', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyDescriptorUpdateTemplate', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorUpdateTemplate', type='VkDescriptorUpdateTemplate', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyDescriptorUpdateTemplateKHR', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorUpdateTemplate', type='VkDescriptorUpdateTemplate', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyDevice', enabled=True, type=Param, stateTrack=True, runWrap=True, customDriver=True, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice', removeMapping=True),
arg2=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyEvent', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='event', type='VkEvent', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyFence', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='fence', type='VkFence', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyFramebuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='framebuffer', type='VkFramebuffer', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyImage', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyImageView', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='imageView', type='VkImageView', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyIndirectCommandsLayoutNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='indirectCommandsLayout', type='VkIndirectCommandsLayoutNV'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyInstance', enabled=True, type=Param, stateTrack=True, runWrap=True, customDriver=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='instance', type='VkInstance', removeMapping=True),
arg2=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyMicromapEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='micromap', type='VkMicromapEXT'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyOpticalFlowSessionNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='session', type='VkOpticalFlowSessionNV'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyPipeline', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyPipelineCache', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipelineCache', type='VkPipelineCache', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyPipelineLayout', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipelineLayout', type='VkPipelineLayout', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyPrivateDataSlot', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='privateDataSlot', type='VkPrivateDataSlot'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyPrivateDataSlotEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='privateDataSlot', type='VkPrivateDataSlot'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyQueryPool', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='queryPool', type='VkQueryPool', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyRenderPass', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='renderPass', type='VkRenderPass', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroySampler', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='sampler', type='VkSampler', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroySamplerYcbcrConversion', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='ycbcrConversion', type='VkSamplerYcbcrConversion'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroySamplerYCbCrConversionKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='ycbcrConversion', type='VkSamplerYcbcrConversion'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroySemaphore', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='semaphore', type='VkSemaphore', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyShaderEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='shader', type='VkShaderEXT'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyShaderModule', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='shaderModule', type='VkShaderModule', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroySurfaceKHR', enabled=True, type=Param, stateTrack=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroySwapchainKHR', enabled=True, type=Param, stateTrack=True, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyValidationCacheEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='validationCache', type='VkValidationCacheEXT'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkDestroyVideoSessionKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='videoSession', type='VkVideoSessionKHR'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDestroyVideoSessionParametersKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='videoSessionParameters', type='VkVideoSessionParametersKHR'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*')
)

Function(name='vkDeviceWaitIdle', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice')
)

Function(name='vkDisplayPowerControlEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR'),
arg3=ArgDef(name='pDisplayPowerInfo', type='const VkDisplayPowerInfoEXT*')
)

Function(name='vkEndCommandBuffer', enabled=True, type=Param, stateTrack=True, recWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer')
)

Function(name='vkEnumerateDeviceExtensionProperties', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pLayerName', type='const char*', wrapType='Cchar::CSArray', wrapParams='pLayerName, \'\\0\', 1'),
arg3=ArgDef(name='pPropertyCount', type='uint32_t*', wrapParams='1, pPropertyCount'),
arg4=ArgDef(name='pProperties', type='VkExtensionProperties*', wrapType='CVkExtensionPropertiesArray', wrapParams='*pPropertyCount, pProperties', count='pPropertyCount')
)

Function(name='vkEnumerateDeviceLayerProperties', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg3=ArgDef(name='pProperties', type='VkLayerProperties*', count='pPropertyCount')
)

Function(name='vkEnumerateInstanceExtensionProperties', enabled=False, type=Param, recExecWrap=True, level=GlobalLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='pLayerName', type='const char*', wrapType='Cchar::CSArray', wrapParams='pLayerName, \'\\0\', 1'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*', wrapParams='1, pPropertyCount'),
arg3=ArgDef(name='pProperties', type='VkExtensionProperties*', wrapType='CVkExtensionPropertiesArray', wrapParams='*pPropertyCount, pProperties', count='pPropertyCount')
)

Function(name='vkEnumerateInstanceLayerProperties', enabled=False, type=Param, recExecWrap=True, level=GlobalLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='pPropertyCount', type='uint32_t*', wrapParams='1, pPropertyCount'),
arg2=ArgDef(name='pProperties', type='VkLayerProperties*', wrapType='CVkLayerPropertiesArray', wrapParams='*pPropertyCount, pProperties', count='pPropertyCount')
)

Function(name='vkEnumerateInstanceVersion', enabled=False, type=Param, level=GlobalLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='pApiVersion', type='uint32_t*', wrapParams='1, pApiVersion')
)

Function(name='vkEnumeratePhysicalDeviceGroups', enabled=True, type=Param, runWrap=True, ccodeWrap=True, stateTrack=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pPhysicalDeviceGroupCount', type='uint32_t*', wrapParams='1, pPhysicalDeviceGroupCount'),
arg3=ArgDef(name='pPhysicalDeviceGroupProperties', type='VkPhysicalDeviceGroupProperties*', wrapType='CVkPhysicalDeviceGroupPropertiesArray', wrapParams='*pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties', count='*pPhysicalDeviceGroupCount')
)

Function(name='vkEnumeratePhysicalDeviceGroupsKHR', enabled=True, type=Param, runWrap=True, ccodeWrap=True, stateTrack=True, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pPhysicalDeviceGroupCount', type='uint32_t*', wrapParams='1, pPhysicalDeviceGroupCount'),
arg3=ArgDef(name='pPhysicalDeviceGroupProperties', type='VkPhysicalDeviceGroupProperties*', wrapType='CVkPhysicalDeviceGroupPropertiesArray', wrapParams='*pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties', count='*pPhysicalDeviceGroupCount')
)

Function(name='vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
arg3=ArgDef(name='pCounterCount', type='uint32_t*'),
arg4=ArgDef(name='pCounters', type='VkPerformanceCounterKHR*'),
arg5=ArgDef(name='pCounterDescriptions', type='VkPerformanceCounterDescriptionKHR*')
)

Function(name='vkEnumeratePhysicalDevices', enabled=True, type=Param, runWrap=True, stateTrack=True, ccodeWrap=True, ccodePostActionNeeded=False, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pPhysicalDeviceCount', type='uint32_t*', wrapParams='1, pPhysicalDeviceCount'),
arg3=ArgDef(name='pPhysicalDevices', type='VkPhysicalDevice*', wrapType='CVkPhysicalDevice::CSMapArray', wrapParams='*pPhysicalDeviceCount, pPhysicalDevices', count='pPhysicalDeviceCount')
)

Function(name='vkFlushMappedMemoryRanges', enabled=True, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memoryRangeCount', type='uint32_t'),
arg3=ArgDef(name='pMemoryRanges', type='const VkMappedMemoryRange*', wrapType='CVkMappedMemoryRangeArray', wrapParams='memoryRangeCount, pMemoryRanges', count='memoryRangeCount')
)

Function(name='vkFreeCommandBuffers', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='commandPool', type='VkCommandPool'),
arg3=ArgDef(name='commandBufferCount', type='uint32_t'),
arg4=ArgDef(name='pCommandBuffers', type='const VkCommandBuffer*', wrapType='CVkCommandBuffer::CSArray', wrapParams='commandBufferCount, pCommandBuffers', count='commandBufferCount', removeMapping=True)
)

Function(name='vkFreeDescriptorSets', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorPool', type='VkDescriptorPool'),
arg3=ArgDef(name='descriptorSetCount', type='uint32_t'),
arg4=ArgDef(name='pDescriptorSets', type='const VkDescriptorSet*', wrapType='CVkDescriptorSet::CSArray', wrapParams='descriptorSetCount, pDescriptorSets', count='descriptorSetCount', removeMapping=True)
)

Function(name='vkFreeMemory', enabled=True, type=Param, stateTrack=True, recExecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory', removeMapping=True),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*', wrapType='CNullWrapper')
)

Function(name='vkGetAccelerationStructureBuildSizesKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='buildType', type='VkAccelerationStructureBuildTypeKHR'),
arg3=ArgDef(name='pBuildInfo', type='const VkAccelerationStructureBuildGeometryInfoKHR*'),
arg4=ArgDef(name='pMaxPrimitiveCounts', type='const uint32_t*'),
arg5=ArgDef(name='pSizeInfo', type='VkAccelerationStructureBuildSizesInfoKHR*')
)

Function(name='vkGetAccelerationStructureDeviceAddressKHR', enabled=False, type=Param,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkAccelerationStructureDeviceAddressInfoKHR*')
)

Function(name='vkGetAccelerationStructureHandleNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='accelerationStructure', type='VkAccelerationStructureNV'),
arg3=ArgDef(name='dataSize', type='size_t'),
arg4=ArgDef(name='pData', type='void*')
)

Function(name='vkGetAccelerationStructureMemoryRequirementsNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkAccelerationStructureMemoryRequirementsInfoNV*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkAccelerationStructureCaptureDescriptorDataInfoEXT*'),
arg3=ArgDef(name='pData', type='void*')
)

Function(name='vkGetBufferDeviceAddress', enabled=False, type=Param, stateTrack=True, runWrap=True, recWrap=True,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferDeviceAddressEXT', enabled=False, type=Param, stateTrack=True, runWrap=True, recWrap=True,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferDeviceAddressKHR', enabled=False, type=Param, stateTrack=True, runWrap=True, recWrap=True,
retV=RetDef(type='VkDeviceAddress'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferMemoryRequirements', enabled=True, type=Param, runWrap=True, recExecWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='buffer', type='VkBuffer'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements*')
)

Function(name='vkGetBufferMemoryRequirements2', enabled=True, type=Param, runWrap=True, recExecWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferMemoryRequirementsInfo2*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetBufferMemoryRequirements2KHR', enabled=True, type=Param, runWrap=True, recExecWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferMemoryRequirementsInfo2*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetBufferOpaqueCaptureAddress', enabled=False, type=Param,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferOpaqueCaptureAddressKHR', enabled=False, type=Param,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferDeviceAddressInfo*')
)

Function(name='vkGetBufferOpaqueCaptureDescriptorDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkBufferCaptureDescriptorDataInfoEXT*'),
arg3=ArgDef(name='pData', type='void*')
)

Function(name='vkGetCalibratedTimestampsEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='timestampCount', type='uint32_t'),
arg3=ArgDef(name='pTimestampInfos', type='const VkCalibratedTimestampInfoEXT*'),
arg4=ArgDef(name='pTimestamps', type='uint64_t*'),
arg5=ArgDef(name='pMaxDeviation', type='uint64_t*')
)

Function(name='vkGetCommandPoolMemoryConsumption', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='commandPool', type='VkCommandPool'),
arg3=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg4=ArgDef(name='pConsumption', type='VkCommandPoolMemoryConsumption*')
)

Function(name='vkGetDeferredOperationMaxConcurrencyKHR', enabled=False, type=Param,
retV=RetDef(type='uint32_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='operation', type='VkDeferredOperationKHR')
)

Function(name='vkGetDeferredOperationResultKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='operation', type='VkDeferredOperationKHR')
)

Function(name='vkGetDescriptorEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pDescriptorInfo', type='const VkDescriptorGetInfoEXT*'),
arg3=ArgDef(name='dataSize', type='size_t'),
arg4=ArgDef(name='pDescriptor', type='void*')
)

Function(name='vkGetDescriptorSetLayoutBindingOffsetEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='layout', type='VkDescriptorSetLayout'),
arg3=ArgDef(name='binding', type='uint32_t'),
arg4=ArgDef(name='pOffset', type='VkDeviceSize*')
)

Function(name='vkGetDescriptorSetLayoutSizeEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='layout', type='VkDescriptorSetLayout'),
arg3=ArgDef(name='pLayoutSizeInBytes', type='VkDeviceSize*')
)

Function(name='vkGetDescriptorSetLayoutSupport', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDescriptorSetLayoutCreateInfo*'),
arg3=ArgDef(name='pSupport', type='VkDescriptorSetLayoutSupport*')
)

Function(name='vkGetDescriptorSetLayoutSupportKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkDescriptorSetLayoutCreateInfo*'),
arg3=ArgDef(name='pSupport', type='VkDescriptorSetLayoutSupport*')
)

Function(name='vkGetDeviceAccelerationStructureCompatibilityKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pVersionInfo', type='const VkAccelerationStructureVersionInfoKHR*'),
arg3=ArgDef(name='pCompatibility', type='VkAccelerationStructureCompatibilityKHR*')
)

Function(name='vkGetDeviceBufferMemoryRequirements', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceBufferMemoryRequirements*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetDeviceBufferMemoryRequirementsKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceBufferMemoryRequirements*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetDeviceFaultInfoEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pFaultCounts', type='VkDeviceFaultCountsEXT*'),
arg3=ArgDef(name='pFaultInfo', type='VkDeviceFaultInfoEXT*')
)

Function(name='vkGetDeviceGroupPeerMemoryFeatures', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='heapIndex', type='uint32_t'),
arg3=ArgDef(name='localDeviceIndex', type='uint32_t'),
arg4=ArgDef(name='remoteDeviceIndex', type='uint32_t'),
arg5=ArgDef(name='pPeerMemoryFeatures', type='VkPeerMemoryFeatureFlags*')
)

Function(name='vkGetDeviceGroupPeerMemoryFeaturesKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='heapIndex', type='uint32_t'),
arg3=ArgDef(name='localDeviceIndex', type='uint32_t'),
arg4=ArgDef(name='remoteDeviceIndex', type='uint32_t'),
arg5=ArgDef(name='pPeerMemoryFeatures', type='VkPeerMemoryFeatureFlags*')
)

Function(name='vkGetDeviceGroupPresentCapabilitiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pDeviceGroupPresentCapabilities', type='VkDeviceGroupPresentCapabilitiesKHR*')
)

Function(name='vkGetDeviceGroupSurfacePresentModes2EXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pSurfaceInfo', type='const VkPhysicalDeviceSurfaceInfo2KHR*'),
arg3=ArgDef(name='pModes', type='VkDeviceGroupPresentModeFlagsKHR*')
)

Function(name='vkGetDeviceGroupSurfacePresentModesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR'),
arg3=ArgDef(name='pModes', type='VkDeviceGroupPresentModeFlagsKHR*')
)

Function(name='vkGetDeviceImageMemoryRequirements', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceImageMemoryRequirements*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetDeviceImageMemoryRequirementsKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceImageMemoryRequirements*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetDeviceImageSparseMemoryRequirements', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceImageMemoryRequirements*'),
arg3=ArgDef(name='pSparseMemoryRequirementCount', type='uint32_t*'),
arg4=ArgDef(name='pSparseMemoryRequirements', type='VkSparseImageMemoryRequirements2*')
)

Function(name='vkGetDeviceImageSparseMemoryRequirementsKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceImageMemoryRequirements*'),
arg3=ArgDef(name='pSparseMemoryRequirementCount', type='uint32_t*'),
arg4=ArgDef(name='pSparseMemoryRequirements', type='VkSparseImageMemoryRequirements2*')
)

Function(name='vkGetDeviceMemoryCommitment', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory'),
arg3=ArgDef(name='pCommittedMemoryInBytes', type='VkDeviceSize*')
)

Function(name='vkGetDeviceMemoryOpaqueCaptureAddress', enabled=False, type=Param,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceMemoryOpaqueCaptureAddressInfo*')
)

Function(name='vkGetDeviceMemoryOpaqueCaptureAddressKHR', enabled=False, type=Param,
retV=RetDef(type='uint64_t'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkDeviceMemoryOpaqueCaptureAddressInfo*')
)

Function(name='vkGetDeviceMicromapCompatibilityEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pVersionInfo', type='const VkMicromapVersionInfoEXT*'),
arg3=ArgDef(name='pCompatibility', type='VkAccelerationStructureCompatibilityKHR*')
)

Function(name='vkGetDeviceProcAddr', enabled=False, type=Param, recExecWrap=True, pluginWrap=True, customDriver=True,
retV=RetDef(type='PFN_vkVoidFunction'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pName', type='const char*')
)

Function(name='vkGetDeviceQueue', enabled=True, type=Param, stateTrack=False, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
arg3=ArgDef(name='queueIndex', type='uint32_t'),
arg4=ArgDef(name='pQueue', type='VkQueue*', wrapType='CVkQueue::CSMapArray', wrapParams='1, pQueue')
)

Function(name='vkGetDeviceQueue2', enabled=True, type=Param, stateTrack=False, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pQueueInfo', type='const VkDeviceQueueInfo2*'),
arg3=ArgDef(name='pQueue', type='VkQueue*', wrapType='CVkQueue::CSMapArray', wrapParams='1, pQueue')
)

Function(name='vkGetDisplayModeProperties2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR'),
arg3=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg4=ArgDef(name='pProperties', type='VkDisplayModeProperties2KHR*')
)

Function(name='vkGetDisplayModePropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR'),
arg3=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg4=ArgDef(name='pProperties', type='VkDisplayModePropertiesKHR*', count='pPropertyCount')
)

Function(name='vkGetDisplayPlaneCapabilities2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pDisplayPlaneInfo', type='const VkDisplayPlaneInfo2KHR*'),
arg3=ArgDef(name='pCapabilities', type='VkDisplayPlaneCapabilities2KHR*')
)

Function(name='vkGetDisplayPlaneCapabilitiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='mode', type='VkDisplayModeKHR'),
arg3=ArgDef(name='planeIndex', type='uint32_t'),
arg4=ArgDef(name='pCapabilities', type='VkDisplayPlaneCapabilitiesKHR*')
)

Function(name='vkGetDisplayPlaneSupportedDisplaysKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='planeIndex', type='uint32_t'),
arg3=ArgDef(name='pDisplayCount', type='uint32_t*'),
arg4=ArgDef(name='pDisplays', type='VkDisplayKHR*', count='pDisplayCount')
)

Function(name='vkGetDrmDisplayEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='drmFd', type='int32_t'),
arg3=ArgDef(name='connectorId', type='uint32_t'),
arg4=ArgDef(name='display', type='VkDisplayKHR*')
)

Function(name='vkGetEventStatus', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='event', type='VkEvent')
)

Function(name='vkGetFaultData', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='faultQueryBehavior', type='VkFaultQueryBehavior'),
arg3=ArgDef(name='pUnrecordedFaults', type='VkBool32*'),
arg4=ArgDef(name='pFaultCount', type='uint32_t*'),
arg5=ArgDef(name='pFaults', type='VkFaultData*')
)

Function(name='vkGetFenceFdKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pGetFdInfo', type='const VkFenceGetFdInfoKHR*'),
arg3=ArgDef(name='pFd', type='int*')
)

Function(name='vkGetFenceStatus', enabled=True, type=Param, runWrap=True, recExecWrap=True, ccodeWriteWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='fence', type='VkFence')
)

Function(name='vkGetFenceWin32HandleKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pGetWin32HandleInfo', type='const VkFenceGetWin32HandleInfoKHR*'),
arg3=ArgDef(name='pHandle', type='HANDLE*')
)

Function(name='vkGetGeneratedCommandsMemoryRequirementsNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkGeneratedCommandsMemoryRequirementsInfoNV*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetImageDrmFormatModifierPropertiesEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='pProperties', type='VkImageDrmFormatModifierPropertiesEXT*')
)

Function(name='vkGetImageMemoryRequirements', enabled=True, type=Param, runWrap=True, recExecWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements*')
)

Function(name='vkGetImageMemoryRequirements2', enabled=True, type=Param, runWrap=True, recExecWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkImageMemoryRequirementsInfo2*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetImageMemoryRequirements2KHR', enabled=True, type=Param, runWrap=True, recExecWrap=True, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkImageMemoryRequirementsInfo2*'),
arg3=ArgDef(name='pMemoryRequirements', type='VkMemoryRequirements2*')
)

Function(name='vkGetImageOpaqueCaptureDescriptorDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkImageCaptureDescriptorDataInfoEXT*'),
arg3=ArgDef(name='pData', type='void*')
)

Function(name='vkGetImageSparseMemoryRequirements', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='pSparseMemoryRequirementCount', type='uint32_t*'),
arg4=ArgDef(name='pSparseMemoryRequirements', type='VkSparseImageMemoryRequirements*', count='pSparseMemoryRequirementCount')
)

Function(name='vkGetImageSparseMemoryRequirements2', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkImageSparseMemoryRequirementsInfo2*'),
arg3=ArgDef(name='pSparseMemoryRequirementCount', type='uint32_t*'),
arg4=ArgDef(name='pSparseMemoryRequirements', type='VkSparseImageMemoryRequirements2*', count='pSparseMemoryRequirementCount')
)

Function(name='vkGetImageSparseMemoryRequirements2KHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkImageSparseMemoryRequirementsInfo2*'),
arg3=ArgDef(name='pSparseMemoryRequirementCount', type='uint32_t*'),
arg4=ArgDef(name='pSparseMemoryRequirements', type='VkSparseImageMemoryRequirements2*', count='pSparseMemoryRequirementCount')
)

Function(name='vkGetImageSubresourceLayout', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='pSubresource', type='const VkImageSubresource*'),
arg4=ArgDef(name='pLayout', type='VkSubresourceLayout*')
)

Function(name='vkGetImageSubresourceLayout2EXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='image', type='VkImage'),
arg3=ArgDef(name='pSubresource', type='const VkImageSubresource2EXT*'),
arg4=ArgDef(name='pLayout', type='VkSubresourceLayout2EXT*')
)

Function(name='vkGetImageViewOpaqueCaptureDescriptorDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkImageViewCaptureDescriptorDataInfoEXT*'),
arg3=ArgDef(name='pData', type='void*')
)

Function(name='vkGetInstanceProcAddr', enabled=False, type=Param, recExecWrap=True, pluginWrap=True, customDriver=True, level=GlobalLevel,
retV=RetDef(type='PFN_vkVoidFunction'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pName', type='const char*')
)

Function(name='vkGetMemoryFdKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pGetFdInfo', type='const VkMemoryGetFdInfoKHR*'),
arg3=ArgDef(name='pFd', type='int*')
)

Function(name='vkGetMemoryFdPropertiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits'),
arg3=ArgDef(name='fd', type='int'),
arg4=ArgDef(name='pMemoryFdProperties', type='VkMemoryFdPropertiesKHR*')
)

Function(name='vkGetMemoryHostPointerPropertiesEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits'),
arg3=ArgDef(name='pHostPointer', type='const void*'),
arg4=ArgDef(name='pMemoryHostPointerProperties', type='VkMemoryHostPointerPropertiesEXT*')
)

Function(name='vkGetMemoryRemoteAddressNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pMemoryGetRemoteAddressInfo', type='const VkMemoryGetRemoteAddressInfoNV*'),
arg3=ArgDef(name='pAddress', type='VkRemoteAddressNV*')
)

Function(name='vkGetMemoryWin32HandleKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pGetWin32HandleInfo', type='const VkMemoryGetWin32HandleInfoKHR*'),
arg3=ArgDef(name='pHandle', type='HANDLE*')
)

Function(name='vkGetMemoryWin32HandleNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory'),
arg3=ArgDef(name='handleType', type='VkExternalMemoryHandleTypeFlagsNV'),
arg4=ArgDef(name='pHandle', type='HANDLE*')
)

Function(name='vkGetMemoryWin32HandlePropertiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits'),
arg3=ArgDef(name='handle', type='HANDLE'),
arg4=ArgDef(name='pMemoryWin32HandleProperties', type='VkMemoryWin32HandlePropertiesKHR*')
)

Function(name='vkGetMicromapBuildSizesEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='buildType', type='VkAccelerationStructureBuildTypeKHR'),
arg3=ArgDef(name='pBuildInfo', type='const VkMicromapBuildInfoEXT*'),
arg4=ArgDef(name='pSizeInfo', type='VkMicromapBuildSizesInfoEXT*')
)

Function(name='vkGetPastPresentationTimingGOOGLE', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR'),
arg3=ArgDef(name='pPresentationTimingCount', type='uint32_t*'),
arg4=ArgDef(name='pPresentationTimings', type='VkPastPresentationTimingGOOGLE*', count='pPresentationTimingCount')
)

Function(name='vkGetPerformanceParameterINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='parameter', type='VkPerformanceParameterTypeINTEL'),
arg3=ArgDef(name='pValue', type='VkPerformanceValueINTEL*')
)

Function(name='vkGetPhysicalDeviceCalibrateableTimeDomainsEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pTimeDomainCount', type='uint32_t*'),
arg3=ArgDef(name='pTimeDomains', type='VkTimeDomainEXT*')
)

Function(name='vkGetPhysicalDeviceCooperativeMatrixPropertiesNV', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg3=ArgDef(name='pProperties', type='VkCooperativeMatrixPropertiesNV*')
)

#Function(name='vkGetPhysicalDeviceDirectFBPresentationSupportEXT', enabled=False, type=Param, level=InstanceLevel,
#retV=RetDef(type='VkBool32'),
#arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
#arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
#arg3=ArgDef(name='dfb', type='IDirectFB*')
#)

Function(name='vkGetPhysicalDeviceDisplayPlaneProperties2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg3=ArgDef(name='pProperties', type='VkDisplayPlaneProperties2KHR*')
)

Function(name='vkGetPhysicalDeviceDisplayPlanePropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg3=ArgDef(name='pProperties', type='VkDisplayPlanePropertiesKHR*', count='pPropertyCount')
)

Function(name='vkGetPhysicalDeviceDisplayPropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg3=ArgDef(name='pProperties', type='VkDisplayPropertiesKHR*', count='pPropertyCount')
)

Function(name='vkGetPhysicalDeviceDisplayPropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg3=ArgDef(name='pProperties', type='VkDisplayPropertiesKHR*')
)

Function(name='vkGetPhysicalDeviceExternalBufferProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pExternalBufferInfo', type='const VkPhysicalDeviceExternalBufferInfo*'),
arg3=ArgDef(name='pExternalBufferProperties', type='VkExternalBufferProperties*')
)

Function(name='vkGetPhysicalDeviceExternalBufferPropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pExternalBufferInfo', type='const VkPhysicalDeviceExternalBufferInfo*'),
arg3=ArgDef(name='pExternalBufferProperties', type='VkExternalBufferProperties*')
)

Function(name='vkGetPhysicalDeviceExternalFenceProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pExternalFenceInfo', type='const VkPhysicalDeviceExternalFenceInfo*'),
arg3=ArgDef(name='pExternalFenceProperties', type='VkExternalFenceProperties*')
)

Function(name='vkGetPhysicalDeviceExternalFencePropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pExternalFenceInfo', type='const VkPhysicalDeviceExternalFenceInfo*'),
arg3=ArgDef(name='pExternalFenceProperties', type='VkExternalFenceProperties*')
)

Function(name='vkGetPhysicalDeviceExternalImageFormatPropertiesNV', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='type', type='VkImageType'),
arg4=ArgDef(name='tiling', type='VkImageTiling'),
arg5=ArgDef(name='usage', type='VkImageUsageFlags'),
arg6=ArgDef(name='flags', type='VkImageCreateFlags'),
arg7=ArgDef(name='externalHandleType', type='VkExternalMemoryHandleTypeFlagsNV'),
arg8=ArgDef(name='pExternalImageFormatProperties', type='VkExternalImageFormatPropertiesNV*')
)

Function(name='vkGetPhysicalDeviceExternalSemaphoreProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pExternalSemaphoreInfo', type='const VkPhysicalDeviceExternalSemaphoreInfo*'),
arg3=ArgDef(name='pExternalSemaphoreProperties', type='VkExternalSemaphoreProperties*')
)

Function(name='vkGetPhysicalDeviceExternalSemaphorePropertiesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pExternalSemaphoreInfo', type='const VkPhysicalDeviceExternalSemaphoreInfo*'),
arg3=ArgDef(name='pExternalSemaphoreProperties', type='VkExternalSemaphoreProperties*')
)

Function(name='vkGetPhysicalDeviceFeatures', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pFeatures', type='VkPhysicalDeviceFeatures*')
)

Function(name='vkGetPhysicalDeviceFeatures2', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pFeatures', type='VkPhysicalDeviceFeatures2*')
)

Function(name='vkGetPhysicalDeviceFeatures2KHR', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pFeatures', type='VkPhysicalDeviceFeatures2*')
)

Function(name='vkGetPhysicalDeviceFormatProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='pFormatProperties', type='VkFormatProperties*')
)

Function(name='vkGetPhysicalDeviceFormatProperties2', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='pFormatProperties', type='VkFormatProperties2*')
)

Function(name='vkGetPhysicalDeviceFormatProperties2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='pFormatProperties', type='VkFormatProperties2*')
)

Function(name='vkGetPhysicalDeviceFragmentShadingRatesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pFragmentShadingRateCount', type='uint32_t*'),
arg3=ArgDef(name='pFragmentShadingRates', type='VkPhysicalDeviceFragmentShadingRateKHR*')
)

Function(name='vkGetPhysicalDeviceImageFormatProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='type', type='VkImageType'),
arg4=ArgDef(name='tiling', type='VkImageTiling'),
arg5=ArgDef(name='usage', type='VkImageUsageFlags'),
arg6=ArgDef(name='flags', type='VkImageCreateFlags'),
arg7=ArgDef(name='pImageFormatProperties', type='VkImageFormatProperties*')
)

Function(name='vkGetPhysicalDeviceImageFormatProperties2', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pImageFormatInfo', type='const VkPhysicalDeviceImageFormatInfo2*'),
arg3=ArgDef(name='pImageFormatProperties', type='VkImageFormatProperties2*')
)

Function(name='vkGetPhysicalDeviceImageFormatProperties2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pImageFormatInfo', type='const VkPhysicalDeviceImageFormatInfo2*'),
arg3=ArgDef(name='pImageFormatProperties', type='VkImageFormatProperties2*')
)

Function(name='vkGetPhysicalDeviceMemoryProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pMemoryProperties', type='VkPhysicalDeviceMemoryProperties*')
)

Function(name='vkGetPhysicalDeviceMemoryProperties2', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pMemoryProperties', type='VkPhysicalDeviceMemoryProperties2*')
)

Function(name='vkGetPhysicalDeviceMemoryProperties2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pMemoryProperties', type='VkPhysicalDeviceMemoryProperties2*')
)

Function(name='vkGetPhysicalDeviceMultisamplePropertiesEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='samples', type='VkSampleCountFlagBits'),
arg3=ArgDef(name='pMultisampleProperties', type='VkMultisamplePropertiesEXT*')
)

Function(name='vkGetPhysicalDeviceOpticalFlowImageFormatsNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pOpticalFlowImageFormatInfo', type='const VkOpticalFlowImageFormatInfoNV*'),
arg3=ArgDef(name='pFormatCount', type='uint32_t*'),
arg4=ArgDef(name='pImageFormatProperties', type='VkOpticalFlowImageFormatPropertiesNV*')
)

Function(name='vkGetPhysicalDevicePresentRectanglesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR'),
arg3=ArgDef(name='pRectCount', type='uint32_t*'),
arg4=ArgDef(name='pRects', type='VkRect2D*', count='pRectCount')
)

Function(name='GetPhysicalDeviceProcAddr', enabled=False, type=Param, customDriver=True, level=InstanceLevel,
retV=RetDef(type='PFN_vkVoidFunction'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pName', type='const char*')
)

Function(name='vkGetPhysicalDeviceProperties', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pProperties', type='VkPhysicalDeviceProperties*')
)

Function(name='vkGetPhysicalDeviceProperties2', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pProperties', type='VkPhysicalDeviceProperties2*')
)

Function(name='vkGetPhysicalDeviceProperties2KHR', enabled=False, type=Param, recExecWrap=True, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pProperties', type='VkPhysicalDeviceProperties2*')
)

Function(name='vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pPerformanceQueryCreateInfo', type='const VkQueryPoolPerformanceCreateInfoKHR*'),
arg3=ArgDef(name='pNumPasses', type='uint32_t*')
)

Function(name='vkGetPhysicalDeviceQueueFamilyProperties', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pQueueFamilyPropertyCount', type='uint32_t*', wrapParams='1, pQueueFamilyPropertyCount'),
arg3=ArgDef(name='pQueueFamilyProperties', type='VkQueueFamilyProperties*', wrapType='CVkQueueFamilyPropertiesArray', wrapParams='*pQueueFamilyPropertyCount, pQueueFamilyProperties', count='*pQueueFamilyPropertyCount')
)

Function(name='vkGetPhysicalDeviceQueueFamilyProperties2', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pQueueFamilyPropertyCount', type='uint32_t*', wrapParams='1, pQueueFamilyPropertyCount'),
arg3=ArgDef(name='pQueueFamilyProperties', type='VkQueueFamilyProperties2*', wrapType='CVkQueueFamilyProperties2Array', wrapParams='*pQueueFamilyPropertyCount, pQueueFamilyProperties', count='*pQueueFamilyPropertyCount')
)

Function(name='vkGetPhysicalDeviceQueueFamilyProperties2KHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pQueueFamilyPropertyCount', type='uint32_t*', wrapParams='1, pQueueFamilyPropertyCount'),
arg3=ArgDef(name='pQueueFamilyProperties', type='VkQueueFamilyProperties2*', wrapType='CVkQueueFamilyProperties2Array', wrapParams='*pQueueFamilyPropertyCount, pQueueFamilyProperties', count='*pQueueFamilyPropertyCount')
)

Function(name='vkGetPhysicalDeviceRefreshableObjectTypesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pRefreshableObjectTypeCount', type='uint32_t*'),
arg3=ArgDef(name='pRefreshableObjectTypes', type='VkObjectType*')
)

Function(name='vkGetPhysicalDeviceSparseImageFormatProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='type', type='VkImageType'),
arg4=ArgDef(name='samples', type='VkSampleCountFlagBits'),
arg5=ArgDef(name='usage', type='VkImageUsageFlags'),
arg6=ArgDef(name='tiling', type='VkImageTiling'),
arg7=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg8=ArgDef(name='pProperties', type='VkSparseImageFormatProperties*', count='pPropertyCount')
)

Function(name='vkGetPhysicalDeviceSparseImageFormatProperties2', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pFormatInfo', type='const VkPhysicalDeviceSparseImageFormatInfo2*'),
arg3=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg4=ArgDef(name='pProperties', type='VkSparseImageFormatProperties2*', count='pPropertyCount')
)

Function(name='vkGetPhysicalDeviceSparseImageFormatProperties2KHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pFormatInfo', type='const VkPhysicalDeviceSparseImageFormatInfo2*'),
arg3=ArgDef(name='pPropertyCount', type='uint32_t*'),
arg4=ArgDef(name='pProperties', type='VkSparseImageFormatProperties2*', count='pPropertyCount')
)

Function(name='vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pCombinationCount', type='uint32_t*'),
arg3=ArgDef(name='pCombinations', type='VkFramebufferMixedSamplesCombinationNV*')
)

Function(name='vkGetPhysicalDeviceSurfaceCapabilities2EXT', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR'),
arg3=ArgDef(name='pSurfaceCapabilities', type='VkSurfaceCapabilities2EXT*')
)

Function(name='vkGetPhysicalDeviceSurfaceCapabilities2KHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pSurfaceInfo', type='const VkPhysicalDeviceSurfaceInfo2KHR*'),
arg3=ArgDef(name='pSurfaceCapabilities', type='VkSurfaceCapabilities2KHR*')
)

Function(name='vkGetPhysicalDeviceSurfaceCapabilitiesKHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR'),
arg3=ArgDef(name='pSurfaceCapabilities', type='VkSurfaceCapabilitiesKHR*')
)

Function(name='vkGetPhysicalDeviceSurfaceFormats2KHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pSurfaceInfo', type='const VkPhysicalDeviceSurfaceInfo2KHR*'),
arg3=ArgDef(name='pSurfaceFormatCount', type='uint32_t*', wrapParams='1, pSurfaceFormatCount'),
arg4=ArgDef(name='pSurfaceFormats', type='VkSurfaceFormat2KHR*', wrapType='CVkSurfaceFormat2KHRArray', wrapParams='*pSurfaceFormatCount, pSurfaceFormats', count='*pSurfaceFormatCount')
)

Function(name='vkGetPhysicalDeviceSurfaceFormatsKHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR'),
arg3=ArgDef(name='pSurfaceFormatCount', type='uint32_t*', wrapParams='1, pSurfaceFormatCount'),
arg4=ArgDef(name='pSurfaceFormats', type='VkSurfaceFormatKHR*', wrapType='CVkSurfaceFormatKHRArray', wrapParams='*pSurfaceFormatCount, pSurfaceFormats', count='*pSurfaceFormatCount')
)

Function(name='vkGetPhysicalDeviceSurfacePresentModes2EXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pSurfaceInfo', type='const VkPhysicalDeviceSurfaceInfo2KHR*'),
arg3=ArgDef(name='pPresentModeCount', type='uint32_t*'),
arg4=ArgDef(name='pPresentModes', type='VkPresentModeKHR*')
)

Function(name='vkGetPhysicalDeviceSurfacePresentModesKHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='surface', type='VkSurfaceKHR'),
arg3=ArgDef(name='pPresentModeCount', type='uint32_t*', wrapParams='1, pPresentModeCount'),
arg4=ArgDef(name='pPresentModes', type='VkPresentModeKHR*', wrapParams='*pPresentModeCount, pPresentModes', count='*pPresentModeCount')
)

Function(name='vkGetPhysicalDeviceSurfaceSupportKHR', enabled=True, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
arg3=ArgDef(name='surface', type='VkSurfaceKHR'),
arg4=ArgDef(name='pSupported', type='VkBool32*', wrapParams='1, pSupported')
)

Function(name='vkGetPhysicalDeviceToolProperties', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pToolCount', type='uint32_t*'),
arg3=ArgDef(name='pToolProperties', type='VkPhysicalDeviceToolProperties*')
)

Function(name='vkGetPhysicalDeviceToolPropertiesEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pToolCount', type='uint32_t*'),
arg3=ArgDef(name='pToolProperties', type='VkPhysicalDeviceToolProperties*')
)

Function(name='vkGetPhysicalDeviceVideoCapabilitiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pVideoProfile', type='const VkVideoProfileInfoKHR*'),
arg3=ArgDef(name='pCapabilities', type='VkVideoCapabilitiesKHR*')
)

Function(name='vkGetPhysicalDeviceVideoFormatPropertiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='pVideoFormatInfo', type='const VkPhysicalDeviceVideoFormatInfoKHR*'),
arg3=ArgDef(name='pVideoFormatPropertyCount', type='uint32_t*'),
arg4=ArgDef(name='pVideoFormatProperties', type='VkVideoFormatPropertiesKHR*')
)

Function(name='vkGetPhysicalDeviceWaylandPresentationSupportKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkBool32'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
arg3=ArgDef(name='display', type='struct wl_display*')
)

Function(name='vkGetPhysicalDeviceWin32PresentationSupportKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkBool32'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t')
)

Function(name='vkGetPhysicalDeviceXcbPresentationSupportKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkBool32'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
arg3=ArgDef(name='connection', type='xcb_connection_t*'),
arg4=ArgDef(name='visual_id', type='xcb_visualid_t')
)

Function(name='vkGetPhysicalDeviceXlibPresentationSupportKHR', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkBool32'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='queueFamilyIndex', type='uint32_t'),
arg3=ArgDef(name='dpy', type='Display*'),
arg4=ArgDef(name='visualID', type='VisualID')
)

Function(name='vkGetPipelineCacheData', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipelineCache', type='VkPipelineCache'),
arg3=ArgDef(name='pDataSize', type='size_t*'),
arg4=ArgDef(name='pData', type='void*')
)

Function(name='vkGetPipelineExecutableInternalRepresentationsKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pExecutableInfo', type='const VkPipelineExecutableInfoKHR*'),
arg3=ArgDef(name='pInternalRepresentationCount', type='uint32_t*'),
arg4=ArgDef(name='pInternalRepresentations', type='VkPipelineExecutableInternalRepresentationKHR*')
)

Function(name='vkGetPipelineExecutablePropertiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pPipelineInfo', type='const VkPipelineInfoKHR*'),
arg3=ArgDef(name='pExecutableCount', type='uint32_t*'),
arg4=ArgDef(name='pProperties', type='VkPipelineExecutablePropertiesKHR*')
)

Function(name='vkGetPipelineExecutableStatisticsKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pExecutableInfo', type='const VkPipelineExecutableInfoKHR*'),
arg3=ArgDef(name='pStatisticCount', type='uint32_t*'),
arg4=ArgDef(name='pStatistics', type='VkPipelineExecutableStatisticKHR*')
)

Function(name='vkGetPipelinePropertiesEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pPipelineInfo', type='const VkPipelineInfoEXT*'),
arg3=ArgDef(name='pPipelineProperties', type='VkBaseOutStructure*')
)

Function(name='vkGetPrivateData', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='objectType', type='VkObjectType'),
arg3=ArgDef(name='objectHandle', type='uint64_t'),
arg4=ArgDef(name='privateDataSlot', type='VkPrivateDataSlot'),
arg5=ArgDef(name='pData', type='uint64_t*')
)

Function(name='vkGetPrivateDataEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='objectType', type='VkObjectType'),
arg3=ArgDef(name='objectHandle', type='uint64_t'),
arg4=ArgDef(name='privateDataSlot', type='VkPrivateDataSlot'),
arg5=ArgDef(name='pData', type='uint64_t*')
)

Function(name='vkGetQueryPoolResults', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='firstQuery', type='uint32_t'),
arg4=ArgDef(name='queryCount', type='uint32_t'),
arg5=ArgDef(name='dataSize', type='size_t'),
arg6=ArgDef(name='pData', type='void*', wrapType='Cuint8_t::CSArray', wrapParams='(size_t)(dataSize), (uint8_t *)pData'),
arg7=ArgDef(name='stride', type='VkDeviceSize'),
arg8=ArgDef(name='flags', type='VkQueryResultFlags')
)

Function(name='vkGetQueueCheckpointData2NV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='pCheckpointDataCount', type='uint32_t*'),
arg3=ArgDef(name='pCheckpointData', type='VkCheckpointData2NV*')
)

Function(name='vkGetQueueCheckpointDataNV', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='pCheckpointDataCount', type='uint32_t*'),
arg3=ArgDef(name='pCheckpointData', type='VkCheckpointDataNV*')
)

Function(name='vkGetRayTracingCaptureReplayShaderGroupHandlesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline'),
arg3=ArgDef(name='firstGroup', type='uint32_t'),
arg4=ArgDef(name='groupCount', type='uint32_t'),
arg5=ArgDef(name='dataSize', type='size_t'),
arg6=ArgDef(name='pData', type='void*')
)

Function(name='vkGetRayTracingShaderGroupHandlesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline'),
arg3=ArgDef(name='firstGroup', type='uint32_t'),
arg4=ArgDef(name='groupCount', type='uint32_t'),
arg5=ArgDef(name='dataSize', type='size_t'),
arg6=ArgDef(name='pData', type='void*')
)

Function(name='vkGetRayTracingShaderGroupHandlesNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline'),
arg3=ArgDef(name='firstGroup', type='uint32_t'),
arg4=ArgDef(name='groupCount', type='uint32_t'),
arg5=ArgDef(name='dataSize', type='size_t'),
arg6=ArgDef(name='pData', type='void*')
)

Function(name='vkGetRayTracingShaderGroupStackSizeKHR', enabled=False, type=Param,
retV=RetDef(type='VkDeviceSize'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline'),
arg3=ArgDef(name='group', type='uint32_t'),
arg4=ArgDef(name='groupShader', type='VkShaderGroupShaderKHR')
)

Function(name='vkGetRefreshCycleDurationGOOGLE', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR'),
arg3=ArgDef(name='pDisplayTimingProperties', type='VkRefreshCycleDurationGOOGLE*')
)

Function(name='vkGetRenderAreaGranularity', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='renderPass', type='VkRenderPass'),
arg3=ArgDef(name='pGranularity', type='VkExtent2D*')
)

Function(name='vkGetSamplerOpaqueCaptureDescriptorDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInfo', type='const VkSamplerCaptureDescriptorDataInfoEXT*'),
arg3=ArgDef(name='pData', type='void*')
)

Function(name='vkGetSemaphoreCounterValue', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='semaphore', type='VkSemaphore'),
arg3=ArgDef(name='pValue', type='uint64_t*', wrapParams='1, pValue')
)

Function(name='vkGetSemaphoreCounterValueKHR', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='semaphore', type='VkSemaphore'),
arg3=ArgDef(name='pValue', type='uint64_t*', wrapParams='1, pValue')
)

Function(name='vkGetSemaphoreFdKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pGetFdInfo', type='const VkSemaphoreGetFdInfoKHR*'),
arg3=ArgDef(name='pFd', type='int*')
)

Function(name='vkGetSemaphoreWin32HandleKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pGetWin32HandleInfo', type='const VkSemaphoreGetWin32HandleInfoKHR*'),
arg3=ArgDef(name='pHandle', type='HANDLE*')
)

Function(name='vkGetShaderBinaryDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='shader', type='VkShaderEXT'),
arg3=ArgDef(name='pDataSize', type='size_t*'),
arg4=ArgDef(name='pData', type='void*')
)

Function(name='vkGetShaderInfoAMD', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pipeline', type='VkPipeline'),
arg3=ArgDef(name='shaderStage', type='VkShaderStageFlagBits'),
arg4=ArgDef(name='infoType', type='VkShaderInfoTypeAMD'),
arg5=ArgDef(name='pInfoSize', type='size_t*'),
arg6=ArgDef(name='pInfo', type='void*')
)

Function(name='vkGetShaderModuleCreateInfoIdentifierEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pCreateInfo', type='const VkShaderModuleCreateInfo*'),
arg3=ArgDef(name='pIdentifier', type='VkShaderModuleIdentifierEXT*')
)

Function(name='vkGetShaderModuleIdentifierEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='shaderModule', type='VkShaderModule'),
arg3=ArgDef(name='pIdentifier', type='VkShaderModuleIdentifierEXT*')
)

Function(name='vkGetSwapchainCounterEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR'),
arg3=ArgDef(name='counter', type='VkSurfaceCounterFlagBitsEXT'),
arg4=ArgDef(name='pCounterValue', type='uint64_t*')
)

#Function(name='vkGetSwapchainGrallocUsage2ANDROID', enabled=False, type=Param,
#retV=RetDef(type='VkResult'),
#arg1=ArgDef(name='device', type='VkDevice'),
#arg2=ArgDef(name='format', type='VkFormat'),
#arg3=ArgDef(name='imageUsage', type='VkImageUsageFlags'),
#arg4=ArgDef(name='swapchainImageUsage', type='VkSwapchainImageUsageFlagsANDROID'),
#arg5=ArgDef(name='grallocConsumerUsage', type='uint64_t*'),
#arg6=ArgDef(name='grallocProducerUsage', type='uint64_t*')
#)

Function(name='vkGetSwapchainGrallocUsageANDROID', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='format', type='VkFormat'),
arg3=ArgDef(name='imageUsage', type='VkImageUsageFlags'),
arg4=ArgDef(name='grallocUsage', type='int*')
)

Function(name='vkGetSwapchainImagesKHR', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR'),
arg3=ArgDef(name='pSwapchainImageCount', type='uint32_t*', wrapParams='1, pSwapchainImageCount'),
arg4=ArgDef(name='pSwapchainImages', type='VkImage*', wrapType='CVkImage::CSMapArray', wrapParams='(pSwapchainImages==nullptr)?0:*pSwapchainImageCount, pSwapchainImages', count='pSwapchainImageCount')
)

Function(name='vkGetSwapchainStatusKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR')
)

Function(name='vkGetValidationCacheDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='validationCache', type='VkValidationCacheEXT'),
arg3=ArgDef(name='pDataSize', type='size_t*'),
arg4=ArgDef(name='pData', type='void*')
)

Function(name='vkGetVideoSessionMemoryRequirementsKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='videoSession', type='VkVideoSessionKHR'),
arg3=ArgDef(name='pMemoryRequirementsCount', type='uint32_t*'),
arg4=ArgDef(name='pMemoryRequirements', type='VkVideoSessionMemoryRequirementsKHR*')
)

Function(name='vkGetWinrtDisplayNV', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='deviceRelativeId', type='uint32_t'),
arg3=ArgDef(name='pDisplay', type='VkDisplayKHR*')
)

Function(name='vkImportFenceFdKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pImportFenceFdInfo', type='const VkImportFenceFdInfoKHR*')
)

Function(name='vkImportFenceWin32HandleKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pImportFenceWin32HandleInfo', type='const VkImportFenceWin32HandleInfoKHR*')
)

Function(name='vkImportSemaphoreFdKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pImportSemaphoreFdInfo', type='const VkImportSemaphoreFdInfoKHR*')
)

Function(name='vkImportSemaphoreWin32HandleKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pImportSemaphoreWin32HandleInfo', type='const VkImportSemaphoreWin32HandleInfoKHR*')
)

Function(name='vkInitializePerformanceApiINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pInitializeInfo', type='const VkInitializePerformanceApiInfoINTEL*')
)

Function(name='vkInvalidateMappedMemoryRanges', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memoryRangeCount', type='uint32_t'),
arg3=ArgDef(name='pMemoryRanges', type='const VkMappedMemoryRange*', count='memoryRangeCount')
)

Function(name='vkMapMemory', enabled=True, type=Param, runWrap=True, stateTrack=True, recExecWrap=True, ccodeWriteWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory'),
arg3=ArgDef(name='offset', type='VkDeviceSize'),
arg4=ArgDef(name='size', type='VkDeviceSize'),
arg5=ArgDef(name='flags', type='VkMemoryMapFlags'),
arg6=ArgDef(name='ppData', type='void**', wrapType='CVoidPtr')
)

Function(name='vkMapMemory2KHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pMemoryMapInfo', type='const VkMemoryMapInfoKHR*'),
arg3=ArgDef(name='ppData', type='void**')
)

Function(name='vkMergePipelineCaches', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='dstCache', type='VkPipelineCache'),
arg3=ArgDef(name='srcCacheCount', type='uint32_t'),
arg4=ArgDef(name='pSrcCaches', type='const VkPipelineCache*', count='srcCacheCount')
)

Function(name='vkMergeValidationCachesEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='dstCache', type='VkValidationCacheEXT'),
arg3=ArgDef(name='srcCacheCount', type='uint32_t'),
arg4=ArgDef(name='pSrcCaches', type='const VkValidationCacheEXT*', count='srcCacheCount')
)

Function(name='vkNegotiateLoaderLayerInterfaceVersion', enabled=False, type=Param, recExecWrap=True, customDriver=True, level=GlobalLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='pVersionStruct', type='VkNegotiateLayerInterface*')
)

Function(name='vkQueueBeginDebugUtilsLabelEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='pLabelInfo', type='const VkDebugUtilsLabelEXT*')
)

Function(name='vkQueueBindSparse', enabled=True, type=Param, stateTrack=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='bindInfoCount', type='uint32_t'),
arg3=ArgDef(name='pBindInfo', type='const VkBindSparseInfo*', wrapType='CVkBindSparseInfoArray', wrapParams='bindInfoCount, pBindInfo', count='bindInfoCount'),
arg4=ArgDef(name='fence', type='VkFence')
)

Function(name='vkQueueEndDebugUtilsLabelEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='queue', type='VkQueue')
)

Function(name='vkQueueInsertDebugUtilsLabelEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='pLabelInfo', type='const VkDebugUtilsLabelEXT*')
)

Function(name='vkQueuePresentKHR', enabled=True, type=Param, stateTrack=True, runWrap=True, recWrap=True, endFrameTag=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='pPresentInfo', type='const VkPresentInfoKHR*')
)

Function(name='vkQueueSetPerformanceConfigurationINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='configuration', type='VkPerformanceConfigurationINTEL')
)

Function(name='vkQueueSignalReleaseImageANDROID', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='waitSemaphoreCount', type='uint32_t'),
arg3=ArgDef(name='pWaitSemaphores', type='const VkSemaphore*'),
arg4=ArgDef(name='image', type='VkImage'),
arg5=ArgDef(name='pNativeFenceFd', type='int*')
)

Function(name='vkQueueSubmit', enabled=True, type=QueueSubmit, stateTrack=True, recWrap=True, runWrap=True, recExecWrap=True, execPostRecWrap=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='submitCount', type='uint32_t'),
arg3=ArgDef(name='pSubmits', type='const VkSubmitInfo*', wrapType='CVkSubmitInfoArray', wrapParams='submitCount, pSubmits', count='submitCount'),
arg4=ArgDef(name='fence', type='VkFence')
)

Function(name='vkQueueSubmit2', enabled=True, type=QueueSubmit, stateTrack=True, recWrap=True, runWrap=True, recExecWrap=True, execPostRecWrap=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='submitCount', type='uint32_t'),
arg3=ArgDef(name='pSubmits', type='const VkSubmitInfo2*', wrapType='CVkSubmitInfo2Array', wrapParams='submitCount, pSubmits', count='submitCount'),
arg4=ArgDef(name='fence', type='VkFence')
)

Function(name='vkQueueSubmit2KHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue'),
arg2=ArgDef(name='submitCount', type='uint32_t'),
arg3=ArgDef(name='pSubmits', type='const VkSubmitInfo2*'),
arg4=ArgDef(name='fence', type='VkFence')
)

Function(name='vkQueueWaitIdle', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='queue', type='VkQueue')
)

Function(name='vkRegisterDeviceEventEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pDeviceEventInfo', type='const VkDeviceEventInfoEXT*'),
arg3=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg4=ArgDef(name='pFence', type='VkFence*')
)

Function(name='vkRegisterDisplayEventEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR'),
arg3=ArgDef(name='pDisplayEventInfo', type='const VkDisplayEventInfoEXT*'),
arg4=ArgDef(name='pAllocator', type='const VkAllocationCallbacks*'),
arg5=ArgDef(name='pFence', type='VkFence*')
)

Function(name='vkReleaseDisplayEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='physicalDevice', type='VkPhysicalDevice'),
arg2=ArgDef(name='display', type='VkDisplayKHR')
)

Function(name='vkReleaseFullScreenExclusiveModeEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR')
)

Function(name='vkReleasePerformanceConfigurationINTEL', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='configuration', type='VkPerformanceConfigurationINTEL')
)

Function(name='vkReleaseProfilingLockKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice')
)

Function(name='vkReleaseSwapchainImagesEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pReleaseInfo', type='const VkReleaseSwapchainImagesInfoEXT*')
)

Function(name='vkResetCommandBuffer', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='commandBuffer', type='VkCommandBuffer'),
arg2=ArgDef(name='flags', type='VkCommandBufferResetFlags')
)

Function(name='vkResetCommandPool', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='commandPool', type='VkCommandPool'),
arg3=ArgDef(name='flags', type='VkCommandPoolResetFlags')
)

Function(name='vkResetDescriptorPool', enabled=True, type=Param, stateTrack=True, recWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorPool', type='VkDescriptorPool'),
arg3=ArgDef(name='flags', type='VkDescriptorPoolResetFlags')
)

Function(name='vkResetEvent', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='event', type='VkEvent')
)

Function(name='vkResetFences', enabled=True, type=Param, stateTrack=True, ccodeWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='fenceCount', type='uint32_t'),
arg3=ArgDef(name='pFences', type='const VkFence*', wrapType='CVkFence::CSArray', wrapParams='fenceCount, pFences', count='fenceCount')
)

Function(name='vkResetQueryPool', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='firstQuery', type='uint32_t'),
arg4=ArgDef(name='queryCount', type='uint32_t')
)

Function(name='vkResetQueryPoolEXT', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='queryPool', type='VkQueryPool'),
arg3=ArgDef(name='firstQuery', type='uint32_t'),
arg4=ArgDef(name='queryCount', type='uint32_t')
)

Function(name='vkSetDebugUtilsObjectNameEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pNameInfo', type='const VkDebugUtilsObjectNameInfoEXT*')
)

Function(name='vkSetDebugUtilsObjectTagEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pTagInfo', type='const VkDebugUtilsObjectTagInfoEXT*')
)

Function(name='vkSetDeviceMemoryPriorityEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory'),
arg3=ArgDef(name='priority', type='float')
)

Function(name='vkSetEvent', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='event', type='VkEvent')
)

Function(name='vkSetHdrMetadataEXT', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchainCount', type='uint32_t'),
arg3=ArgDef(name='pSwapchains', type='const VkSwapchainKHR*', count='swapchainCount'),
arg4=ArgDef(name='pMetadata', type='const VkHdrMetadataEXT*', count='swapchainCount')
)

Function(name='vkSetLocalDimmingAMD', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapChain', type='VkSwapchainKHR'),
arg3=ArgDef(name='localDimmingEnable', type='VkBool32')
)

Function(name='vkSetPrivateData', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='objectType', type='VkObjectType'),
arg3=ArgDef(name='objectHandle', type='uint64_t'),
arg4=ArgDef(name='privateDataSlot', type='VkPrivateDataSlot'),
arg5=ArgDef(name='data', type='uint64_t')
)

Function(name='vkSetPrivateDataEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='objectType', type='VkObjectType'),
arg3=ArgDef(name='objectHandle', type='uint64_t'),
arg4=ArgDef(name='privateDataSlot', type='VkPrivateDataSlot'),
arg5=ArgDef(name='data', type='uint64_t')
)

Function(name='vkSignalSemaphore', enabled=True, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pSignalInfo', type='const VkSemaphoreSignalInfo*')
)

Function(name='vkSignalSemaphoreKHR', enabled=True, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pSignalInfo', type='const VkSemaphoreSignalInfo*')
)

Function(name='vkSubmitDebugUtilsMessageEXT', enabled=False, type=Param, level=InstanceLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='messageSeverity', type='VkDebugUtilsMessageSeverityFlagBitsEXT'),
arg3=ArgDef(name='messageTypes', type='VkDebugUtilsMessageTypeFlagsEXT'),
arg4=ArgDef(name='pCallbackData', type='const VkDebugUtilsMessengerCallbackDataEXT*')
)

Function(name='vkTrimCommandPool', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='commandPool', type='VkCommandPool'),
arg3=ArgDef(name='flags', type='VkCommandPoolTrimFlags')
)

Function(name='vkTrimCommandPoolKHR', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='commandPool', type='VkCommandPool'),
arg3=ArgDef(name='flags', type='VkCommandPoolTrimFlags')
)

Function(name='vkUninitializePerformanceApiINTEL', enabled=False, type=Param,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice')
)

Function(name='vkUnmapMemory', enabled=True, type=Param, preToken='CGitsVkMemoryUpdate2(memory, true)', recWrap=True, stateTrack=True, execPostRecWrap=True, ccodeWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='memory', type='VkDeviceMemory')
)

Function(name='vkUnmapMemory2KHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pMemoryUnmapInfo', type='const VkMemoryUnmapInfoKHR*')
)

Function(name='vkUpdateDescriptorSetWithTemplate', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorSet', type='VkDescriptorSet'),
arg3=ArgDef(name='descriptorUpdateTemplate', type='VkDescriptorUpdateTemplate'),
arg4=ArgDef(name='pData', type='const void*', wrapType='CUpdateDescriptorSetWithTemplateArray', wrapParams='descriptorUpdateTemplate, pData')
)

Function(name='vkUpdateDescriptorSetWithTemplateKHR', enabled=True, type=Param, stateTrack=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorSet', type='VkDescriptorSet'),
arg3=ArgDef(name='descriptorUpdateTemplate', type='VkDescriptorUpdateTemplate'),
arg4=ArgDef(name='pData', type='const void*', wrapType='CUpdateDescriptorSetWithTemplateArray', wrapParams='descriptorUpdateTemplate, pData')
)

Function(name='vkUpdateDescriptorSets', enabled=True, type=Param, stateTrack=True, ccodeWriteWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='descriptorWriteCount', type='uint32_t'),
arg3=ArgDef(name='pDescriptorWrites', type='const VkWriteDescriptorSet*', wrapType='CVkWriteDescriptorSetArray', wrapParams='descriptorWriteCount, pDescriptorWrites', count='descriptorWriteCount'),
arg4=ArgDef(name='descriptorCopyCount', type='uint32_t'),
arg5=ArgDef(name='pDescriptorCopies', type='const VkCopyDescriptorSet*', wrapType='CVkCopyDescriptorSetArray', wrapParams='descriptorCopyCount, pDescriptorCopies', count='descriptorCopyCount')
)

Function(name='vkUpdateVideoSessionParametersKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='videoSessionParameters', type='VkVideoSessionParametersKHR'),
arg3=ArgDef(name='pUpdateInfo', type='const VkVideoSessionParametersUpdateInfoKHR*')
)

Function(name='vkVoidFunction', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='', type='void')
)

Function(name='vkWaitForFences', enabled=True, type=Param, runWrap=True, recExecWrap=True, ccodeWriteWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='fenceCount', type='uint32_t'),
arg3=ArgDef(name='pFences', type='const VkFence*', wrapType='CVkFence::CSArray', wrapParams='fenceCount, pFences', count='fenceCount'),
arg4=ArgDef(name='waitAll', type='VkBool32'),
arg5=ArgDef(name='timeout', type='uint64_t')
)

Function(name='vkWaitForPresentKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='swapchain', type='VkSwapchainKHR'),
arg3=ArgDef(name='presentId', type='uint64_t'),
arg4=ArgDef(name='timeout', type='uint64_t')
)

Function(name='vkWaitSemaphores', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pWaitInfo', type='const VkSemaphoreWaitInfo*'),
arg3=ArgDef(name='timeout', type='uint64_t')
)

Function(name='vkWaitSemaphoresKHR', enabled=True, type=Param, runWrap=True,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='pWaitInfo', type='const VkSemaphoreWaitInfo*'),
arg3=ArgDef(name='timeout', type='uint64_t')
)

Function(name='vkWriteAccelerationStructuresPropertiesKHR', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='accelerationStructureCount', type='uint32_t'),
arg3=ArgDef(name='pAccelerationStructures', type='const VkAccelerationStructureKHR*'),
arg4=ArgDef(name='queryType', type='VkQueryType'),
arg5=ArgDef(name='dataSize', type='size_t'),
arg6=ArgDef(name='pData', type='void*'),
arg7=ArgDef(name='stride', type='size_t')
)

Function(name='vkWriteMicromapsPropertiesEXT', enabled=False, type=Param,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='device', type='VkDevice'),
arg2=ArgDef(name='micromapCount', type='uint32_t'),
arg3=ArgDef(name='pMicromaps', type='const VkMicromapEXT*'),
arg4=ArgDef(name='queryType', type='VkQueryType'),
arg5=ArgDef(name='dataSize', type='size_t'),
arg6=ArgDef(name='pData', type='void*'),
arg7=ArgDef(name='stride', type='size_t')
)

Function(name='vk_icdGetInstanceProcAddr', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='PFN_vkVoidFunction'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pName', type='const char*')
)

Function(name='vk_icdGetPhysicalDeviceProcAddr', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='PFN_vkVoidFunction'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pName', type='const char*')
)

Function(name='vk_icdNegotiateLoaderICDInterfaceVersion', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='VkResult'),
arg1=ArgDef(name='pVersion', type='uint32_t*')
)

###############################################

Enum(name='VkAccelerationStructureBuildTypeKHR', enumerators = [
VarDef(name='VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR', value='0'),
VarDef(name='VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR', value='1'),
VarDef(name='VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR', value='2'),
VarDef(name='VK_ACCELERATION_STRUCTURE_BUILD_TYPE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkAccelerationStructureCompatibilityKHR', enumerators = [
VarDef(name='VK_ACCELERATION_STRUCTURE_COMPATIBILITY_COMPATIBLE_KHR', value='0'),
VarDef(name='VK_ACCELERATION_STRUCTURE_COMPATIBILITY_INCOMPATIBLE_KHR', value='1'),
VarDef(name='VK_ACCELERATION_STRUCTURE_COMPATIBILITY_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkAccelerationStructureCreateFlagBitsKHR', enumerators = [
VarDef(name='VK_ACCELERATION_STRUCTURE_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_KHR', value='1'),
VarDef(name='VK_ACCELERATION_STRUCTURE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT', value='8'),
VarDef(name='VK_ACCELERATION_STRUCTURE_CREATE_MOTION_BIT_NV', value='4'),
VarDef(name='VK_ACCELERATION_STRUCTURE_CREATE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkAccelerationStructureMemoryRequirementsTypeNV', enumerators = [
VarDef(name='VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV', value='0'),
VarDef(name='VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV', value='1'),
VarDef(name='VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV', value='2'),
VarDef(name='VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkAccelerationStructureMotionInstanceTypeNV', enumerators = [
VarDef(name='VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_STATIC_NV', value='0'),
VarDef(name='VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_MATRIX_MOTION_NV', value='1'),
VarDef(name='VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_SRT_MOTION_NV', value='2'),
VarDef(name='VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkAccelerationStructureTypeKHR', enumerators = [
VarDef(name='VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR', value='0'),
VarDef(name='VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR', value='1'),
VarDef(name='VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR', value='2'),
VarDef(name='VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkAccessFlagBits', enumerators = [
VarDef(name='VK_ACCESS_INDIRECT_COMMAND_READ_BIT', value='1'),
VarDef(name='VK_ACCESS_INDEX_READ_BIT', value='2'),
VarDef(name='VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT', value='4'),
VarDef(name='VK_ACCESS_UNIFORM_READ_BIT', value='8'),
VarDef(name='VK_ACCESS_INPUT_ATTACHMENT_READ_BIT', value='16'),
VarDef(name='VK_ACCESS_SHADER_READ_BIT', value='32'),
VarDef(name='VK_ACCESS_SHADER_WRITE_BIT', value='64'),
VarDef(name='VK_ACCESS_COLOR_ATTACHMENT_READ_BIT', value='128'),
VarDef(name='VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT', value='256'),
VarDef(name='VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT', value='512'),
VarDef(name='VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT', value='1024'),
VarDef(name='VK_ACCESS_TRANSFER_READ_BIT', value='2048'),
VarDef(name='VK_ACCESS_TRANSFER_WRITE_BIT', value='4096'),
VarDef(name='VK_ACCESS_HOST_READ_BIT', value='8192'),
VarDef(name='VK_ACCESS_HOST_WRITE_BIT', value='16384'),
VarDef(name='VK_ACCESS_MEMORY_READ_BIT', value='32768'),
VarDef(name='VK_ACCESS_MEMORY_WRITE_BIT', value='65536'),
VarDef(name='VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR', value='2097152'),
VarDef(name='VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR', value='4194304'),
VarDef(name='VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT', value='524288'),
VarDef(name='VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV', value='131072'),
VarDef(name='VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV', value='262144'),
VarDef(name='VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT', value='1048576'),
VarDef(name='VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT', value='16777216'),
VarDef(name='VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR', value='8388608'),
VarDef(name='VK_ACCESS_NONE', value='0'),
VarDef(name='VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT', value='67108864'),
VarDef(name='VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT', value='134217728'),
VarDef(name='VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT', value='33554432'),
VarDef(name='VK_ACCESS_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkAccessFlagBits2', size=64, enumerators = [
VarDef(name='VK_ACCESS_2_NONE', value='0'),
VarDef(name='VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT', value='1'),
VarDef(name='VK_ACCESS_2_INDEX_READ_BIT', value='2'),
VarDef(name='VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT', value='4'),
VarDef(name='VK_ACCESS_2_UNIFORM_READ_BIT', value='8'),
VarDef(name='VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT', value='16'),
VarDef(name='VK_ACCESS_2_SHADER_READ_BIT', value='32'),
VarDef(name='VK_ACCESS_2_SHADER_WRITE_BIT', value='64'),
VarDef(name='VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT', value='128'),
VarDef(name='VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT', value='256'),
VarDef(name='VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT', value='512'),
VarDef(name='VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT', value='1024'),
VarDef(name='VK_ACCESS_2_TRANSFER_READ_BIT', value='2048'),
VarDef(name='VK_ACCESS_2_TRANSFER_WRITE_BIT', value='4096'),
VarDef(name='VK_ACCESS_2_HOST_READ_BIT', value='8192'),
VarDef(name='VK_ACCESS_2_HOST_WRITE_BIT', value='16384'),
VarDef(name='VK_ACCESS_2_MEMORY_READ_BIT', value='32768'),
VarDef(name='VK_ACCESS_2_MEMORY_WRITE_BIT', value='65536'),
VarDef(name='VK_ACCESS_2_SHADER_SAMPLED_READ_BIT', value='4294967296'),
VarDef(name='VK_ACCESS_2_SHADER_STORAGE_READ_BIT', value='8589934592'),
VarDef(name='VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT', value='17179869184'),
VarDef(name='VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR', value='2097152'),
VarDef(name='VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR', value='4194304'),
VarDef(name='VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT', value='524288'),
VarDef(name='VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV', value='131072'),
VarDef(name='VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV', value='262144'),
VarDef(name='VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT', value='1048576'),
VarDef(name='VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT', value='2199023255552'),
VarDef(name='VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT', value='16777216'),
VarDef(name='VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR', value='8388608'),
VarDef(name='VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI', value='549755813888'),
VarDef(name='VK_ACCESS_2_MICROMAP_READ_BIT_EXT', value='17592186044416'),
VarDef(name='VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT', value='35184372088832'),
VarDef(name='VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV', value='4398046511104'),
VarDef(name='VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV', value='8796093022208'),
VarDef(name='VK_ACCESS_2_RESERVED_46_BIT_EXT', value='70368744177664'),
VarDef(name='VK_ACCESS_2_RESERVED_47_BIT_EXT', value='140737488355328'),
VarDef(name='VK_ACCESS_2_RESERVED_48_BIT_EXT', value='281474976710656'),
VarDef(name='VK_ACCESS_2_RESERVED_49_BIT_ARM', value='562949953421312'),
VarDef(name='VK_ACCESS_2_RESERVED_50_BIT_ARM', value='1125899906842624'),
VarDef(name='VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR', value='1099511627776'),
VarDef(name='VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT', value='67108864'),
VarDef(name='VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT', value='134217728'),
VarDef(name='VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT', value='33554432'),
VarDef(name='VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR', value='34359738368'),
VarDef(name='VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR', value='68719476736'),
VarDef(name='VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR', value='137438953472'),
VarDef(name='VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR', value='274877906944'),
])

Enum(name='VkAcquireProfilingLockFlagBitsKHR', enumerators = [
VarDef(name='VK_ACQUIRE_PROFILING_LOCK_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkAttachmentDescriptionFlagBits', enumerators = [
VarDef(name='VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT', value='1'),
VarDef(name='VK_ATTACHMENT_DESCRIPTION_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkAttachmentLoadOp', enumerators = [
VarDef(name='VK_ATTACHMENT_LOAD_OP_LOAD', value='0'),
VarDef(name='VK_ATTACHMENT_LOAD_OP_CLEAR', value='1'),
VarDef(name='VK_ATTACHMENT_LOAD_OP_DONT_CARE', value='2'),
VarDef(name='VK_ATTACHMENT_LOAD_OP_NONE_EXT', value='1000400000'),
VarDef(name='VK_ATTACHMENT_LOAD_OP_MAX_ENUM', value='2147483647'),
])

Enum(name='VkAttachmentStoreOp', enumerators = [
VarDef(name='VK_ATTACHMENT_STORE_OP_STORE', value='0'),
VarDef(name='VK_ATTACHMENT_STORE_OP_DONT_CARE', value='1'),
VarDef(name='VK_ATTACHMENT_STORE_OP_NONE', value='1000301000'),
VarDef(name='VK_ATTACHMENT_STORE_OP_MAX_ENUM', value='2147483647'),
])

Enum(name='VkBlendFactor', enumerators = [
VarDef(name='VK_BLEND_FACTOR_ZERO', value='0'),
VarDef(name='VK_BLEND_FACTOR_ONE', value='1'),
VarDef(name='VK_BLEND_FACTOR_SRC_COLOR', value='2'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR', value='3'),
VarDef(name='VK_BLEND_FACTOR_DST_COLOR', value='4'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR', value='5'),
VarDef(name='VK_BLEND_FACTOR_SRC_ALPHA', value='6'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA', value='7'),
VarDef(name='VK_BLEND_FACTOR_DST_ALPHA', value='8'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA', value='9'),
VarDef(name='VK_BLEND_FACTOR_CONSTANT_COLOR', value='10'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR', value='11'),
VarDef(name='VK_BLEND_FACTOR_CONSTANT_ALPHA', value='12'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA', value='13'),
VarDef(name='VK_BLEND_FACTOR_SRC_ALPHA_SATURATE', value='14'),
VarDef(name='VK_BLEND_FACTOR_SRC1_COLOR', value='15'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR', value='16'),
VarDef(name='VK_BLEND_FACTOR_SRC1_ALPHA', value='17'),
VarDef(name='VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA', value='18'),
VarDef(name='VK_BLEND_FACTOR_MAX_ENUM', value='2147483647'),
])

Enum(name='VkBlendOp', enumerators = [
VarDef(name='VK_BLEND_OP_ADD', value='0'),
VarDef(name='VK_BLEND_OP_SUBTRACT', value='1'),
VarDef(name='VK_BLEND_OP_REVERSE_SUBTRACT', value='2'),
VarDef(name='VK_BLEND_OP_MIN', value='3'),
VarDef(name='VK_BLEND_OP_MAX', value='4'),
VarDef(name='VK_BLEND_OP_BLUE_EXT', value='1000148045'),
VarDef(name='VK_BLEND_OP_COLORBURN_EXT', value='1000148018'),
VarDef(name='VK_BLEND_OP_COLORDODGE_EXT', value='1000148017'),
VarDef(name='VK_BLEND_OP_CONTRAST_EXT', value='1000148041'),
VarDef(name='VK_BLEND_OP_DARKEN_EXT', value='1000148015'),
VarDef(name='VK_BLEND_OP_DIFFERENCE_EXT', value='1000148021'),
VarDef(name='VK_BLEND_OP_DST_ATOP_EXT', value='1000148010'),
VarDef(name='VK_BLEND_OP_DST_EXT', value='1000148002'),
VarDef(name='VK_BLEND_OP_DST_IN_EXT', value='1000148006'),
VarDef(name='VK_BLEND_OP_DST_OUT_EXT', value='1000148008'),
VarDef(name='VK_BLEND_OP_DST_OVER_EXT', value='1000148004'),
VarDef(name='VK_BLEND_OP_EXCLUSION_EXT', value='1000148022'),
VarDef(name='VK_BLEND_OP_GREEN_EXT', value='1000148044'),
VarDef(name='VK_BLEND_OP_HARDLIGHT_EXT', value='1000148019'),
VarDef(name='VK_BLEND_OP_HARDMIX_EXT', value='1000148030'),
VarDef(name='VK_BLEND_OP_HSL_COLOR_EXT', value='1000148033'),
VarDef(name='VK_BLEND_OP_HSL_HUE_EXT', value='1000148031'),
VarDef(name='VK_BLEND_OP_HSL_LUMINOSITY_EXT', value='1000148034'),
VarDef(name='VK_BLEND_OP_HSL_SATURATION_EXT', value='1000148032'),
VarDef(name='VK_BLEND_OP_INVERT_EXT', value='1000148023'),
VarDef(name='VK_BLEND_OP_INVERT_OVG_EXT', value='1000148042'),
VarDef(name='VK_BLEND_OP_INVERT_RGB_EXT', value='1000148024'),
VarDef(name='VK_BLEND_OP_LIGHTEN_EXT', value='1000148016'),
VarDef(name='VK_BLEND_OP_LINEARBURN_EXT', value='1000148026'),
VarDef(name='VK_BLEND_OP_LINEARDODGE_EXT', value='1000148025'),
VarDef(name='VK_BLEND_OP_LINEARLIGHT_EXT', value='1000148028'),
VarDef(name='VK_BLEND_OP_MINUS_CLAMPED_EXT', value='1000148040'),
VarDef(name='VK_BLEND_OP_MINUS_EXT', value='1000148039'),
VarDef(name='VK_BLEND_OP_MULTIPLY_EXT', value='1000148012'),
VarDef(name='VK_BLEND_OP_OVERLAY_EXT', value='1000148014'),
VarDef(name='VK_BLEND_OP_PINLIGHT_EXT', value='1000148029'),
VarDef(name='VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT', value='1000148037'),
VarDef(name='VK_BLEND_OP_PLUS_CLAMPED_EXT', value='1000148036'),
VarDef(name='VK_BLEND_OP_PLUS_DARKER_EXT', value='1000148038'),
VarDef(name='VK_BLEND_OP_PLUS_EXT', value='1000148035'),
VarDef(name='VK_BLEND_OP_RED_EXT', value='1000148043'),
VarDef(name='VK_BLEND_OP_SCREEN_EXT', value='1000148013'),
VarDef(name='VK_BLEND_OP_SOFTLIGHT_EXT', value='1000148020'),
VarDef(name='VK_BLEND_OP_SRC_ATOP_EXT', value='1000148009'),
VarDef(name='VK_BLEND_OP_SRC_EXT', value='1000148001'),
VarDef(name='VK_BLEND_OP_SRC_IN_EXT', value='1000148005'),
VarDef(name='VK_BLEND_OP_SRC_OUT_EXT', value='1000148007'),
VarDef(name='VK_BLEND_OP_SRC_OVER_EXT', value='1000148003'),
VarDef(name='VK_BLEND_OP_VIVIDLIGHT_EXT', value='1000148027'),
VarDef(name='VK_BLEND_OP_XOR_EXT', value='1000148011'),
VarDef(name='VK_BLEND_OP_ZERO_EXT', value='1000148000'),
VarDef(name='VK_BLEND_OP_MAX_ENUM', value='2147483647'),
])

Enum(name='VkBlendOverlapEXT', enumerators = [
VarDef(name='VK_BLEND_OVERLAP_UNCORRELATED_EXT', value='0'),
VarDef(name='VK_BLEND_OVERLAP_DISJOINT_EXT', value='1'),
VarDef(name='VK_BLEND_OVERLAP_CONJOINT_EXT', value='2'),
VarDef(name='VK_BLEND_OVERLAP_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkBorderColor', enumerators = [
VarDef(name='VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK', value='0'),
VarDef(name='VK_BORDER_COLOR_INT_TRANSPARENT_BLACK', value='1'),
VarDef(name='VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK', value='2'),
VarDef(name='VK_BORDER_COLOR_INT_OPAQUE_BLACK', value='3'),
VarDef(name='VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE', value='4'),
VarDef(name='VK_BORDER_COLOR_INT_OPAQUE_WHITE', value='5'),
VarDef(name='VK_BORDER_COLOR_FLOAT_CUSTOM_EXT', value='1000287003'),
VarDef(name='VK_BORDER_COLOR_INT_CUSTOM_EXT', value='1000287004'),
VarDef(name='VK_BORDER_COLOR_MAX_ENUM', value='2147483647'),
])

Enum(name='VkBufferCreateFlagBits', enumerators = [
VarDef(name='VK_BUFFER_CREATE_SPARSE_BINDING_BIT', value='1'),
VarDef(name='VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT', value='2'),
VarDef(name='VK_BUFFER_CREATE_SPARSE_ALIASED_BIT', value='4'),
VarDef(name='VK_BUFFER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT', value='32'),
VarDef(name='VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT', value='16'),
VarDef(name='VK_BUFFER_CREATE_PROTECTED_BIT', value='8'),
VarDef(name='VK_BUFFER_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkBufferUsageFlagBits', enumerators = [
VarDef(name='VK_BUFFER_USAGE_TRANSFER_SRC_BIT', value='1'),
VarDef(name='VK_BUFFER_USAGE_TRANSFER_DST_BIT', value='2'),
VarDef(name='VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT', value='4'),
VarDef(name='VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT', value='8'),
VarDef(name='VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT', value='16'),
VarDef(name='VK_BUFFER_USAGE_STORAGE_BUFFER_BIT', value='32'),
VarDef(name='VK_BUFFER_USAGE_INDEX_BUFFER_BIT', value='64'),
VarDef(name='VK_BUFFER_USAGE_VERTEX_BUFFER_BIT', value='128'),
VarDef(name='VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT', value='256'),
VarDef(name='VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR', value='524288'),
VarDef(name='VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR', value='1048576'),
VarDef(name='VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT', value='512'),
VarDef(name='VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT', value='8388608'),
VarDef(name='VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT', value='16777216'),
VarDef(name='VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT', value='67108864'),
VarDef(name='VK_BUFFER_USAGE_RESERVED_18_BIT_QCOM', value='262144'),
VarDef(name='VK_BUFFER_USAGE_RESERVED_25_BIT_AMD', value='33554432'),
VarDef(name='VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT', value='4194304'),
VarDef(name='VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT', value='2097152'),
VarDef(name='VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR', value='1024'),
VarDef(name='VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT', value='131072'),
VarDef(name='VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT', value='2048'),
VarDef(name='VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT', value='4096'),
VarDef(name='VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR', value='16384'),
VarDef(name='VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR', value='8192'),
VarDef(name='VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR', value='32768'),
VarDef(name='VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR', value='65536'),
VarDef(name='VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkBuildAccelerationStructureFlagBitsKHR', enumerators = [
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR', value='1'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR', value='2'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR', value='4'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR', value='8'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR', value='16'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DISABLE_OPACITY_MICROMAPS_EXT', value='128'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DISPLACEMENT_MICROMAP_UPDATE_NV', value='512'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_OPACITY_MICROMAP_DATA_UPDATE_EXT', value='256'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_OPACITY_MICROMAP_UPDATE_EXT', value='64'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV', value='32'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkBuildAccelerationStructureModeKHR', enumerators = [
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR', value='0'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR', value='1'),
VarDef(name='VK_BUILD_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkBuildMicromapFlagBitsEXT', enumerators = [
VarDef(name='VK_BUILD_MICROMAP_PREFER_FAST_TRACE_BIT_EXT', value='1'),
VarDef(name='VK_BUILD_MICROMAP_PREFER_FAST_BUILD_BIT_EXT', value='2'),
VarDef(name='VK_BUILD_MICROMAP_ALLOW_COMPACTION_BIT_EXT', value='4'),
VarDef(name='VK_BUILD_MICROMAP_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkBuildMicromapModeEXT', enumerators = [
VarDef(name='VK_BUILD_MICROMAP_MODE_BUILD_EXT', value='0'),
VarDef(name='VK_BUILD_MICROMAP_MODE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkChromaLocation', enumerators = [
VarDef(name='VK_CHROMA_LOCATION_COSITED_EVEN', value='0'),
VarDef(name='VK_CHROMA_LOCATION_MIDPOINT', value='1'),
VarDef(name='VK_CHROMA_LOCATION_MAX_ENUM', value='2147483647'),
])

Enum(name='VkCoarseSampleOrderTypeNV', enumerators = [
VarDef(name='VK_COARSE_SAMPLE_ORDER_TYPE_DEFAULT_NV', value='0'),
VarDef(name='VK_COARSE_SAMPLE_ORDER_TYPE_CUSTOM_NV', value='1'),
VarDef(name='VK_COARSE_SAMPLE_ORDER_TYPE_PIXEL_MAJOR_NV', value='2'),
VarDef(name='VK_COARSE_SAMPLE_ORDER_TYPE_SAMPLE_MAJOR_NV', value='3'),
VarDef(name='VK_COARSE_SAMPLE_ORDER_TYPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkColorComponentFlagBits', enumerators = [
VarDef(name='VK_COLOR_COMPONENT_R_BIT', value='1'),
VarDef(name='VK_COLOR_COMPONENT_G_BIT', value='2'),
VarDef(name='VK_COLOR_COMPONENT_B_BIT', value='4'),
VarDef(name='VK_COLOR_COMPONENT_A_BIT', value='8'),
VarDef(name='VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkColorSpaceKHR', enumerators = [
VarDef(name='VK_COLOR_SPACE_SRGB_NONLINEAR_KHR', value='0'),
VarDef(name='VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT', value='1000104011'),
VarDef(name='VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT', value='1000104012'),
VarDef(name='VK_COLOR_SPACE_BT2020_LINEAR_EXT', value='1000104007'),
VarDef(name='VK_COLOR_SPACE_BT709_LINEAR_EXT', value='1000104005'),
VarDef(name='VK_COLOR_SPACE_BT709_NONLINEAR_EXT', value='1000104006'),
VarDef(name='VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT', value='1000104004'),
VarDef(name='VK_COLOR_SPACE_DISPLAY_NATIVE_AMD', value='1000213000'),
VarDef(name='VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT', value='1000104003'),
VarDef(name='VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT', value='1000104001'),
VarDef(name='VK_COLOR_SPACE_DOLBYVISION_EXT', value='1000104009'),
VarDef(name='VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT', value='1000104002'),
VarDef(name='VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT', value='1000104014'),
VarDef(name='VK_COLOR_SPACE_HDR10_HLG_EXT', value='1000104010'),
VarDef(name='VK_COLOR_SPACE_HDR10_ST2084_EXT', value='1000104008'),
VarDef(name='VK_COLOR_SPACE_PASS_THROUGH_EXT', value='1000104013'),
VarDef(name='VK_COLOR_SPACE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkCommandBufferLevel', enumerators = [
VarDef(name='VK_COMMAND_BUFFER_LEVEL_PRIMARY', value='0'),
VarDef(name='VK_COMMAND_BUFFER_LEVEL_SECONDARY', value='1'),
VarDef(name='VK_COMMAND_BUFFER_LEVEL_MAX_ENUM', value='2147483647'),
])

Enum(name='VkCommandBufferResetFlagBits', enumerators = [
VarDef(name='VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT', value='1'),
VarDef(name='VK_COMMAND_BUFFER_RESET_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkCommandBufferUsageFlagBits', enumerators = [
VarDef(name='VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT', value='1'),
VarDef(name='VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT', value='2'),
VarDef(name='VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT', value='4'),
VarDef(name='VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkCommandPoolCreateFlagBits', enumerators = [
VarDef(name='VK_COMMAND_POOL_CREATE_TRANSIENT_BIT', value='1'),
VarDef(name='VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT', value='2'),
VarDef(name='VK_COMMAND_POOL_CREATE_PROTECTED_BIT', value='4'),
VarDef(name='VK_COMMAND_POOL_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkCommandPoolResetFlagBits', enumerators = [
VarDef(name='VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT', value='1'),
VarDef(name='VK_COMMAND_POOL_RESET_RESERVED_1_BIT_COREAVI', value='2'),
VarDef(name='VK_COMMAND_POOL_RESET_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkCompareOp', enumerators = [
VarDef(name='VK_COMPARE_OP_NEVER', value='0'),
VarDef(name='VK_COMPARE_OP_LESS', value='1'),
VarDef(name='VK_COMPARE_OP_EQUAL', value='2'),
VarDef(name='VK_COMPARE_OP_LESS_OR_EQUAL', value='3'),
VarDef(name='VK_COMPARE_OP_GREATER', value='4'),
VarDef(name='VK_COMPARE_OP_NOT_EQUAL', value='5'),
VarDef(name='VK_COMPARE_OP_GREATER_OR_EQUAL', value='6'),
VarDef(name='VK_COMPARE_OP_ALWAYS', value='7'),
VarDef(name='VK_COMPARE_OP_MAX_ENUM', value='2147483647'),
])

Enum(name='VkComponentSwizzle', enumerators = [
VarDef(name='VK_COMPONENT_SWIZZLE_IDENTITY', value='0'),
VarDef(name='VK_COMPONENT_SWIZZLE_ZERO', value='1'),
VarDef(name='VK_COMPONENT_SWIZZLE_ONE', value='2'),
VarDef(name='VK_COMPONENT_SWIZZLE_R', value='3'),
VarDef(name='VK_COMPONENT_SWIZZLE_G', value='4'),
VarDef(name='VK_COMPONENT_SWIZZLE_B', value='5'),
VarDef(name='VK_COMPONENT_SWIZZLE_A', value='6'),
VarDef(name='VK_COMPONENT_SWIZZLE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkComponentTypeNV', enumerators = [
VarDef(name='VK_COMPONENT_TYPE_FLOAT16_NV', value='0'),
VarDef(name='VK_COMPONENT_TYPE_FLOAT32_NV', value='1'),
VarDef(name='VK_COMPONENT_TYPE_FLOAT64_NV', value='2'),
VarDef(name='VK_COMPONENT_TYPE_SINT8_NV', value='3'),
VarDef(name='VK_COMPONENT_TYPE_SINT16_NV', value='4'),
VarDef(name='VK_COMPONENT_TYPE_SINT32_NV', value='5'),
VarDef(name='VK_COMPONENT_TYPE_SINT64_NV', value='6'),
VarDef(name='VK_COMPONENT_TYPE_UINT8_NV', value='7'),
VarDef(name='VK_COMPONENT_TYPE_UINT16_NV', value='8'),
VarDef(name='VK_COMPONENT_TYPE_UINT32_NV', value='9'),
VarDef(name='VK_COMPONENT_TYPE_UINT64_NV', value='10'),
VarDef(name='VK_COMPONENT_TYPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkCompositeAlphaFlagBitsKHR', enumerators = [
VarDef(name='VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR', value='1'),
VarDef(name='VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR', value='2'),
VarDef(name='VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR', value='4'),
VarDef(name='VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR', value='8'),
VarDef(name='VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkConditionalRenderingFlagBitsEXT', enumerators = [
VarDef(name='VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT', value='1'),
VarDef(name='VK_CONDITIONAL_RENDERING_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkConservativeRasterizationModeEXT', enumerators = [
VarDef(name='VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT', value='0'),
VarDef(name='VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT', value='1'),
VarDef(name='VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT', value='2'),
VarDef(name='VK_CONSERVATIVE_RASTERIZATION_MODE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkCopyAccelerationStructureModeKHR', enumerators = [
VarDef(name='VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR', value='0'),
VarDef(name='VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR', value='1'),
VarDef(name='VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR', value='2'),
VarDef(name='VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR', value='3'),
VarDef(name='VK_COPY_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkCopyMicromapModeEXT', enumerators = [
VarDef(name='VK_COPY_MICROMAP_MODE_CLONE_EXT', value='0'),
VarDef(name='VK_COPY_MICROMAP_MODE_SERIALIZE_EXT', value='1'),
VarDef(name='VK_COPY_MICROMAP_MODE_DESERIALIZE_EXT', value='2'),
VarDef(name='VK_COPY_MICROMAP_MODE_COMPACT_EXT', value='3'),
VarDef(name='VK_COPY_MICROMAP_MODE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkCoverageModulationModeNV', enumerators = [
VarDef(name='VK_COVERAGE_MODULATION_MODE_NONE_NV', value='0'),
VarDef(name='VK_COVERAGE_MODULATION_MODE_RGB_NV', value='1'),
VarDef(name='VK_COVERAGE_MODULATION_MODE_ALPHA_NV', value='2'),
VarDef(name='VK_COVERAGE_MODULATION_MODE_RGBA_NV', value='3'),
VarDef(name='VK_COVERAGE_MODULATION_MODE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkCoverageReductionModeNV', enumerators = [
VarDef(name='VK_COVERAGE_REDUCTION_MODE_MERGE_NV', value='0'),
VarDef(name='VK_COVERAGE_REDUCTION_MODE_TRUNCATE_NV', value='1'),
VarDef(name='VK_COVERAGE_REDUCTION_MODE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkCullModeFlagBits', enumerators = [
VarDef(name='VK_CULL_MODE_NONE', value='0'),
VarDef(name='VK_CULL_MODE_FRONT_BIT', value='1'),
VarDef(name='VK_CULL_MODE_BACK_BIT', value='2'),
VarDef(name='VK_CULL_MODE_FRONT_AND_BACK', value='3'),
VarDef(name='VK_CULL_MODE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDebugReportFlagBitsEXT', enumerators = [
VarDef(name='VK_DEBUG_REPORT_INFORMATION_BIT_EXT', value='1'),
VarDef(name='VK_DEBUG_REPORT_WARNING_BIT_EXT', value='2'),
VarDef(name='VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT', value='4'),
VarDef(name='VK_DEBUG_REPORT_ERROR_BIT_EXT', value='8'),
VarDef(name='VK_DEBUG_REPORT_DEBUG_BIT_EXT', value='16'),
VarDef(name='VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDebugReportObjectTypeEXT', enumerators = [
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT', value='0'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT', value='1'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT', value='2'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT', value='3'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT', value='4'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT', value='5'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT', value='6'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT', value='7'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT', value='8'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT', value='9'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT', value='10'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT', value='11'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT', value='12'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT', value='13'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT', value='14'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT', value='15'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT', value='16'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT', value='17'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT', value='18'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT', value='19'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT', value='20'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT', value='21'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT', value='22'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT', value='23'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT', value='24'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT', value='25'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT', value='26'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT', value='27'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT', value='28'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT', value='29'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT', value='30'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT', value='33'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT', value='1000150000'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT', value='1000165000'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA_EXT', value='1000366000'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_CU_FUNCTION_NVX_EXT', value='1000029001'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_CU_MODULE_NVX_EXT', value='1000029000'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT', value='1000085000'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT', value='1000156000'),
VarDef(name='VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDebugUtilsMessageSeverityFlagBitsEXT', enumerators = [
VarDef(name='VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT', value='1'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT', value='16'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT', value='256'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT', value='4096'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDebugUtilsMessageTypeFlagBitsEXT', enumerators = [
VarDef(name='VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT', value='1'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT', value='2'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT', value='4'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT', value='8'),
VarDef(name='VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDependencyFlagBits', enumerators = [
VarDef(name='VK_DEPENDENCY_BY_REGION_BIT', value='1'),
VarDef(name='VK_DEPENDENCY_DEVICE_GROUP_BIT', value='4'),
VarDef(name='VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT', value='8'),
VarDef(name='VK_DEPENDENCY_VIEW_LOCAL_BIT', value='2'),
VarDef(name='VK_DEPENDENCY_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDescriptorBindingFlagBits', enumerators = [
VarDef(name='VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT', value='1'),
VarDef(name='VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT', value='2'),
VarDef(name='VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT', value='4'),
VarDef(name='VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT', value='8'),
VarDef(name='VK_DESCRIPTOR_BINDING_RESERVED_4_BIT_QCOM', value='16'),
VarDef(name='VK_DESCRIPTOR_BINDING_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDescriptorPoolCreateFlagBits', enumerators = [
VarDef(name='VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT', value='1'),
VarDef(name='VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_EXT', value='4'),
VarDef(name='VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT', value='2'),
VarDef(name='VK_DESCRIPTOR_POOL_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDescriptorSetLayoutCreateFlagBits', enumerators = [
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT', value='16'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_EMBEDDED_IMMUTABLE_SAMPLERS_BIT_EXT', value='32'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_HOST_ONLY_POOL_BIT_EXT', value='4'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR', value='1'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_RESERVED_3_BIT_AMD', value='8'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_RESERVED_6_BIT_EXT', value='64'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT', value='2'),
VarDef(name='VK_DESCRIPTOR_SET_LAYOUT_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDescriptorType', enumerators = [
VarDef(name='VK_DESCRIPTOR_TYPE_SAMPLER', value='0'),
VarDef(name='VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER', value='1'),
VarDef(name='VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE', value='2'),
VarDef(name='VK_DESCRIPTOR_TYPE_STORAGE_IMAGE', value='3'),
VarDef(name='VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER', value='4'),
VarDef(name='VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER', value='5'),
VarDef(name='VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER', value='6'),
VarDef(name='VK_DESCRIPTOR_TYPE_STORAGE_BUFFER', value='7'),
VarDef(name='VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC', value='8'),
VarDef(name='VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC', value='9'),
VarDef(name='VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT', value='10'),
VarDef(name='VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR', value='1000150000'),
VarDef(name='VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV', value='1000165000'),
VarDef(name='VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM', value='1000440001'),
VarDef(name='VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK', value='1000138000'),
VarDef(name='VK_DESCRIPTOR_TYPE_MUTABLE_EXT', value='1000351000'),
VarDef(name='VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM', value='1000440000'),
VarDef(name='VK_DESCRIPTOR_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDescriptorUpdateTemplateType', enumerators = [
VarDef(name='VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET', value='0'),
VarDef(name='VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR', value='1'),
VarDef(name='VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDeviceAddressBindingFlagBitsEXT', enumerators = [
VarDef(name='VK_DEVICE_ADDRESS_BINDING_INTERNAL_OBJECT_BIT_EXT', value='1'),
VarDef(name='VK_DEVICE_ADDRESS_BINDING_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDeviceAddressBindingTypeEXT', enumerators = [
VarDef(name='VK_DEVICE_ADDRESS_BINDING_TYPE_BIND_EXT', value='0'),
VarDef(name='VK_DEVICE_ADDRESS_BINDING_TYPE_UNBIND_EXT', value='1'),
VarDef(name='VK_DEVICE_ADDRESS_BINDING_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDeviceDiagnosticsConfigFlagBitsNV', enumerators = [
VarDef(name='VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV', value='1'),
VarDef(name='VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV', value='2'),
VarDef(name='VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV', value='4'),
VarDef(name='VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV', value='8'),
VarDef(name='VK_DEVICE_DIAGNOSTICS_CONFIG_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkDeviceEventTypeEXT', enumerators = [
VarDef(name='VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT', value='0'),
VarDef(name='VK_DEVICE_EVENT_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDeviceFaultAddressTypeEXT', enumerators = [
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT', value='0'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT', value='1'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT', value='2'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT', value='3'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT', value='4'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT', value='5'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT', value='6'),
VarDef(name='VK_DEVICE_FAULT_ADDRESS_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDeviceFaultVendorBinaryHeaderVersionEXT', enumerators = [
VarDef(name='VK_DEVICE_FAULT_VENDOR_BINARY_HEADER_VERSION_ONE_EXT', value='1'),
VarDef(name='VK_DEVICE_FAULT_VENDOR_BINARY_HEADER_VERSION_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDeviceGroupPresentModeFlagBitsKHR', enumerators = [
VarDef(name='VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_BIT_KHR', value='1'),
VarDef(name='VK_DEVICE_GROUP_PRESENT_MODE_REMOTE_BIT_KHR', value='2'),
VarDef(name='VK_DEVICE_GROUP_PRESENT_MODE_SUM_BIT_KHR', value='4'),
VarDef(name='VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_MULTI_DEVICE_BIT_KHR', value='8'),
VarDef(name='VK_DEVICE_GROUP_PRESENT_MODE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkDeviceMemoryReportEventTypeEXT', enumerators = [
VarDef(name='VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_ALLOCATE_EXT', value='0'),
VarDef(name='VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_FREE_EXT', value='1'),
VarDef(name='VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_IMPORT_EXT', value='2'),
VarDef(name='VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_UNIMPORT_EXT', value='3'),
VarDef(name='VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_ALLOCATION_FAILED_EXT', value='4'),
VarDef(name='VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDeviceQueueCreateFlagBits', enumerators = [
VarDef(name='VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT', value='1'),
VarDef(name='VK_DEVICE_QUEUE_CREATE_RESERVED_1_BIT_QCOM', value='2'),
VarDef(name='VK_DEVICE_QUEUE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDirectDriverLoadingModeLUNARG', enumerators = [
VarDef(name='VK_DIRECT_DRIVER_LOADING_MODE_EXCLUSIVE_LUNARG', value='0'),
VarDef(name='VK_DIRECT_DRIVER_LOADING_MODE_INCLUSIVE_LUNARG', value='1'),
VarDef(name='VK_DIRECT_DRIVER_LOADING_MODE_MAX_ENUM_LUNARG', value='2147483647'),
])

Enum(name='VkDiscardRectangleModeEXT', enumerators = [
VarDef(name='VK_DISCARD_RECTANGLE_MODE_INCLUSIVE_EXT', value='0'),
VarDef(name='VK_DISCARD_RECTANGLE_MODE_EXCLUSIVE_EXT', value='1'),
VarDef(name='VK_DISCARD_RECTANGLE_MODE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDisplacementMicromapFormatNV', enumerators = [
VarDef(name='VK_DISPLACEMENT_MICROMAP_FORMAT_64_TRIANGLES_64_BYTES_NV', value='1'),
VarDef(name='VK_DISPLACEMENT_MICROMAP_FORMAT_256_TRIANGLES_128_BYTES_NV', value='2'),
VarDef(name='VK_DISPLACEMENT_MICROMAP_FORMAT_1024_TRIANGLES_128_BYTES_NV', value='3'),
VarDef(name='VK_DISPLACEMENT_MICROMAP_FORMAT_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkDisplayEventTypeEXT', enumerators = [
VarDef(name='VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT', value='0'),
VarDef(name='VK_DISPLAY_EVENT_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDisplayPlaneAlphaFlagBitsKHR', enumerators = [
VarDef(name='VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR', value='1'),
VarDef(name='VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR', value='2'),
VarDef(name='VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR', value='4'),
VarDef(name='VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR', value='8'),
VarDef(name='VK_DISPLAY_PLANE_ALPHA_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkDisplayPowerStateEXT', enumerators = [
VarDef(name='VK_DISPLAY_POWER_STATE_OFF_EXT', value='0'),
VarDef(name='VK_DISPLAY_POWER_STATE_SUSPEND_EXT', value='1'),
VarDef(name='VK_DISPLAY_POWER_STATE_ON_EXT', value='2'),
VarDef(name='VK_DISPLAY_POWER_STATE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkDriverId', enumerators = [
VarDef(name='VK_DRIVER_ID_AMD_PROPRIETARY', value='1'),
VarDef(name='VK_DRIVER_ID_AMD_OPEN_SOURCE', value='2'),
VarDef(name='VK_DRIVER_ID_MESA_RADV', value='3'),
VarDef(name='VK_DRIVER_ID_NVIDIA_PROPRIETARY', value='4'),
VarDef(name='VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS', value='5'),
VarDef(name='VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA', value='6'),
VarDef(name='VK_DRIVER_ID_IMAGINATION_PROPRIETARY', value='7'),
VarDef(name='VK_DRIVER_ID_QUALCOMM_PROPRIETARY', value='8'),
VarDef(name='VK_DRIVER_ID_ARM_PROPRIETARY', value='9'),
VarDef(name='VK_DRIVER_ID_GOOGLE_SWIFTSHADER', value='10'),
VarDef(name='VK_DRIVER_ID_GGP_PROPRIETARY', value='11'),
VarDef(name='VK_DRIVER_ID_BROADCOM_PROPRIETARY', value='12'),
VarDef(name='VK_DRIVER_ID_MESA_LLVMPIPE', value='13'),
VarDef(name='VK_DRIVER_ID_MOLTENVK', value='14'),
VarDef(name='VK_DRIVER_ID_COREAVI_PROPRIETARY', value='15'),
VarDef(name='VK_DRIVER_ID_JUICE_PROPRIETARY', value='16'),
VarDef(name='VK_DRIVER_ID_VERISILICON_PROPRIETARY', value='17'),
VarDef(name='VK_DRIVER_ID_MESA_TURNIP', value='18'),
VarDef(name='VK_DRIVER_ID_MESA_V3DV', value='19'),
VarDef(name='VK_DRIVER_ID_MESA_PANVK', value='20'),
VarDef(name='VK_DRIVER_ID_SAMSUNG_PROPRIETARY', value='21'),
VarDef(name='VK_DRIVER_ID_MESA_VENUS', value='22'),
VarDef(name='VK_DRIVER_ID_MESA_DOZEN', value='23'),
VarDef(name='VK_DRIVER_ID_MESA_NVK', value='24'),
VarDef(name='VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA', value='25'),
VarDef(name='VK_DRIVER_ID_MAX_ENUM', value='2147483647'),
])

Enum(name='VkDynamicState', enumerators = [
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT', value='0'),
VarDef(name='VK_DYNAMIC_STATE_SCISSOR', value='1'),
VarDef(name='VK_DYNAMIC_STATE_LINE_WIDTH', value='2'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_BIAS', value='3'),
VarDef(name='VK_DYNAMIC_STATE_BLEND_CONSTANTS', value='4'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_BOUNDS', value='5'),
VarDef(name='VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK', value='6'),
VarDef(name='VK_DYNAMIC_STATE_STENCIL_WRITE_MASK', value='7'),
VarDef(name='VK_DYNAMIC_STATE_STENCIL_REFERENCE', value='8'),
VarDef(name='VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT', value='1000455007'),
VarDef(name='VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT', value='1000455008'),
VarDef(name='VK_DYNAMIC_STATE_COLOR_BLEND_ADVANCED_EXT', value='1000455018'),
VarDef(name='VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT', value='1000455010'),
VarDef(name='VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT', value='1000455011'),
VarDef(name='VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT', value='1000381000'),
VarDef(name='VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT', value='1000455012'),
VarDef(name='VK_DYNAMIC_STATE_CONSERVATIVE_RASTERIZATION_MODE_EXT', value='1000455014'),
VarDef(name='VK_DYNAMIC_STATE_COVERAGE_MODULATION_MODE_NV', value='1000455027'),
VarDef(name='VK_DYNAMIC_STATE_COVERAGE_MODULATION_TABLE_ENABLE_NV', value='1000455028'),
VarDef(name='VK_DYNAMIC_STATE_COVERAGE_MODULATION_TABLE_NV', value='1000455029'),
VarDef(name='VK_DYNAMIC_STATE_COVERAGE_REDUCTION_MODE_NV', value='1000455032'),
VarDef(name='VK_DYNAMIC_STATE_COVERAGE_TO_COLOR_ENABLE_NV', value='1000455025'),
VarDef(name='VK_DYNAMIC_STATE_COVERAGE_TO_COLOR_LOCATION_NV', value='1000455026'),
VarDef(name='VK_DYNAMIC_STATE_CULL_MODE', value='1000267000'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE', value='1000377002'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE', value='1000267009'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT', value='1000455003'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT', value='1000455016'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE_EXT', value='1000455022'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_COMPARE_OP', value='1000267008'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE', value='1000267006'),
VarDef(name='VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE', value='1000267007'),
VarDef(name='VK_DYNAMIC_STATE_DISCARD_RECTANGLE_ENABLE_EXT', value='1000099001'),
VarDef(name='VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT', value='1000099000'),
VarDef(name='VK_DYNAMIC_STATE_DISCARD_RECTANGLE_MODE_EXT', value='1000099002'),
VarDef(name='VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_ENABLE_NV', value='1000205000'),
VarDef(name='VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV', value='1000205001'),
VarDef(name='VK_DYNAMIC_STATE_EXTRA_PRIMITIVE_OVERESTIMATION_SIZE_EXT', value='1000455015'),
VarDef(name='VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR', value='1000226000'),
VarDef(name='VK_DYNAMIC_STATE_FRONT_FACE', value='1000267001'),
VarDef(name='VK_DYNAMIC_STATE_LINE_RASTERIZATION_MODE_EXT', value='1000455020'),
VarDef(name='VK_DYNAMIC_STATE_LINE_STIPPLE_ENABLE_EXT', value='1000455021'),
VarDef(name='VK_DYNAMIC_STATE_LINE_STIPPLE_EXT', value='1000259000'),
VarDef(name='VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT', value='1000455009'),
VarDef(name='VK_DYNAMIC_STATE_LOGIC_OP_EXT', value='1000377003'),
VarDef(name='VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT', value='1000377000'),
VarDef(name='VK_DYNAMIC_STATE_POLYGON_MODE_EXT', value='1000455004'),
VarDef(name='VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE', value='1000377004'),
VarDef(name='VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY', value='1000267002'),
VarDef(name='VK_DYNAMIC_STATE_PROVOKING_VERTEX_MODE_EXT', value='1000455019'),
VarDef(name='VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT', value='1000455005'),
VarDef(name='VK_DYNAMIC_STATE_RASTERIZATION_STREAM_EXT', value='1000455013'),
VarDef(name='VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE', value='1000377001'),
VarDef(name='VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR', value='1000347000'),
VarDef(name='VK_DYNAMIC_STATE_REPRESENTATIVE_FRAGMENT_TEST_ENABLE_NV', value='1000455031'),
VarDef(name='VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_ENABLE_EXT', value='1000455017'),
VarDef(name='VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT', value='1000143000'),
VarDef(name='VK_DYNAMIC_STATE_SAMPLE_MASK_EXT', value='1000455006'),
VarDef(name='VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT', value='1000267004'),
VarDef(name='VK_DYNAMIC_STATE_SHADING_RATE_IMAGE_ENABLE_NV', value='1000455030'),
VarDef(name='VK_DYNAMIC_STATE_STENCIL_OP', value='1000267011'),
VarDef(name='VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE', value='1000267010'),
VarDef(name='VK_DYNAMIC_STATE_TESSELLATION_DOMAIN_ORIGIN_EXT', value='1000455002'),
VarDef(name='VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE', value='1000267005'),
VarDef(name='VK_DYNAMIC_STATE_VERTEX_INPUT_EXT', value='1000352000'),
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV', value='1000164006'),
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV', value='1000164004'),
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT_SWIZZLE_NV', value='1000455024'),
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT', value='1000267003'),
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_ENABLE_NV', value='1000455023'),
VarDef(name='VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV', value='1000087000'),
VarDef(name='VK_DYNAMIC_STATE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkEventCreateFlagBits', enumerators = [
VarDef(name='VK_EVENT_CREATE_DEVICE_ONLY_BIT', value='1'),
VarDef(name='VK_EVENT_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkExportMetalObjectTypeFlagBitsEXT', enumerators = [
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_METAL_DEVICE_BIT_EXT', value='1'),
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_METAL_COMMAND_QUEUE_BIT_EXT', value='2'),
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_METAL_BUFFER_BIT_EXT', value='4'),
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_METAL_TEXTURE_BIT_EXT', value='8'),
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_METAL_IOSURFACE_BIT_EXT', value='16'),
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_METAL_SHARED_EVENT_BIT_EXT', value='32'),
VarDef(name='VK_EXPORT_METAL_OBJECT_TYPE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkExternalFenceFeatureFlagBits', enumerators = [
VarDef(name='VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT', value='1'),
VarDef(name='VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT', value='2'),
VarDef(name='VK_EXTERNAL_FENCE_FEATURE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkExternalFenceHandleTypeFlagBits', enumerators = [
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT', value='1'),
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT', value='2'),
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT', value='4'),
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT', value='8'),
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_SCI_SYNC_FENCE_BIT_NV', value='32'),
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_SCI_SYNC_OBJ_BIT_NV', value='16'),
VarDef(name='VK_EXTERNAL_FENCE_HANDLE_TYPE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkExternalMemoryFeatureFlagBits', enumerators = [
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT', value='1'),
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT', value='2'),
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT', value='4'),
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkExternalMemoryFeatureFlagBitsNV', enumerators = [
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV', value='1'),
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV', value='2'),
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV', value='4'),
VarDef(name='VK_EXTERNAL_MEMORY_FEATURE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkExternalMemoryHandleTypeFlagBits', enumerators = [
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT', value='1'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT', value='2'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT', value='4'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT', value='8'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT', value='16'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT', value='32'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT', value='64'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID', value='1024'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT', value='512'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT', value='128'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT', value='256'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_RDMA_ADDRESS_BIT_NV', value='4096'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_SCI_BUF_BIT_NV', value='8192'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA', value='2048'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkExternalMemoryHandleTypeFlagBitsNV', enumerators = [
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV', value='1'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV', value='2'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV', value='4'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV', value='8'),
VarDef(name='VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkExternalSemaphoreFeatureFlagBits', enumerators = [
VarDef(name='VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT', value='1'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT', value='2'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_FEATURE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkExternalSemaphoreHandleTypeFlagBits', enumerators = [
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT', value='1'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT', value='2'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT', value='4'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT', value='8'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT', value='16'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SCI_SYNC_OBJ_BIT_NV', value='32'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_ZIRCON_EVENT_BIT_FUCHSIA', value='128'),
VarDef(name='VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFaultLevel', enumerators = [
VarDef(name='VK_FAULT_LEVEL_UNASSIGNED', value='0'),
VarDef(name='VK_FAULT_LEVEL_CRITICAL', value='1'),
VarDef(name='VK_FAULT_LEVEL_RECOVERABLE', value='2'),
VarDef(name='VK_FAULT_LEVEL_WARNING', value='3'),
VarDef(name='VK_FAULT_LEVEL_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFaultQueryBehavior', enumerators = [
VarDef(name='VK_FAULT_QUERY_BEHAVIOR_GET_AND_CLEAR_ALL_FAULTS', value='0'),
VarDef(name='VK_FAULT_QUERY_BEHAVIOR_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFaultType', enumerators = [
VarDef(name='VK_FAULT_TYPE_INVALID', value='0'),
VarDef(name='VK_FAULT_TYPE_UNASSIGNED', value='1'),
VarDef(name='VK_FAULT_TYPE_IMPLEMENTATION', value='2'),
VarDef(name='VK_FAULT_TYPE_SYSTEM', value='3'),
VarDef(name='VK_FAULT_TYPE_PHYSICAL_DEVICE', value='4'),
VarDef(name='VK_FAULT_TYPE_COMMAND_BUFFER_FULL', value='5'),
VarDef(name='VK_FAULT_TYPE_INVALID_API_USAGE', value='6'),
VarDef(name='VK_FAULT_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFenceCreateFlagBits', enumerators = [
VarDef(name='VK_FENCE_CREATE_SIGNALED_BIT', value='1'),
VarDef(name='VK_FENCE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFenceImportFlagBits', enumerators = [
VarDef(name='VK_FENCE_IMPORT_TEMPORARY_BIT', value='1'),
VarDef(name='VK_FENCE_IMPORT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFilter', enumerators = [
VarDef(name='VK_FILTER_NEAREST', value='0'),
VarDef(name='VK_FILTER_LINEAR', value='1'),
VarDef(name='VK_FILTER_CUBIC_EXT', value='1000015000'),
VarDef(name='VK_FILTER_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFormat', enumerators = [
VarDef(name='VK_FORMAT_UNDEFINED', value='0'),
VarDef(name='VK_FORMAT_R4G4_UNORM_PACK8', value='1'),
VarDef(name='VK_FORMAT_R4G4B4A4_UNORM_PACK16', value='2'),
VarDef(name='VK_FORMAT_B4G4R4A4_UNORM_PACK16', value='3'),
VarDef(name='VK_FORMAT_R5G6B5_UNORM_PACK16', value='4'),
VarDef(name='VK_FORMAT_B5G6R5_UNORM_PACK16', value='5'),
VarDef(name='VK_FORMAT_R5G5B5A1_UNORM_PACK16', value='6'),
VarDef(name='VK_FORMAT_B5G5R5A1_UNORM_PACK16', value='7'),
VarDef(name='VK_FORMAT_A1R5G5B5_UNORM_PACK16', value='8'),
VarDef(name='VK_FORMAT_R8_UNORM', value='9'),
VarDef(name='VK_FORMAT_R8_SNORM', value='10'),
VarDef(name='VK_FORMAT_R8_USCALED', value='11'),
VarDef(name='VK_FORMAT_R8_SSCALED', value='12'),
VarDef(name='VK_FORMAT_R8_UINT', value='13'),
VarDef(name='VK_FORMAT_R8_SINT', value='14'),
VarDef(name='VK_FORMAT_R8_SRGB', value='15'),
VarDef(name='VK_FORMAT_R8G8_UNORM', value='16'),
VarDef(name='VK_FORMAT_R8G8_SNORM', value='17'),
VarDef(name='VK_FORMAT_R8G8_USCALED', value='18'),
VarDef(name='VK_FORMAT_R8G8_SSCALED', value='19'),
VarDef(name='VK_FORMAT_R8G8_UINT', value='20'),
VarDef(name='VK_FORMAT_R8G8_SINT', value='21'),
VarDef(name='VK_FORMAT_R8G8_SRGB', value='22'),
VarDef(name='VK_FORMAT_R8G8B8_UNORM', value='23'),
VarDef(name='VK_FORMAT_R8G8B8_SNORM', value='24'),
VarDef(name='VK_FORMAT_R8G8B8_USCALED', value='25'),
VarDef(name='VK_FORMAT_R8G8B8_SSCALED', value='26'),
VarDef(name='VK_FORMAT_R8G8B8_UINT', value='27'),
VarDef(name='VK_FORMAT_R8G8B8_SINT', value='28'),
VarDef(name='VK_FORMAT_R8G8B8_SRGB', value='29'),
VarDef(name='VK_FORMAT_B8G8R8_UNORM', value='30'),
VarDef(name='VK_FORMAT_B8G8R8_SNORM', value='31'),
VarDef(name='VK_FORMAT_B8G8R8_USCALED', value='32'),
VarDef(name='VK_FORMAT_B8G8R8_SSCALED', value='33'),
VarDef(name='VK_FORMAT_B8G8R8_UINT', value='34'),
VarDef(name='VK_FORMAT_B8G8R8_SINT', value='35'),
VarDef(name='VK_FORMAT_B8G8R8_SRGB', value='36'),
VarDef(name='VK_FORMAT_R8G8B8A8_UNORM', value='37'),
VarDef(name='VK_FORMAT_R8G8B8A8_SNORM', value='38'),
VarDef(name='VK_FORMAT_R8G8B8A8_USCALED', value='39'),
VarDef(name='VK_FORMAT_R8G8B8A8_SSCALED', value='40'),
VarDef(name='VK_FORMAT_R8G8B8A8_UINT', value='41'),
VarDef(name='VK_FORMAT_R8G8B8A8_SINT', value='42'),
VarDef(name='VK_FORMAT_R8G8B8A8_SRGB', value='43'),
VarDef(name='VK_FORMAT_B8G8R8A8_UNORM', value='44'),
VarDef(name='VK_FORMAT_B8G8R8A8_SNORM', value='45'),
VarDef(name='VK_FORMAT_B8G8R8A8_USCALED', value='46'),
VarDef(name='VK_FORMAT_B8G8R8A8_SSCALED', value='47'),
VarDef(name='VK_FORMAT_B8G8R8A8_UINT', value='48'),
VarDef(name='VK_FORMAT_B8G8R8A8_SINT', value='49'),
VarDef(name='VK_FORMAT_B8G8R8A8_SRGB', value='50'),
VarDef(name='VK_FORMAT_A8B8G8R8_UNORM_PACK32', value='51'),
VarDef(name='VK_FORMAT_A8B8G8R8_SNORM_PACK32', value='52'),
VarDef(name='VK_FORMAT_A8B8G8R8_USCALED_PACK32', value='53'),
VarDef(name='VK_FORMAT_A8B8G8R8_SSCALED_PACK32', value='54'),
VarDef(name='VK_FORMAT_A8B8G8R8_UINT_PACK32', value='55'),
VarDef(name='VK_FORMAT_A8B8G8R8_SINT_PACK32', value='56'),
VarDef(name='VK_FORMAT_A8B8G8R8_SRGB_PACK32', value='57'),
VarDef(name='VK_FORMAT_A2R10G10B10_UNORM_PACK32', value='58'),
VarDef(name='VK_FORMAT_A2R10G10B10_SNORM_PACK32', value='59'),
VarDef(name='VK_FORMAT_A2R10G10B10_USCALED_PACK32', value='60'),
VarDef(name='VK_FORMAT_A2R10G10B10_SSCALED_PACK32', value='61'),
VarDef(name='VK_FORMAT_A2R10G10B10_UINT_PACK32', value='62'),
VarDef(name='VK_FORMAT_A2R10G10B10_SINT_PACK32', value='63'),
VarDef(name='VK_FORMAT_A2B10G10R10_UNORM_PACK32', value='64'),
VarDef(name='VK_FORMAT_A2B10G10R10_SNORM_PACK32', value='65'),
VarDef(name='VK_FORMAT_A2B10G10R10_USCALED_PACK32', value='66'),
VarDef(name='VK_FORMAT_A2B10G10R10_SSCALED_PACK32', value='67'),
VarDef(name='VK_FORMAT_A2B10G10R10_UINT_PACK32', value='68'),
VarDef(name='VK_FORMAT_A2B10G10R10_SINT_PACK32', value='69'),
VarDef(name='VK_FORMAT_R16_UNORM', value='70'),
VarDef(name='VK_FORMAT_R16_SNORM', value='71'),
VarDef(name='VK_FORMAT_R16_USCALED', value='72'),
VarDef(name='VK_FORMAT_R16_SSCALED', value='73'),
VarDef(name='VK_FORMAT_R16_UINT', value='74'),
VarDef(name='VK_FORMAT_R16_SINT', value='75'),
VarDef(name='VK_FORMAT_R16_SFLOAT', value='76'),
VarDef(name='VK_FORMAT_R16G16_UNORM', value='77'),
VarDef(name='VK_FORMAT_R16G16_SNORM', value='78'),
VarDef(name='VK_FORMAT_R16G16_USCALED', value='79'),
VarDef(name='VK_FORMAT_R16G16_SSCALED', value='80'),
VarDef(name='VK_FORMAT_R16G16_UINT', value='81'),
VarDef(name='VK_FORMAT_R16G16_SINT', value='82'),
VarDef(name='VK_FORMAT_R16G16_SFLOAT', value='83'),
VarDef(name='VK_FORMAT_R16G16B16_UNORM', value='84'),
VarDef(name='VK_FORMAT_R16G16B16_SNORM', value='85'),
VarDef(name='VK_FORMAT_R16G16B16_USCALED', value='86'),
VarDef(name='VK_FORMAT_R16G16B16_SSCALED', value='87'),
VarDef(name='VK_FORMAT_R16G16B16_UINT', value='88'),
VarDef(name='VK_FORMAT_R16G16B16_SINT', value='89'),
VarDef(name='VK_FORMAT_R16G16B16_SFLOAT', value='90'),
VarDef(name='VK_FORMAT_R16G16B16A16_UNORM', value='91'),
VarDef(name='VK_FORMAT_R16G16B16A16_SNORM', value='92'),
VarDef(name='VK_FORMAT_R16G16B16A16_USCALED', value='93'),
VarDef(name='VK_FORMAT_R16G16B16A16_SSCALED', value='94'),
VarDef(name='VK_FORMAT_R16G16B16A16_UINT', value='95'),
VarDef(name='VK_FORMAT_R16G16B16A16_SINT', value='96'),
VarDef(name='VK_FORMAT_R16G16B16A16_SFLOAT', value='97'),
VarDef(name='VK_FORMAT_R32_UINT', value='98'),
VarDef(name='VK_FORMAT_R32_SINT', value='99'),
VarDef(name='VK_FORMAT_R32_SFLOAT', value='100'),
VarDef(name='VK_FORMAT_R32G32_UINT', value='101'),
VarDef(name='VK_FORMAT_R32G32_SINT', value='102'),
VarDef(name='VK_FORMAT_R32G32_SFLOAT', value='103'),
VarDef(name='VK_FORMAT_R32G32B32_UINT', value='104'),
VarDef(name='VK_FORMAT_R32G32B32_SINT', value='105'),
VarDef(name='VK_FORMAT_R32G32B32_SFLOAT', value='106'),
VarDef(name='VK_FORMAT_R32G32B32A32_UINT', value='107'),
VarDef(name='VK_FORMAT_R32G32B32A32_SINT', value='108'),
VarDef(name='VK_FORMAT_R32G32B32A32_SFLOAT', value='109'),
VarDef(name='VK_FORMAT_R64_UINT', value='110'),
VarDef(name='VK_FORMAT_R64_SINT', value='111'),
VarDef(name='VK_FORMAT_R64_SFLOAT', value='112'),
VarDef(name='VK_FORMAT_R64G64_UINT', value='113'),
VarDef(name='VK_FORMAT_R64G64_SINT', value='114'),
VarDef(name='VK_FORMAT_R64G64_SFLOAT', value='115'),
VarDef(name='VK_FORMAT_R64G64B64_UINT', value='116'),
VarDef(name='VK_FORMAT_R64G64B64_SINT', value='117'),
VarDef(name='VK_FORMAT_R64G64B64_SFLOAT', value='118'),
VarDef(name='VK_FORMAT_R64G64B64A64_UINT', value='119'),
VarDef(name='VK_FORMAT_R64G64B64A64_SINT', value='120'),
VarDef(name='VK_FORMAT_R64G64B64A64_SFLOAT', value='121'),
VarDef(name='VK_FORMAT_B10G11R11_UFLOAT_PACK32', value='122'),
VarDef(name='VK_FORMAT_E5B9G9R9_UFLOAT_PACK32', value='123'),
VarDef(name='VK_FORMAT_D16_UNORM', value='124'),
VarDef(name='VK_FORMAT_X8_D24_UNORM_PACK32', value='125'),
VarDef(name='VK_FORMAT_D32_SFLOAT', value='126'),
VarDef(name='VK_FORMAT_S8_UINT', value='127'),
VarDef(name='VK_FORMAT_D16_UNORM_S8_UINT', value='128'),
VarDef(name='VK_FORMAT_D24_UNORM_S8_UINT', value='129'),
VarDef(name='VK_FORMAT_D32_SFLOAT_S8_UINT', value='130'),
VarDef(name='VK_FORMAT_BC1_RGB_UNORM_BLOCK', value='131'),
VarDef(name='VK_FORMAT_BC1_RGB_SRGB_BLOCK', value='132'),
VarDef(name='VK_FORMAT_BC1_RGBA_UNORM_BLOCK', value='133'),
VarDef(name='VK_FORMAT_BC1_RGBA_SRGB_BLOCK', value='134'),
VarDef(name='VK_FORMAT_BC2_UNORM_BLOCK', value='135'),
VarDef(name='VK_FORMAT_BC2_SRGB_BLOCK', value='136'),
VarDef(name='VK_FORMAT_BC3_UNORM_BLOCK', value='137'),
VarDef(name='VK_FORMAT_BC3_SRGB_BLOCK', value='138'),
VarDef(name='VK_FORMAT_BC4_UNORM_BLOCK', value='139'),
VarDef(name='VK_FORMAT_BC4_SNORM_BLOCK', value='140'),
VarDef(name='VK_FORMAT_BC5_UNORM_BLOCK', value='141'),
VarDef(name='VK_FORMAT_BC5_SNORM_BLOCK', value='142'),
VarDef(name='VK_FORMAT_BC6H_UFLOAT_BLOCK', value='143'),
VarDef(name='VK_FORMAT_BC6H_SFLOAT_BLOCK', value='144'),
VarDef(name='VK_FORMAT_BC7_UNORM_BLOCK', value='145'),
VarDef(name='VK_FORMAT_BC7_SRGB_BLOCK', value='146'),
VarDef(name='VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK', value='147'),
VarDef(name='VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK', value='148'),
VarDef(name='VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK', value='149'),
VarDef(name='VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK', value='150'),
VarDef(name='VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK', value='151'),
VarDef(name='VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK', value='152'),
VarDef(name='VK_FORMAT_EAC_R11_UNORM_BLOCK', value='153'),
VarDef(name='VK_FORMAT_EAC_R11_SNORM_BLOCK', value='154'),
VarDef(name='VK_FORMAT_EAC_R11G11_UNORM_BLOCK', value='155'),
VarDef(name='VK_FORMAT_EAC_R11G11_SNORM_BLOCK', value='156'),
VarDef(name='VK_FORMAT_ASTC_4x4_UNORM_BLOCK', value='157'),
VarDef(name='VK_FORMAT_ASTC_4x4_SRGB_BLOCK', value='158'),
VarDef(name='VK_FORMAT_ASTC_5x4_UNORM_BLOCK', value='159'),
VarDef(name='VK_FORMAT_ASTC_5x4_SRGB_BLOCK', value='160'),
VarDef(name='VK_FORMAT_ASTC_5x5_UNORM_BLOCK', value='161'),
VarDef(name='VK_FORMAT_ASTC_5x5_SRGB_BLOCK', value='162'),
VarDef(name='VK_FORMAT_ASTC_6x5_UNORM_BLOCK', value='163'),
VarDef(name='VK_FORMAT_ASTC_6x5_SRGB_BLOCK', value='164'),
VarDef(name='VK_FORMAT_ASTC_6x6_UNORM_BLOCK', value='165'),
VarDef(name='VK_FORMAT_ASTC_6x6_SRGB_BLOCK', value='166'),
VarDef(name='VK_FORMAT_ASTC_8x5_UNORM_BLOCK', value='167'),
VarDef(name='VK_FORMAT_ASTC_8x5_SRGB_BLOCK', value='168'),
VarDef(name='VK_FORMAT_ASTC_8x6_UNORM_BLOCK', value='169'),
VarDef(name='VK_FORMAT_ASTC_8x6_SRGB_BLOCK', value='170'),
VarDef(name='VK_FORMAT_ASTC_8x8_UNORM_BLOCK', value='171'),
VarDef(name='VK_FORMAT_ASTC_8x8_SRGB_BLOCK', value='172'),
VarDef(name='VK_FORMAT_ASTC_10x5_UNORM_BLOCK', value='173'),
VarDef(name='VK_FORMAT_ASTC_10x5_SRGB_BLOCK', value='174'),
VarDef(name='VK_FORMAT_ASTC_10x6_UNORM_BLOCK', value='175'),
VarDef(name='VK_FORMAT_ASTC_10x6_SRGB_BLOCK', value='176'),
VarDef(name='VK_FORMAT_ASTC_10x8_UNORM_BLOCK', value='177'),
VarDef(name='VK_FORMAT_ASTC_10x8_SRGB_BLOCK', value='178'),
VarDef(name='VK_FORMAT_ASTC_10x10_UNORM_BLOCK', value='179'),
VarDef(name='VK_FORMAT_ASTC_10x10_SRGB_BLOCK', value='180'),
VarDef(name='VK_FORMAT_ASTC_12x10_UNORM_BLOCK', value='181'),
VarDef(name='VK_FORMAT_ASTC_12x10_SRGB_BLOCK', value='182'),
VarDef(name='VK_FORMAT_ASTC_12x12_UNORM_BLOCK', value='183'),
VarDef(name='VK_FORMAT_ASTC_12x12_SRGB_BLOCK', value='184'),
VarDef(name='VK_FORMAT_A4B4G4R4_UNORM_PACK16', value='1000340001'),
VarDef(name='VK_FORMAT_A4R4G4B4_UNORM_PACK16', value='1000340000'),
VarDef(name='VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK', value='1000066011'),
VarDef(name='VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK', value='1000066008'),
VarDef(name='VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK', value='1000066009'),
VarDef(name='VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK', value='1000066010'),
VarDef(name='VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK', value='1000066012'),
VarDef(name='VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK', value='1000066013'),
VarDef(name='VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT', value='1000288002'),
VarDef(name='VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT', value='1000288001'),
VarDef(name='VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT', value='1000288000'),
VarDef(name='VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT', value='1000288005'),
VarDef(name='VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT', value='1000288004'),
VarDef(name='VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT', value='1000288003'),
VarDef(name='VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK', value='1000066000'),
VarDef(name='VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT', value='1000288008'),
VarDef(name='VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT', value='1000288007'),
VarDef(name='VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT', value='1000288006'),
VarDef(name='VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT', value='1000288011'),
VarDef(name='VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT', value='1000288010'),
VarDef(name='VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT', value='1000288009'),
VarDef(name='VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK', value='1000066001'),
VarDef(name='VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT', value='1000288014'),
VarDef(name='VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT', value='1000288013'),
VarDef(name='VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT', value='1000288012'),
VarDef(name='VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK', value='1000066002'),
VarDef(name='VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT', value='1000288017'),
VarDef(name='VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT', value='1000288016'),
VarDef(name='VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT', value='1000288015'),
VarDef(name='VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT', value='1000288020'),
VarDef(name='VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT', value='1000288019'),
VarDef(name='VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT', value='1000288018'),
VarDef(name='VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK', value='1000066003'),
VarDef(name='VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT', value='1000288023'),
VarDef(name='VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT', value='1000288022'),
VarDef(name='VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT', value='1000288021'),
VarDef(name='VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK', value='1000066004'),
VarDef(name='VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT', value='1000288026'),
VarDef(name='VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT', value='1000288025'),
VarDef(name='VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT', value='1000288024'),
VarDef(name='VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT', value='1000288029'),
VarDef(name='VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT', value='1000288028'),
VarDef(name='VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT', value='1000288027'),
VarDef(name='VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK', value='1000066005'),
VarDef(name='VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK', value='1000066006'),
VarDef(name='VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK', value='1000066007'),
VarDef(name='VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16', value='1000156011'),
VarDef(name='VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16', value='1000156021'),
VarDef(name='VK_FORMAT_B16G16R16G16_422_UNORM', value='1000156028'),
VarDef(name='VK_FORMAT_B8G8R8G8_422_UNORM', value='1000156001'),
VarDef(name='VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16', value='1000156010'),
VarDef(name='VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16', value='1000156013'),
VarDef(name='VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16', value='1000156015'),
VarDef(name='VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16', value='1000330001'),
VarDef(name='VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16', value='1000156012'),
VarDef(name='VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16', value='1000156014'),
VarDef(name='VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16', value='1000156016'),
VarDef(name='VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16', value='1000156020'),
VarDef(name='VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16', value='1000156023'),
VarDef(name='VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16', value='1000156025'),
VarDef(name='VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16', value='1000330002'),
VarDef(name='VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16', value='1000156022'),
VarDef(name='VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16', value='1000156024'),
VarDef(name='VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16', value='1000156026'),
VarDef(name='VK_FORMAT_G16B16G16R16_422_UNORM', value='1000156027'),
VarDef(name='VK_FORMAT_G16_B16R16_2PLANE_420_UNORM', value='1000156030'),
VarDef(name='VK_FORMAT_G16_B16R16_2PLANE_422_UNORM', value='1000156032'),
VarDef(name='VK_FORMAT_G16_B16R16_2PLANE_444_UNORM', value='1000330003'),
VarDef(name='VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM', value='1000156029'),
VarDef(name='VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM', value='1000156031'),
VarDef(name='VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM', value='1000156033'),
VarDef(name='VK_FORMAT_G8B8G8R8_422_UNORM', value='1000156000'),
VarDef(name='VK_FORMAT_G8_B8R8_2PLANE_420_UNORM', value='1000156003'),
VarDef(name='VK_FORMAT_G8_B8R8_2PLANE_422_UNORM', value='1000156005'),
VarDef(name='VK_FORMAT_G8_B8R8_2PLANE_444_UNORM', value='1000330000'),
VarDef(name='VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM', value='1000156002'),
VarDef(name='VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM', value='1000156004'),
VarDef(name='VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM', value='1000156006'),
VarDef(name='VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG', value='1000054004'),
VarDef(name='VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG', value='1000054000'),
VarDef(name='VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG', value='1000054005'),
VarDef(name='VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG', value='1000054001'),
VarDef(name='VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG', value='1000054006'),
VarDef(name='VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG', value='1000054002'),
VarDef(name='VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG', value='1000054007'),
VarDef(name='VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG', value='1000054003'),
VarDef(name='VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16', value='1000156009'),
VarDef(name='VK_FORMAT_R10X6G10X6_UNORM_2PACK16', value='1000156008'),
VarDef(name='VK_FORMAT_R10X6_UNORM_PACK16', value='1000156007'),
VarDef(name='VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16', value='1000156019'),
VarDef(name='VK_FORMAT_R12X4G12X4_UNORM_2PACK16', value='1000156018'),
VarDef(name='VK_FORMAT_R12X4_UNORM_PACK16', value='1000156017'),
VarDef(name='VK_FORMAT_R16G16_S10_5_NV', value='1000464000'),
VarDef(name='VK_FORMAT_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFormatFeatureFlagBits', enumerators = [
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT', value='1'),
VarDef(name='VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT', value='2'),
VarDef(name='VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT', value='4'),
VarDef(name='VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT', value='8'),
VarDef(name='VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT', value='16'),
VarDef(name='VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT', value='32'),
VarDef(name='VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT', value='64'),
VarDef(name='VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT', value='128'),
VarDef(name='VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT', value='256'),
VarDef(name='VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT', value='512'),
VarDef(name='VK_FORMAT_FEATURE_BLIT_SRC_BIT', value='1024'),
VarDef(name='VK_FORMAT_FEATURE_BLIT_DST_BIT', value='2048'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT', value='4096'),
VarDef(name='VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR', value='536870912'),
VarDef(name='VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT', value='8388608'),
VarDef(name='VK_FORMAT_FEATURE_DISJOINT_BIT', value='4194304'),
VarDef(name='VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT', value='16777216'),
VarDef(name='VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR', value='1073741824'),
VarDef(name='VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT', value='131072'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT', value='8192'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT', value='65536'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT', value='1048576'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT', value='2097152'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT', value='262144'),
VarDef(name='VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT', value='524288'),
VarDef(name='VK_FORMAT_FEATURE_TRANSFER_DST_BIT', value='32768'),
VarDef(name='VK_FORMAT_FEATURE_TRANSFER_SRC_BIT', value='16384'),
VarDef(name='VK_FORMAT_FEATURE_VIDEO_DECODE_DPB_BIT_KHR', value='67108864'),
VarDef(name='VK_FORMAT_FEATURE_VIDEO_DECODE_OUTPUT_BIT_KHR', value='33554432'),
VarDef(name='VK_FORMAT_FEATURE_VIDEO_ENCODE_DPB_BIT_KHR', value='268435456'),
VarDef(name='VK_FORMAT_FEATURE_VIDEO_ENCODE_INPUT_BIT_KHR', value='134217728'),
VarDef(name='VK_FORMAT_FEATURE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFormatFeatureFlagBits2', size=64, enumerators = [
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT', value='1'),
VarDef(name='VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT', value='2'),
VarDef(name='VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT', value='4'),
VarDef(name='VK_FORMAT_FEATURE_2_UNIFORM_TEXEL_BUFFER_BIT', value='8'),
VarDef(name='VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT', value='16'),
VarDef(name='VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_ATOMIC_BIT', value='32'),
VarDef(name='VK_FORMAT_FEATURE_2_VERTEX_BUFFER_BIT', value='64'),
VarDef(name='VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT', value='128'),
VarDef(name='VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT', value='256'),
VarDef(name='VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT', value='512'),
VarDef(name='VK_FORMAT_FEATURE_2_BLIT_SRC_BIT', value='1024'),
VarDef(name='VK_FORMAT_FEATURE_2_BLIT_DST_BIT', value='2048'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT', value='4096'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_CUBIC_BIT', value='8192'),
VarDef(name='VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT', value='16384'),
VarDef(name='VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT', value='32768'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_MINMAX_BIT', value='65536'),
VarDef(name='VK_FORMAT_FEATURE_2_MIDPOINT_CHROMA_SAMPLES_BIT', value='131072'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT', value='262144'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT', value='524288'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT', value='1048576'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT', value='2097152'),
VarDef(name='VK_FORMAT_FEATURE_2_DISJOINT_BIT', value='4194304'),
VarDef(name='VK_FORMAT_FEATURE_2_COSITED_CHROMA_SAMPLES_BIT', value='8388608'),
VarDef(name='VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT', value='2147483648'),
VarDef(name='VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT', value='4294967296'),
VarDef(name='VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_DEPTH_COMPARISON_BIT', value='8589934592'),
VarDef(name='VK_FORMAT_FEATURE_2_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR', value='536870912'),
VarDef(name='VK_FORMAT_FEATURE_2_BLOCK_MATCHING_BIT_QCOM', value='68719476736'),
VarDef(name='VK_FORMAT_FEATURE_2_BOX_FILTER_SAMPLED_BIT_QCOM', value='137438953472'),
VarDef(name='VK_FORMAT_FEATURE_2_FRAGMENT_DENSITY_MAP_BIT_EXT', value='16777216'),
VarDef(name='VK_FORMAT_FEATURE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR', value='1073741824'),
VarDef(name='VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV', value='274877906944'),
VarDef(name='VK_FORMAT_FEATURE_2_OPTICAL_FLOW_COST_BIT_NV', value='4398046511104'),
VarDef(name='VK_FORMAT_FEATURE_2_OPTICAL_FLOW_IMAGE_BIT_NV', value='1099511627776'),
VarDef(name='VK_FORMAT_FEATURE_2_OPTICAL_FLOW_VECTOR_BIT_NV', value='2199023255552'),
VarDef(name='VK_FORMAT_FEATURE_2_RESERVED_39_BIT_EXT', value='549755813888'),
VarDef(name='VK_FORMAT_FEATURE_2_RESERVED_44_BIT_EXT', value='17592186044416'),
VarDef(name='VK_FORMAT_FEATURE_2_RESERVED_45_BIT_EXT', value='35184372088832'),
VarDef(name='VK_FORMAT_FEATURE_2_VIDEO_DECODE_DPB_BIT_KHR', value='67108864'),
VarDef(name='VK_FORMAT_FEATURE_2_VIDEO_DECODE_OUTPUT_BIT_KHR', value='33554432'),
VarDef(name='VK_FORMAT_FEATURE_2_VIDEO_ENCODE_DPB_BIT_KHR', value='268435456'),
VarDef(name='VK_FORMAT_FEATURE_2_VIDEO_ENCODE_INPUT_BIT_KHR', value='134217728'),
VarDef(name='VK_FORMAT_FEATURE_2_WEIGHT_IMAGE_BIT_QCOM', value='17179869184'),
VarDef(name='VK_FORMAT_FEATURE_2_WEIGHT_SAMPLED_IMAGE_BIT_QCOM', value='34359738368'),
VarDef(name='VK_FORMAT_FEATURE_2_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFragmentShadingRateCombinerOpKHR', enumerators = [
VarDef(name='VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR', value='0'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR', value='1'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR', value='2'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR', value='3'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR', value='4'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkFragmentShadingRateNV', enumerators = [
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV', value='0'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV', value='1'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV', value='4'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV', value='5'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV', value='6'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV', value='9'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV', value='10'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV', value='11'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV', value='12'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV', value='13'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV', value='14'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_NO_INVOCATIONS_NV', value='15'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkFragmentShadingRateTypeNV', enumerators = [
VarDef(name='VK_FRAGMENT_SHADING_RATE_TYPE_FRAGMENT_SIZE_NV', value='0'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_TYPE_ENUMS_NV', value='1'),
VarDef(name='VK_FRAGMENT_SHADING_RATE_TYPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkFramebufferCreateFlagBits', enumerators = [
VarDef(name='VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT', value='1'),
VarDef(name='VK_FRAMEBUFFER_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFrontFace', enumerators = [
VarDef(name='VK_FRONT_FACE_COUNTER_CLOCKWISE', value='0'),
VarDef(name='VK_FRONT_FACE_CLOCKWISE', value='1'),
VarDef(name='VK_FRONT_FACE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkFullScreenExclusiveEXT', enumerators = [
VarDef(name='VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT', value='0'),
VarDef(name='VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT', value='1'),
VarDef(name='VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT', value='2'),
VarDef(name='VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT', value='3'),
VarDef(name='VK_FULL_SCREEN_EXCLUSIVE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkGeometryFlagBitsKHR', enumerators = [
VarDef(name='VK_GEOMETRY_OPAQUE_BIT_KHR', value='1'),
VarDef(name='VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR', value='2'),
VarDef(name='VK_GEOMETRY_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkGeometryInstanceFlagBitsKHR', enumerators = [
VarDef(name='VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR', value='1'),
VarDef(name='VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR', value='2'),
VarDef(name='VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR', value='4'),
VarDef(name='VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR', value='8'),
VarDef(name='VK_GEOMETRY_INSTANCE_DISABLE_OPACITY_MICROMAPS_EXT', value='32'),
VarDef(name='VK_GEOMETRY_INSTANCE_FORCE_OPACITY_MICROMAP_2_STATE_EXT', value='16'),
VarDef(name='VK_GEOMETRY_INSTANCE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkGeometryTypeKHR', enumerators = [
VarDef(name='VK_GEOMETRY_TYPE_TRIANGLES_KHR', value='0'),
VarDef(name='VK_GEOMETRY_TYPE_AABBS_KHR', value='1'),
VarDef(name='VK_GEOMETRY_TYPE_INSTANCES_KHR', value='2'),
VarDef(name='VK_GEOMETRY_TYPE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkGraphicsPipelineLibraryFlagBitsEXT', enumerators = [
VarDef(name='VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT', value='1'),
VarDef(name='VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT', value='2'),
VarDef(name='VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT', value='4'),
VarDef(name='VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT', value='8'),
VarDef(name='VK_GRAPHICS_PIPELINE_LIBRARY_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkImageAspectFlagBits', enumerators = [
VarDef(name='VK_IMAGE_ASPECT_COLOR_BIT', value='1'),
VarDef(name='VK_IMAGE_ASPECT_DEPTH_BIT', value='2'),
VarDef(name='VK_IMAGE_ASPECT_STENCIL_BIT', value='4'),
VarDef(name='VK_IMAGE_ASPECT_METADATA_BIT', value='8'),
VarDef(name='VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT', value='128'),
VarDef(name='VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT', value='256'),
VarDef(name='VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT', value='512'),
VarDef(name='VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT', value='1024'),
VarDef(name='VK_IMAGE_ASPECT_NONE', value='0'),
VarDef(name='VK_IMAGE_ASPECT_PLANE_0_BIT', value='16'),
VarDef(name='VK_IMAGE_ASPECT_PLANE_1_BIT', value='32'),
VarDef(name='VK_IMAGE_ASPECT_PLANE_2_BIT', value='64'),
VarDef(name='VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageCompressionFixedRateFlagBitsEXT', enumerators = [
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT', value='0'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_1BPC_BIT_EXT', value='1'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_2BPC_BIT_EXT', value='2'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_3BPC_BIT_EXT', value='4'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_4BPC_BIT_EXT', value='8'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_5BPC_BIT_EXT', value='16'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_6BPC_BIT_EXT', value='32'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_7BPC_BIT_EXT', value='64'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_8BPC_BIT_EXT', value='128'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_9BPC_BIT_EXT', value='256'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_10BPC_BIT_EXT', value='512'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_11BPC_BIT_EXT', value='1024'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_12BPC_BIT_EXT', value='2048'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_13BPC_BIT_EXT', value='4096'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_14BPC_BIT_EXT', value='8192'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_15BPC_BIT_EXT', value='16384'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_16BPC_BIT_EXT', value='32768'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_17BPC_BIT_EXT', value='65536'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_18BPC_BIT_EXT', value='131072'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_19BPC_BIT_EXT', value='262144'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_20BPC_BIT_EXT', value='524288'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_21BPC_BIT_EXT', value='1048576'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_22BPC_BIT_EXT', value='2097152'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_23BPC_BIT_EXT', value='4194304'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_24BPC_BIT_EXT', value='8388608'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkImageCompressionFlagBitsEXT', enumerators = [
VarDef(name='VK_IMAGE_COMPRESSION_DEFAULT_EXT', value='0'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_DEFAULT_EXT', value='1'),
VarDef(name='VK_IMAGE_COMPRESSION_FIXED_RATE_EXPLICIT_EXT', value='2'),
VarDef(name='VK_IMAGE_COMPRESSION_DISABLED_EXT', value='4'),
VarDef(name='VK_IMAGE_COMPRESSION_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkImageCreateFlagBits', enumerators = [
VarDef(name='VK_IMAGE_CREATE_SPARSE_BINDING_BIT', value='1'),
VarDef(name='VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT', value='2'),
VarDef(name='VK_IMAGE_CREATE_SPARSE_ALIASED_BIT', value='4'),
VarDef(name='VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT', value='8'),
VarDef(name='VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT', value='16'),
VarDef(name='VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT', value='32'),
VarDef(name='VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT', value='131072'),
VarDef(name='VK_IMAGE_CREATE_ALIAS_BIT', value='1024'),
VarDef(name='VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT', value='128'),
VarDef(name='VK_IMAGE_CREATE_CORNER_SAMPLED_BIT_NV', value='8192'),
VarDef(name='VK_IMAGE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT', value='65536'),
VarDef(name='VK_IMAGE_CREATE_DISJOINT_BIT', value='512'),
VarDef(name='VK_IMAGE_CREATE_EXTENDED_USAGE_BIT', value='256'),
VarDef(name='VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM', value='32768'),
VarDef(name='VK_IMAGE_CREATE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_BIT_EXT', value='262144'),
VarDef(name='VK_IMAGE_CREATE_PROTECTED_BIT', value='2048'),
VarDef(name='VK_IMAGE_CREATE_RESERVED_19_BIT_EXT', value='524288'),
VarDef(name='VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT', value='4096'),
VarDef(name='VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT', value='64'),
VarDef(name='VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT', value='16384'),
VarDef(name='VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageLayout', enumerators = [
VarDef(name='VK_IMAGE_LAYOUT_UNDEFINED', value='0'),
VarDef(name='VK_IMAGE_LAYOUT_GENERAL', value='1'),
VarDef(name='VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL', value='2'),
VarDef(name='VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL', value='3'),
VarDef(name='VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL', value='4'),
VarDef(name='VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL', value='5'),
VarDef(name='VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL', value='6'),
VarDef(name='VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL', value='7'),
VarDef(name='VK_IMAGE_LAYOUT_PREINITIALIZED', value='8'),
VarDef(name='VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT', value='1000339000'),
VarDef(name='VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL', value='1000314001'),
VarDef(name='VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL', value='1000241000'),
VarDef(name='VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL', value='1000117001'),
VarDef(name='VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL', value='1000241001'),
VarDef(name='VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL', value='1000117000'),
VarDef(name='VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT', value='1000218000'),
VarDef(name='VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR', value='1000164003'),
VarDef(name='VK_IMAGE_LAYOUT_PRESENT_SRC_KHR', value='1000001002'),
VarDef(name='VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL', value='1000314000'),
VarDef(name='VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR', value='1000111000'),
VarDef(name='VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL', value='1000241002'),
VarDef(name='VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL', value='1000241003'),
VarDef(name='VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR', value='1000024002'),
VarDef(name='VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR', value='1000024000'),
VarDef(name='VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR', value='1000024001'),
VarDef(name='VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR', value='1000299002'),
VarDef(name='VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR', value='1000299000'),
VarDef(name='VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR', value='1000299001'),
VarDef(name='VK_IMAGE_LAYOUT_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageTiling', enumerators = [
VarDef(name='VK_IMAGE_TILING_OPTIMAL', value='0'),
VarDef(name='VK_IMAGE_TILING_LINEAR', value='1'),
VarDef(name='VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT', value='1000158000'),
VarDef(name='VK_IMAGE_TILING_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageType', enumerators = [
VarDef(name='VK_IMAGE_TYPE_1D', value='0'),
VarDef(name='VK_IMAGE_TYPE_2D', value='1'),
VarDef(name='VK_IMAGE_TYPE_3D', value='2'),
VarDef(name='VK_IMAGE_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageUsageFlagBits', enumerators = [
VarDef(name='VK_IMAGE_USAGE_TRANSFER_SRC_BIT', value='1'),
VarDef(name='VK_IMAGE_USAGE_TRANSFER_DST_BIT', value='2'),
VarDef(name='VK_IMAGE_USAGE_SAMPLED_BIT', value='4'),
VarDef(name='VK_IMAGE_USAGE_STORAGE_BIT', value='8'),
VarDef(name='VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT', value='16'),
VarDef(name='VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT', value='32'),
VarDef(name='VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT', value='64'),
VarDef(name='VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT', value='128'),
VarDef(name='VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT', value='524288'),
VarDef(name='VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT', value='512'),
VarDef(name='VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR', value='256'),
VarDef(name='VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI', value='262144'),
VarDef(name='VK_IMAGE_USAGE_RESERVED_16_BIT_QCOM', value='65536'),
VarDef(name='VK_IMAGE_USAGE_RESERVED_17_BIT_QCOM', value='131072'),
VarDef(name='VK_IMAGE_USAGE_RESERVED_22_BIT_EXT', value='4194304'),
VarDef(name='VK_IMAGE_USAGE_RESERVED_23_BIT_EXT', value='8388608'),
VarDef(name='VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM', value='2097152'),
VarDef(name='VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM', value='1048576'),
VarDef(name='VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR', value='4096'),
VarDef(name='VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR', value='1024'),
VarDef(name='VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR', value='2048'),
VarDef(name='VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR', value='32768'),
VarDef(name='VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR', value='8192'),
VarDef(name='VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR', value='16384'),
VarDef(name='VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageViewCreateFlagBits', enumerators = [
VarDef(name='VK_IMAGE_VIEW_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT', value='4'),
VarDef(name='VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DEFERRED_BIT_EXT', value='2'),
VarDef(name='VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT', value='1'),
VarDef(name='VK_IMAGE_VIEW_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkImageViewType', enumerators = [
VarDef(name='VK_IMAGE_VIEW_TYPE_1D', value='0'),
VarDef(name='VK_IMAGE_VIEW_TYPE_2D', value='1'),
VarDef(name='VK_IMAGE_VIEW_TYPE_3D', value='2'),
VarDef(name='VK_IMAGE_VIEW_TYPE_CUBE', value='3'),
VarDef(name='VK_IMAGE_VIEW_TYPE_1D_ARRAY', value='4'),
VarDef(name='VK_IMAGE_VIEW_TYPE_2D_ARRAY', value='5'),
VarDef(name='VK_IMAGE_VIEW_TYPE_CUBE_ARRAY', value='6'),
VarDef(name='VK_IMAGE_VIEW_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkIndexType', enumerators = [
VarDef(name='VK_INDEX_TYPE_UINT16', value='0'),
VarDef(name='VK_INDEX_TYPE_UINT32', value='1'),
VarDef(name='VK_INDEX_TYPE_NONE_KHR', value='1000165000'),
VarDef(name='VK_INDEX_TYPE_UINT8_EXT', value='1000265000'),
VarDef(name='VK_INDEX_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkIndirectCommandsLayoutUsageFlagBitsNV', enumerators = [
VarDef(name='VK_INDIRECT_COMMANDS_LAYOUT_USAGE_EXPLICIT_PREPROCESS_BIT_NV', value='1'),
VarDef(name='VK_INDIRECT_COMMANDS_LAYOUT_USAGE_INDEXED_SEQUENCES_BIT_NV', value='2'),
VarDef(name='VK_INDIRECT_COMMANDS_LAYOUT_USAGE_UNORDERED_SEQUENCES_BIT_NV', value='4'),
VarDef(name='VK_INDIRECT_COMMANDS_LAYOUT_USAGE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkIndirectCommandsTokenTypeNV', enumerators = [
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_SHADER_GROUP_NV', value='0'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_STATE_FLAGS_NV', value='1'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV', value='2'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV', value='3'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV', value='4'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV', value='5'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV', value='6'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_TASKS_NV', value='7'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_MESH_TASKS_NV', value='1000328000'),
VarDef(name='VK_INDIRECT_COMMANDS_TOKEN_TYPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkIndirectStateFlagBitsNV', enumerators = [
VarDef(name='VK_INDIRECT_STATE_FLAG_FRONTFACE_BIT_NV', value='1'),
VarDef(name='VK_INDIRECT_STATE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkInstanceCreateFlagBits', enumerators = [
VarDef(name='VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR', value='1'),
VarDef(name='VK_INSTANCE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkInternalAllocationType', enumerators = [
VarDef(name='VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE', value='0'),
VarDef(name='VK_INTERNAL_ALLOCATION_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkLineRasterizationModeEXT', enumerators = [
VarDef(name='VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT', value='0'),
VarDef(name='VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT', value='1'),
VarDef(name='VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT', value='2'),
VarDef(name='VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT', value='3'),
VarDef(name='VK_LINE_RASTERIZATION_MODE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkLogicOp', enumerators = [
VarDef(name='VK_LOGIC_OP_CLEAR', value='0'),
VarDef(name='VK_LOGIC_OP_AND', value='1'),
VarDef(name='VK_LOGIC_OP_AND_REVERSE', value='2'),
VarDef(name='VK_LOGIC_OP_COPY', value='3'),
VarDef(name='VK_LOGIC_OP_AND_INVERTED', value='4'),
VarDef(name='VK_LOGIC_OP_NO_OP', value='5'),
VarDef(name='VK_LOGIC_OP_XOR', value='6'),
VarDef(name='VK_LOGIC_OP_OR', value='7'),
VarDef(name='VK_LOGIC_OP_NOR', value='8'),
VarDef(name='VK_LOGIC_OP_EQUIVALENT', value='9'),
VarDef(name='VK_LOGIC_OP_INVERT', value='10'),
VarDef(name='VK_LOGIC_OP_OR_REVERSE', value='11'),
VarDef(name='VK_LOGIC_OP_COPY_INVERTED', value='12'),
VarDef(name='VK_LOGIC_OP_OR_INVERTED', value='13'),
VarDef(name='VK_LOGIC_OP_NAND', value='14'),
VarDef(name='VK_LOGIC_OP_SET', value='15'),
VarDef(name='VK_LOGIC_OP_MAX_ENUM', value='2147483647'),
])

Enum(name='VkMemoryAllocateFlagBits', enumerators = [
VarDef(name='VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT', value='1'),
VarDef(name='VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT', value='2'),
VarDef(name='VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT', value='4'),
VarDef(name='VK_MEMORY_ALLOCATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkMemoryDecompressionMethodFlagBitsNV', enumerators = [
VarDef(name='VK_MEMORY_DECOMPRESSION_METHOD_GDEFLATE_1_0_BIT_NV', value='1'),
VarDef(name='VK_MEMORY_DECOMPRESSION_METHOD_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkMemoryHeapFlagBits', enumerators = [
VarDef(name='VK_MEMORY_HEAP_DEVICE_LOCAL_BIT', value='1'),
VarDef(name='VK_MEMORY_HEAP_MULTI_INSTANCE_BIT', value='2'),
VarDef(name='VK_MEMORY_HEAP_SEU_SAFE_BIT', value='4'),
VarDef(name='VK_MEMORY_HEAP_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkMemoryOverallocationBehaviorAMD', enumerators = [
VarDef(name='VK_MEMORY_OVERALLOCATION_BEHAVIOR_DEFAULT_AMD', value='0'),
VarDef(name='VK_MEMORY_OVERALLOCATION_BEHAVIOR_ALLOWED_AMD', value='1'),
VarDef(name='VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD', value='2'),
VarDef(name='VK_MEMORY_OVERALLOCATION_BEHAVIOR_MAX_ENUM_AMD', value='2147483647'),
])

Enum(name='VkMemoryPropertyFlagBits', enumerators = [
VarDef(name='VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT', value='1'),
VarDef(name='VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT', value='2'),
VarDef(name='VK_MEMORY_PROPERTY_HOST_COHERENT_BIT', value='4'),
VarDef(name='VK_MEMORY_PROPERTY_HOST_CACHED_BIT', value='8'),
VarDef(name='VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT', value='16'),
VarDef(name='VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD', value='64'),
VarDef(name='VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD', value='128'),
VarDef(name='VK_MEMORY_PROPERTY_PROTECTED_BIT', value='32'),
VarDef(name='VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV', value='256'),
VarDef(name='VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkMicromapCreateFlagBitsEXT', enumerators = [
VarDef(name='VK_MICROMAP_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_EXT', value='1'),
VarDef(name='VK_MICROMAP_CREATE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkMicromapTypeEXT', enumerators = [
VarDef(name='VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT', value='0'),
VarDef(name='VK_MICROMAP_TYPE_DISPLACEMENT_MICROMAP_NV', value='1000397000'),
VarDef(name='VK_MICROMAP_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkObjectType', enumerators = [
VarDef(name='VK_OBJECT_TYPE_UNKNOWN', value='0'),
VarDef(name='VK_OBJECT_TYPE_INSTANCE', value='1'),
VarDef(name='VK_OBJECT_TYPE_PHYSICAL_DEVICE', value='2'),
VarDef(name='VK_OBJECT_TYPE_DEVICE', value='3'),
VarDef(name='VK_OBJECT_TYPE_QUEUE', value='4'),
VarDef(name='VK_OBJECT_TYPE_SEMAPHORE', value='5'),
VarDef(name='VK_OBJECT_TYPE_COMMAND_BUFFER', value='6'),
VarDef(name='VK_OBJECT_TYPE_FENCE', value='7'),
VarDef(name='VK_OBJECT_TYPE_DEVICE_MEMORY', value='8'),
VarDef(name='VK_OBJECT_TYPE_BUFFER', value='9'),
VarDef(name='VK_OBJECT_TYPE_IMAGE', value='10'),
VarDef(name='VK_OBJECT_TYPE_EVENT', value='11'),
VarDef(name='VK_OBJECT_TYPE_QUERY_POOL', value='12'),
VarDef(name='VK_OBJECT_TYPE_BUFFER_VIEW', value='13'),
VarDef(name='VK_OBJECT_TYPE_IMAGE_VIEW', value='14'),
VarDef(name='VK_OBJECT_TYPE_SHADER_MODULE', value='15'),
VarDef(name='VK_OBJECT_TYPE_PIPELINE_CACHE', value='16'),
VarDef(name='VK_OBJECT_TYPE_PIPELINE_LAYOUT', value='17'),
VarDef(name='VK_OBJECT_TYPE_RENDER_PASS', value='18'),
VarDef(name='VK_OBJECT_TYPE_PIPELINE', value='19'),
VarDef(name='VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT', value='20'),
VarDef(name='VK_OBJECT_TYPE_SAMPLER', value='21'),
VarDef(name='VK_OBJECT_TYPE_DESCRIPTOR_POOL', value='22'),
VarDef(name='VK_OBJECT_TYPE_DESCRIPTOR_SET', value='23'),
VarDef(name='VK_OBJECT_TYPE_FRAMEBUFFER', value='24'),
VarDef(name='VK_OBJECT_TYPE_COMMAND_POOL', value='25'),
VarDef(name='VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR', value='1000150000'),
VarDef(name='VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV', value='1000165000'),
VarDef(name='VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA', value='1000366000'),
VarDef(name='VK_OBJECT_TYPE_CU_FUNCTION_NVX', value='1000029001'),
VarDef(name='VK_OBJECT_TYPE_CU_MODULE_NVX', value='1000029000'),
VarDef(name='VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT', value='1000011000'),
VarDef(name='VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT', value='1000128000'),
VarDef(name='VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR', value='1000268000'),
VarDef(name='VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE', value='1000085000'),
VarDef(name='VK_OBJECT_TYPE_DISPLAY_KHR', value='1000002000'),
VarDef(name='VK_OBJECT_TYPE_DISPLAY_MODE_KHR', value='1000002001'),
VarDef(name='VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV', value='1000277000'),
VarDef(name='VK_OBJECT_TYPE_MICROMAP_EXT', value='1000396000'),
VarDef(name='VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV', value='1000464000'),
VarDef(name='VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL', value='1000210000'),
VarDef(name='VK_OBJECT_TYPE_PRIVATE_DATA_SLOT', value='1000295000'),
VarDef(name='VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION', value='1000156000'),
VarDef(name='VK_OBJECT_TYPE_SEMAPHORE_SCI_SYNC_POOL_NV', value='1000489000'),
VarDef(name='VK_OBJECT_TYPE_SHADER_EXT', value='1000482000'),
VarDef(name='VK_OBJECT_TYPE_SURFACE_KHR', value='1000000000'),
VarDef(name='VK_OBJECT_TYPE_SWAPCHAIN_KHR', value='1000001000'),
VarDef(name='VK_OBJECT_TYPE_VALIDATION_CACHE_EXT', value='1000160000'),
VarDef(name='VK_OBJECT_TYPE_VIDEO_SESSION_KHR', value='1000023000'),
VarDef(name='VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR', value='1000023001'),
VarDef(name='VK_OBJECT_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkOpacityMicromapFormatEXT', enumerators = [
VarDef(name='VK_OPACITY_MICROMAP_FORMAT_2_STATE_EXT', value='1'),
VarDef(name='VK_OPACITY_MICROMAP_FORMAT_4_STATE_EXT', value='2'),
VarDef(name='VK_OPACITY_MICROMAP_FORMAT_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkOpacityMicromapSpecialIndexEXT', enumerators = [
VarDef(name='VK_OPACITY_MICROMAP_SPECIAL_INDEX_FULLY_TRANSPARENT_EXT', value='-1'),
VarDef(name='VK_OPACITY_MICROMAP_SPECIAL_INDEX_FULLY_OPAQUE_EXT', value='-2'),
VarDef(name='VK_OPACITY_MICROMAP_SPECIAL_INDEX_FULLY_UNKNOWN_TRANSPARENT_EXT', value='-3'),
VarDef(name='VK_OPACITY_MICROMAP_SPECIAL_INDEX_FULLY_UNKNOWN_OPAQUE_EXT', value='-4'),
VarDef(name='VK_OPACITY_MICROMAP_SPECIAL_INDEX_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkOpticalFlowExecuteFlagBitsNV', enumerators = [
VarDef(name='VK_OPTICAL_FLOW_EXECUTE_DISABLE_TEMPORAL_HINTS_BIT_NV', value='1'),
VarDef(name='VK_OPTICAL_FLOW_EXECUTE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkOpticalFlowGridSizeFlagBitsNV', enumerators = [
VarDef(name='VK_OPTICAL_FLOW_GRID_SIZE_UNKNOWN_NV', value='0'),
VarDef(name='VK_OPTICAL_FLOW_GRID_SIZE_1X1_BIT_NV', value='1'),
VarDef(name='VK_OPTICAL_FLOW_GRID_SIZE_2X2_BIT_NV', value='2'),
VarDef(name='VK_OPTICAL_FLOW_GRID_SIZE_4X4_BIT_NV', value='4'),
VarDef(name='VK_OPTICAL_FLOW_GRID_SIZE_8X8_BIT_NV', value='8'),
VarDef(name='VK_OPTICAL_FLOW_GRID_SIZE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkOpticalFlowPerformanceLevelNV', enumerators = [
VarDef(name='VK_OPTICAL_FLOW_PERFORMANCE_LEVEL_UNKNOWN_NV', value='0'),
VarDef(name='VK_OPTICAL_FLOW_PERFORMANCE_LEVEL_SLOW_NV', value='1'),
VarDef(name='VK_OPTICAL_FLOW_PERFORMANCE_LEVEL_MEDIUM_NV', value='2'),
VarDef(name='VK_OPTICAL_FLOW_PERFORMANCE_LEVEL_FAST_NV', value='3'),
VarDef(name='VK_OPTICAL_FLOW_PERFORMANCE_LEVEL_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkOpticalFlowSessionBindingPointNV', enumerators = [
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_UNKNOWN_NV', value='0'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_INPUT_NV', value='1'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_REFERENCE_NV', value='2'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_HINT_NV', value='3'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_FLOW_VECTOR_NV', value='4'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_BACKWARD_FLOW_VECTOR_NV', value='5'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_COST_NV', value='6'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_BACKWARD_COST_NV', value='7'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_GLOBAL_FLOW_NV', value='8'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_BINDING_POINT_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkOpticalFlowSessionCreateFlagBitsNV', enumerators = [
VarDef(name='VK_OPTICAL_FLOW_SESSION_CREATE_ENABLE_HINT_BIT_NV', value='1'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_CREATE_ENABLE_COST_BIT_NV', value='2'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_CREATE_ENABLE_GLOBAL_FLOW_BIT_NV', value='4'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_CREATE_ALLOW_REGIONS_BIT_NV', value='8'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_CREATE_BOTH_DIRECTIONS_BIT_NV', value='16'),
VarDef(name='VK_OPTICAL_FLOW_SESSION_CREATE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkOpticalFlowUsageFlagBitsNV', enumerators = [
VarDef(name='VK_OPTICAL_FLOW_USAGE_UNKNOWN_NV', value='0'),
VarDef(name='VK_OPTICAL_FLOW_USAGE_INPUT_BIT_NV', value='1'),
VarDef(name='VK_OPTICAL_FLOW_USAGE_OUTPUT_BIT_NV', value='2'),
VarDef(name='VK_OPTICAL_FLOW_USAGE_HINT_BIT_NV', value='4'),
VarDef(name='VK_OPTICAL_FLOW_USAGE_COST_BIT_NV', value='8'),
VarDef(name='VK_OPTICAL_FLOW_USAGE_GLOBAL_FLOW_BIT_NV', value='16'),
VarDef(name='VK_OPTICAL_FLOW_USAGE_FLAG_BITS_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkPeerMemoryFeatureFlagBits', enumerators = [
VarDef(name='VK_PEER_MEMORY_FEATURE_COPY_SRC_BIT', value='1'),
VarDef(name='VK_PEER_MEMORY_FEATURE_COPY_DST_BIT', value='2'),
VarDef(name='VK_PEER_MEMORY_FEATURE_GENERIC_SRC_BIT', value='4'),
VarDef(name='VK_PEER_MEMORY_FEATURE_GENERIC_DST_BIT', value='8'),
VarDef(name='VK_PEER_MEMORY_FEATURE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPerformanceConfigurationTypeINTEL', enumerators = [
VarDef(name='VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL', value='0'),
VarDef(name='VK_PERFORMANCE_CONFIGURATION_TYPE_MAX_ENUM_INTEL', value='2147483647'),
])

Enum(name='VkPerformanceCounterDescriptionFlagBitsKHR', enumerators = [
VarDef(name='VK_PERFORMANCE_COUNTER_DESCRIPTION_PERFORMANCE_IMPACTING_BIT_KHR', value='1'),
VarDef(name='VK_PERFORMANCE_COUNTER_DESCRIPTION_CONCURRENTLY_IMPACTED_BIT_KHR', value='2'),
VarDef(name='VK_PERFORMANCE_COUNTER_DESCRIPTION_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkPerformanceCounterScopeKHR', enumerators = [
VarDef(name='VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_BUFFER_KHR', value='0'),
VarDef(name='VK_PERFORMANCE_COUNTER_SCOPE_RENDER_PASS_KHR', value='1'),
VarDef(name='VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_KHR', value='2'),
VarDef(name='VK_PERFORMANCE_COUNTER_SCOPE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkPerformanceCounterStorageKHR', enumerators = [
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_INT32_KHR', value='0'),
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_INT64_KHR', value='1'),
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR', value='2'),
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR', value='3'),
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR', value='4'),
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR', value='5'),
VarDef(name='VK_PERFORMANCE_COUNTER_STORAGE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkPerformanceCounterUnitKHR', enumerators = [
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_GENERIC_KHR', value='0'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_KHR', value='1'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_KHR', value='2'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_BYTES_KHR', value='3'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_KHR', value='4'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_KELVIN_KHR', value='5'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_WATTS_KHR', value='6'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_VOLTS_KHR', value='7'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_AMPS_KHR', value='8'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_HERTZ_KHR', value='9'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_CYCLES_KHR', value='10'),
VarDef(name='VK_PERFORMANCE_COUNTER_UNIT_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkPerformanceOverrideTypeINTEL', enumerators = [
VarDef(name='VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL', value='0'),
VarDef(name='VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL', value='1'),
VarDef(name='VK_PERFORMANCE_OVERRIDE_TYPE_MAX_ENUM_INTEL', value='2147483647'),
])

Enum(name='VkPerformanceParameterTypeINTEL', enumerators = [
VarDef(name='VK_PERFORMANCE_PARAMETER_TYPE_HW_COUNTERS_SUPPORTED_INTEL', value='0'),
VarDef(name='VK_PERFORMANCE_PARAMETER_TYPE_STREAM_MARKER_VALID_BITS_INTEL', value='1'),
VarDef(name='VK_PERFORMANCE_PARAMETER_TYPE_MAX_ENUM_INTEL', value='2147483647'),
])

Enum(name='VkPerformanceValueTypeINTEL', enumerators = [
VarDef(name='VK_PERFORMANCE_VALUE_TYPE_UINT32_INTEL', value='0'),
VarDef(name='VK_PERFORMANCE_VALUE_TYPE_UINT64_INTEL', value='1'),
VarDef(name='VK_PERFORMANCE_VALUE_TYPE_FLOAT_INTEL', value='2'),
VarDef(name='VK_PERFORMANCE_VALUE_TYPE_BOOL_INTEL', value='3'),
VarDef(name='VK_PERFORMANCE_VALUE_TYPE_STRING_INTEL', value='4'),
VarDef(name='VK_PERFORMANCE_VALUE_TYPE_MAX_ENUM_INTEL', value='2147483647'),
])

Enum(name='VkPhysicalDeviceType', enumerators = [
VarDef(name='VK_PHYSICAL_DEVICE_TYPE_OTHER', value='0'),
VarDef(name='VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU', value='1'),
VarDef(name='VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU', value='2'),
VarDef(name='VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU', value='3'),
VarDef(name='VK_PHYSICAL_DEVICE_TYPE_CPU', value='4'),
VarDef(name='VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineBindPoint', enumerators = [
VarDef(name='VK_PIPELINE_BIND_POINT_GRAPHICS', value='0'),
VarDef(name='VK_PIPELINE_BIND_POINT_COMPUTE', value='1'),
VarDef(name='VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR', value='1000165000'),
VarDef(name='VK_PIPELINE_BIND_POINT_SUBPASS_SHADING_HUAWEI', value='1000369003'),
VarDef(name='VK_PIPELINE_BIND_POINT_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineCacheCreateFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT', value='1'),
VarDef(name='VK_PIPELINE_CACHE_CREATE_READ_ONLY_BIT', value='2'),
VarDef(name='VK_PIPELINE_CACHE_CREATE_USE_APPLICATION_STORAGE_BIT', value='4'),
VarDef(name='VK_PIPELINE_CACHE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineCacheHeaderVersion', enumerators = [
VarDef(name='VK_PIPELINE_CACHE_HEADER_VERSION_ONE', value='1'),
VarDef(name='VK_PIPELINE_CACHE_HEADER_VERSION_SAFETY_CRITICAL_ONE', value='1000298001'),
VarDef(name='VK_PIPELINE_CACHE_HEADER_VERSION_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineCacheValidationVersion', enumerators = [
VarDef(name='VK_PIPELINE_CACHE_VALIDATION_VERSION_SAFETY_CRITICAL_ONE', value='1'),
VarDef(name='VK_PIPELINE_CACHE_VALIDATION_VERSION_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineColorBlendStateCreateFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_COLOR_BLEND_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_BIT_EXT', value='1'),
VarDef(name='VK_PIPELINE_COLOR_BLEND_STATE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineCompilerControlFlagBitsAMD', enumerators = [
VarDef(name='VK_PIPELINE_COMPILER_CONTROL_FLAG_BITS_MAX_ENUM_AMD', value='2147483647'),
])

Enum(name='VkPipelineCreateFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT', value='1'),
VarDef(name='VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT', value='2'),
VarDef(name='VK_PIPELINE_CREATE_DERIVATIVE_BIT', value='4'),
VarDef(name='VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR', value='128'),
VarDef(name='VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR', value='64'),
VarDef(name='VK_PIPELINE_CREATE_COLOR_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT', value='33554432'),
VarDef(name='VK_PIPELINE_CREATE_DEFER_COMPILE_BIT_NV', value='32'),
VarDef(name='VK_PIPELINE_CREATE_DEPTH_STENCIL_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT', value='67108864'),
VarDef(name='VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT', value='536870912'),
VarDef(name='VK_PIPELINE_CREATE_DISPATCH_BASE_BIT', value='16'),
VarDef(name='VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT', value='512'),
VarDef(name='VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT', value='256'),
VarDef(name='VK_PIPELINE_CREATE_INDIRECT_BINDABLE_BIT_NV', value='262144'),
VarDef(name='VK_PIPELINE_CREATE_LIBRARY_BIT_KHR', value='2048'),
VarDef(name='VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT', value='1024'),
VarDef(name='VK_PIPELINE_CREATE_NO_PROTECTED_ACCESS_BIT_EXT', value='134217728'),
VarDef(name='VK_PIPELINE_CREATE_PROTECTED_ACCESS_ONLY_BIT_EXT', value='1073741824'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_ALLOW_MOTION_BIT_NV', value='1048576'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_DISPLACEMENT_MICROMAP_BIT_NV', value='268435456'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR', value='16384'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR', value='32768'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_INTERSECTION_SHADERS_BIT_KHR', value='131072'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR', value='65536'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_OPACITY_MICROMAP_BIT_EXT', value='16777216'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR', value='524288'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_SKIP_AABBS_BIT_KHR', value='8192'),
VarDef(name='VK_PIPELINE_CREATE_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR', value='4096'),
VarDef(name='VK_PIPELINE_CREATE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_BIT_EXT', value='4194304'),
VarDef(name='VK_PIPELINE_CREATE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR', value='2097152'),
VarDef(name='VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT', value='8388608'),
VarDef(name='VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT', value='8'),
VarDef(name='VK_PIPELINE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineCreationFeedbackFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT', value='1'),
VarDef(name='VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT', value='2'),
VarDef(name='VK_PIPELINE_CREATION_FEEDBACK_BASE_PIPELINE_ACCELERATION_BIT', value='4'),
VarDef(name='VK_PIPELINE_CREATION_FEEDBACK_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineDepthStencilStateCreateFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_DEPTH_ACCESS_BIT_EXT', value='1'),
VarDef(name='VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_STENCIL_ACCESS_BIT_EXT', value='2'),
VarDef(name='VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineExecutableStatisticFormatKHR', enumerators = [
VarDef(name='VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR', value='0'),
VarDef(name='VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR', value='1'),
VarDef(name='VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR', value='2'),
VarDef(name='VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR', value='3'),
VarDef(name='VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkPipelineLayoutCreateFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT', value='2'),
VarDef(name='VK_PIPELINE_LAYOUT_CREATE_RESERVED_0_BIT_AMD', value='1'),
VarDef(name='VK_PIPELINE_LAYOUT_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineMatchControl', enumerators = [
VarDef(name='VK_PIPELINE_MATCH_CONTROL_APPLICATION_UUID_EXACT_MATCH', value='0'),
VarDef(name='VK_PIPELINE_MATCH_CONTROL_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineRobustnessBufferBehaviorEXT', enumerators = [
VarDef(name='VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT', value='0'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT', value='1'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT', value='2'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT', value='3'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkPipelineRobustnessImageBehaviorEXT', enumerators = [
VarDef(name='VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT_EXT', value='0'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED_EXT', value='1'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_EXT', value='2'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2_EXT', value='3'),
VarDef(name='VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkPipelineShaderStageCreateFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT', value='1'),
VarDef(name='VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT', value='2'),
VarDef(name='VK_PIPELINE_SHADER_STAGE_CREATE_RESERVED_3_BIT_KHR', value='8'),
VarDef(name='VK_PIPELINE_SHADER_STAGE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineStageFlagBits', enumerators = [
VarDef(name='VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT', value='1'),
VarDef(name='VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT', value='2'),
VarDef(name='VK_PIPELINE_STAGE_VERTEX_INPUT_BIT', value='4'),
VarDef(name='VK_PIPELINE_STAGE_VERTEX_SHADER_BIT', value='8'),
VarDef(name='VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT', value='16'),
VarDef(name='VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT', value='32'),
VarDef(name='VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT', value='64'),
VarDef(name='VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT', value='128'),
VarDef(name='VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT', value='256'),
VarDef(name='VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT', value='512'),
VarDef(name='VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT', value='1024'),
VarDef(name='VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT', value='2048'),
VarDef(name='VK_PIPELINE_STAGE_TRANSFER_BIT', value='4096'),
VarDef(name='VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT', value='8192'),
VarDef(name='VK_PIPELINE_STAGE_HOST_BIT', value='16384'),
VarDef(name='VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT', value='32768'),
VarDef(name='VK_PIPELINE_STAGE_ALL_COMMANDS_BIT', value='65536'),
VarDef(name='VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR', value='33554432'),
VarDef(name='VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV', value='131072'),
VarDef(name='VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT', value='262144'),
VarDef(name='VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT', value='8388608'),
VarDef(name='VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR', value='4194304'),
VarDef(name='VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT', value='1048576'),
VarDef(name='VK_PIPELINE_STAGE_NONE', value='0'),
VarDef(name='VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR', value='2097152'),
VarDef(name='VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT', value='524288'),
VarDef(name='VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT', value='16777216'),
VarDef(name='VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPipelineStageFlagBits2', size=64, enumerators = [
VarDef(name='VK_PIPELINE_STAGE_2_NONE', value='0'),
VarDef(name='VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT', value='1'),
VarDef(name='VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT', value='2'),
VarDef(name='VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT', value='4'),
VarDef(name='VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT', value='8'),
VarDef(name='VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT', value='16'),
VarDef(name='VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT', value='32'),
VarDef(name='VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT', value='64'),
VarDef(name='VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT', value='128'),
VarDef(name='VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT', value='256'),
VarDef(name='VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT', value='512'),
VarDef(name='VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT', value='1024'),
VarDef(name='VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT', value='2048'),
VarDef(name='VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT', value='4096'),
VarDef(name='VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT', value='8192'),
VarDef(name='VK_PIPELINE_STAGE_2_HOST_BIT', value='16384'),
VarDef(name='VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT', value='32768'),
VarDef(name='VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT', value='65536'),
VarDef(name='VK_PIPELINE_STAGE_2_COPY_BIT', value='4294967296'),
VarDef(name='VK_PIPELINE_STAGE_2_RESOLVE_BIT', value='8589934592'),
VarDef(name='VK_PIPELINE_STAGE_2_BLIT_BIT', value='17179869184'),
VarDef(name='VK_PIPELINE_STAGE_2_CLEAR_BIT', value='34359738368'),
VarDef(name='VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT', value='68719476736'),
VarDef(name='VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT', value='137438953472'),
VarDef(name='VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT', value='274877906944'),
VarDef(name='VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR', value='33554432'),
VarDef(name='VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR', value='268435456'),
VarDef(name='VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI', value='2199023255552'),
VarDef(name='VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV', value='131072'),
VarDef(name='VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT', value='262144'),
VarDef(name='VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT', value='8388608'),
VarDef(name='VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR', value='4194304'),
VarDef(name='VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI', value='1099511627776'),
VarDef(name='VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT', value='1048576'),
VarDef(name='VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT', value='1073741824'),
VarDef(name='VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV', value='536870912'),
VarDef(name='VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR', value='2097152'),
VarDef(name='VK_PIPELINE_STAGE_2_RESERVED_42_BIT_EXT', value='4398046511104'),
VarDef(name='VK_PIPELINE_STAGE_2_RESERVED_43_BIT_ARM', value='8796093022208'),
VarDef(name='VK_PIPELINE_STAGE_2_SUBPASS_SHADING_BIT_HUAWEI', value='549755813888'),
VarDef(name='VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT', value='524288'),
VarDef(name='VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT', value='16777216'),
VarDef(name='VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR', value='67108864'),
VarDef(name='VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR', value='134217728'),
])

Enum(name='VkPointClippingBehavior', enumerators = [
VarDef(name='VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES', value='0'),
VarDef(name='VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY', value='1'),
VarDef(name='VK_POINT_CLIPPING_BEHAVIOR_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPolygonMode', enumerators = [
VarDef(name='VK_POLYGON_MODE_FILL', value='0'),
VarDef(name='VK_POLYGON_MODE_LINE', value='1'),
VarDef(name='VK_POLYGON_MODE_POINT', value='2'),
VarDef(name='VK_POLYGON_MODE_FILL_RECTANGLE_NV', value='1000153000'),
VarDef(name='VK_POLYGON_MODE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPresentGravityFlagBitsEXT', enumerators = [
VarDef(name='VK_PRESENT_GRAVITY_MIN_BIT_EXT', value='1'),
VarDef(name='VK_PRESENT_GRAVITY_MAX_BIT_EXT', value='2'),
VarDef(name='VK_PRESENT_GRAVITY_CENTERED_BIT_EXT', value='4'),
VarDef(name='VK_PRESENT_GRAVITY_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkPresentModeKHR', enumerators = [
VarDef(name='VK_PRESENT_MODE_IMMEDIATE_KHR', value='0'),
VarDef(name='VK_PRESENT_MODE_MAILBOX_KHR', value='1'),
VarDef(name='VK_PRESENT_MODE_FIFO_KHR', value='2'),
VarDef(name='VK_PRESENT_MODE_FIFO_RELAXED_KHR', value='3'),
VarDef(name='VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR', value='1000111001'),
VarDef(name='VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR', value='1000111000'),
VarDef(name='VK_PRESENT_MODE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkPresentScalingFlagBitsEXT', enumerators = [
VarDef(name='VK_PRESENT_SCALING_ONE_TO_ONE_BIT_EXT', value='1'),
VarDef(name='VK_PRESENT_SCALING_ASPECT_RATIO_STRETCH_BIT_EXT', value='2'),
VarDef(name='VK_PRESENT_SCALING_STRETCH_BIT_EXT', value='4'),
VarDef(name='VK_PRESENT_SCALING_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkPrimitiveTopology', enumerators = [
VarDef(name='VK_PRIMITIVE_TOPOLOGY_POINT_LIST', value='0'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_LINE_LIST', value='1'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_LINE_STRIP', value='2'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST', value='3'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP', value='4'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN', value='5'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY', value='6'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY', value='7'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY', value='8'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY', value='9'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_PATCH_LIST', value='10'),
VarDef(name='VK_PRIMITIVE_TOPOLOGY_MAX_ENUM', value='2147483647'),
])

Enum(name='VkPrivateDataSlotCreateFlagBits', enumerators = [
VarDef(name='VK_PRIVATE_DATA_SLOT_CREATE_RESERVED_0_BIT_NV', value='1'),
VarDef(name='VK_PRIVATE_DATA_SLOT_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkProvokingVertexModeEXT', enumerators = [
VarDef(name='VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT', value='0'),
VarDef(name='VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT', value='1'),
VarDef(name='VK_PROVOKING_VERTEX_MODE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkQueryControlFlagBits', enumerators = [
VarDef(name='VK_QUERY_CONTROL_PRECISE_BIT', value='1'),
VarDef(name='VK_QUERY_CONTROL_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkQueryPipelineStatisticFlagBits', enumerators = [
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT', value='1'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT', value='2'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT', value='4'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT', value='8'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT', value='16'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT', value='32'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT', value='64'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT', value='128'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT', value='256'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT', value='512'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT', value='1024'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_CLUSTER_CULLING_SHADER_INVOCATIONS_BIT_HUAWEI', value='8192'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT', value='4096'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT', value='2048'),
VarDef(name='VK_QUERY_PIPELINE_STATISTIC_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkQueryPoolSamplingModeINTEL', enumerators = [
VarDef(name='VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL', value='0'),
VarDef(name='VK_QUERY_POOL_SAMPLING_MODE_MAX_ENUM_INTEL', value='2147483647'),
])

Enum(name='VkQueryResultFlagBits', enumerators = [
VarDef(name='VK_QUERY_RESULT_64_BIT', value='1'),
VarDef(name='VK_QUERY_RESULT_WAIT_BIT', value='2'),
VarDef(name='VK_QUERY_RESULT_WITH_AVAILABILITY_BIT', value='4'),
VarDef(name='VK_QUERY_RESULT_PARTIAL_BIT', value='8'),
VarDef(name='VK_QUERY_RESULT_WITH_STATUS_BIT_KHR', value='16'),
VarDef(name='VK_QUERY_RESULT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkQueryResultStatusKHR', enumerators = [
VarDef(name='VK_QUERY_RESULT_STATUS_ERROR_KHR', value='-1'),
VarDef(name='VK_QUERY_RESULT_STATUS_NOT_READY_KHR', value='0'),
VarDef(name='VK_QUERY_RESULT_STATUS_COMPLETE_KHR', value='1'),
VarDef(name='VK_QUERY_RESULT_STATUS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkQueryType', enumerators = [
VarDef(name='VK_QUERY_TYPE_OCCLUSION', value='0'),
VarDef(name='VK_QUERY_TYPE_PIPELINE_STATISTICS', value='1'),
VarDef(name='VK_QUERY_TYPE_TIMESTAMP', value='2'),
VarDef(name='VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR', value='1000150000'),
VarDef(name='VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV', value='1000165000'),
VarDef(name='VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR', value='1000386000'),
VarDef(name='VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR', value='1000150001'),
VarDef(name='VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR', value='1000386001'),
VarDef(name='VK_QUERY_TYPE_MESH_PRIMITIVES_GENERATED_EXT', value='1000328000'),
VarDef(name='VK_QUERY_TYPE_MICROMAP_COMPACTED_SIZE_EXT', value='1000396001'),
VarDef(name='VK_QUERY_TYPE_MICROMAP_SERIALIZATION_SIZE_EXT', value='1000396000'),
VarDef(name='VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL', value='1000210000'),
VarDef(name='VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR', value='1000116000'),
VarDef(name='VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT', value='1000382000'),
VarDef(name='VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR', value='1000023000'),
VarDef(name='VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT', value='1000028004'),
VarDef(name='VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR', value='1000299000'),
VarDef(name='VK_QUERY_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkQueueFlagBits', enumerators = [
VarDef(name='VK_QUEUE_GRAPHICS_BIT', value='1'),
VarDef(name='VK_QUEUE_COMPUTE_BIT', value='2'),
VarDef(name='VK_QUEUE_TRANSFER_BIT', value='4'),
VarDef(name='VK_QUEUE_SPARSE_BINDING_BIT', value='8'),
VarDef(name='VK_QUEUE_OPTICAL_FLOW_BIT_NV', value='256'),
VarDef(name='VK_QUEUE_PROTECTED_BIT', value='16'),
VarDef(name='VK_QUEUE_RESERVED_10_BIT_EXT', value='1024'),
VarDef(name='VK_QUEUE_RESERVED_11_BIT_ARM', value='2048'),
VarDef(name='VK_QUEUE_RESERVED_7_BIT_QCOM', value='128'),
VarDef(name='VK_QUEUE_RESERVED_9_BIT_EXT', value='512'),
VarDef(name='VK_QUEUE_VIDEO_DECODE_BIT_KHR', value='32'),
VarDef(name='VK_QUEUE_VIDEO_ENCODE_BIT_KHR', value='64'),
VarDef(name='VK_QUEUE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkQueueGlobalPriorityKHR', enumerators = [
VarDef(name='VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR', value='128'),
VarDef(name='VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR', value='256'),
VarDef(name='VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR', value='512'),
VarDef(name='VK_QUEUE_GLOBAL_PRIORITY_REALTIME_KHR', value='1024'),
VarDef(name='VK_QUEUE_GLOBAL_PRIORITY_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkRasterizationOrderAMD', enumerators = [
VarDef(name='VK_RASTERIZATION_ORDER_STRICT_AMD', value='0'),
VarDef(name='VK_RASTERIZATION_ORDER_RELAXED_AMD', value='1'),
VarDef(name='VK_RASTERIZATION_ORDER_MAX_ENUM_AMD', value='2147483647'),
])

Enum(name='VkRayTracingInvocationReorderModeNV', enumerators = [
VarDef(name='VK_RAY_TRACING_INVOCATION_REORDER_MODE_NONE_NV', value='0'),
VarDef(name='VK_RAY_TRACING_INVOCATION_REORDER_MODE_REORDER_NV', value='1'),
VarDef(name='VK_RAY_TRACING_INVOCATION_REORDER_MODE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkRayTracingShaderGroupTypeKHR', enumerators = [
VarDef(name='VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR', value='0'),
VarDef(name='VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR', value='1'),
VarDef(name='VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR', value='2'),
VarDef(name='VK_RAY_TRACING_SHADER_GROUP_TYPE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkRefreshObjectFlagBitsKHR', enumerators = [
VarDef(name='VK_REFRESH_OBJECT_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkRenderPassCreateFlagBits', enumerators = [
VarDef(name='VK_RENDER_PASS_CREATE_RESERVED_0_BIT_KHR', value='1'),
VarDef(name='VK_RENDER_PASS_CREATE_TRANSFORM_BIT_QCOM', value='2'),
VarDef(name='VK_RENDER_PASS_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkRenderingFlagBits', enumerators = [
VarDef(name='VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT', value='1'),
VarDef(name='VK_RENDERING_SUSPENDING_BIT', value='2'),
VarDef(name='VK_RENDERING_RESUMING_BIT', value='4'),
VarDef(name='VK_RENDERING_ENABLE_LEGACY_DITHERING_BIT_EXT', value='8'),
VarDef(name='VK_RENDERING_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkResolveModeFlagBits', enumerators = [
VarDef(name='VK_RESOLVE_MODE_NONE', value='0'),
VarDef(name='VK_RESOLVE_MODE_SAMPLE_ZERO_BIT', value='1'),
VarDef(name='VK_RESOLVE_MODE_AVERAGE_BIT', value='2'),
VarDef(name='VK_RESOLVE_MODE_MIN_BIT', value='4'),
VarDef(name='VK_RESOLVE_MODE_MAX_BIT', value='8'),
VarDef(name='VK_RESOLVE_MODE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkResult', enumerators = [
VarDef(name='VK_SUCCESS', value='0'),
VarDef(name='VK_NOT_READY', value='1'),
VarDef(name='VK_TIMEOUT', value='2'),
VarDef(name='VK_EVENT_SET', value='3'),
VarDef(name='VK_EVENT_RESET', value='4'),
VarDef(name='VK_INCOMPLETE', value='5'),
VarDef(name='VK_ERROR_OUT_OF_HOST_MEMORY', value='-1'),
VarDef(name='VK_ERROR_OUT_OF_DEVICE_MEMORY', value='-2'),
VarDef(name='VK_ERROR_INITIALIZATION_FAILED', value='-3'),
VarDef(name='VK_ERROR_DEVICE_LOST', value='-4'),
VarDef(name='VK_ERROR_MEMORY_MAP_FAILED', value='-5'),
VarDef(name='VK_ERROR_LAYER_NOT_PRESENT', value='-6'),
VarDef(name='VK_ERROR_EXTENSION_NOT_PRESENT', value='-7'),
VarDef(name='VK_ERROR_FEATURE_NOT_PRESENT', value='-8'),
VarDef(name='VK_ERROR_INCOMPATIBLE_DRIVER', value='-9'),
VarDef(name='VK_ERROR_TOO_MANY_OBJECTS', value='-10'),
VarDef(name='VK_ERROR_FORMAT_NOT_SUPPORTED', value='-11'),
VarDef(name='VK_ERROR_FRAGMENTED_POOL', value='-12'),
VarDef(name='VK_ERROR_UNKNOWN', value='-13'),
VarDef(name='VK_ERROR_COMPRESSION_EXHAUSTED_EXT', value='-1000338000'),
VarDef(name='VK_ERROR_FRAGMENTATION', value='-1000161000'),
VarDef(name='VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT', value='-1000255000'),
VarDef(name='VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR', value='-1000023000'),
VarDef(name='VK_ERROR_INCOMPATIBLE_DISPLAY_KHR', value='-1000003001'),
VarDef(name='VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT', value='1000482000'),
VarDef(name='VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT', value='-1000158000'),
VarDef(name='VK_ERROR_INVALID_EXTERNAL_HANDLE', value='-1000072003'),
VarDef(name='VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS', value='-1000257000'),
VarDef(name='VK_ERROR_INVALID_PIPELINE_CACHE_DATA', value='-1000298000'),
VarDef(name='VK_ERROR_INVALID_SHADER_NV', value='-1000012000'),
VarDef(name='VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR', value='-1000299000'),
VarDef(name='VK_ERROR_NATIVE_WINDOW_IN_USE_KHR', value='-1000000001'),
VarDef(name='VK_ERROR_NOT_PERMITTED_KHR', value='-1000174001'),
VarDef(name='VK_ERROR_NO_PIPELINE_MATCH', value='-1000298001'),
VarDef(name='VK_ERROR_OUT_OF_DATE_KHR', value='-1000001004'),
VarDef(name='VK_ERROR_OUT_OF_POOL_MEMORY', value='-1000069000'),
VarDef(name='VK_ERROR_SURFACE_LOST_KHR', value='-1000000000'),
VarDef(name='VK_ERROR_VALIDATION_FAILED', value='-1000011001'),
VarDef(name='VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR', value='-1000023001'),
VarDef(name='VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR', value='-1000023004'),
VarDef(name='VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR', value='-1000023003'),
VarDef(name='VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR', value='-1000023002'),
VarDef(name='VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR', value='-1000023005'),
VarDef(name='VK_OPERATION_DEFERRED_KHR', value='1000268002'),
VarDef(name='VK_OPERATION_NOT_DEFERRED_KHR', value='1000268003'),
VarDef(name='VK_PIPELINE_COMPILE_REQUIRED', value='1000297000'),
VarDef(name='VK_SUBOPTIMAL_KHR', value='1000001003'),
VarDef(name='VK_THREAD_DONE_KHR', value='1000268001'),
VarDef(name='VK_THREAD_IDLE_KHR', value='1000268000'),
VarDef(name='VK_RESULT_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSampleCountFlagBits', enumerators = [
VarDef(name='VK_SAMPLE_COUNT_1_BIT', value='1'),
VarDef(name='VK_SAMPLE_COUNT_2_BIT', value='2'),
VarDef(name='VK_SAMPLE_COUNT_4_BIT', value='4'),
VarDef(name='VK_SAMPLE_COUNT_8_BIT', value='8'),
VarDef(name='VK_SAMPLE_COUNT_16_BIT', value='16'),
VarDef(name='VK_SAMPLE_COUNT_32_BIT', value='32'),
VarDef(name='VK_SAMPLE_COUNT_64_BIT', value='64'),
VarDef(name='VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSamplerAddressMode', enumerators = [
VarDef(name='VK_SAMPLER_ADDRESS_MODE_REPEAT', value='0'),
VarDef(name='VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT', value='1'),
VarDef(name='VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE', value='2'),
VarDef(name='VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER', value='3'),
VarDef(name='VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE', value='4'),
VarDef(name='VK_SAMPLER_ADDRESS_MODE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSamplerCreateFlagBits', enumerators = [
VarDef(name='VK_SAMPLER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT', value='8'),
VarDef(name='VK_SAMPLER_CREATE_IMAGE_PROCESSING_BIT_QCOM', value='16'),
VarDef(name='VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT', value='4'),
VarDef(name='VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT', value='1'),
VarDef(name='VK_SAMPLER_CREATE_SUBSAMPLED_COARSE_RECONSTRUCTION_BIT_EXT', value='2'),
VarDef(name='VK_SAMPLER_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSamplerMipmapMode', enumerators = [
VarDef(name='VK_SAMPLER_MIPMAP_MODE_NEAREST', value='0'),
VarDef(name='VK_SAMPLER_MIPMAP_MODE_LINEAR', value='1'),
VarDef(name='VK_SAMPLER_MIPMAP_MODE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSamplerReductionMode', enumerators = [
VarDef(name='VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE', value='0'),
VarDef(name='VK_SAMPLER_REDUCTION_MODE_MIN', value='1'),
VarDef(name='VK_SAMPLER_REDUCTION_MODE_MAX', value='2'),
VarDef(name='VK_SAMPLER_REDUCTION_MODE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSamplerYcbcrModelConversion', enumerators = [
VarDef(name='VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY', value='0'),
VarDef(name='VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY', value='1'),
VarDef(name='VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709', value='2'),
VarDef(name='VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601', value='3'),
VarDef(name='VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020', value='4'),
VarDef(name='VK_SAMPLER_YCBCR_MODEL_CONVERSION_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSamplerYcbcrRange', enumerators = [
VarDef(name='VK_SAMPLER_YCBCR_RANGE_ITU_FULL', value='0'),
VarDef(name='VK_SAMPLER_YCBCR_RANGE_ITU_NARROW', value='1'),
VarDef(name='VK_SAMPLER_YCBCR_RANGE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkScopeNV', enumerators = [
VarDef(name='VK_SCOPE_DEVICE_NV', value='1'),
VarDef(name='VK_SCOPE_WORKGROUP_NV', value='2'),
VarDef(name='VK_SCOPE_SUBGROUP_NV', value='3'),
VarDef(name='VK_SCOPE_QUEUE_FAMILY_NV', value='5'),
VarDef(name='VK_SCOPE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkSemaphoreCreateFlagBits', enumerators = [
VarDef(name='VK_SEMAPHORE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSemaphoreImportFlagBits', enumerators = [
VarDef(name='VK_SEMAPHORE_IMPORT_TEMPORARY_BIT', value='1'),
VarDef(name='VK_SEMAPHORE_IMPORT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSemaphoreType', enumerators = [
VarDef(name='VK_SEMAPHORE_TYPE_BINARY', value='0'),
VarDef(name='VK_SEMAPHORE_TYPE_TIMELINE', value='1'),
VarDef(name='VK_SEMAPHORE_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSemaphoreWaitFlagBits', enumerators = [
VarDef(name='VK_SEMAPHORE_WAIT_ANY_BIT', value='1'),
VarDef(name='VK_SEMAPHORE_WAIT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkShaderCodeTypeEXT', enumerators = [
VarDef(name='VK_SHADER_CODE_TYPE_BINARY_EXT', value='0'),
VarDef(name='VK_SHADER_CODE_TYPE_SPIRV_EXT', value='1'),
VarDef(name='VK_SHADER_CODE_TYPE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkShaderCorePropertiesFlagBitsAMD', enumerators = [
VarDef(name='VK_SHADER_CORE_PROPERTIES_FLAG_BITS_MAX_ENUM_AMD', value='2147483647'),
])

Enum(name='VkShaderCreateFlagBitsEXT', enumerators = [
VarDef(name='VK_SHADER_CREATE_LINK_STAGE_BIT_EXT', value='1'),
VarDef(name='VK_SHADER_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT', value='2'),
VarDef(name='VK_SHADER_CREATE_DISPATCH_BASE_BIT_EXT', value='16'),
VarDef(name='VK_SHADER_CREATE_FRAGMENT_DENSITY_MAP_ATTACHMENT_BIT_EXT', value='64'),
VarDef(name='VK_SHADER_CREATE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_EXT', value='32'),
VarDef(name='VK_SHADER_CREATE_NO_TASK_SHADER_BIT_EXT', value='8'),
VarDef(name='VK_SHADER_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT', value='4'),
VarDef(name='VK_SHADER_CREATE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkShaderFloatControlsIndependence', enumerators = [
VarDef(name='VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY', value='0'),
VarDef(name='VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL', value='1'),
VarDef(name='VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE', value='2'),
VarDef(name='VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkShaderGroupShaderKHR', enumerators = [
VarDef(name='VK_SHADER_GROUP_SHADER_GENERAL_KHR', value='0'),
VarDef(name='VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR', value='1'),
VarDef(name='VK_SHADER_GROUP_SHADER_ANY_HIT_KHR', value='2'),
VarDef(name='VK_SHADER_GROUP_SHADER_INTERSECTION_KHR', value='3'),
VarDef(name='VK_SHADER_GROUP_SHADER_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkShaderInfoTypeAMD', enumerators = [
VarDef(name='VK_SHADER_INFO_TYPE_STATISTICS_AMD', value='0'),
VarDef(name='VK_SHADER_INFO_TYPE_BINARY_AMD', value='1'),
VarDef(name='VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD', value='2'),
VarDef(name='VK_SHADER_INFO_TYPE_MAX_ENUM_AMD', value='2147483647'),
])

Enum(name='VkShaderModuleCreateFlagBits', enumerators = [
VarDef(name='VK_SHADER_MODULE_CREATE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkShaderStageFlagBits', enumerators = [
VarDef(name='VK_SHADER_STAGE_VERTEX_BIT', value='1'),
VarDef(name='VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT', value='2'),
VarDef(name='VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT', value='4'),
VarDef(name='VK_SHADER_STAGE_GEOMETRY_BIT', value='8'),
VarDef(name='VK_SHADER_STAGE_FRAGMENT_BIT', value='16'),
VarDef(name='VK_SHADER_STAGE_COMPUTE_BIT', value='32'),
VarDef(name='VK_SHADER_STAGE_ALL_GRAPHICS', value='31'),
VarDef(name='VK_SHADER_STAGE_ALL', value='2147483647'),
VarDef(name='VK_SHADER_STAGE_ANY_HIT_BIT_KHR', value='512'),
VarDef(name='VK_SHADER_STAGE_CALLABLE_BIT_KHR', value='8192'),
VarDef(name='VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR', value='1024'),
VarDef(name='VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI', value='524288'),
VarDef(name='VK_SHADER_STAGE_INTERSECTION_BIT_KHR', value='4096'),
VarDef(name='VK_SHADER_STAGE_MESH_BIT_EXT', value='128'),
VarDef(name='VK_SHADER_STAGE_MISS_BIT_KHR', value='2048'),
VarDef(name='VK_SHADER_STAGE_RAYGEN_BIT_KHR', value='256'),
VarDef(name='VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI', value='16384'),
VarDef(name='VK_SHADER_STAGE_TASK_BIT_EXT', value='64'),
])

Enum(name='VkShadingRatePaletteEntryNV', enumerators = [
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV', value='0'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_16_INVOCATIONS_PER_PIXEL_NV', value='1'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_8_INVOCATIONS_PER_PIXEL_NV', value='2'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_4_INVOCATIONS_PER_PIXEL_NV', value='3'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_2_INVOCATIONS_PER_PIXEL_NV', value='4'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV', value='5'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV', value='6'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV', value='7'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV', value='8'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV', value='9'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV', value='10'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV', value='11'),
VarDef(name='VK_SHADING_RATE_PALETTE_ENTRY_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkSharingMode', enumerators = [
VarDef(name='VK_SHARING_MODE_EXCLUSIVE', value='0'),
VarDef(name='VK_SHARING_MODE_CONCURRENT', value='1'),
VarDef(name='VK_SHARING_MODE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSparseImageFormatFlagBits', enumerators = [
VarDef(name='VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT', value='1'),
VarDef(name='VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT', value='2'),
VarDef(name='VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT', value='4'),
VarDef(name='VK_SPARSE_IMAGE_FORMAT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSparseMemoryBindFlagBits', enumerators = [
VarDef(name='VK_SPARSE_MEMORY_BIND_METADATA_BIT', value='1'),
VarDef(name='VK_SPARSE_MEMORY_BIND_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkStencilFaceFlagBits', enumerators = [
VarDef(name='VK_STENCIL_FACE_FRONT_BIT', value='1'),
VarDef(name='VK_STENCIL_FACE_BACK_BIT', value='2'),
VarDef(name='VK_STENCIL_FACE_FRONT_AND_BACK', value='3'),
VarDef(name='VK_STENCIL_FACE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkStencilOp', enumerators = [
VarDef(name='VK_STENCIL_OP_KEEP', value='0'),
VarDef(name='VK_STENCIL_OP_ZERO', value='1'),
VarDef(name='VK_STENCIL_OP_REPLACE', value='2'),
VarDef(name='VK_STENCIL_OP_INCREMENT_AND_CLAMP', value='3'),
VarDef(name='VK_STENCIL_OP_DECREMENT_AND_CLAMP', value='4'),
VarDef(name='VK_STENCIL_OP_INVERT', value='5'),
VarDef(name='VK_STENCIL_OP_INCREMENT_AND_WRAP', value='6'),
VarDef(name='VK_STENCIL_OP_DECREMENT_AND_WRAP', value='7'),
VarDef(name='VK_STENCIL_OP_MAX_ENUM', value='2147483647'),
])

Enum(name='VkStructureType', enumerators = [
VarDef(name='VK_STRUCTURE_TYPE_APPLICATION_INFO', value='0'),
VarDef(name='VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO', value='1'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO', value='2'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO', value='3'),
VarDef(name='VK_STRUCTURE_TYPE_SUBMIT_INFO', value='4'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO', value='5'),
VarDef(name='VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE', value='6'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_SPARSE_INFO', value='7'),
VarDef(name='VK_STRUCTURE_TYPE_FENCE_CREATE_INFO', value='8'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO', value='9'),
VarDef(name='VK_STRUCTURE_TYPE_EVENT_CREATE_INFO', value='10'),
VarDef(name='VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO', value='11'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO', value='12'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO', value='13'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO', value='14'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO', value='15'),
VarDef(name='VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO', value='16'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO', value='17'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO', value='18'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO', value='19'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO', value='20'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO', value='21'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO', value='22'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO', value='23'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO', value='24'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO', value='25'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO', value='26'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO', value='27'),
VarDef(name='VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO', value='28'),
VarDef(name='VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO', value='29'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO', value='30'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO', value='31'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO', value='32'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO', value='33'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO', value='34'),
VarDef(name='VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET', value='35'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET', value='36'),
VarDef(name='VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO', value='37'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO', value='38'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO', value='39'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO', value='40'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO', value='41'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO', value='42'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO', value='43'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER', value='44'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER', value='45'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_BARRIER', value='46'),
VarDef(name='VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO', value='47'),
VarDef(name='VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO', value='48'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR', value='1000150000'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR', value='1000150020'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CAPTURE_DESCRIPTOR_DATA_INFO_EXT', value='1000316009'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR', value='1000150017'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV', value='1000165001'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR', value='1000150002'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR', value='1000150003'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR', value='1000150004'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR', value='1000150006'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MOTION_TRIANGLES_DATA_NV', value='1000327000'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR', value='1000150005'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV', value='1000165012'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV', value='1000165008'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MOTION_INFO_NV', value='1000327002'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_DISPLACEMENT_MICROMAP_NV', value='1000397002'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT', value='1000396009'),
VarDef(name='VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_VERSION_INFO_KHR', value='1000150009'),
VarDef(name='VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR', value='1000060010'),
VarDef(name='VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR', value='1000116004'),
VarDef(name='VK_STRUCTURE_TYPE_AMIGO_PROFILING_SUBMIT_INFO_SEC', value='1000485001'),
VarDef(name='VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_2_ANDROID', value='1000129006'),
VarDef(name='VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID', value='1000129002'),
VarDef(name='VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID', value='1000129001'),
VarDef(name='VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID', value='1000129000'),
VarDef(name='VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR', value='1000008000'),
VarDef(name='VK_STRUCTURE_TYPE_APPLICATION_PARAMETERS_EXT', value='1000435000'),
VarDef(name='VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2', value='1000109000'),
VarDef(name='VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT', value='1000241002'),
VarDef(name='VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2', value='1000109001'),
VarDef(name='VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT', value='1000241001'),
VarDef(name='VK_STRUCTURE_TYPE_ATTACHMENT_SAMPLE_COUNT_INFO_AMD', value='1000044008'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV', value='1000165006'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO', value='1000060013'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO', value='1000157000'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO', value='1000060014'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO', value='1000157001'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR', value='1000060009'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO', value='1000156002'),
VarDef(name='VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR', value='1000023004'),
VarDef(name='VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2', value='1000337004'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_CAPTURE_DESCRIPTOR_DATA_INFO_EXT', value='1000316005'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_COLLECTION_BUFFER_CREATE_INFO_FUCHSIA', value='1000366005'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CONSTRAINTS_INFO_FUCHSIA', value='1000366009'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CREATE_INFO_FUCHSIA', value='1000366000'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_COLLECTION_IMAGE_CREATE_INFO_FUCHSIA', value='1000366002'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_COLLECTION_PROPERTIES_FUCHSIA', value='1000366003'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_CONSTRAINTS_INFO_FUCHSIA', value='1000366004'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_COPY_2', value='1000337006'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT', value='1000244002'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO', value='1000244001'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2', value='1000337009'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2', value='1000314001'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2', value='1000146000'),
VarDef(name='VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO', value='1000257002'),
VarDef(name='VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT', value='1000184000'),
VarDef(name='VK_STRUCTURE_TYPE_CHECKPOINT_DATA_2_NV', value='1000314009'),
VarDef(name='VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV', value='1000206000'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT', value='1000081000'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO', value='1000044004'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDER_PASS_TRANSFORM_INFO_QCOM', value='1000282000'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV', value='1000278001'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO', value='1000314006'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_POOL_MEMORY_CONSUMPTION', value='1000298004'),
VarDef(name='VK_STRUCTURE_TYPE_COMMAND_POOL_MEMORY_RESERVATION_CREATE_INFO', value='1000298003'),
VarDef(name='VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT', value='1000081002'),
VarDef(name='VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV', value='1000249001'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR', value='1000150010'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR', value='1000150011'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2', value='1000337000'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2', value='1000337002'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_COMMAND_TRANSFORM_INFO_QCOM', value='1000333000'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2', value='1000337001'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2', value='1000337003'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR', value='1000150012'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_MEMORY_TO_MICROMAP_INFO_EXT', value='1000396004'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_MICROMAP_INFO_EXT', value='1000396002'),
VarDef(name='VK_STRUCTURE_TYPE_COPY_MICROMAP_TO_MEMORY_INFO_EXT', value='1000396003'),
VarDef(name='VK_STRUCTURE_TYPE_CU_FUNCTION_CREATE_INFO_NVX', value='1000029001'),
VarDef(name='VK_STRUCTURE_TYPE_CU_LAUNCH_INFO_NVX', value='1000029002'),
VarDef(name='VK_STRUCTURE_TYPE_CU_MODULE_CREATE_INFO_NVX', value='1000029000'),
VarDef(name='VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR', value='1000078002'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT', value='1000022002'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT', value='1000022000'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT', value='1000022001'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT', value='1000011000'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT', value='1000128002'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT', value='1000128003'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT', value='1000128004'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT', value='1000128000'),
VarDef(name='VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT', value='1000128001'),
VarDef(name='VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV', value='1000026001'),
VarDef(name='VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV', value='1000026000'),
VarDef(name='VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV', value='1000026002'),
VarDef(name='VK_STRUCTURE_TYPE_DEPENDENCY_INFO', value='1000314003'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT', value='1000316003'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT', value='1000316011'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_PUSH_DESCRIPTOR_BUFFER_HANDLE_EXT', value='1000316012'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT', value='1000316004'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO', value='1000138003'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_BINDING_REFERENCE_VALVE', value='1000420001'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO', value='1000161000'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_HOST_MAPPING_INFO_VALVE', value='1000420002'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT', value='1000168001'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO', value='1000161003'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT', value='1000161004'),
VarDef(name='VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO', value='1000085000'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_ADDRESS_BINDING_CALLBACK_DATA_EXT', value='1000354001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS', value='1000413002'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT', value='1000284001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV', value='1000300001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_EVENT_INFO_EXT', value='1000091001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT', value='1000341001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT', value='1000341002'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO', value='1000060006'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO', value='1000060004'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO', value='1000070001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR', value='1000060007'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR', value='1000060011'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO', value='1000060003'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO', value='1000060005'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR', value='1000060012'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS', value='1000413003'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO', value='1000257004'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD', value='1000189000'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_MEMORY_REPORT_CALLBACK_DATA_EXT', value='1000284002'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_OBJECT_RESERVATION_CREATE_INFO', value='1000298002'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO', value='1000295001'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR', value='1000174000'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2', value='1000145003'),
VarDef(name='VK_STRUCTURE_TYPE_DEVICE_SEMAPHORE_SCI_SYNC_POOL_RESERVATION_CREATE_INFO_NV', value='1000489003'),
VarDef(name='VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT', value='1000346000'),
VarDef(name='VK_STRUCTURE_TYPE_DIRECT_DRIVER_LOADING_INFO_LUNARG', value='1000459000'),
VarDef(name='VK_STRUCTURE_TYPE_DIRECT_DRIVER_LOADING_LIST_LUNARG', value='1000459001'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT', value='1000091002'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR', value='1000002000'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR', value='1000121002'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD', value='1000213000'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_PLANE_CAPABILITIES_2_KHR', value='1000121004'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_PLANE_INFO_2_KHR', value='1000121003'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_PLANE_PROPERTIES_2_KHR', value='1000121001'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT', value='1000091000'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR', value='1000003000'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR', value='1000121000'),
VarDef(name='VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR', value='1000002001'),
VarDef(name='VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT', value='1000158006'),
VarDef(name='VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT', value='1000158000'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO', value='1000113000'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_FENCE_SCI_SYNC_INFO_NV', value='1000373001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR', value='1000114001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO', value='1000072002'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV', value='1000056001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_MEMORY_SCI_BUF_INFO_NV', value='1000374001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR', value='1000073001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV', value='1000057001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_BUFFER_INFO_EXT', value='1000311004'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_COMMAND_QUEUE_INFO_EXT', value='1000311003'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_DEVICE_INFO_EXT', value='1000311002'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_IO_SURFACE_INFO_EXT', value='1000311008'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT', value='1000311001'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECT_CREATE_INFO_EXT', value='1000311000'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_SHARED_EVENT_INFO_EXT', value='1000311010'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_METAL_TEXTURE_INFO_EXT', value='1000311006'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO', value='1000077000'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_SCI_SYNC_INFO_NV', value='1000373005'),
VarDef(name='VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR', value='1000078001'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES', value='1000071003'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES', value='1000112001'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID', value='1000129005'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES', value='1000071001'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO', value='1000072000'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO', value='1000072001'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV', value='1000056000'),
VarDef(name='VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES', value='1000076001'),
VarDef(name='VK_STRUCTURE_TYPE_FAULT_CALLBACK_INFO', value='1000298008'),
VarDef(name='VK_STRUCTURE_TYPE_FAULT_DATA', value='1000298007'),
VarDef(name='VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR', value='1000115001'),
VarDef(name='VK_STRUCTURE_TYPE_FENCE_GET_SCI_SYNC_INFO_NV', value='1000373002'),
VarDef(name='VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR', value='1000114002'),
VarDef(name='VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT', value='1000170001'),
VarDef(name='VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2', value='1000059002'),
VarDef(name='VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3', value='1000360000'),
VarDef(name='VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR', value='1000226000'),
VarDef(name='VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO', value='1000108001'),
VarDef(name='VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO', value='1000108002'),
VarDef(name='VK_STRUCTURE_TYPE_FRAMEBUFFER_MIXED_SAMPLES_COMBINATION_NV', value='1000250002'),
VarDef(name='VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_NV', value='1000277005'),
VarDef(name='VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_NV', value='1000277006'),
VarDef(name='VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV', value='1000165005'),
VarDef(name='VK_STRUCTURE_TYPE_GEOMETRY_NV', value='1000165003'),
VarDef(name='VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV', value='1000165004'),
VarDef(name='VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT', value='1000320002'),
VarDef(name='VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_SHADER_GROUPS_CREATE_INFO_NV', value='1000277002'),
VarDef(name='VK_STRUCTURE_TYPE_GRAPHICS_SHADER_GROUP_CREATE_INFO_NV', value='1000277001'),
VarDef(name='VK_STRUCTURE_TYPE_HDR_METADATA_EXT', value='1000105000'),
VarDef(name='VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT', value='1000256000'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGEPIPE_SURFACE_CREATE_INFO_FUCHSIA', value='1000214000'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_BLIT_2', value='1000337008'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_CAPTURE_DESCRIPTOR_DATA_INFO_EXT', value='1000316006'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_CONTROL_EXT', value='1000338001'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT', value='1000338004'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_CONSTRAINTS_INFO_FUCHSIA', value='1000366006'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_COPY_2', value='1000337007'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT', value='1000158004'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT', value='1000158003'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT', value='1000158005'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_FORMAT_CONSTRAINTS_INFO_FUCHSIA', value='1000366007'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO', value='1000147000'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2', value='1000059003'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2', value='1000314002'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2', value='1000146001'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO', value='1000156003'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2', value='1000337010'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2', value='1000146002'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO', value='1000246000'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_SUBRESOURCE_2_EXT', value='1000338003'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR', value='1000060008'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_ADDRESS_PROPERTIES_NVX', value='1000030001'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT', value='1000067000'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_CAPTURE_DESCRIPTOR_DATA_INFO_EXT', value='1000316007'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_HANDLE_INFO_NVX', value='1000030000'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT', value='1000391001'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_SAMPLE_WEIGHT_CREATE_INFO_QCOM', value='1000440002'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT', value='1000418001'),
VarDef(name='VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO', value='1000117002'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID', value='1000129003'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR', value='1000115000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_FENCE_SCI_SYNC_INFO_NV', value='1000373000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR', value='1000114000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_BUFFER_COLLECTION_FUCHSIA', value='1000366001'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR', value='1000074000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT', value='1000178000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_SCI_BUF_INFO_NV', value='1000374000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR', value='1000073000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV', value='1000057000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA', value='1000364000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_METAL_BUFFER_INFO_EXT', value='1000311005'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_METAL_IO_SURFACE_INFO_EXT', value='1000311009'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_METAL_SHARED_EVENT_INFO_EXT', value='1000311011'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_METAL_TEXTURE_INFO_EXT', value='1000311007'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR', value='1000079000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_SCI_SYNC_INFO_NV', value='1000373004'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR', value='1000078000'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_ZIRCON_HANDLE_INFO_FUCHSIA', value='1000365000'),
VarDef(name='VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NV', value='1000277004'),
VarDef(name='VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV', value='1000277003'),
VarDef(name='VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL', value='1000210001'),
VarDef(name='VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK', value='1000122000'),
VarDef(name='VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK', value='1000123000'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO', value='1000060000'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_BARRIER_2', value='1000314000'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO', value='1000127001'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS', value='1000127000'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR', value='1000074001'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID', value='1000129004'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR', value='1000074002'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_GET_REMOTE_ADDRESS_INFO_NV', value='1000371000'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_GET_SCI_BUF_INFO_NV', value='1000374002'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR', value='1000073003'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_GET_ZIRCON_HANDLE_INFO_FUCHSIA', value='1000364002'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT', value='1000178001'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR', value='1000271000'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO', value='1000257003'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT', value='1000238001'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2', value='1000146003'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_SCI_BUF_PROPERTIES_NV', value='1000374003'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO_KHR', value='1000271001'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR', value='1000073002'),
VarDef(name='VK_STRUCTURE_TYPE_MEMORY_ZIRCON_HANDLE_PROPERTIES_FUCHSIA', value='1000364001'),
VarDef(name='VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT', value='1000217000'),
VarDef(name='VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT', value='1000396000'),
VarDef(name='VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT', value='1000396008'),
VarDef(name='VK_STRUCTURE_TYPE_MICROMAP_CREATE_INFO_EXT', value='1000396007'),
VarDef(name='VK_STRUCTURE_TYPE_MICROMAP_VERSION_INFO_EXT', value='1000396001'),
VarDef(name='VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT', value='1000376002'),
VarDef(name='VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT', value='1000143004'),
VarDef(name='VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_ATTRIBUTES_INFO_NVX', value='1000044009'),
VarDef(name='VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_RENDER_AREAS_RENDER_PASS_BEGIN_INFO_QCOM', value='1000510001'),
VarDef(name='VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT', value='1000351002'),
VarDef(name='VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID', value='1000010000'),
VarDef(name='VK_STRUCTURE_TYPE_OPAQUE_CAPTURE_DESCRIPTOR_DATA_CREATE_INFO_EXT', value='1000316010'),
VarDef(name='VK_STRUCTURE_TYPE_OPTICAL_FLOW_EXECUTE_INFO_NV', value='1000464005'),
VarDef(name='VK_STRUCTURE_TYPE_OPTICAL_FLOW_IMAGE_FORMAT_INFO_NV', value='1000464002'),
VarDef(name='VK_STRUCTURE_TYPE_OPTICAL_FLOW_IMAGE_FORMAT_PROPERTIES_NV', value='1000464003'),
VarDef(name='VK_STRUCTURE_TYPE_OPTICAL_FLOW_SESSION_CREATE_INFO_NV', value='1000464004'),
VarDef(name='VK_STRUCTURE_TYPE_OPTICAL_FLOW_SESSION_CREATE_PRIVATE_DATA_INFO_NV', value='1000464010'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL', value='1000210005'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR', value='1000116006'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR', value='1000116005'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_MARKER_INFO_INTEL', value='1000210002'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_OVERRIDE_INFO_INTEL', value='1000210004'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_RESERVATION_INFO_KHR', value='1000116007'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR', value='1000116003'),
VarDef(name='VK_STRUCTURE_TYPE_PERFORMANCE_STREAM_MARKER_INFO_INTEL', value='1000210003'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES', value='1000083000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT', value='1000340000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES', value='1000177000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR', value='1000150013'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR', value='1000150014'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ADDRESS_BINDING_REPORT_FEATURES_EXT', value='1000354000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_AMIGO_PROFILING_FEATURES_SEC', value='1000485000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT', value='1000067001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT', value='1000339000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT', value='1000148000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT', value='1000148001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT', value='1000411000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES', value='1000257000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT', value='1000244000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_FEATURES_HUAWEI', value='1000404000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_PROPERTIES_HUAWEI', value='1000404001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD', value='1000229000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT', value='1000381000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV', value='1000201000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT', value='1000081001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT', value='1000101000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV', value='1000249000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV', value='1000249002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_FEATURES_NV', value='1000426000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_PROPERTIES_NV', value='1000426001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV', value='1000050000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV', value='1000250000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT', value='1000287002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT', value='1000287001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV', value='1000240000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT', value='1000421000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT', value='1000355000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT', value='1000102000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES', value='1000199000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_DENSITY_MAP_PROPERTIES_EXT', value='1000316001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT', value='1000316002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT', value='1000316000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES', value='1000161001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES', value='1000161002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE', value='1000420000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV', value='1000277007'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV', value='1000277000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT', value='1000284000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV', value='1000300000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT', value='1000099000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISPLACEMENT_MICROMAP_FEATURES_NV', value='1000397000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISPLACEMENT_MICROMAP_PROPERTIES_NV', value='1000397001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES', value='1000196000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT', value='1000353000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES', value='1000044003'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV', value='1000205002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT', value='1000377000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT', value='1000455000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT', value='1000455001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT', value='1000267000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO', value='1000071002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO', value='1000112000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO', value='1000071000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT', value='1000178002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV', value='1000371001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_SCI_BUF_FEATURES_NV', value='1000374004'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SCI_SYNC_2_FEATURES_NV', value='1000489002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SCI_SYNC_FEATURES_NV', value='1000373007'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO', value='1000076000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT', value='1000341000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2', value='1000059000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES', value='1000197000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT', value='1000332000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT', value='1000332001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT', value='1000218000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM', value='1000425000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_QCOM', value='1000425001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT', value='1000218001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR', value='1000203000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR', value='1000322000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT', value='1000251000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV', value='1000326001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV', value='1000326000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR', value='1000226003'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR', value='1000226004'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR', value='1000226002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR', value='1000388000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT', value='1000320000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT', value='1000320001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES', value='1000070000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES', value='1000261000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES', value='1000071004'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES', value='1000108000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT', value='1000393000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT', value='1000338000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT', value='1000437000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT', value='1000158002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2', value='1000059004'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_FEATURES_QCOM', value='1000440000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_PROPERTIES_QCOM', value='1000440001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES', value='1000335000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT', value='1000418000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT', value='1000170000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT', value='1000391000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT', value='1000265000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV', value='1000278000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES', value='1000138000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES', value='1000138001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI', value='1000370000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_DITHERING_FEATURES_EXT', value='1000465000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV', value='1000430000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT', value='1000259000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT', value='1000259002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES', value='1000168000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES', value='1000413000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES', value='1000413001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT', value='1000237000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_FEATURES_NV', value='1000427000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_PROPERTIES_NV', value='1000427001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT', value='1000238000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2', value='1000059006'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT', value='1000328000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV', value='1000202000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT', value='1000328001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV', value='1000202001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT', value='1000376000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES', value='1000053001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX', value='1000097000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_RENDER_AREAS_FEATURES_QCOM', value='1000510000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_VIEWPORTS_FEATURES_QCOM', value='1000488000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES', value='1000053002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT', value='1000392000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT', value='1000392001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT', value='1000351000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT', value='1000422000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT', value='1000396005'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_PROPERTIES_EXT', value='1000396006'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_FEATURES_NV', value='1000464000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_PROPERTIES_NV', value='1000464001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT', value='1000412000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT', value='1000212000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR', value='1000116000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR', value='1000116001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES', value='1000297000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR', value='1000269000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_LIBRARY_GROUP_HANDLES_FEATURES_EXT', value='1000498000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROPERTIES_FEATURES_EXT', value='1000372001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROTECTED_ACCESS_FEATURES_EXT', value='1000466000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT', value='1000068001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES_EXT', value='1000068002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES', value='1000117000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR', value='1000163000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR', value='1000163001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID', value='1000010002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_BARRIER_FEATURES_NV', value='1000292000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR', value='1000294001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR', value='1000248000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT', value='1000382000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT', value='1000356000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES', value='1000295000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2', value='1000059001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES', value='1000145001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES', value='1000145002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT', value='1000254000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT', value='1000254002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR', value='1000080000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT', value='1000342000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR', value='1000348013'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_NV', value='1000490000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_NV', value='1000490001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR', value='1000386000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV', value='1000327001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR', value='1000347000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR', value='1000347001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV', value='1000165009'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV', value='1000166000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT', value='1000344000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT', value='1000286000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT', value='1000286001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES', value='1000130000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES', value='1000156004'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT', value='1000143003'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES', value='1000221000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES', value='1000241000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT', value='1000273000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT', value='1000260000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES', value='1000180000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR', value='1000181000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_FEATURES_ARM', value='1000497000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_PROPERTIES_ARM', value='1000497001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD', value='1000227000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD', value='1000185000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_ARM', value='1000415000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES', value='1000276000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES', value='1000063000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EARLY_AND_LATE_FRAGMENT_TESTS_FEATURES_AMD', value='1000321000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES', value='1000082000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT', value='1000234000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV', value='1000204000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES', value='1000280000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES', value='1000280001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL', value='1000209000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT', value='1000462000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT', value='1000462001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT', value='1000482000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_PROPERTIES_EXT', value='1000482001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV', value='1000154000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV', value='1000154001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES', value='1000175000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR', value='1000323000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES', value='1000215000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_FEATURES_EXT', value='1000395000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_PROPERTIES_EXT', value='1000395001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV', value='1000164001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV', value='1000164002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2', value='1000059008'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES', value='1000094000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES', value='1000225002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES', value='1000225000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT', value='1000458000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI', value='1000369001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI', value='1000369002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR', value='1000119000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT', value='1000275000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES', value='1000314007'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT', value='1000281000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES', value='1000281001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES', value='1000066000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_PROPERTIES_FEATURES_QCOM', value='1000484000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES', value='1000207000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES', value='1000207001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES', value='1000245000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT', value='1000028000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT', value='1000028001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES', value='1000253000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES', value='1000120000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT', value='1000190002'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT', value='1000190000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT', value='1000352000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR', value='1000023014'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES', value='49'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES', value='50'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES', value='51'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES', value='52'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES', value='53'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES', value='54'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES', value='1000211000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_SC_1_0_FEATURES', value='1000298000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_SC_1_0_PROPERTIES', value='1000298001'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR', value='1000336000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT', value='1000330000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT', value='1000252000'),
VarDef(name='VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES', value='1000325000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT', value='1000148002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT', value='1000381001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD', value='1000183000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV', value='1000152000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV', value='1000250001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV', value='1000149000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO', value='1000192000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT', value='1000099001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR', value='1000269003'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR', value='1000269005'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR', value='1000269002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR', value='1000269004'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_ENUM_STATE_CREATE_INFO_NV', value='1000326002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR', value='1000226001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR', value='1000269001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR', value='1000290000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_OFFLINE_CREATE_INFO', value='1000298010'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_POOL_SIZE', value='1000298005'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_PROPERTIES_IDENTIFIER_EXT', value='1000372000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT', value='1000101001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT', value='1000102001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT', value='1000259001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT', value='1000254001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD', value='1000018000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT', value='1000028002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO', value='1000044002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV', value='1000166001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT', value='1000068000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT', value='1000143002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT', value='1000462002'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO', value='1000225001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO', value='1000117003'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT', value='1000190001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_COARSE_SAMPLE_ORDER_STATE_CREATE_INFO_NV', value='1000164005'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT', value='1000355001'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV', value='1000205000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV', value='1000164000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV', value='1000098000'),
VarDef(name='VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV', value='1000087000'),
VarDef(name='VK_STRUCTURE_TYPE_PRESENT_FRAME_TOKEN_GGP', value='1000191000'),
VarDef(name='VK_STRUCTURE_TYPE_PRESENT_ID_KHR', value='1000294000'),
VarDef(name='VK_STRUCTURE_TYPE_PRESENT_INFO_KHR', value='1000001001'),
VarDef(name='VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR', value='1000084000'),
VarDef(name='VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE', value='1000092000'),
VarDef(name='VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO', value='1000295002'),
VarDef(name='VK_STRUCTURE_TYPE_PRIVATE_VENDOR_INFO_RESERVED_OFFSET_0_NV', value='1000051000'),
VarDef(name='VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO', value='1000145000'),
VarDef(name='VK_STRUCTURE_TYPE_QUERY_LOW_LATENCY_SUPPORT_NV', value='1000310000'),
VarDef(name='VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR', value='1000116002'),
VarDef(name='VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_QUERY_CREATE_INFO_INTEL', value='1000210000'),
VarDef(name='VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR', value='1000299005'),
VarDef(name='VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV', value='1000314008'),
VarDef(name='VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV', value='1000206001'),
VarDef(name='VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR', value='1000388001'),
VarDef(name='VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2', value='1000059005'),
VarDef(name='VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_KHR', value='1000023016'),
VarDef(name='VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR', value='1000023012'),
VarDef(name='VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR', value='1000150015'),
VarDef(name='VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV', value='1000165000'),
VarDef(name='VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR', value='1000150018'),
VarDef(name='VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR', value='1000150016'),
VarDef(name='VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV', value='1000165011'),
VarDef(name='VK_STRUCTURE_TYPE_REFRESH_OBJECT_LIST_KHR', value='1000308000'),
VarDef(name='VK_STRUCTURE_TYPE_RELEASE_SWAPCHAIN_IMAGES_INFO_EXT', value='1000275005'),
VarDef(name='VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO', value='1000044001'),
VarDef(name='VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_INFO_EXT', value='1000044007'),
VarDef(name='VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR', value='1000044006'),
VarDef(name='VK_STRUCTURE_TYPE_RENDERING_INFO', value='1000044000'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO', value='1000108003'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2', value='1000109004'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_CONTROL_EXT', value='1000458001'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_FEEDBACK_CREATE_INFO_EXT', value='1000458002'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT', value='1000218002'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO', value='1000117001'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO', value='1000053000'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT', value='1000143001'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_SUBPASS_FEEDBACK_CREATE_INFO_EXT', value='1000458003'),
VarDef(name='VK_STRUCTURE_TYPE_RENDER_PASS_TRANSFORM_BEGIN_INFO_QCOM', value='1000282001'),
VarDef(name='VK_STRUCTURE_TYPE_RESERVED_QCOM', value='1000309000'),
VarDef(name='VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2', value='1000337005'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_BORDER_COLOR_COMPONENT_MAPPING_CREATE_INFO_EXT', value='1000411001'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_CAPTURE_DESCRIPTOR_DATA_INFO_EXT', value='1000316008'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT', value='1000287000'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO', value='1000130001'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO', value='1000156000'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES', value='1000156005'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO', value='1000156001'),
VarDef(name='VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT', value='1000143000'),
VarDef(name='VK_STRUCTURE_TYPE_SCI_SYNC_ATTRIBUTES_INFO_NV', value='1000373003'),
VarDef(name='VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX', value='1000378000'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR', value='1000079001'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_GET_SCI_SYNC_INFO_NV', value='1000373006'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR', value='1000078003'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_GET_ZIRCON_HANDLE_INFO_FUCHSIA', value='1000365001'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_SCI_SYNC_CREATE_INFO_NV', value='1000489001'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_SCI_SYNC_POOL_CREATE_INFO_NV', value='1000489000'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO', value='1000207005'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO', value='1000314005'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO', value='1000207002'),
VarDef(name='VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO', value='1000207004'),
VarDef(name='VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT', value='1000482002'),
VarDef(name='VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT', value='1000462003'),
VarDef(name='VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT', value='1000160001'),
VarDef(name='VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR', value='1000111000'),
VarDef(name='VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2', value='1000059007'),
VarDef(name='VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2', value='1000146004'),
VarDef(name='VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP', value='1000049000'),
VarDef(name='VK_STRUCTURE_TYPE_SUBMIT_INFO_2', value='1000314004'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO', value='1000109005'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2', value='1000109003'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2', value='1000109002'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE', value='1000199001'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_END_INFO', value='1000109006'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM', value='1000425002'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_RESOLVE_PERFORMANCE_QUERY_EXT', value='1000376001'),
VarDef(name='VK_STRUCTURE_TYPE_SUBPASS_SHADING_PIPELINE_CREATE_INFO_HUAWEI', value='1000369000'),
VarDef(name='VK_STRUCTURE_TYPE_SUBRESOURCE_LAYOUT_2_EXT', value='1000338002'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT', value='1000090000'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR', value='1000119001'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT', value='1000255002'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_PRESENT_BARRIER_NV', value='1000292001'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR', value='1000119002'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT', value='1000255000'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT', value='1000255001'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_EXT', value='1000274002'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_EXT', value='1000274000'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_PRESENT_SCALING_CAPABILITIES_EXT', value='1000274001'),
VarDef(name='VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR', value='1000239000'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT', value='1000091003'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR', value='1000001000'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD', value='1000213001'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_IMAGE_CREATE_INFO_ANDROID', value='1000010001'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_BARRIER_CREATE_INFO_NV', value='1000292002'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT', value='1000275001'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_EXT', value='1000275002'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT', value='1000275003'),
VarDef(name='VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT', value='1000275004'),
VarDef(name='VK_STRUCTURE_TYPE_SYSMEM_COLOR_SPACE_FUCHSIA', value='1000366008'),
VarDef(name='VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD', value='1000041000'),
VarDef(name='VK_STRUCTURE_TYPE_TILE_PROPERTIES_QCOM', value='1000484001'),
VarDef(name='VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO', value='1000207003'),
VarDef(name='VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT', value='1000160000'),
VarDef(name='VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT', value='1000247000'),
VarDef(name='VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT', value='1000061000'),
VarDef(name='VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT', value='1000352002'),
VarDef(name='VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT', value='1000352001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR', value='1000023008'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR', value='1000023001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR', value='1000023010'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR', value='1000024001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR', value='1000040000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR', value='1000040006'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR', value='1000040001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR', value='1000040003'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR', value='1000040005'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR', value='1000040004'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_KHR', value='1000187000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR', value='1000187005'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_KHR', value='1000187004'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR', value='1000187003'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR', value='1000187002'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR', value='1000187001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR', value='1000024000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR', value='1000024002'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR', value='1000299003'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_EXT', value='1000038000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_EXT', value='1000038004'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_EXT', value='1000038005'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_EXT', value='1000038007'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_EXT', value='1000038008'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_LAYER_INFO_EXT', value='1000038009'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT', value='1000038002'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT', value='1000038001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_VCL_FRAME_INFO_EXT', value='1000038003'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_EXT', value='1000039000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_EXT', value='1000039004'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_INFO_EXT', value='1000039005'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_EXT', value='1000039007'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_EXT', value='1000039009'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_LAYER_INFO_EXT', value='1000039010'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT', value='1000039002'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT', value='1000039001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_VCL_FRAME_INFO_EXT', value='1000039003'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR', value='1000299000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR', value='1000299001'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR', value='1000299002'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR', value='1000299004'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR', value='1000023009'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR', value='1000023015'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR', value='1000023002'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR', value='1000023000'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR', value='1000023013'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR', value='1000023011'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR', value='1000023005'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR', value='1000023003'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR', value='1000023006'),
VarDef(name='VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_UPDATE_INFO_KHR', value='1000023007'),
VarDef(name='VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN', value='1000062000'),
VarDef(name='VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR', value='1000006000'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR', value='1000075000'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV', value='1000058000'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR', value='1000009000'),
VarDef(name='VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR', value='1000150007'),
VarDef(name='VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV', value='1000165007'),
VarDef(name='VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK', value='1000138002'),
VarDef(name='VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR', value='1000005000'),
VarDef(name='VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR', value='1000004000'),
# CUSTOM / INTERNAL enum values
VarDef(name='VK_STRUCTURE_TYPE_WIN32_INSTANCE_CREATE_INFO_INTEL', value='808600005'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_DEVICE_CREATE_INFO_INTEL', value='808600006'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_IMAGE_CREATE_INFO_INTEL', value='808600007'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_IMAGE_CREATE_INFO_REFLECTION_INTEL', value='808600008'),
VarDef(name='VK_STRUCTURE_TYPE_IMPORT_WIN32_HANDLES_INTEL', value='808600013'),
VarDef(name='VK_STRUCTURE_TYPE_WIN32_SEMAPHORE_CREATE_INFO_REFLECTION_INTEL', value='808600015'),
VarDef(name='VK_STRUCTURE_TYPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSubgroupFeatureFlagBits', enumerators = [
VarDef(name='VK_SUBGROUP_FEATURE_BASIC_BIT', value='1'),
VarDef(name='VK_SUBGROUP_FEATURE_VOTE_BIT', value='2'),
VarDef(name='VK_SUBGROUP_FEATURE_ARITHMETIC_BIT', value='4'),
VarDef(name='VK_SUBGROUP_FEATURE_BALLOT_BIT', value='8'),
VarDef(name='VK_SUBGROUP_FEATURE_SHUFFLE_BIT', value='16'),
VarDef(name='VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT', value='32'),
VarDef(name='VK_SUBGROUP_FEATURE_CLUSTERED_BIT', value='64'),
VarDef(name='VK_SUBGROUP_FEATURE_QUAD_BIT', value='128'),
VarDef(name='VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV', value='256'),
VarDef(name='VK_SUBGROUP_FEATURE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSubmitFlagBits', enumerators = [
VarDef(name='VK_SUBMIT_PROTECTED_BIT', value='1'),
VarDef(name='VK_SUBMIT_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSubpassContents', enumerators = [
VarDef(name='VK_SUBPASS_CONTENTS_INLINE', value='0'),
VarDef(name='VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS', value='1'),
VarDef(name='VK_SUBPASS_CONTENTS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSubpassDescriptionFlagBits', enumerators = [
VarDef(name='VK_SUBPASS_DESCRIPTION_ENABLE_LEGACY_DITHERING_BIT_EXT', value='128'),
VarDef(name='VK_SUBPASS_DESCRIPTION_FRAGMENT_REGION_BIT_QCOM', value='4'),
VarDef(name='VK_SUBPASS_DESCRIPTION_PER_VIEW_ATTRIBUTES_BIT_NVX', value='1'),
VarDef(name='VK_SUBPASS_DESCRIPTION_PER_VIEW_POSITION_X_ONLY_BIT_NVX', value='2'),
VarDef(name='VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_COLOR_ACCESS_BIT_EXT', value='16'),
VarDef(name='VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_DEPTH_ACCESS_BIT_EXT', value='32'),
VarDef(name='VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_STENCIL_ACCESS_BIT_EXT', value='64'),
VarDef(name='VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM', value='8'),
VarDef(name='VK_SUBPASS_DESCRIPTION_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkSubpassMergeStatusEXT', enumerators = [
VarDef(name='VK_SUBPASS_MERGE_STATUS_MERGED_EXT', value='0'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_DISALLOWED_EXT', value='1'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_SIDE_EFFECTS_EXT', value='2'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_SAMPLES_MISMATCH_EXT', value='3'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_VIEWS_MISMATCH_EXT', value='4'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_ALIASING_EXT', value='5'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_DEPENDENCIES_EXT', value='6'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_INCOMPATIBLE_INPUT_ATTACHMENT_EXT', value='7'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_TOO_MANY_ATTACHMENTS_EXT', value='8'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_INSUFFICIENT_STORAGE_EXT', value='9'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_DEPTH_STENCIL_COUNT_EXT', value='10'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_RESOLVE_ATTACHMENT_REUSE_EXT', value='11'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_SINGLE_SUBPASS_EXT', value='12'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_NOT_MERGED_UNSPECIFIED_EXT', value='13'),
VarDef(name='VK_SUBPASS_MERGE_STATUS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkSurfaceCounterFlagBitsEXT', enumerators = [
VarDef(name='VK_SURFACE_COUNTER_VBLANK_BIT_EXT', value='1'),
VarDef(name='VK_SURFACE_COUNTER_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkSurfaceTransformFlagBitsKHR', enumerators = [
VarDef(name='VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR', value='1'),
VarDef(name='VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR', value='2'),
VarDef(name='VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR', value='4'),
VarDef(name='VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR', value='8'),
VarDef(name='VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR', value='16'),
VarDef(name='VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR', value='32'),
VarDef(name='VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR', value='64'),
VarDef(name='VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR', value='128'),
VarDef(name='VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR', value='256'),
VarDef(name='VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkSwapchainCreateFlagBitsKHR', enumerators = [
VarDef(name='VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_EXT', value='8'),
VarDef(name='VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR', value='4'),
VarDef(name='VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR', value='2'),
VarDef(name='VK_SWAPCHAIN_CREATE_RESERVED_4_BIT_EXT', value='16'),
VarDef(name='VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR', value='1'),
VarDef(name='VK_SWAPCHAIN_CREATE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkSwapchainImageUsageFlagBitsANDROID', enumerators = [
VarDef(name='VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID', value='1'),
VarDef(name='VK_SWAPCHAIN_IMAGE_USAGE_FLAG_BITS_MAX_ENUM_ANDROID', value='2147483647'),
])

Enum(name='VkSystemAllocationScope', enumerators = [
VarDef(name='VK_SYSTEM_ALLOCATION_SCOPE_COMMAND', value='0'),
VarDef(name='VK_SYSTEM_ALLOCATION_SCOPE_OBJECT', value='1'),
VarDef(name='VK_SYSTEM_ALLOCATION_SCOPE_CACHE', value='2'),
VarDef(name='VK_SYSTEM_ALLOCATION_SCOPE_DEVICE', value='3'),
VarDef(name='VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE', value='4'),
VarDef(name='VK_SYSTEM_ALLOCATION_SCOPE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkTessellationDomainOrigin', enumerators = [
VarDef(name='VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT', value='0'),
VarDef(name='VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT', value='1'),
VarDef(name='VK_TESSELLATION_DOMAIN_ORIGIN_MAX_ENUM', value='2147483647'),
])

Enum(name='VkTimeDomainEXT', enumerators = [
VarDef(name='VK_TIME_DOMAIN_DEVICE_EXT', value='0'),
VarDef(name='VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT', value='1'),
VarDef(name='VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT', value='2'),
VarDef(name='VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT', value='3'),
VarDef(name='VK_TIME_DOMAIN_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkToolPurposeFlagBits', enumerators = [
VarDef(name='VK_TOOL_PURPOSE_VALIDATION_BIT', value='1'),
VarDef(name='VK_TOOL_PURPOSE_PROFILING_BIT', value='2'),
VarDef(name='VK_TOOL_PURPOSE_TRACING_BIT', value='4'),
VarDef(name='VK_TOOL_PURPOSE_ADDITIONAL_FEATURES_BIT', value='8'),
VarDef(name='VK_TOOL_PURPOSE_MODIFYING_FEATURES_BIT', value='16'),
VarDef(name='VK_TOOL_PURPOSE_DEBUG_MARKERS_BIT_EXT', value='64'),
VarDef(name='VK_TOOL_PURPOSE_DEBUG_REPORTING_BIT_EXT', value='32'),
VarDef(name='VK_TOOL_PURPOSE_FLAG_BITS_MAX_ENUM', value='2147483647'),
])

Enum(name='VkValidationCacheHeaderVersionEXT', enumerators = [
VarDef(name='VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT', value='1'),
VarDef(name='VK_VALIDATION_CACHE_HEADER_VERSION_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkValidationCheckEXT', enumerators = [
VarDef(name='VK_VALIDATION_CHECK_ALL_EXT', value='0'),
VarDef(name='VK_VALIDATION_CHECK_SHADERS_EXT', value='1'),
VarDef(name='VK_VALIDATION_CHECK_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkValidationFeatureDisableEXT', enumerators = [
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_ALL_EXT', value='0'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT', value='1'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT', value='2'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_API_PARAMETERS_EXT', value='3'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES_EXT', value='4'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT', value='5'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT', value='6'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT', value='7'),
VarDef(name='VK_VALIDATION_FEATURE_DISABLE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkValidationFeatureEnableEXT', enumerators = [
VarDef(name='VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT', value='0'),
VarDef(name='VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT', value='1'),
VarDef(name='VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT', value='2'),
VarDef(name='VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT', value='3'),
VarDef(name='VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT', value='4'),
VarDef(name='VK_VALIDATION_FEATURE_ENABLE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVendorId', enumerators = [
VarDef(name='VK_VENDOR_ID_VIV', value='65537'),
VarDef(name='VK_VENDOR_ID_VSI', value='65538'),
VarDef(name='VK_VENDOR_ID_KAZAN', value='65539'),
VarDef(name='VK_VENDOR_ID_CODEPLAY', value='65540'),
VarDef(name='VK_VENDOR_ID_MESA', value='65541'),
VarDef(name='VK_VENDOR_ID_POCL', value='65542'),
VarDef(name='VK_VENDOR_ID_MOBILEYE', value='65543'),
VarDef(name='VK_VENDOR_ID_MAX_ENUM', value='2147483647'),
])

Enum(name='VkVertexInputRate', enumerators = [
VarDef(name='VK_VERTEX_INPUT_RATE_VERTEX', value='0'),
VarDef(name='VK_VERTEX_INPUT_RATE_INSTANCE', value='1'),
VarDef(name='VK_VERTEX_INPUT_RATE_MAX_ENUM', value='2147483647'),
])

Enum(name='VkVideoCapabilityFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_CAPABILITY_PROTECTED_CONTENT_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_CAPABILITY_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoChromaSubsamplingFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_CHROMA_SUBSAMPLING_INVALID_KHR', value='0'),
VarDef(name='VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR', value='8'),
VarDef(name='VK_VIDEO_CHROMA_SUBSAMPLING_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoCodecOperationFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_CODEC_OPERATION_NONE_KHR', value='0'),
VarDef(name='VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_EXT', value='65536'),
VarDef(name='VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_EXT', value='131072'),
VarDef(name='VK_VIDEO_CODEC_OPERATION_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoCodingControlFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_LAYER_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_CODING_CONTROL_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoComponentBitDepthFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR', value='0'),
VarDef(name='VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR', value='16'),
VarDef(name='VK_VIDEO_COMPONENT_BIT_DEPTH_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoDecodeCapabilityFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_COINCIDE_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_DISTINCT_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_DECODE_CAPABILITY_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoDecodeH264PictureLayoutFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR', value='0'),
VarDef(name='VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_SEPARATE_PLANES_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_DECODE_H_PICTURE_LAYOUT_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoDecodeUsageFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_DECODE_USAGE_DEFAULT_KHR', value='0'),
VarDef(name='VK_VIDEO_DECODE_USAGE_TRANSCODING_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_DECODE_USAGE_OFFLINE_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_DECODE_USAGE_STREAMING_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_DECODE_USAGE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoEncodeCapabilityFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_CAPABILITY_PRECEDING_EXTERNALLY_ENCODED_BYTES_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_ENCODE_CAPABILITY_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoEncodeContentFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_CONTENT_DEFAULT_KHR', value='0'),
VarDef(name='VK_VIDEO_ENCODE_CONTENT_CAMERA_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_ENCODE_CONTENT_DESKTOP_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_ENCODE_CONTENT_RENDERED_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_ENCODE_CONTENT_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoEncodeFeedbackFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_ENCODE_FEEDBACK_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoEncodeH264CapabilityFlagBitsEXT', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DIRECT_8X8_INFERENCE_ENABLED_BIT_EXT', value='1'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DIRECT_8X8_INFERENCE_DISABLED_BIT_EXT', value='2'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_SEPARATE_COLOUR_PLANE_BIT_EXT', value='4'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_QPPRIME_Y_ZERO_TRANSFORM_BYPASS_BIT_EXT', value='8'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_SCALING_LISTS_BIT_EXT', value='16'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_HRD_COMPLIANCE_BIT_EXT', value='32'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_CHROMA_QP_OFFSET_BIT_EXT', value='64'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_SECOND_CHROMA_QP_OFFSET_BIT_EXT', value='128'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_PIC_INIT_QP_MINUS26_BIT_EXT', value='256'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_PRED_BIT_EXT', value='512'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_BIPRED_EXPLICIT_BIT_EXT', value='1024'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_BIPRED_IMPLICIT_BIT_EXT', value='2048'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_PRED_NO_TABLE_BIT_EXT', value='4096'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_TRANSFORM_8X8_BIT_EXT', value='8192'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_CABAC_BIT_EXT', value='16384'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_CAVLC_BIT_EXT', value='32768'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DEBLOCKING_FILTER_DISABLED_BIT_EXT', value='65536'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DEBLOCKING_FILTER_ENABLED_BIT_EXT', value='131072'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DEBLOCKING_FILTER_PARTIAL_BIT_EXT', value='262144'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DISABLE_DIRECT_SPATIAL_MV_PRED_BIT_EXT', value='524288'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_MULTIPLE_SLICE_PER_FRAME_BIT_EXT', value='1048576'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_SLICE_MB_COUNT_BIT_EXT', value='2097152'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_ROW_UNALIGNED_SLICE_BIT_EXT', value='4194304'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DIFFERENT_SLICE_TYPE_BIT_EXT', value='8388608'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_B_FRAME_IN_L1_LIST_BIT_EXT', value='16777216'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_DIFFERENT_REFERENCE_FINAL_LISTS_BIT_EXT', value='33554432'),
VarDef(name='VK_VIDEO_ENCODE_H264_CAPABILITY_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVideoEncodeH264RateControlStructureEXT', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_UNKNOWN_EXT', value='0'),
VarDef(name='VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_FLAT_EXT', value='1'),
VarDef(name='VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_DYADIC_EXT', value='2'),
VarDef(name='VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVideoEncodeH265CapabilityFlagBitsEXT', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_SEPARATE_COLOUR_PLANE_BIT_EXT', value='1'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_SCALING_LISTS_BIT_EXT', value='2'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_SAMPLE_ADAPTIVE_OFFSET_ENABLED_BIT_EXT', value='4'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_PCM_ENABLE_BIT_EXT', value='8'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_SPS_TEMPORAL_MVP_ENABLED_BIT_EXT', value='16'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_HRD_COMPLIANCE_BIT_EXT', value='32'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_INIT_QP_MINUS26_BIT_EXT', value='64'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_LOG2_PARALLEL_MERGE_LEVEL_MINUS2_BIT_EXT', value='128'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_SIGN_DATA_HIDING_ENABLED_BIT_EXT', value='256'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_TRANSFORM_SKIP_ENABLED_BIT_EXT', value='512'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_TRANSFORM_SKIP_DISABLED_BIT_EXT', value='1024'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_PPS_SLICE_CHROMA_QP_OFFSETS_PRESENT_BIT_EXT', value='2048'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_WEIGHTED_PRED_BIT_EXT', value='4096'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_WEIGHTED_BIPRED_BIT_EXT', value='8192'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_WEIGHTED_PRED_NO_TABLE_BIT_EXT', value='16384'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_TRANSQUANT_BYPASS_ENABLED_BIT_EXT', value='32768'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_ENTROPY_CODING_SYNC_ENABLED_BIT_EXT', value='65536'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_DEBLOCKING_FILTER_OVERRIDE_ENABLED_BIT_EXT', value='131072'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_MULTIPLE_TILE_PER_FRAME_BIT_EXT', value='262144'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_MULTIPLE_SLICE_PER_TILE_BIT_EXT', value='524288'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_MULTIPLE_TILE_PER_SLICE_BIT_EXT', value='1048576'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_SLICE_SEGMENT_CTB_COUNT_BIT_EXT', value='2097152'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_ROW_UNALIGNED_SLICE_SEGMENT_BIT_EXT', value='4194304'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_DEPENDENT_SLICE_SEGMENT_BIT_EXT', value='8388608'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_DIFFERENT_SLICE_TYPE_BIT_EXT', value='16777216'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_B_FRAME_IN_L1_LIST_BIT_EXT', value='33554432'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_DIFFERENT_REFERENCE_FINAL_LISTS_BIT_EXT', value='67108864'),
VarDef(name='VK_VIDEO_ENCODE_H265_CAPABILITY_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVideoEncodeH265CtbSizeFlagBitsEXT', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_H265_CTB_SIZE_16_BIT_EXT', value='1'),
VarDef(name='VK_VIDEO_ENCODE_H265_CTB_SIZE_32_BIT_EXT', value='2'),
VarDef(name='VK_VIDEO_ENCODE_H265_CTB_SIZE_64_BIT_EXT', value='4'),
VarDef(name='VK_VIDEO_ENCODE_H_CTB_SIZE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVideoEncodeH265RateControlStructureEXT', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_UNKNOWN_EXT', value='0'),
VarDef(name='VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_FLAT_EXT', value='1'),
VarDef(name='VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_DYADIC_EXT', value='2'),
VarDef(name='VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVideoEncodeH265TransformBlockSizeFlagBitsEXT', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_4_BIT_EXT', value='1'),
VarDef(name='VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_8_BIT_EXT', value='2'),
VarDef(name='VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_16_BIT_EXT', value='4'),
VarDef(name='VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_32_BIT_EXT', value='8'),
VarDef(name='VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_FLAG_BITS_MAX_ENUM_EXT', value='2147483647'),
])

Enum(name='VkVideoEncodeRateControlModeFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DEFAULT_KHR', value='0'),
VarDef(name='VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_ENCODE_RATE_CONTROL_MODE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoEncodeTuningModeKHR', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_TUNING_MODE_DEFAULT_KHR', value='0'),
VarDef(name='VK_VIDEO_ENCODE_TUNING_MODE_HIGH_QUALITY_KHR', value='1'),
VarDef(name='VK_VIDEO_ENCODE_TUNING_MODE_LOW_LATENCY_KHR', value='2'),
VarDef(name='VK_VIDEO_ENCODE_TUNING_MODE_ULTRA_LOW_LATENCY_KHR', value='3'),
VarDef(name='VK_VIDEO_ENCODE_TUNING_MODE_LOSSLESS_KHR', value='4'),
VarDef(name='VK_VIDEO_ENCODE_TUNING_MODE_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoEncodeUsageFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR', value='0'),
VarDef(name='VK_VIDEO_ENCODE_USAGE_TRANSCODING_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_ENCODE_USAGE_STREAMING_BIT_KHR', value='2'),
VarDef(name='VK_VIDEO_ENCODE_USAGE_RECORDING_BIT_KHR', value='4'),
VarDef(name='VK_VIDEO_ENCODE_USAGE_CONFERENCING_BIT_KHR', value='8'),
VarDef(name='VK_VIDEO_ENCODE_USAGE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkVideoSessionCreateFlagBitsKHR', enumerators = [
VarDef(name='VK_VIDEO_SESSION_CREATE_PROTECTED_CONTENT_BIT_KHR', value='1'),
VarDef(name='VK_VIDEO_SESSION_CREATE_FLAG_BITS_MAX_ENUM_KHR', value='2147483647'),
])

Enum(name='VkViewportCoordinateSwizzleNV', enumerators = [
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV', value='0'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_X_NV', value='1'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Y_NV', value='2'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Y_NV', value='3'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Z_NV', value='4'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Z_NV', value='5'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_W_NV', value='6'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV', value='7'),
VarDef(name='VK_VIEWPORT_COORDINATE_SWIZZLE_MAX_ENUM_NV', value='2147483647'),
])

Enum(name='VkNegotiateLayerStructType', enumerators = [
VarDef(name='LAYER_NEGOTIATE_UNINTIALIZED', value='0'),
VarDef(name='LAYER_NEGOTIATE_INTERFACE_STRUCT', value='1'),
])

###############################################

Struct(name='VkAabbPositionsKHR_', enabled=False,
var1=VarDef(name='minX', type='float'),
var2=VarDef(name='minY', type='float'),
var3=VarDef(name='minZ', type='float'),
var4=VarDef(name='maxX', type='float'),
var5=VarDef(name='maxY', type='float'),
var6=VarDef(name='maxZ', type='float')
)

Struct(name='VkAabbPositionsNV_', enabled=False,
var1=VarDef(name='minX', type='float'),
var2=VarDef(name='minY', type='float'),
var3=VarDef(name='minZ', type='float'),
var4=VarDef(name='maxX', type='float'),
var5=VarDef(name='maxY', type='float'),
var6=VarDef(name='maxZ', type='float')
)

Struct(name='VkAccelerationStructureBuildGeometryInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkAccelerationStructureTypeKHR'),
var4=VarDef(name='flags', type='VkBuildAccelerationStructureFlagsKHR'),
var5=VarDef(name='mode', type='VkBuildAccelerationStructureModeKHR'),
var6=VarDef(name='srcAccelerationStructure', type='VkAccelerationStructureKHR'),
var7=VarDef(name='dstAccelerationStructure', type='VkAccelerationStructureKHR'),
var8=VarDef(name='geometryCount', type='uint32_t'),
var9=VarDef(name='pGeometries', type='const VkAccelerationStructureGeometryKHR*'),
var10=VarDef(name='ppGeometries', type='const VkAccelerationStructureGeometryKHR* const*'),
var11=VarDef(name='scratchData', type='VkDeviceOrHostAddressKHR')
)

Struct(name='VkAccelerationStructureBuildRangeInfoKHR_', enabled=False,
var1=VarDef(name='primitiveCount', type='uint32_t'),
var2=VarDef(name='primitiveOffset', type='uint32_t'),
var3=VarDef(name='firstVertex', type='uint32_t'),
var4=VarDef(name='transformOffset', type='uint32_t')
)

Struct(name='VkAccelerationStructureBuildSizesInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='accelerationStructureSize', type='VkDeviceSize'),
var4=VarDef(name='updateScratchSize', type='VkDeviceSize'),
var5=VarDef(name='buildScratchSize', type='VkDeviceSize')
)

Struct(name='VkAccelerationStructureCaptureDescriptorDataInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='accelerationStructure', type='VkAccelerationStructureKHR'),
var4=VarDef(name='accelerationStructureNV', type='VkAccelerationStructureNV')
)

Struct(name='VkAccelerationStructureCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='createFlags', type='VkAccelerationStructureCreateFlagsKHR'),
var4=VarDef(name='buffer', type='VkBuffer'),
var5=VarDef(name='offset', type='VkDeviceSize'),
var6=VarDef(name='size', type='VkDeviceSize'),
var7=VarDef(name='type', type='VkAccelerationStructureTypeKHR'),
var8=VarDef(name='deviceAddress', type='VkDeviceAddress')
)

Struct(name='VkAccelerationStructureCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='compactedSize', type='VkDeviceSize'),
var4=VarDef(name='info', type='VkAccelerationStructureInfoNV')
)

Struct(name='VkAccelerationStructureDeviceAddressInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='accelerationStructure', type='VkAccelerationStructureKHR')
)

Struct(name='VkAccelerationStructureGeometryAabbsDataKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='data', type='VkDeviceOrHostAddressConstKHR'),
var4=VarDef(name='stride', type='VkDeviceSize')
)

Struct(name='VkAccelerationStructureGeometryInstancesDataKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='arrayOfPointers', type='VkBool32'),
var4=VarDef(name='data', type='VkDeviceOrHostAddressConstKHR')
)

Struct(name='VkAccelerationStructureGeometryKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='geometryType', type='VkGeometryTypeKHR'),
var4=VarDef(name='geometry', type='VkAccelerationStructureGeometryDataKHR'),
var5=VarDef(name='flags', type='VkGeometryFlagsKHR')
)

Struct(name='VkAccelerationStructureGeometryMotionTrianglesDataNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='vertexData', type='VkDeviceOrHostAddressConstKHR')
)

Struct(name='VkAccelerationStructureGeometryDataKHR_', type='union', enabled=False,
var1=VarDef(name='triangles', type='VkAccelerationStructureGeometryTrianglesDataKHR'),
var2=VarDef(name='aabbs', type='VkAccelerationStructureGeometryAabbsDataKHR'),
var3=VarDef(name='instances', type='VkAccelerationStructureGeometryInstancesDataKHR'),
)

Struct(name='VkAccelerationStructureGeometryTrianglesDataKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='vertexFormat', type='VkFormat'),
var4=VarDef(name='vertexData', type='VkDeviceOrHostAddressConstKHR'),
var5=VarDef(name='vertexStride', type='VkDeviceSize'),
var6=VarDef(name='maxVertex', type='uint32_t'),
var7=VarDef(name='indexType', type='VkIndexType'),
var8=VarDef(name='indexData', type='VkDeviceOrHostAddressConstKHR'),
var9=VarDef(name='transformData', type='VkDeviceOrHostAddressConstKHR')
)

Struct(name='VkAccelerationStructureInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkAccelerationStructureTypeKHR'),
var4=VarDef(name='flags', type='VkBuildAccelerationStructureFlagsKHR'),
var5=VarDef(name='instanceCount', type='uint32_t'),
var6=VarDef(name='geometryCount', type='uint32_t'),
var7=VarDef(name='pGeometries', type='const VkGeometryNV*')
)

Struct(name='VkAccelerationStructureInstanceKHR_', enabled=False,
var1=VarDef(name='transform', type='VkTransformMatrixKHR'),
var2=VarDef(name='instanceCustomIndex', type='uint32_t:24'),
var3=VarDef(name='mask', type='uint32_t:8'),
var4=VarDef(name='instanceShaderBindingTableRecordOffset', type='uint32_t:24'),
var5=VarDef(name='flags', type='VkGeometryInstanceFlagsKHR:8'),
var6=VarDef(name='accelerationStructureReference', type='uint64_t')
)

Struct(name='VkAccelerationStructureInstanceNV_', enabled=False,
var1=VarDef(name='transform', type='VkTransformMatrixKHR'),
var2=VarDef(name='instanceCustomIndex', type='uint32_t:24'),
var3=VarDef(name='mask', type='uint32_t:8'),
var4=VarDef(name='instanceShaderBindingTableRecordOffset', type='uint32_t:24'),
var5=VarDef(name='flags', type='VkGeometryInstanceFlagsKHR:8'),
var6=VarDef(name='accelerationStructureReference', type='uint64_t')
)

Struct(name='VkAccelerationStructureMatrixMotionInstanceNV_', enabled=False,
var1=VarDef(name='transformT0', type='VkTransformMatrixKHR'),
var2=VarDef(name='transformT1', type='VkTransformMatrixKHR'),
var3=VarDef(name='instanceCustomIndex', type='uint32_t:24'),
var4=VarDef(name='mask', type='uint32_t:8'),
var5=VarDef(name='instanceShaderBindingTableRecordOffset', type='uint32_t:24'),
var6=VarDef(name='flags', type='VkGeometryInstanceFlagsKHR:8'),
var7=VarDef(name='accelerationStructureReference', type='uint64_t')
)

Struct(name='VkAccelerationStructureMemoryRequirementsInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkAccelerationStructureMemoryRequirementsTypeNV'),
var4=VarDef(name='accelerationStructure', type='VkAccelerationStructureNV')
)

Struct(name='VkAccelerationStructureMotionInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='maxInstances', type='uint32_t'),
var4=VarDef(name='flags', type='VkAccelerationStructureMotionInfoFlagsNV')
)

Struct(name='VkAccelerationStructureMotionInstanceDataNV_', type='union', enabled=False,
var1=VarDef(name='staticInstance', type='VkAccelerationStructureInstanceKHR'),
var2=VarDef(name='matrixMotionInstance', type='VkAccelerationStructureMatrixMotionInstanceNV'),
var3=VarDef(name='srtMotionInstance', type='VkAccelerationStructureSRTMotionInstanceNV')
)

Struct(name='VkAccelerationStructureMotionInstanceNV_', enabled=False,
var1=VarDef(name='type', type='VkAccelerationStructureMotionInstanceTypeNV'),
var2=VarDef(name='flags', type='VkAccelerationStructureMotionInstanceFlagsNV'),
var3=VarDef(name='data', type='VkAccelerationStructureMotionInstanceDataNV')
)

Struct(name='VkAccelerationStructureSRTMotionInstanceNV_', enabled=False,
var1=VarDef(name='transformT0', type='VkSRTDataNV'),
var2=VarDef(name='transformT1', type='VkSRTDataNV'),
var3=VarDef(name='instanceCustomIndex', type='uint32_t:24'),
var4=VarDef(name='mask', type='uint32_t:8'),
var5=VarDef(name='instanceShaderBindingTableRecordOffset', type='uint32_t:24'),
var6=VarDef(name='flags', type='VkGeometryInstanceFlagsKHR:8'),
var7=VarDef(name='accelerationStructureReference', type='uint64_t')
)

Struct(name='VkAccelerationStructureTrianglesDisplacementMicromapNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='displacementBiasAndScaleFormat', type='VkFormat'),
var4=VarDef(name='displacementVectorFormat', type='VkFormat'),
var5=VarDef(name='displacementBiasAndScaleBuffer', type='VkDeviceOrHostAddressConstKHR'),
var6=VarDef(name='displacementBiasAndScaleStride', type='VkDeviceSize'),
var7=VarDef(name='displacementVectorBuffer', type='VkDeviceOrHostAddressConstKHR'),
var8=VarDef(name='displacementVectorStride', type='VkDeviceSize'),
var9=VarDef(name='displacedMicromapPrimitiveFlags', type='VkDeviceOrHostAddressConstKHR'),
var10=VarDef(name='displacedMicromapPrimitiveFlagsStride', type='VkDeviceSize'),
var11=VarDef(name='indexType', type='VkIndexType'),
var12=VarDef(name='indexBuffer', type='VkDeviceOrHostAddressConstKHR'),
var13=VarDef(name='indexStride', type='VkDeviceSize'),
var14=VarDef(name='baseTriangle', type='uint32_t'),
var15=VarDef(name='usageCountsCount', type='uint32_t'),
var16=VarDef(name='pUsageCounts', type='const VkMicromapUsageEXT*'),
var17=VarDef(name='ppUsageCounts', type='const VkMicromapUsageEXT* const*'),
var18=VarDef(name='micromap', type='VkMicromapEXT')
)

Struct(name='VkAccelerationStructureTrianglesOpacityMicromapEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='indexType', type='VkIndexType'),
var4=VarDef(name='indexBuffer', type='VkDeviceOrHostAddressConstKHR'),
var5=VarDef(name='indexStride', type='VkDeviceSize'),
var6=VarDef(name='baseTriangle', type='uint32_t'),
var7=VarDef(name='usageCountsCount', type='uint32_t'),
var8=VarDef(name='pUsageCounts', type='const VkMicromapUsageEXT*'),
var9=VarDef(name='ppUsageCounts', type='const VkMicromapUsageEXT* const*'),
var10=VarDef(name='micromap', type='VkMicromapEXT')
)

Struct(name='VkAccelerationStructureVersionInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pVersionData', type='const uint8_t*')
)

Struct(name='VkAcquireNextImageInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchain', type='VkSwapchainKHR'),
var4=VarDef(name='timeout', type='uint64_t'),
var5=VarDef(name='semaphore', type='VkSemaphore'),
var6=VarDef(name='fence', type='VkFence'),
var7=VarDef(name='deviceMask', type='uint32_t')
)

Struct(name='VkAcquireProfilingLockInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkAcquireProfilingLockFlagsKHR'),
var4=VarDef(name='timeout', type='uint64_t')
)

Struct(name='VkAllocationCallbacks_', enabled=False,
var1=VarDef(name='pUserData', type='void*'),
var2=VarDef(name='pfnAllocation', type='PFN_vkAllocationFunction'),
var3=VarDef(name='pfnReallocation', type='PFN_vkReallocationFunction'),
var4=VarDef(name='pfnFree', type='PFN_vkFreeFunction'),
var5=VarDef(name='pfnInternalAllocation', type='PFN_vkInternalAllocationNotification'),
var6=VarDef(name='pfnInternalFree', type='PFN_vkInternalFreeNotification')
)

Struct(name='VkApplicationInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pApplicationName', type='const char*', wrapType='Cchar::CSArray', wrapParams='applicationinfo->pApplicationName, \'\\0\', 1'),
var4=VarDef(name='applicationVersion', type='uint32_t'),
var5=VarDef(name='pEngineName', type='const char*', wrapType='Cchar::CSArray', wrapParams='applicationinfo->pEngineName, \'\\0\', 1'),
var6=VarDef(name='engineVersion', type='uint32_t'),
var7=VarDef(name='apiVersion', type='uint32_t')
)

Struct(name='VkApplicationParametersEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='vendorID', type='uint32_t'),
var4=VarDef(name='deviceID', type='uint32_t'),
var5=VarDef(name='key', type='uint32_t'),
var6=VarDef(name='value', type='uint64_t')
)

Struct(name='VkAttachmentDescription_', enabled=True, declareArray=True,
var1=VarDef(name='flags', type='VkAttachmentDescriptionFlags'),
var2=VarDef(name='format', type='VkFormat'),
var3=VarDef(name='samples', type='VkSampleCountFlagBits'),
var4=VarDef(name='loadOp', type='VkAttachmentLoadOp'),
var5=VarDef(name='storeOp', type='VkAttachmentStoreOp'),
var6=VarDef(name='stencilLoadOp', type='VkAttachmentLoadOp'),
var7=VarDef(name='stencilStoreOp', type='VkAttachmentStoreOp'),
var8=VarDef(name='initialLayout', type='VkImageLayout'),
var9=VarDef(name='finalLayout', type='VkImageLayout')
)

Struct(name='VkAttachmentDescription2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkAttachmentDescriptionFlags'),
var4=VarDef(name='format', type='VkFormat'),
var5=VarDef(name='samples', type='VkSampleCountFlagBits'),
var6=VarDef(name='loadOp', type='VkAttachmentLoadOp'),
var7=VarDef(name='storeOp', type='VkAttachmentStoreOp'),
var8=VarDef(name='stencilLoadOp', type='VkAttachmentLoadOp'),
var9=VarDef(name='stencilStoreOp', type='VkAttachmentStoreOp'),
var10=VarDef(name='initialLayout', type='VkImageLayout'),
var11=VarDef(name='finalLayout', type='VkImageLayout')
)

Struct(name='VkAttachmentDescriptionStencilLayout_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stencilInitialLayout', type='VkImageLayout'),
var4=VarDef(name='stencilFinalLayout', type='VkImageLayout')
)

Struct(name='VkAttachmentDescriptionStencilLayoutKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stencilInitialLayout', type='VkImageLayout'),
var4=VarDef(name='stencilFinalLayout', type='VkImageLayout')
)

Struct(name='VkAttachmentReference_', enabled=True, declareArray=True,
var1=VarDef(name='attachment', type='uint32_t'),
var2=VarDef(name='layout', type='VkImageLayout')
)

Struct(name='VkAttachmentReference2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='attachment', type='uint32_t'),
var4=VarDef(name='layout', type='VkImageLayout'),
var5=VarDef(name='aspectMask', type='VkImageAspectFlags')
)

Struct(name='VkAttachmentReferenceStencilLayout_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stencilLayout', type='VkImageLayout')
)

Struct(name='VkAttachmentReferenceStencilLayoutKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stencilLayout', type='VkImageLayout')
)

Struct(name='VkAttachmentSampleCountInfoAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='colorAttachmentCount', type='uint32_t'),
var4=VarDef(name='pColorAttachmentSamples', type='const VkSampleCountFlagBits*'),
var5=VarDef(name='depthStencilAttachmentSamples', type='VkSampleCountFlagBits')
)

Struct(name='VkAttachmentSampleCountInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='colorAttachmentCount', type='uint32_t'),
var4=VarDef(name='pColorAttachmentSamples', type='const VkSampleCountFlagBits*'),
var5=VarDef(name='depthStencilAttachmentSamples', type='VkSampleCountFlagBits')
)

Struct(name='VkAttachmentSampleLocationsEXT_', enabled=False,
var1=VarDef(name='attachmentIndex', type='uint32_t'),
var2=VarDef(name='sampleLocationsInfo', type='VkSampleLocationsInfoEXT')
)

Struct(name='VkBaseInStructure_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*')
)

Struct(name='VkBaseOutStructure_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*')
)

Struct(name='VkBindAccelerationStructureMemoryInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='accelerationStructure', type='VkAccelerationStructureNV'),
var4=VarDef(name='memory', type='VkDeviceMemory'),
var5=VarDef(name='memoryOffset', type='VkDeviceSize'),
var6=VarDef(name='deviceIndexCount', type='uint32_t'),
var7=VarDef(name='pDeviceIndices', type='const uint32_t*')
)

Struct(name='VkBindBufferMemoryDeviceGroupInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='deviceIndexCount', type='uint32_t'),
var4=VarDef(name='pDeviceIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='bindbuffermemorydevicegroupinfo->deviceIndexCount, bindbuffermemorydevicegroupinfo->pDeviceIndices', count='deviceIndexCount')
)

Struct(name='VkBindBufferMemoryInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='buffer', type='VkBuffer'),
var4=VarDef(name='memory', type='VkDeviceMemory'),
var5=VarDef(name='memoryOffset', type='VkDeviceSize')
)

Struct(name='VkBindImageMemoryDeviceGroupInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='deviceIndexCount', type='uint32_t'),
var4=VarDef(name='pDeviceIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='bindimagememorydevicegroupinfo->deviceIndexCount, bindimagememorydevicegroupinfo->pDeviceIndices', count='deviceIndexCount'),
var5=VarDef(name='splitInstanceBindRegionCount', type='uint32_t'),
var6=VarDef(name='pSplitInstanceBindRegions', type='const VkRect2D*', wrapType='CVkRect2DArray', wrapParams='bindimagememorydevicegroupinfo->splitInstanceBindRegionCount, bindimagememorydevicegroupinfo->pSplitInstanceBindRegions', count='splitInstanceBindRegionCount')
)

Struct(name='VkBindImageMemoryInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='image', type='VkImage'),
var4=VarDef(name='memory', type='VkDeviceMemory'),
var5=VarDef(name='memoryOffset', type='VkDeviceSize')
)

Struct(name='VkBindImageMemorySwapchainInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchain', type='VkSwapchainKHR'),
var4=VarDef(name='imageIndex', type='uint32_t')
)

Struct(name='VkBindImagePlaneMemoryInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='planeAspect', type='VkImageAspectFlagBits')
)

Struct(name='VkBindIndexBufferIndirectCommandNV_', enabled=False,
var1=VarDef(name='bufferAddress', type='VkDeviceAddress'),
var2=VarDef(name='size', type='uint32_t'),
var3=VarDef(name='indexType', type='VkIndexType')
)

Struct(name='VkBindShaderGroupIndirectCommandNV_', enabled=False,
var1=VarDef(name='groupIndex', type='uint32_t')
)

Struct(name='VkBindSparseInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphores', type='const VkSemaphore*', wrapType='CVkSemaphore::CSArray', wrapParams='bindsparseinfo->waitSemaphoreCount, bindsparseinfo->pWaitSemaphores', count='waitSemaphoreCount'),
var5=VarDef(name='bufferBindCount', type='uint32_t'),
var6=VarDef(name='pBufferBinds', type='const VkSparseBufferMemoryBindInfo*', wrapType='CVkSparseBufferMemoryBindInfoArray', wrapParams='bindsparseinfo->bufferBindCount, bindsparseinfo->pBufferBinds', count='bufferBindCount'),
var7=VarDef(name='imageOpaqueBindCount', type='uint32_t'),
var8=VarDef(name='pImageOpaqueBinds', type='const VkSparseImageOpaqueMemoryBindInfo*', wrapType='CVkSparseImageOpaqueMemoryBindInfoArray', wrapParams='bindsparseinfo->imageOpaqueBindCount, bindsparseinfo->pImageOpaqueBinds', count='imageOpaqueBindCount'),
var9=VarDef(name='imageBindCount', type='uint32_t'),
var10=VarDef(name='pImageBinds', type='const VkSparseImageMemoryBindInfo*', wrapType='CVkSparseImageMemoryBindInfoArray', wrapParams='bindsparseinfo->imageBindCount, bindsparseinfo->pImageBinds', count='imageBindCount'),
var11=VarDef(name='signalSemaphoreCount', type='uint32_t'),
var12=VarDef(name='pSignalSemaphores', type='const VkSemaphore*', wrapType='CVkSemaphore::CSArray', wrapParams='bindsparseinfo->signalSemaphoreCount, bindsparseinfo->pSignalSemaphores', count='signalSemaphoreCount')
)

Struct(name='VkBindVertexBufferIndirectCommandNV_', enabled=False,
var1=VarDef(name='bufferAddress', type='VkDeviceAddress'),
var2=VarDef(name='size', type='uint32_t'),
var3=VarDef(name='stride', type='uint32_t')
)

Struct(name='VkBindVideoSessionMemoryInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='memoryBindIndex', type='uint32_t'),
var4=VarDef(name='memory', type='VkDeviceMemory'),
var5=VarDef(name='memoryOffset', type='VkDeviceSize'),
var6=VarDef(name='memorySize', type='VkDeviceSize')
)

Struct(name='VkBlitImageInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcImage', type='VkImage'),
var4=VarDef(name='srcImageLayout', type='VkImageLayout'),
var5=VarDef(name='dstImage', type='VkImage'),
var6=VarDef(name='dstImageLayout', type='VkImageLayout'),
var7=VarDef(name='regionCount', type='uint32_t'),
var8=VarDef(name='pRegions', type='const VkImageBlit2*', wrapType='CVkImageBlit2Array', wrapParams='blitimageinfo2->regionCount, blitimageinfo2->pRegions', count='regionCount'),
var9=VarDef(name='filter', type='VkFilter')
)

Struct(name='VkBufferCaptureDescriptorDataInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='buffer', type='VkBuffer')
)

Struct(name='VkBufferCopy_', enabled=True, declareArray=True,
var1=VarDef(name='srcOffset', type='VkDeviceSize'),
var2=VarDef(name='dstOffset', type='VkDeviceSize'),
var3=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkBufferCopy2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcOffset', type='VkDeviceSize'),
var4=VarDef(name='dstOffset', type='VkDeviceSize'),
var5=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkBufferCopy2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcOffset', type='VkDeviceSize'),
var4=VarDef(name='dstOffset', type='VkDeviceSize'),
var5=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkBufferCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkBufferCreateFlags'),
var4=VarDef(name='size', type='VkDeviceSize'),
var5=VarDef(name='usage', type='VkBufferUsageFlags'),
var6=VarDef(name='sharingMode', type='VkSharingMode'),
var7=VarDef(name='queueFamilyIndexCount', type='uint32_t'),
var8=VarDef(name='pQueueFamilyIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='buffercreateinfo->queueFamilyIndexCount, buffercreateinfo->pQueueFamilyIndices', count='queueFamilyIndexCount')
)

Struct(name='VkBufferDeviceAddressCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='deviceAddress', type='VkDeviceAddress')
)

Struct(name='VkBufferDeviceAddressInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='buffer', type='VkBuffer')
)

Struct(name='VkBufferImageCopy_', enabled=True, declareArray=True,
var1=VarDef(name='bufferOffset', type='VkDeviceSize'),
var2=VarDef(name='bufferRowLength', type='uint32_t'),
var3=VarDef(name='bufferImageHeight', type='uint32_t'),
var4=VarDef(name='imageSubresource', type='VkImageSubresourceLayers'),
var5=VarDef(name='imageOffset', type='VkOffset3D'),
var6=VarDef(name='imageExtent', type='VkExtent3D')
)

Struct(name='VkBufferImageCopy2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='bufferOffset', type='VkDeviceSize'),
var4=VarDef(name='bufferRowLength', type='uint32_t'),
var5=VarDef(name='bufferImageHeight', type='uint32_t'),
var6=VarDef(name='imageSubresource', type='VkImageSubresourceLayers'),
var7=VarDef(name='imageOffset', type='VkOffset3D'),
var8=VarDef(name='imageExtent', type='VkExtent3D')
)

Struct(name='VkBufferMemoryBarrier_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcAccessMask', type='VkAccessFlags'),
var4=VarDef(name='dstAccessMask', type='VkAccessFlags'),
var5=VarDef(name='srcQueueFamilyIndex', type='uint32_t'),
var6=VarDef(name='dstQueueFamilyIndex', type='uint32_t'),
var7=VarDef(name='buffer', type='VkBuffer'),
var8=VarDef(name='offset', type='VkDeviceSize'),
var9=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkBufferMemoryBarrier2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcStageMask', type='VkPipelineStageFlags2'),
var4=VarDef(name='srcAccessMask', type='VkAccessFlags2'),
var5=VarDef(name='dstStageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='dstAccessMask', type='VkAccessFlags2'),
var7=VarDef(name='srcQueueFamilyIndex', type='uint32_t'),
var8=VarDef(name='dstQueueFamilyIndex', type='uint32_t'),
var9=VarDef(name='buffer', type='VkBuffer'),
var10=VarDef(name='offset', type='VkDeviceSize'),
var11=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkBufferMemoryRequirementsInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='buffer', type='VkBuffer')
)

Struct(name='VkBufferOpaqueCaptureAddressCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='opaqueCaptureAddress', type='uint64_t')
)

Struct(name='VkBufferViewCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkBufferViewCreateFlags'),
var4=VarDef(name='buffer', type='VkBuffer'),
var5=VarDef(name='format', type='VkFormat'),
var6=VarDef(name='offset', type='VkDeviceSize'),
var7=VarDef(name='range', type='VkDeviceSize')
)

Struct(name='VkCalibratedTimestampInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='timeDomain', type='VkTimeDomainEXT')
)

Struct(name='VkCheckpointData2NV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stage', type='VkPipelineStageFlags2'),
var4=VarDef(name='pCheckpointMarker', type='void*')
)

Struct(name='VkCheckpointDataNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stage', type='VkPipelineStageFlagBits'),
var4=VarDef(name='pCheckpointMarker', type='void*')
)

Struct(name='VkClearAttachment_', enabled=True, declareArray=True,
var1=VarDef(name='aspectMask', type='VkImageAspectFlags'),
var2=VarDef(name='colorAttachment', type='uint32_t'),
var3=VarDef(name='clearValue', type='VkClearValue')
)

Struct(name='VkClearColorValue_', type='union', enabled=True, custom=True,
var1=VarDef(name='float32', type='float[4]', count='4'),
var2=VarDef(name='int32', type='int32_t[4]', count='4'),
var3=VarDef(name='uint32', type='uint32_t[4]', count='4')
)

Struct(name='VkClearDepthStencilValue_', enabled=True,
var1=VarDef(name='depth', type='float'),
var2=VarDef(name='stencil', type='uint32_t')
)

Struct(name='VkClearRect_', enabled=True, declareArray=True,
var1=VarDef(name='rect', type='VkRect2D'),
var2=VarDef(name='baseArrayLayer', type='uint32_t'),
var3=VarDef(name='layerCount', type='uint32_t')
)

Struct(name='VkClearValue_', type='union', enabled=True, custom=True, declareArray=True,
var1=VarDef(name='color', type='VkClearColorValue'),
var2=VarDef(name='depthStencil', type='VkClearDepthStencilValue')
)

Struct(name='VkCoarseSampleLocationNV_', enabled=False,
var1=VarDef(name='pixelX', type='uint32_t'),
var2=VarDef(name='pixelY', type='uint32_t'),
var3=VarDef(name='sample', type='uint32_t')
)

Struct(name='VkCoarseSampleOrderCustomNV_', enabled=False,
var1=VarDef(name='shadingRate', type='VkShadingRatePaletteEntryNV'),
var2=VarDef(name='sampleCount', type='uint32_t'),
var3=VarDef(name='sampleLocationCount', type='uint32_t'),
var4=VarDef(name='pSampleLocations', type='const VkCoarseSampleLocationNV*')
)

Struct(name='VkColorBlendAdvancedEXT_', enabled=False,
var1=VarDef(name='advancedBlendOp', type='VkBlendOp'),
var2=VarDef(name='srcPremultiplied', type='VkBool32'),
var3=VarDef(name='dstPremultiplied', type='VkBool32'),
var4=VarDef(name='blendOverlap', type='VkBlendOverlapEXT'),
var5=VarDef(name='clampResults', type='VkBool32')
)

Struct(name='VkColorBlendEquationEXT_', enabled=False,
var1=VarDef(name='srcColorBlendFactor', type='VkBlendFactor'),
var2=VarDef(name='dstColorBlendFactor', type='VkBlendFactor'),
var3=VarDef(name='colorBlendOp', type='VkBlendOp'),
var4=VarDef(name='srcAlphaBlendFactor', type='VkBlendFactor'),
var5=VarDef(name='dstAlphaBlendFactor', type='VkBlendFactor'),
var6=VarDef(name='alphaBlendOp', type='VkBlendOp')
)

Struct(name='VkCommandBufferAllocateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='commandPool', type='VkCommandPool'),
var4=VarDef(name='level', type='VkCommandBufferLevel'),
var5=VarDef(name='commandBufferCount', type='uint32_t')
)

Struct(name='VkCommandBufferBeginInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkCommandBufferUsageFlags'),
var4=VarDef(name='pInheritanceInfo', type='const VkCommandBufferInheritanceInfo*', wrapType='CVkCommandBufferInheritanceInfoArray', wrapParams='1, commandbufferbegininfo->pInheritanceInfo')
)

Struct(name='VkCommandBufferInheritanceConditionalRenderingInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='conditionalRenderingEnable', type='VkBool32')
)

Struct(name='VkCommandBufferInheritanceInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='renderPass', type='VkRenderPass'),
var4=VarDef(name='subpass', type='uint32_t'),
var5=VarDef(name='framebuffer', type='VkFramebuffer'),
var6=VarDef(name='occlusionQueryEnable', type='VkBool32'),
var7=VarDef(name='queryFlags', type='VkQueryControlFlags'),
var8=VarDef(name='pipelineStatistics', type='VkQueryPipelineStatisticFlags')
)

Struct(name='VkCommandBufferInheritanceRenderPassTransformInfoQCOM_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='transform', type='VkSurfaceTransformFlagBitsKHR'),
var4=VarDef(name='renderArea', type='VkRect2D')
)

Struct(name='VkCommandBufferInheritanceRenderingInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderingFlags'),
var4=VarDef(name='viewMask', type='uint32_t'),
var5=VarDef(name='colorAttachmentCount', type='uint32_t'),
var6=VarDef(name='pColorAttachmentFormats', type='const VkFormat*'),
var7=VarDef(name='depthAttachmentFormat', type='VkFormat'),
var8=VarDef(name='stencilAttachmentFormat', type='VkFormat'),
var9=VarDef(name='rasterizationSamples', type='VkSampleCountFlagBits')
)

Struct(name='VkCommandBufferInheritanceRenderingInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderingFlags'),
var4=VarDef(name='viewMask', type='uint32_t'),
var5=VarDef(name='colorAttachmentCount', type='uint32_t'),
var6=VarDef(name='pColorAttachmentFormats', type='const VkFormat*'),
var7=VarDef(name='depthAttachmentFormat', type='VkFormat'),
var8=VarDef(name='stencilAttachmentFormat', type='VkFormat'),
var9=VarDef(name='rasterizationSamples', type='VkSampleCountFlagBits')
)

Struct(name='VkCommandBufferInheritanceViewportScissorInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='viewportScissor2D', type='VkBool32'),
var4=VarDef(name='viewportDepthCount', type='uint32_t'),
var5=VarDef(name='pViewportDepths', type='const VkViewport*')
)

Struct(name='VkCommandBufferSubmitInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='commandBuffer', type='VkCommandBuffer'),
var4=VarDef(name='deviceMask', type='uint32_t')
)

Struct(name='VkCommandPoolCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkCommandPoolCreateFlags'),
var4=VarDef(name='queueFamilyIndex', type='uint32_t')
)

Struct(name='VkCommandPoolMemoryConsumption_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='commandPoolAllocated', type='VkDeviceSize'),
var4=VarDef(name='commandPoolReservedSize', type='VkDeviceSize'),
var5=VarDef(name='commandBufferAllocated', type='VkDeviceSize')
)

Struct(name='VkCommandPoolMemoryReservationCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='commandPoolReservedSize', type='VkDeviceSize'),
var4=VarDef(name='commandPoolMaxCommandBuffers', type='uint32_t')
)

Struct(name='VkComponentMapping_', enabled=True,
var1=VarDef(name='r', type='VkComponentSwizzle'),
var2=VarDef(name='g', type='VkComponentSwizzle'),
var3=VarDef(name='b', type='VkComponentSwizzle'),
var4=VarDef(name='a', type='VkComponentSwizzle')
)

Struct(name='VkComputePipelineCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCreateFlags'),
var4=VarDef(name='stage', type='VkPipelineShaderStageCreateInfo'),
var5=VarDef(name='layout', type='VkPipelineLayout'),
var6=VarDef(name='basePipelineHandle', type='VkPipeline'),
var7=VarDef(name='basePipelineIndex', type='int32_t')
)

Struct(name='VkConditionalRenderingBeginInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='buffer', type='VkBuffer'),
var4=VarDef(name='offset', type='VkDeviceSize'),
var5=VarDef(name='flags', type='VkConditionalRenderingFlagsEXT')
)

Struct(name='VkConformanceVersion_', enabled=False,
var1=VarDef(name='major', type='uint8_t'),
var2=VarDef(name='minor', type='uint8_t'),
var3=VarDef(name='subminor', type='uint8_t'),
var4=VarDef(name='patch', type='uint8_t')
)

Struct(name='VkCooperativeMatrixPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='MSize', type='uint32_t'),
var4=VarDef(name='NSize', type='uint32_t'),
var5=VarDef(name='KSize', type='uint32_t'),
var6=VarDef(name='AType', type='VkComponentTypeNV'),
var7=VarDef(name='BType', type='VkComponentTypeNV'),
var8=VarDef(name='CType', type='VkComponentTypeNV'),
var9=VarDef(name='DType', type='VkComponentTypeNV'),
var10=VarDef(name='scope', type='VkScopeNV')
)

Struct(name='VkCopyAccelerationStructureInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='src', type='VkAccelerationStructureKHR'),
var4=VarDef(name='dst', type='VkAccelerationStructureKHR'),
var5=VarDef(name='mode', type='VkCopyAccelerationStructureModeKHR')
)

Struct(name='VkCopyAccelerationStructureToMemoryInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='src', type='VkAccelerationStructureKHR'),
var4=VarDef(name='dst', type='VkDeviceOrHostAddressKHR'),
var5=VarDef(name='mode', type='VkCopyAccelerationStructureModeKHR')
)

Struct(name='VkCopyBufferInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcBuffer', type='VkBuffer'),
var4=VarDef(name='dstBuffer', type='VkBuffer'),
var5=VarDef(name='regionCount', type='uint32_t'),
var6=VarDef(name='pRegions', type='const VkBufferCopy2*', wrapType='CVkBufferCopy2Array', wrapParams='copybufferinfo2->regionCount, copybufferinfo2->pRegions', count='regionCount')
)

Struct(name='VkCopyBufferToImageInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcBuffer', type='VkBuffer'),
var4=VarDef(name='dstImage', type='VkImage'),
var5=VarDef(name='dstImageLayout', type='VkImageLayout'),
var6=VarDef(name='regionCount', type='uint32_t'),
var7=VarDef(name='pRegions', type='const VkBufferImageCopy2*', wrapType='CVkBufferImageCopy2Array', wrapParams='copybuffertoimageinfo2->regionCount, copybuffertoimageinfo2->pRegions', count='regionCount')
)

Struct(name='VkCopyCommandTransformInfoQCOM_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='transform', type='VkSurfaceTransformFlagBitsKHR')
)

Struct(name='VkCopyDescriptorSet_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSet', type='VkDescriptorSet'),
var4=VarDef(name='srcBinding', type='uint32_t'),
var5=VarDef(name='srcArrayElement', type='uint32_t'),
var6=VarDef(name='dstSet', type='VkDescriptorSet'),
var7=VarDef(name='dstBinding', type='uint32_t'),
var8=VarDef(name='dstArrayElement', type='uint32_t'),
var9=VarDef(name='descriptorCount', type='uint32_t')
)

Struct(name='VkCopyImageInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcImage', type='VkImage'),
var4=VarDef(name='srcImageLayout', type='VkImageLayout'),
var5=VarDef(name='dstImage', type='VkImage'),
var6=VarDef(name='dstImageLayout', type='VkImageLayout'),
var7=VarDef(name='regionCount', type='uint32_t'),
var8=VarDef(name='pRegions', type='const VkImageCopy2*', wrapType='CVkImageCopy2Array', wrapParams='copyimageinfo2->regionCount, copyimageinfo2->pRegions', count='regionCount')
)

Struct(name='VkCopyImageToBufferInfo2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcImage', type='VkImage'),
var4=VarDef(name='srcImageLayout', type='VkImageLayout'),
var5=VarDef(name='dstBuffer', type='VkBuffer'),
var6=VarDef(name='regionCount', type='uint32_t'),
var7=VarDef(name='pRegions', type='const VkBufferImageCopy2*', wrapType='CVkBufferImageCopy2Array', wrapParams='copyimagetobufferinfo2->regionCount, copyimagetobufferinfo2->pRegions', count='regionCount')
)

Struct(name='VkCopyMemoryIndirectCommandNV_', enabled=False,
var1=VarDef(name='srcAddress', type='VkDeviceAddress'),
var2=VarDef(name='dstAddress', type='VkDeviceAddress'),
var3=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkCopyMemoryToAccelerationStructureInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='src', type='VkDeviceOrHostAddressConstKHR'),
var4=VarDef(name='dst', type='VkAccelerationStructureKHR'),
var5=VarDef(name='mode', type='VkCopyAccelerationStructureModeKHR')
)

Struct(name='VkCopyMemoryToImageIndirectCommandNV_', enabled=False,
var1=VarDef(name='srcAddress', type='VkDeviceAddress'),
var2=VarDef(name='bufferRowLength', type='uint32_t'),
var3=VarDef(name='bufferImageHeight', type='uint32_t'),
var4=VarDef(name='imageSubresource', type='VkImageSubresourceLayers'),
var5=VarDef(name='imageOffset', type='VkOffset3D'),
var6=VarDef(name='imageExtent', type='VkExtent3D')
)

Struct(name='VkCopyMemoryToMicromapInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='src', type='VkDeviceOrHostAddressConstKHR'),
var4=VarDef(name='dst', type='VkMicromapEXT'),
var5=VarDef(name='mode', type='VkCopyMicromapModeEXT')
)

Struct(name='VkCopyMicromapInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='src', type='VkMicromapEXT'),
var4=VarDef(name='dst', type='VkMicromapEXT'),
var5=VarDef(name='mode', type='VkCopyMicromapModeEXT')
)

Struct(name='VkCopyMicromapToMemoryInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='src', type='VkMicromapEXT'),
var4=VarDef(name='dst', type='VkDeviceOrHostAddressKHR'),
var5=VarDef(name='mode', type='VkCopyMicromapModeEXT')
)

Struct(name='VkD3D12FenceSubmitInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreValuesCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphoreValues', type='const uint64_t*', count='waitSemaphoreValuesCount'),
var5=VarDef(name='signalSemaphoreValuesCount', type='uint32_t'),
var6=VarDef(name='pSignalSemaphoreValues', type='const uint64_t*', count='signalSemaphoreValuesCount')
)

Struct(name='VkDebugMarkerMarkerInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pMarkerName', type='const char*', wrapType='Cchar::CSArray', wrapParams='debugmarkermarkerinfoext->pMarkerName, \'\\0\', 1'),
var4=VarDef(name='color', type='float[4]', count='4')
)

Struct(name='VkDebugMarkerObjectNameInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='objectType', type='VkDebugReportObjectTypeEXT'),
var4=VarDef(name='object', type='uint64_t'),
var5=VarDef(name='pObjectName', type='const char*', wrapType='Cchar::CSArray', wrapParams='debugmarkerobjectnameinfoext->pObjectName, \'\\0\', 1')
)

Struct(name='VkDebugMarkerObjectTagInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='objectType', type='VkDebugReportObjectTypeEXT'),
var4=VarDef(name='object', type='uint64_t'),
var5=VarDef(name='tagName', type='uint64_t'),
var6=VarDef(name='tagSize', type='size_t'),
var7=VarDef(name='pTag', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='debugmarkerobjecttaginfoext->tagSize, (uint8_t *)debugmarkerobjecttaginfoext->pTag')
)

Struct(name='VkDebugReportCallbackCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDebugReportFlagsEXT'),
var4=VarDef(name='pfnCallback', type='PFN_vkDebugReportCallbackEXT'),
var5=VarDef(name='pUserData', type='void*')
)

Struct(name='VkDebugUtilsLabelEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pLabelName', type='const char*'),
var4=VarDef(name='color', type='float[4]')
)

Struct(name='VkDebugUtilsMessengerCallbackDataEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDebugUtilsMessengerCallbackDataFlagsEXT'),
var4=VarDef(name='pMessageIdName', type='const char*'),
var5=VarDef(name='messageIdNumber', type='int32_t'),
var6=VarDef(name='pMessage', type='const char*'),
var7=VarDef(name='queueLabelCount', type='uint32_t'),
var8=VarDef(name='pQueueLabels', type='const VkDebugUtilsLabelEXT*'),
var9=VarDef(name='cmdBufLabelCount', type='uint32_t'),
var10=VarDef(name='pCmdBufLabels', type='const VkDebugUtilsLabelEXT*'),
var11=VarDef(name='objectCount', type='uint32_t'),
var12=VarDef(name='pObjects', type='const VkDebugUtilsObjectNameInfoEXT*')
)

Struct(name='VkDebugUtilsMessengerCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDebugUtilsMessengerCreateFlagsEXT'),
var4=VarDef(name='messageSeverity', type='VkDebugUtilsMessageSeverityFlagsEXT'),
var5=VarDef(name='messageType', type='VkDebugUtilsMessageTypeFlagsEXT'),
var6=VarDef(name='pfnUserCallback', type='PFN_vkDebugUtilsMessengerCallbackEXT'),
var7=VarDef(name='pUserData', type='void*')
)

Struct(name='VkDebugUtilsObjectNameInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='objectType', type='VkObjectType'),
var4=VarDef(name='objectHandle', type='uint64_t'),
var5=VarDef(name='pObjectName', type='const char*')
)

Struct(name='VkDebugUtilsObjectTagInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='objectType', type='VkObjectType'),
var4=VarDef(name='objectHandle', type='uint64_t'),
var5=VarDef(name='tagName', type='uint64_t'),
var6=VarDef(name='tagSize', type='size_t'),
var7=VarDef(name='pTag', type='const void*')
)

Struct(name='VkDecompressMemoryRegionNV_', enabled=False,
var1=VarDef(name='srcAddress', type='VkDeviceAddress'),
var2=VarDef(name='dstAddress', type='VkDeviceAddress'),
var3=VarDef(name='compressedSize', type='VkDeviceSize'),
var4=VarDef(name='decompressedSize', type='VkDeviceSize'),
var5=VarDef(name='decompressionMethod', type='VkMemoryDecompressionMethodFlagsNV')
)

Struct(name='VkDedicatedAllocationBufferCreateInfoNV_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='dedicatedAllocation', type='VkBool32')
)

Struct(name='VkDedicatedAllocationImageCreateInfoNV_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='dedicatedAllocation', type='VkBool32')
)

Struct(name='VkDedicatedAllocationMemoryAllocateInfoNV_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='image', type='VkImage'),
var4=VarDef(name='buffer', type='VkBuffer')
)

Struct(name='VkDependencyInfo_', enabled=True, custom=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='dependencyFlags', type='VkDependencyFlags'),
var4=VarDef(name='memoryBarrierCount', type='uint32_t'),
var5=VarDef(name='pMemoryBarriers', type='const VkMemoryBarrier2*', wrapType='CVkMemoryBarrier2Array', wrapParams='dependencyinfo->memoryBarrierCount, dependencyinfo->pMemoryBarriers', count='memoryBarrierCount'),
var6=VarDef(name='bufferMemoryBarrierCount', type='uint32_t'),
var7=VarDef(name='pBufferMemoryBarriers', type='const VkBufferMemoryBarrier2*', wrapType='CVkBufferMemoryBarrier2Array', wrapParams='dependencyinfo->bufferMemoryBarrierCount, dependencyinfo->pBufferMemoryBarriers', count='bufferMemoryBarrierCount'),
var8=VarDef(name='imageMemoryBarrierCount', type='uint32_t'),
var9=VarDef(name='pImageMemoryBarriers', type='const VkImageMemoryBarrier2*', wrapType='CVkImageMemoryBarrier2Array', wrapParams='dependencyinfo->imageMemoryBarrierCount, dependencyinfo->pImageMemoryBarriers', count='imageMemoryBarrierCount')
)

Struct(name='VkDescriptorAddressInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='address', type='VkDeviceAddress'),
var4=VarDef(name='range', type='VkDeviceSize'),
var5=VarDef(name='format', type='VkFormat')
)

Struct(name='VkDescriptorBufferBindingInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='address', type='VkDeviceAddress'),
var4=VarDef(name='usage', type='VkBufferUsageFlags')
)

Struct(name='VkDescriptorBufferBindingPushDescriptorBufferHandleEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='buffer', type='VkBuffer')
)

Struct(name='VkDescriptorDataEXT_', type='union', enabled=False,
var1=VarDef(name='pSampler', type='VkSampler*'),
var2=VarDef(name='pCombinedImageSampler', type='VkDescriptorImageInfo*'),
var3=VarDef(name='pSampledImage', type='VkDescriptorImageInfo*'),
var4=VarDef(name='pStorageImage', type='VkDescriptorImageInfo*'),
var5=VarDef(name='pUniformTexelBuffer', type='VkDescriptorAddressInfoEXT*'),
var6=VarDef(name='pStorageTexelBuffer', type='VkDescriptorAddressInfoEXT*'),
var7=VarDef(name='pUniformBuffer', type='VkDescriptorAddressInfoEXT*'),
var8=VarDef(name='pStorageBuffer', type='VkDescriptorAddressInfoEXT*'),
var9=VarDef(name='accelerationStructure', type='VkDeviceAddress'),
)

Struct(name='VkDescriptorBufferInfo_', enabled=True, declareArray=True,
var1=VarDef(name='buffer', type='VkBuffer'),
var2=VarDef(name='offset', type='VkDeviceSize'),
var3=VarDef(name='range', type='VkDeviceSize')
)

Struct(name='VkDescriptorGetInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkDescriptorType'),
var4=VarDef(name='data', type='VkDescriptorDataEXT')
)

Struct(name='VkDescriptorImageInfo_', enabled=True, declareArray=True, custom=True,
var1=VarDef(name='sampler', type='VkSampler'),
var2=VarDef(name='imageView', type='VkImageView'),
var3=VarDef(name='imageLayout', type='VkImageLayout')
)

Struct(name='VkDescriptorPoolCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDescriptorPoolCreateFlags'),
var4=VarDef(name='maxSets', type='uint32_t'),
var5=VarDef(name='poolSizeCount', type='uint32_t'),
var6=VarDef(name='pPoolSizes', type='const VkDescriptorPoolSize*', wrapType='CVkDescriptorPoolSizeArray', wrapParams='descriptorpoolcreateinfo->poolSizeCount, descriptorpoolcreateinfo->pPoolSizes', count='poolSizeCount')
)

Struct(name='VkDescriptorPoolInlineUniformBlockCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='maxInlineUniformBlockBindings', type='uint32_t')
)

Struct(name='VkDescriptorPoolInlineUniformBlockCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='maxInlineUniformBlockBindings', type='uint32_t')
)

Struct(name='VkDescriptorPoolSize_', enabled=True, declareArray=True,
var1=VarDef(name='type', type='VkDescriptorType'),
var2=VarDef(name='descriptorCount', type='uint32_t')
)

Struct(name='VkDescriptorSetAllocateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='descriptorPool', type='VkDescriptorPool'),
var4=VarDef(name='descriptorSetCount', type='uint32_t'),
var5=VarDef(name='pSetLayouts', type='const VkDescriptorSetLayout*', wrapType='CVkDescriptorSetLayout::CSArray', wrapParams='descriptorsetallocateinfo->descriptorSetCount, descriptorsetallocateinfo->pSetLayouts', count='descriptorSetCount')
)

Struct(name='VkDescriptorSetLayoutBinding_', enabled=True, declareArray=True,
var1=VarDef(name='binding', type='uint32_t'),
var2=VarDef(name='descriptorType', type='VkDescriptorType'),
var3=VarDef(name='descriptorCount', type='uint32_t'),
var4=VarDef(name='stageFlags', type='VkShaderStageFlags'),
var5=VarDef(name='pImmutableSamplers', type='const VkSampler*', wrapType='CVkSampler::CSArray', wrapParams='descriptorsetlayoutbinding->descriptorCount, descriptorsetlayoutbinding->pImmutableSamplers', count='descriptorCount')
)

Struct(name='VkDescriptorSetLayoutBindingFlagsCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='bindingCount', type='uint32_t'),
var4=VarDef(name='pBindingFlags', type='const VkDescriptorBindingFlags*', wrapType='Cuint32_t::CSArray', wrapParams='descriptorsetlayoutbindingflagscreateinfo->bindingCount, descriptorsetlayoutbindingflagscreateinfo->pBindingFlags', count='bindingCount')
)

Struct(name='VkDescriptorSetLayoutCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDescriptorSetLayoutCreateFlags'),
var4=VarDef(name='bindingCount', type='uint32_t'),
var5=VarDef(name='pBindings', type='const VkDescriptorSetLayoutBinding*', wrapType='CVkDescriptorSetLayoutBindingArray', wrapParams='descriptorsetlayoutcreateinfo->bindingCount, descriptorsetlayoutcreateinfo->pBindings', count='bindingCount')
)

Struct(name='VkDescriptorSetLayoutSupport_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='supported', type='VkBool32')
)

Struct(name='VkDescriptorSetVariableDescriptorCountAllocateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='descriptorSetCount', type='uint32_t'),
var4=VarDef(name='pDescriptorCounts', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='descriptorsetvariabledescriptorcountallocateinfo->descriptorSetCount, descriptorsetvariabledescriptorcountallocateinfo->pDescriptorCounts', count='descriptorSetCount')
)

Struct(name='VkDescriptorSetVariableDescriptorCountLayoutSupport_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxVariableDescriptorCount', type='uint32_t')
)

Struct(name='VkDescriptorUpdateTemplateCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDescriptorUpdateTemplateCreateFlags'),
var4=VarDef(name='descriptorUpdateEntryCount', type='uint32_t'),
var5=VarDef(name='pDescriptorUpdateEntries', type='const VkDescriptorUpdateTemplateEntry*', wrapType='CVkDescriptorUpdateTemplateEntryArray', wrapParams='descriptorupdatetemplatecreateinfo->descriptorUpdateEntryCount, descriptorupdatetemplatecreateinfo->pDescriptorUpdateEntries', count='descriptorUpdateEntryCount'),
var6=VarDef(name='templateType', type='VkDescriptorUpdateTemplateType'),
var7=VarDef(name='descriptorSetLayout', type='VkDescriptorSetLayout'),
var8=VarDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
var9=VarDef(name='pipelineLayout', type='VkPipelineLayout'),
var10=VarDef(name='set', type='uint32_t')
)

Struct(name='VkDescriptorUpdateTemplateEntry_', enabled=True, declareArray=True,
var1=VarDef(name='dstBinding', type='uint32_t'),
var2=VarDef(name='dstArrayElement', type='uint32_t'),
var3=VarDef(name='descriptorCount', type='uint32_t'),
var4=VarDef(name='descriptorType', type='VkDescriptorType'),
var5=VarDef(name='offset', type='size_t'),
var6=VarDef(name='stride', type='size_t')
)

Struct(name='VkDescriptorUpdateTemplateEntryKHR_', enabled=False,
var1=VarDef(name='dstBinding', type='uint32_t'),
var2=VarDef(name='dstArrayElement', type='uint32_t'),
var3=VarDef(name='descriptorCount', type='uint32_t'),
var4=VarDef(name='descriptorType', type='VkDescriptorType'),
var5=VarDef(name='offset', type='size_t'),
var6=VarDef(name='stride', type='size_t')
)

Struct(name='VkDeviceAddressBindingCallbackDataEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkDeviceAddressBindingFlagsEXT'),
var4=VarDef(name='baseAddress', type='VkDeviceAddress'),
var5=VarDef(name='size', type='VkDeviceSize'),
var6=VarDef(name='bindingType', type='VkDeviceAddressBindingTypeEXT')
)

Struct(name='VkDeviceBufferMemoryRequirements_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pCreateInfo', type='const VkBufferCreateInfo*')
)

Struct(name='VkDeviceCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDeviceCreateFlags'),
var4=VarDef(name='queueCreateInfoCount', type='uint32_t'),
var5=VarDef(name='pQueueCreateInfos', type='const VkDeviceQueueCreateInfo*', wrapType='CVkDeviceQueueCreateInfoArray', wrapParams='devicecreateinfo->queueCreateInfoCount, devicecreateinfo->pQueueCreateInfos', count='queueCreateInfoCount'),
var6=VarDef(name='enabledLayerCount', type='uint32_t'),
var7=VarDef(name='ppEnabledLayerNames', type='const char*const*', wrapType='CStringArray', wrapParams='devicecreateinfo->enabledLayerCount, (const char**)devicecreateinfo->ppEnabledLayerNames', count='enabledLayerCount'),
var8=VarDef(name='enabledExtensionCount', type='uint32_t'),
var9=VarDef(name='ppEnabledExtensionNames', type='const char*const*', wrapType='CStringArray', wrapParams='devicecreateinfo->enabledExtensionCount, (const char**)devicecreateinfo->ppEnabledExtensionNames', count='enabledExtensionCount'),
var10=VarDef(name='pEnabledFeatures', type='const VkPhysicalDeviceFeatures*', wrapType='CVkPhysicalDeviceFeaturesArray',wrapParams='1, devicecreateinfo->pEnabledFeatures')
)

Struct(name='VkDeviceDeviceMemoryReportCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDeviceMemoryReportFlagsEXT'),
var4=VarDef(name='pfnUserCallback', type='PFN_vkDeviceMemoryReportCallbackEXT'),
var5=VarDef(name='pUserData', type='void*')
)

Struct(name='VkDeviceDiagnosticsConfigCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDeviceDiagnosticsConfigFlagsNV')
)

Struct(name='VkDeviceEventInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='deviceEvent', type='VkDeviceEventTypeEXT')
)

Struct(name='VkDeviceFaultAddressInfoEXT_', enabled=False,
var1=VarDef(name='addressType', type='VkDeviceFaultAddressTypeEXT'),
var2=VarDef(name='reportedAddress', type='VkDeviceAddress'),
var3=VarDef(name='addressPrecision', type='VkDeviceSize')
)

Struct(name='VkDeviceFaultCountsEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='addressInfoCount', type='uint32_t'),
var4=VarDef(name='vendorInfoCount', type='uint32_t'),
var5=VarDef(name='vendorBinarySize', type='VkDeviceSize')
)

Struct(name='VkDeviceFaultInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='description', type='char[256]'),
var4=VarDef(name='pAddressInfos', type='VkDeviceFaultAddressInfoEXT*'),
var5=VarDef(name='pVendorInfos', type='VkDeviceFaultVendorInfoEXT*'),
var6=VarDef(name='pVendorBinaryData', type='void*')
)

Struct(name='VkDeviceFaultVendorBinaryHeaderVersionOneEXT_', enabled=False,
var1=VarDef(name='headerSize', type='uint32_t'),
var2=VarDef(name='headerVersion', type='VkDeviceFaultVendorBinaryHeaderVersionEXT'),
var3=VarDef(name='vendorID', type='uint32_t'),
var4=VarDef(name='deviceID', type='uint32_t'),
var5=VarDef(name='driverVersion', type='uint32_t'),
var6=VarDef(name='pipelineCacheUUID', type='uint8_t[16]'),
var7=VarDef(name='applicationNameOffset', type='uint32_t'),
var8=VarDef(name='applicationVersion', type='uint32_t'),
var9=VarDef(name='engineNameOffset', type='uint32_t'),
var10=VarDef(name='engineVersion', type='uint32_t'),
var11=VarDef(name='apiVersion', type='uint32_t')
)

Struct(name='VkDeviceFaultVendorInfoEXT_', enabled=False,
var1=VarDef(name='description', type='char[256]'),
var2=VarDef(name='vendorFaultCode', type='uint64_t'),
var3=VarDef(name='vendorFaultData', type='uint64_t')
)

Struct(name='VkDeviceGroupBindSparseInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='resourceDeviceIndex', type='uint32_t'),
var4=VarDef(name='memoryDeviceIndex', type='uint32_t')
)

Struct(name='VkDeviceGroupCommandBufferBeginInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='deviceMask', type='uint32_t')
)

Struct(name='VkDeviceGroupDeviceCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='physicalDeviceCount', type='uint32_t'),
var4=VarDef(name='pPhysicalDevices', type='const VkPhysicalDevice*', count='physicalDeviceCount')
)

Struct(name='VkDeviceGroupPresentCapabilitiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentMask', type='uint32_t[32]', count='32'),
var4=VarDef(name='modes', type='VkDeviceGroupPresentModeFlagsKHR')
)

Struct(name='VkDeviceGroupPresentInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchainCount', type='uint32_t'),
var4=VarDef(name='pDeviceMasks', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='devicegrouppresentinfokhr->swapchainCount, devicegrouppresentinfokhr->pDeviceMasks', count='swapchainCount'),
var5=VarDef(name='mode', type='VkDeviceGroupPresentModeFlagBitsKHR')
)

Struct(name='VkDeviceGroupRenderPassBeginInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='deviceMask', type='uint32_t'),
var4=VarDef(name='deviceRenderAreaCount', type='uint32_t'),
var5=VarDef(name='pDeviceRenderAreas', type='const VkRect2D*', count='deviceRenderAreaCount')
)

Struct(name='VkDeviceGroupSubmitInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphoreDeviceIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='devicegroupsubmitinfo->waitSemaphoreCount, devicegroupsubmitinfo->pWaitSemaphoreDeviceIndices', count='waitSemaphoreCount'),
var5=VarDef(name='commandBufferCount', type='uint32_t'),
var6=VarDef(name='pCommandBufferDeviceMasks', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='devicegroupsubmitinfo->commandBufferCount, devicegroupsubmitinfo->pCommandBufferDeviceMasks', count='commandBufferCount'),
var7=VarDef(name='signalSemaphoreCount', type='uint32_t'),
var8=VarDef(name='pSignalSemaphoreDeviceIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='devicegroupsubmitinfo->signalSemaphoreCount, devicegroupsubmitinfo->pSignalSemaphoreDeviceIndices', count='signalSemaphoreCount')
)

Struct(name='VkDeviceGroupSwapchainCreateInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='modes', type='VkDeviceGroupPresentModeFlagsKHR')
)

Struct(name='VkDeviceImageMemoryRequirements_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pCreateInfo', type='const VkImageCreateInfo*'),
var4=VarDef(name='planeAspect', type='VkImageAspectFlagBits')
)

Struct(name='VkDeviceMemoryOpaqueCaptureAddressInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='memory', type='VkDeviceMemory')
)

Struct(name='VkDeviceMemoryOverallocationCreateInfoAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='overallocationBehavior', type='VkMemoryOverallocationBehaviorAMD')
)

Struct(name='VkDeviceMemoryReportCallbackDataEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkDeviceMemoryReportFlagsEXT'),
var4=VarDef(name='type', type='VkDeviceMemoryReportEventTypeEXT'),
var5=VarDef(name='memoryObjectId', type='uint64_t'),
var6=VarDef(name='size', type='VkDeviceSize'),
var7=VarDef(name='objectType', type='VkObjectType'),
var8=VarDef(name='objectHandle', type='uint64_t'),
var9=VarDef(name='heapIndex', type='uint32_t')
)

Struct(name='VkDeviceObjectReservationCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipelineCacheCreateInfoCount', type='uint32_t'),
var4=VarDef(name='pPipelineCacheCreateInfos', type='const VkPipelineCacheCreateInfo*'),
var5=VarDef(name='pipelinePoolSizeCount', type='uint32_t'),
var6=VarDef(name='pPipelinePoolSizes', type='const VkPipelinePoolSize*'),
var7=VarDef(name='semaphoreRequestCount', type='uint32_t'),
var8=VarDef(name='commandBufferRequestCount', type='uint32_t'),
var9=VarDef(name='fenceRequestCount', type='uint32_t'),
var10=VarDef(name='deviceMemoryRequestCount', type='uint32_t'),
var11=VarDef(name='bufferRequestCount', type='uint32_t'),
var12=VarDef(name='imageRequestCount', type='uint32_t'),
var13=VarDef(name='eventRequestCount', type='uint32_t'),
var14=VarDef(name='queryPoolRequestCount', type='uint32_t'),
var15=VarDef(name='bufferViewRequestCount', type='uint32_t'),
var16=VarDef(name='imageViewRequestCount', type='uint32_t'),
var17=VarDef(name='layeredImageViewRequestCount', type='uint32_t'),
var18=VarDef(name='pipelineCacheRequestCount', type='uint32_t'),
var19=VarDef(name='pipelineLayoutRequestCount', type='uint32_t'),
var20=VarDef(name='renderPassRequestCount', type='uint32_t'),
var21=VarDef(name='graphicsPipelineRequestCount', type='uint32_t'),
var22=VarDef(name='computePipelineRequestCount', type='uint32_t'),
var23=VarDef(name='descriptorSetLayoutRequestCount', type='uint32_t'),
var24=VarDef(name='samplerRequestCount', type='uint32_t'),
var25=VarDef(name='descriptorPoolRequestCount', type='uint32_t'),
var26=VarDef(name='descriptorSetRequestCount', type='uint32_t'),
var27=VarDef(name='framebufferRequestCount', type='uint32_t'),
var28=VarDef(name='commandPoolRequestCount', type='uint32_t'),
var29=VarDef(name='samplerYcbcrConversionRequestCount', type='uint32_t'),
var30=VarDef(name='surfaceRequestCount', type='uint32_t'),
var31=VarDef(name='swapchainRequestCount', type='uint32_t'),
var32=VarDef(name='displayModeRequestCount', type='uint32_t'),
var33=VarDef(name='subpassDescriptionRequestCount', type='uint32_t'),
var34=VarDef(name='attachmentDescriptionRequestCount', type='uint32_t'),
var35=VarDef(name='descriptorSetLayoutBindingRequestCount', type='uint32_t'),
var36=VarDef(name='descriptorSetLayoutBindingLimit', type='uint32_t'),
var37=VarDef(name='maxImageViewMipLevels', type='uint32_t'),
var38=VarDef(name='maxImageViewArrayLayers', type='uint32_t'),
var39=VarDef(name='maxLayeredImageViewMipLevels', type='uint32_t'),
var40=VarDef(name='maxOcclusionQueriesPerPool', type='uint32_t'),
var41=VarDef(name='maxPipelineStatisticsQueriesPerPool', type='uint32_t'),
var42=VarDef(name='maxTimestampQueriesPerPool', type='uint32_t'),
var43=VarDef(name='maxImmutableSamplersPerDescriptorSetLayout', type='uint32_t')
)

Struct(name='VkDevicePrivateDataCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='privateDataSlotRequestCount', type='uint32_t')
)

Struct(name='VkDevicePrivateDataCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='privateDataSlotRequestCount', type='uint32_t')
)

Struct(name='VkDeviceOrHostAddressKHR_', type='union', enabled=False,
var1=VarDef(name='deviceAddress', type='VkDeviceAddress'),
var2=VarDef(name='hostAddress', type='void *'),
)

Struct(name='VkDeviceOrHostAddressConstKHR_', type='union', enabled=False,
var1=VarDef(name='deviceAddress', type='VkDeviceAddress'),
var2=VarDef(name='hostAddress', type='const void *'),
)

Struct(name='VkDeviceQueueCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDeviceQueueCreateFlags'),
var4=VarDef(name='queueFamilyIndex', type='uint32_t'),
var5=VarDef(name='queueCount', type='uint32_t'),
var6=VarDef(name='pQueuePriorities', type='const float*', wrapType="Cfloat::CSArray", wrapParams='devicequeuecreateinfo->queueCount, devicequeuecreateinfo->pQueuePriorities', count='queueCount')
)

Struct(name='VkDeviceQueueGlobalPriorityCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='globalPriority', type='VkQueueGlobalPriorityKHR')
)

Struct(name='VkDeviceQueueGlobalPriorityCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='globalPriority', type='VkQueueGlobalPriorityKHR')
)

Struct(name='VkDeviceQueueInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDeviceQueueCreateFlags'),
var4=VarDef(name='queueFamilyIndex', type='uint32_t'),
var5=VarDef(name='queueIndex', type='uint32_t')
)

Struct(name='VkDirectDriverLoadingInfoLUNARG_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkDirectDriverLoadingFlagsLUNARG'),
var4=VarDef(name='pfnGetInstanceProcAddr', type='PFN_vkGetInstanceProcAddrLUNARG')
)

Struct(name='VkDirectDriverLoadingListLUNARG_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='mode', type='VkDirectDriverLoadingModeLUNARG'),
var4=VarDef(name='driverCount', type='uint32_t'),
var5=VarDef(name='pDrivers', type='const VkDirectDriverLoadingInfoLUNARG*')
)

#Struct(name='VkDirectFBSurfaceCreateInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='flags', type='VkDirectFBSurfaceCreateFlagsEXT'),
#var4=VarDef(name='dfb', type='IDirectFB*'),
#var5=VarDef(name='surface', type='IDirectFBSurface*')
#)

Struct(name='VkDispatchIndirectCommand_', enabled=False,
var1=VarDef(name='x', type='uint32_t'),
var2=VarDef(name='y', type='uint32_t'),
var3=VarDef(name='z', type='uint32_t')
)

Struct(name='VkDisplayEventInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='displayEvent', type='VkDisplayEventTypeEXT')
)

Struct(name='VkDisplayModeCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDisplayModeCreateFlagsKHR'),
var4=VarDef(name='parameters', type='VkDisplayModeParametersKHR')
)

Struct(name='VkDisplayModeParametersKHR_', enabled=False,
var1=VarDef(name='visibleRegion', type='VkExtent2D'),
var2=VarDef(name='refreshRate', type='uint32_t')
)

Struct(name='VkDisplayModeProperties2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='displayModeProperties', type='VkDisplayModePropertiesKHR')
)

Struct(name='VkDisplayModePropertiesKHR_', enabled=False,
var1=VarDef(name='displayMode', type='VkDisplayModeKHR'),
var2=VarDef(name='parameters', type='VkDisplayModeParametersKHR')
)

Struct(name='VkDisplayNativeHdrSurfaceCapabilitiesAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='localDimmingSupport', type='VkBool32')
)

Struct(name='VkDisplayPlaneCapabilities2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='capabilities', type='VkDisplayPlaneCapabilitiesKHR')
)

Struct(name='VkDisplayPlaneCapabilitiesKHR_', enabled=False,
var1=VarDef(name='supportedAlpha', type='VkDisplayPlaneAlphaFlagsKHR'),
var2=VarDef(name='minSrcPosition', type='VkOffset2D'),
var3=VarDef(name='maxSrcPosition', type='VkOffset2D'),
var4=VarDef(name='minSrcExtent', type='VkExtent2D'),
var5=VarDef(name='maxSrcExtent', type='VkExtent2D'),
var6=VarDef(name='minDstPosition', type='VkOffset2D'),
var7=VarDef(name='maxDstPosition', type='VkOffset2D'),
var8=VarDef(name='minDstExtent', type='VkExtent2D'),
var9=VarDef(name='maxDstExtent', type='VkExtent2D')
)

Struct(name='VkDisplayPlaneInfo2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='mode', type='VkDisplayModeKHR'),
var4=VarDef(name='planeIndex', type='uint32_t')
)

Struct(name='VkDisplayPlaneProperties2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='displayPlaneProperties', type='VkDisplayPlanePropertiesKHR')
)

Struct(name='VkDisplayPlanePropertiesKHR_', enabled=False,
var1=VarDef(name='currentDisplay', type='VkDisplayKHR'),
var2=VarDef(name='currentStackIndex', type='uint32_t')
)

Struct(name='VkDisplayPowerInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='powerState', type='VkDisplayPowerStateEXT')
)

Struct(name='VkDisplayPresentInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcRect', type='VkRect2D'),
var4=VarDef(name='dstRect', type='VkRect2D'),
var5=VarDef(name='persistent', type='VkBool32')
)

Struct(name='VkDisplayProperties2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='displayProperties', type='VkDisplayPropertiesKHR')
)

Struct(name='VkDisplayPropertiesKHR_', enabled=False,
var1=VarDef(name='display', type='VkDisplayKHR'),
var2=VarDef(name='displayName', type='const char*'),
var3=VarDef(name='physicalDimensions', type='VkExtent2D'),
var4=VarDef(name='physicalResolution', type='VkExtent2D'),
var5=VarDef(name='supportedTransforms', type='VkSurfaceTransformFlagsKHR'),
var6=VarDef(name='planeReorderPossible', type='VkBool32'),
var7=VarDef(name='persistentContent', type='VkBool32')
)

Struct(name='VkDisplaySurfaceCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkDisplaySurfaceCreateFlagsKHR'),
var4=VarDef(name='displayMode', type='VkDisplayModeKHR'),
var5=VarDef(name='planeIndex', type='uint32_t'),
var6=VarDef(name='planeStackIndex', type='uint32_t'),
var7=VarDef(name='transform', type='VkSurfaceTransformFlagBitsKHR'),
var8=VarDef(name='globalAlpha', type='float'),
var9=VarDef(name='alphaMode', type='VkDisplayPlaneAlphaFlagBitsKHR'),
var10=VarDef(name='imageExtent', type='VkExtent2D')
)

Struct(name='VkDrawIndexedIndirectCommand_', enabled=False,
var1=VarDef(name='indexCount', type='uint32_t'),
var2=VarDef(name='instanceCount', type='uint32_t'),
var3=VarDef(name='firstIndex', type='uint32_t'),
var4=VarDef(name='vertexOffset', type='int32_t'),
var5=VarDef(name='firstInstance', type='uint32_t')
)

Struct(name='VkDrawIndirectCommand_', enabled=False,
var1=VarDef(name='vertexCount', type='uint32_t'),
var2=VarDef(name='instanceCount', type='uint32_t'),
var3=VarDef(name='firstVertex', type='uint32_t'),
var4=VarDef(name='firstInstance', type='uint32_t')
)

Struct(name='VkDrawMeshTasksIndirectCommandEXT_', enabled=True,
var1=VarDef(name='groupCountX', type='uint32_t'),
var2=VarDef(name='groupCountY', type='uint32_t'),
var3=VarDef(name='groupCountZ', type='uint32_t')
)

Struct(name='VkDrawMeshTasksIndirectCommandNV_', enabled=False,
var1=VarDef(name='taskCount', type='uint32_t'),
var2=VarDef(name='firstTask', type='uint32_t')
)

Struct(name='VkDrmFormatModifierProperties2EXT_', enabled=False,
var1=VarDef(name='drmFormatModifier', type='uint64_t'),
var2=VarDef(name='drmFormatModifierPlaneCount', type='uint32_t'),
var3=VarDef(name='drmFormatModifierTilingFeatures', type='VkFormatFeatureFlags2')
)

Struct(name='VkDrmFormatModifierPropertiesEXT_', enabled=False,
var1=VarDef(name='drmFormatModifier', type='uint64_t'),
var2=VarDef(name='drmFormatModifierPlaneCount', type='uint32_t'),
var3=VarDef(name='drmFormatModifierTilingFeatures', type='VkFormatFeatureFlags')
)

Struct(name='VkDrmFormatModifierPropertiesList2EXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='drmFormatModifierCount', type='uint32_t'),
var4=VarDef(name='pDrmFormatModifierProperties', type='VkDrmFormatModifierProperties2EXT*')
)

Struct(name='VkDrmFormatModifierPropertiesListEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='drmFormatModifierCount', type='uint32_t'),
var4=VarDef(name='pDrmFormatModifierProperties', type='VkDrmFormatModifierPropertiesEXT*')
)

Struct(name='VkEventCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkEventCreateFlags')
)

Struct(name='VkExportFenceCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalFenceHandleTypeFlags')
)

Struct(name='VkExportFenceWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pAttributes', type='const SECURITY_ATTRIBUTES*'),
var4=VarDef(name='dwAccess', type='DWORD'),
var5=VarDef(name='name', type='LPCWSTR')
)

Struct(name='VkExportMemoryAllocateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalMemoryHandleTypeFlags')
)

Struct(name='VkExportMemoryAllocateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalMemoryHandleTypeFlagsNV')
)

Struct(name='VkExportMemoryWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pAttributes', type='const SECURITY_ATTRIBUTES*'),
var4=VarDef(name='dwAccess', type='DWORD'),
var5=VarDef(name='name', type='LPCWSTR')
)

Struct(name='VkExportMemoryWin32HandleInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pAttributes', type='const SECURITY_ATTRIBUTES*'),
var4=VarDef(name='dwAccess', type='DWORD')
)

Struct(name='VkExportSemaphoreCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalSemaphoreHandleTypeFlags')
)

Struct(name='VkExportSemaphoreWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pAttributes', type='const SECURITY_ATTRIBUTES*'),
var4=VarDef(name='dwAccess', type='DWORD'),
var5=VarDef(name='name', type='LPCWSTR')
)

Struct(name='VkExtensionProperties_', enabled=True, declareArray=True,
var1=VarDef(name='extensionName', type='char[256]'),
var2=VarDef(name='specVersion', type='uint32_t')
)

Struct(name='VkExtent2D_', enabled=True,
var1=VarDef(name='width', type='uint32_t'),
var2=VarDef(name='height', type='uint32_t')
)

Struct(name='VkExtent3D_', enabled=True,
var1=VarDef(name='width', type='uint32_t'),
var2=VarDef(name='height', type='uint32_t'),
var3=VarDef(name='depth', type='uint32_t')
)

Struct(name='VkExternalBufferProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='externalMemoryProperties', type='VkExternalMemoryProperties')
)

Struct(name='VkExternalFenceProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='exportFromImportedHandleTypes', type='VkExternalFenceHandleTypeFlags'),
var4=VarDef(name='compatibleHandleTypes', type='VkExternalFenceHandleTypeFlags'),
var5=VarDef(name='externalFenceFeatures', type='VkExternalFenceFeatureFlags')
)

Struct(name='VkExternalFormatANDROID_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='externalFormat', type='uint64_t')
)

Struct(name='VkExternalImageFormatProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='externalMemoryProperties', type='VkExternalMemoryProperties')
)

Struct(name='VkExternalImageFormatPropertiesNV_', enabled=False,
var1=VarDef(name='imageFormatProperties', type='VkImageFormatProperties'),
var2=VarDef(name='externalMemoryFeatures', type='VkExternalMemoryFeatureFlagsNV'),
var3=VarDef(name='exportFromImportedHandleTypes', type='VkExternalMemoryHandleTypeFlagsNV'),
var4=VarDef(name='compatibleHandleTypes', type='VkExternalMemoryHandleTypeFlagsNV')
)

Struct(name='VkExternalMemoryBufferCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalMemoryHandleTypeFlags')
)

Struct(name='VkExternalMemoryImageCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalMemoryHandleTypeFlags')
)

Struct(name='VkExternalMemoryImageCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleTypes', type='VkExternalMemoryHandleTypeFlagsNV')
)

Struct(name='VkExternalMemoryProperties_', enabled=False,
var1=VarDef(name='externalMemoryFeatures', type='VkExternalMemoryFeatureFlags'),
var2=VarDef(name='exportFromImportedHandleTypes', type='VkExternalMemoryHandleTypeFlags'),
var3=VarDef(name='compatibleHandleTypes', type='VkExternalMemoryHandleTypeFlags')
)

Struct(name='VkExternalSemaphoreProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='exportFromImportedHandleTypes', type='VkExternalSemaphoreHandleTypeFlags'),
var4=VarDef(name='compatibleHandleTypes', type='VkExternalSemaphoreHandleTypeFlags'),
var5=VarDef(name='externalSemaphoreFeatures', type='VkExternalSemaphoreFeatureFlags')
)

Struct(name='VkFaultCallbackInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='faultCount', type='uint32_t'),
var4=VarDef(name='pFaults', type='VkFaultData*'),
var5=VarDef(name='pfnFaultCallback', type='PFN_vkFaultCallbackFunction')
)

Struct(name='VkFaultData_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='faultLevel', type='VkFaultLevel'),
var4=VarDef(name='faultType', type='VkFaultType')
)

Struct(name='VkFenceCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkFenceCreateFlags')
)

Struct(name='VkFenceGetFdInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='fence', type='VkFence'),
var4=VarDef(name='handleType', type='VkExternalFenceHandleTypeFlagBits')
)

Struct(name='VkFenceGetWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='fence', type='VkFence'),
var4=VarDef(name='handleType', type='VkExternalFenceHandleTypeFlagBits')
)

Struct(name='VkFilterCubicImageViewImageFormatPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='filterCubic', type='VkBool32'),
var4=VarDef(name='filterCubicMinmax', type='VkBool32')
)

Struct(name='VkFormatProperties_', enabled=False,
var1=VarDef(name='linearTilingFeatures', type='VkFormatFeatureFlags'),
var2=VarDef(name='optimalTilingFeatures', type='VkFormatFeatureFlags'),
var3=VarDef(name='bufferFeatures', type='VkFormatFeatureFlags')
)

Struct(name='VkFormatProperties2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='formatProperties', type='VkFormatProperties')
)

Struct(name='VkFormatProperties3_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='linearTilingFeatures', type='VkFormatFeatureFlags2'),
var4=VarDef(name='optimalTilingFeatures', type='VkFormatFeatureFlags2'),
var5=VarDef(name='bufferFeatures', type='VkFormatFeatureFlags2')
)

Struct(name='VkFormatProperties3KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='linearTilingFeatures', type='VkFormatFeatureFlags2'),
var4=VarDef(name='optimalTilingFeatures', type='VkFormatFeatureFlags2'),
var5=VarDef(name='bufferFeatures', type='VkFormatFeatureFlags2')
)

Struct(name='VkFragmentShadingRateAttachmentInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pFragmentShadingRateAttachment', type='const VkAttachmentReference2*'),
var4=VarDef(name='shadingRateAttachmentTexelSize', type='VkExtent2D')
)

Struct(name='VkFramebufferAttachmentImageInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkImageCreateFlags'),
var4=VarDef(name='usage', type='VkImageUsageFlags'),
var5=VarDef(name='width', type='uint32_t'),
var6=VarDef(name='height', type='uint32_t'),
var7=VarDef(name='layerCount', type='uint32_t'),
var8=VarDef(name='viewFormatCount', type='uint32_t'),
var9=VarDef(name='pViewFormats', type='const VkFormat*', wrapType='CVkFormat::CSArray', wrapParams='framebufferattachmentimageinfo->viewFormatCount, framebufferattachmentimageinfo->pViewFormats', count='viewFormatCount')
)

Struct(name='VkFramebufferAttachmentsCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='attachmentImageInfoCount', type='uint32_t'),
var4=VarDef(name='pAttachmentImageInfos', type='const VkFramebufferAttachmentImageInfo*', wrapType='CVkFramebufferAttachmentImageInfoArray', wrapParams='framebufferattachmentscreateinfo->attachmentImageInfoCount, framebufferattachmentscreateinfo->pAttachmentImageInfos', count='attachmentImageInfoCount')
)

Struct(name='VkFramebufferCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkFramebufferCreateFlags'),
var4=VarDef(name='renderPass', type='VkRenderPass'),
var5=VarDef(name='attachmentCount', type='uint32_t'),
var6=VarDef(name='pAttachments', type='const VkImageView*', wrapType='CVkImageView::CSArray', wrapParams='framebuffercreateinfo->attachmentCount, framebuffercreateinfo->pAttachments', count='attachmentCount'),
var7=VarDef(name='width', type='uint32_t'),
var8=VarDef(name='height', type='uint32_t'),
var9=VarDef(name='layers', type='uint32_t')
)

Struct(name='VkFramebufferMixedSamplesCombinationNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='coverageReductionMode', type='VkCoverageReductionModeNV'),
var4=VarDef(name='rasterizationSamples', type='VkSampleCountFlagBits'),
var5=VarDef(name='depthStencilSamples', type='VkSampleCountFlags'),
var6=VarDef(name='colorSamples', type='VkSampleCountFlags')
)

Struct(name='VkGeneratedCommandsInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
var4=VarDef(name='pipeline', type='VkPipeline'),
var5=VarDef(name='indirectCommandsLayout', type='VkIndirectCommandsLayoutNV'),
var6=VarDef(name='streamCount', type='uint32_t'),
var7=VarDef(name='pStreams', type='const VkIndirectCommandsStreamNV*'),
var8=VarDef(name='sequencesCount', type='uint32_t'),
var9=VarDef(name='preprocessBuffer', type='VkBuffer'),
var10=VarDef(name='preprocessOffset', type='VkDeviceSize'),
var11=VarDef(name='preprocessSize', type='VkDeviceSize'),
var12=VarDef(name='sequencesCountBuffer', type='VkBuffer'),
var13=VarDef(name='sequencesCountOffset', type='VkDeviceSize'),
var14=VarDef(name='sequencesIndexBuffer', type='VkBuffer'),
var15=VarDef(name='sequencesIndexOffset', type='VkDeviceSize')
)

Struct(name='VkGeneratedCommandsMemoryRequirementsInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
var4=VarDef(name='pipeline', type='VkPipeline'),
var5=VarDef(name='indirectCommandsLayout', type='VkIndirectCommandsLayoutNV'),
var6=VarDef(name='maxSequencesCount', type='uint32_t')
)

Struct(name='VkGeometryAABBNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='aabbData', type='VkBuffer'),
var4=VarDef(name='numAABBs', type='uint32_t'),
var5=VarDef(name='stride', type='uint32_t'),
var6=VarDef(name='offset', type='VkDeviceSize')
)

Struct(name='VkGeometryDataNV_', enabled=False,
var1=VarDef(name='triangles', type='VkGeometryTrianglesNV'),
var2=VarDef(name='aabbs', type='VkGeometryAABBNV')
)

Struct(name='VkGeometryNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='geometryType', type='VkGeometryTypeKHR'),
var4=VarDef(name='geometry', type='VkGeometryDataNV'),
var5=VarDef(name='flags', type='VkGeometryFlagsKHR')
)

Struct(name='VkGeometryTrianglesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='vertexData', type='VkBuffer'),
var4=VarDef(name='vertexOffset', type='VkDeviceSize'),
var5=VarDef(name='vertexCount', type='uint32_t'),
var6=VarDef(name='vertexStride', type='VkDeviceSize'),
var7=VarDef(name='vertexFormat', type='VkFormat'),
var8=VarDef(name='indexData', type='VkBuffer'),
var9=VarDef(name='indexOffset', type='VkDeviceSize'),
var10=VarDef(name='indexCount', type='uint32_t'),
var11=VarDef(name='indexType', type='VkIndexType'),
var12=VarDef(name='transformData', type='VkBuffer'),
var13=VarDef(name='transformOffset', type='VkDeviceSize')
)

Struct(name='VkGraphicsPipelineCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCreateFlags'),
var4=VarDef(name='stageCount', type='uint32_t'),
var5=VarDef(name='pStages', type='const VkPipelineShaderStageCreateInfo*', wrapType='CVkPipelineShaderStageCreateInfoArray', wrapParams='graphicspipelinecreateinfo->stageCount, graphicspipelinecreateinfo->pStages', count='stageCount'),
var6=VarDef(name='pVertexInputState', type='const VkPipelineVertexInputStateCreateInfo*', wrapType='CVkPipelineVertexInputStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pVertexInputState'),
var7=VarDef(name='pInputAssemblyState', type='const VkPipelineInputAssemblyStateCreateInfo*', wrapType='CVkPipelineInputAssemblyStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pInputAssemblyState'),
var8=VarDef(name='pTessellationState', type='const VkPipelineTessellationStateCreateInfo*', wrapType='CVkPipelineTessellationStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pTessellationState'),
var9=VarDef(name='pViewportState', type='const VkPipelineViewportStateCreateInfo*', wrapType='CVkPipelineViewportStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pViewportState'),
var10=VarDef(name='pRasterizationState', type='const VkPipelineRasterizationStateCreateInfo*', wrapType='CVkPipelineRasterizationStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pRasterizationState'),
var11=VarDef(name='pMultisampleState', type='const VkPipelineMultisampleStateCreateInfo*', wrapType='CVkPipelineMultisampleStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pMultisampleState'),
var12=VarDef(name='pDepthStencilState', type='const VkPipelineDepthStencilStateCreateInfo*', wrapType='CVkPipelineDepthStencilStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pDepthStencilState'),
var13=VarDef(name='pColorBlendState', type='const VkPipelineColorBlendStateCreateInfo*', wrapType='CVkPipelineColorBlendStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pColorBlendState'),
var14=VarDef(name='pDynamicState', type='const VkPipelineDynamicStateCreateInfo*', wrapType='CVkPipelineDynamicStateCreateInfoArray', wrapParams='1, graphicspipelinecreateinfo->pDynamicState'),
var15=VarDef(name='layout', type='VkPipelineLayout'),
var16=VarDef(name='renderPass', type='VkRenderPass'),
var17=VarDef(name='subpass', type='uint32_t'),
var18=VarDef(name='basePipelineHandle', type='VkPipeline'),
var19=VarDef(name='basePipelineIndex', type='int32_t')
)

Struct(name='VkGraphicsPipelineLibraryCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkGraphicsPipelineLibraryFlagsEXT')
)

Struct(name='VkGraphicsPipelineShaderGroupsCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='groupCount', type='uint32_t'),
var4=VarDef(name='pGroups', type='const VkGraphicsShaderGroupCreateInfoNV*'),
var5=VarDef(name='pipelineCount', type='uint32_t'),
var6=VarDef(name='pPipelines', type='const VkPipeline*')
)

Struct(name='VkGraphicsShaderGroupCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='stageCount', type='uint32_t'),
var4=VarDef(name='pStages', type='const VkPipelineShaderStageCreateInfo*'),
var5=VarDef(name='pVertexInputState', type='const VkPipelineVertexInputStateCreateInfo*'),
var6=VarDef(name='pTessellationState', type='const VkPipelineTessellationStateCreateInfo*')
)

Struct(name='VkHdrMetadataEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='displayPrimaryRed', type='VkXYColorEXT'),
var4=VarDef(name='displayPrimaryGreen', type='VkXYColorEXT'),
var5=VarDef(name='displayPrimaryBlue', type='VkXYColorEXT'),
var6=VarDef(name='whitePoint', type='VkXYColorEXT'),
var7=VarDef(name='maxLuminance', type='float'),
var8=VarDef(name='minLuminance', type='float'),
var9=VarDef(name='maxContentLightLevel', type='float'),
var10=VarDef(name='maxFrameAverageLightLevel', type='float')
)

Struct(name='VkHeadlessSurfaceCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkHeadlessSurfaceCreateFlagsEXT')
)

Struct(name='VkIOSSurfaceCreateInfoMVK_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkIOSSurfaceCreateFlagsMVK'),
var4=VarDef(name='pView', type='const void*')
)

Struct(name='VkImageBlit_', enabled=True, declareArray=True,
var1=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var2=VarDef(name='srcOffsets', type='VkOffset3D[2]', wrapType='CVkOffset3DArray', count='2'),
var3=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='dstOffsets', type='VkOffset3D[2]', wrapType='CVkOffset3DArray', count='2')
)

Struct(name='VkImageBlit2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='srcOffsets', type='VkOffset3D[2]', wrapType='CVkOffset3DArray', count='2'),
var5=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var6=VarDef(name='dstOffsets', type='VkOffset3D[2]', wrapType='CVkOffset3DArray', count='2')
)

Struct(name='VkImageCaptureDescriptorDataInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='image', type='VkImage')
)

Struct(name='VkImageCompressionControlEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkImageCompressionFlagsEXT'),
var4=VarDef(name='compressionControlPlaneCount', type='uint32_t'),
var5=VarDef(name='pFixedRateFlags', type='VkImageCompressionFixedRateFlagsEXT*')
)

Struct(name='VkImageCompressionPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageCompressionFlags', type='VkImageCompressionFlagsEXT'),
var4=VarDef(name='imageCompressionFixedRateFlags', type='VkImageCompressionFixedRateFlagsEXT')
)

Struct(name='VkImageCopy_', enabled=True, declareArray=True,
var1=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var2=VarDef(name='srcOffset', type='VkOffset3D'),
var3=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='dstOffset', type='VkOffset3D'),
var5=VarDef(name='extent', type='VkExtent3D')
)

Struct(name='VkImageCopy2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='srcOffset', type='VkOffset3D'),
var5=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var6=VarDef(name='dstOffset', type='VkOffset3D'),
var7=VarDef(name='extent', type='VkExtent3D')
)

Struct(name='VkImageCopy2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='srcOffset', type='VkOffset3D'),
var5=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var6=VarDef(name='dstOffset', type='VkOffset3D'),
var7=VarDef(name='extent', type='VkExtent3D')
)

Struct(name='VkImageCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkImageCreateFlags'),
var4=VarDef(name='imageType', type='VkImageType'),
var5=VarDef(name='format', type='VkFormat'),
var6=VarDef(name='extent', type='VkExtent3D'),
var7=VarDef(name='mipLevels', type='uint32_t'),
var8=VarDef(name='arrayLayers', type='uint32_t'),
var9=VarDef(name='samples', type='VkSampleCountFlagBits'),
var10=VarDef(name='tiling', type='VkImageTiling'),
var11=VarDef(name='usage', type='VkImageUsageFlags'),
var12=VarDef(name='sharingMode', type='VkSharingMode'),
var13=VarDef(name='queueFamilyIndexCount', type='uint32_t'),
var14=VarDef(name='pQueueFamilyIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='(imagecreateinfo->sharingMode == VK_SHARING_MODE_CONCURRENT) ? imagecreateinfo->queueFamilyIndexCount : 0, (imagecreateinfo->sharingMode == VK_SHARING_MODE_CONCURRENT) ? imagecreateinfo->pQueueFamilyIndices : nullptr', count='queueFamilyIndexCount', logCondition='c.sharingMode == VK_SHARING_MODE_CONCURRENT'),
var15=VarDef(name='initialLayout', type='VkImageLayout')
)

Struct(name='VkImageDrmFormatModifierExplicitCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='drmFormatModifier', type='uint64_t'),
var4=VarDef(name='drmFormatModifierPlaneCount', type='uint32_t'),
var5=VarDef(name='pPlaneLayouts', type='const VkSubresourceLayout*')
)

Struct(name='VkImageDrmFormatModifierListCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='drmFormatModifierCount', type='uint32_t'),
var4=VarDef(name='pDrmFormatModifiers', type='const uint64_t*')
)

Struct(name='VkImageDrmFormatModifierPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='drmFormatModifier', type='uint64_t')
)

Struct(name='VkImageFormatListCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='viewFormatCount', type='uint32_t'),
var4=VarDef(name='pViewFormats', type='const VkFormat*', wrapType='CVkFormat::CSArray', wrapParams='imageformatlistcreateinfo->viewFormatCount, imageformatlistcreateinfo->pViewFormats', count='viewFormatCount')
)

Struct(name='VkImageFormatProperties_', enabled=False,
var1=VarDef(name='maxExtent', type='VkExtent3D'),
var2=VarDef(name='maxMipLevels', type='uint32_t'),
var3=VarDef(name='maxArrayLayers', type='uint32_t'),
var4=VarDef(name='sampleCounts', type='VkSampleCountFlags'),
var5=VarDef(name='maxResourceSize', type='VkDeviceSize')
)

Struct(name='VkImageFormatProperties2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageFormatProperties', type='VkImageFormatProperties')
)

Struct(name='VkImageMemoryBarrier_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcAccessMask', type='VkAccessFlags'),
var4=VarDef(name='dstAccessMask', type='VkAccessFlags'),
var5=VarDef(name='oldLayout', type='VkImageLayout'),
var6=VarDef(name='newLayout', type='VkImageLayout'),
var7=VarDef(name='srcQueueFamilyIndex', type='uint32_t'),
var8=VarDef(name='dstQueueFamilyIndex', type='uint32_t'),
var9=VarDef(name='image', type='VkImage'),
var10=VarDef(name='subresourceRange', type='VkImageSubresourceRange')
)

Struct(name='VkImageMemoryBarrier2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcStageMask', type='VkPipelineStageFlags2'),
var4=VarDef(name='srcAccessMask', type='VkAccessFlags2'),
var5=VarDef(name='dstStageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='dstAccessMask', type='VkAccessFlags2'),
var7=VarDef(name='oldLayout', type='VkImageLayout'),
var8=VarDef(name='newLayout', type='VkImageLayout'),
var9=VarDef(name='srcQueueFamilyIndex', type='uint32_t'),
var10=VarDef(name='dstQueueFamilyIndex', type='uint32_t'),
var11=VarDef(name='image', type='VkImage'),
var12=VarDef(name='subresourceRange', type='VkImageSubresourceRange')
)

Struct(name='VkImageMemoryBarrier2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcStageMask', type='VkPipelineStageFlags2'),
var4=VarDef(name='srcAccessMask', type='VkAccessFlags2'),
var5=VarDef(name='dstStageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='dstAccessMask', type='VkAccessFlags2'),
var7=VarDef(name='oldLayout', type='VkImageLayout'),
var8=VarDef(name='newLayout', type='VkImageLayout'),
var9=VarDef(name='srcQueueFamilyIndex', type='uint32_t'),
var10=VarDef(name='dstQueueFamilyIndex', type='uint32_t'),
var11=VarDef(name='image', type='VkImage'),
var12=VarDef(name='subresourceRange', type='VkImageSubresourceRange')
)

Struct(name='VkImageMemoryRequirementsInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='image', type='VkImage')
)

Struct(name='VkImagePlaneMemoryRequirementsInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='planeAspect', type='VkImageAspectFlagBits')
)

Struct(name='VkImageResolve_', enabled=True, declareArray=True,
var1=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var2=VarDef(name='srcOffset', type='VkOffset3D'),
var3=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='dstOffset', type='VkOffset3D'),
var5=VarDef(name='extent', type='VkExtent3D')
)

Struct(name='VkImageResolve2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='srcOffset', type='VkOffset3D'),
var5=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var6=VarDef(name='dstOffset', type='VkOffset3D'),
var7=VarDef(name='extent', type='VkExtent3D')
)

Struct(name='VkImageResolve2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSubresource', type='VkImageSubresourceLayers'),
var4=VarDef(name='srcOffset', type='VkOffset3D'),
var5=VarDef(name='dstSubresource', type='VkImageSubresourceLayers'),
var6=VarDef(name='dstOffset', type='VkOffset3D'),
var7=VarDef(name='extent', type='VkExtent3D')
)

Struct(name='VkImageSparseMemoryRequirementsInfo2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='image', type='VkImage')
)

Struct(name='VkImageStencilUsageCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='stencilUsage', type='VkImageUsageFlags')
)

Struct(name='VkImageSubresource_', enabled=True,
var1=VarDef(name='aspectMask', type='VkImageAspectFlags'),
var2=VarDef(name='mipLevel', type='uint32_t'),
var3=VarDef(name='arrayLayer', type='uint32_t')
)

Struct(name='VkImageSubresource2EXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageSubresource', type='VkImageSubresource')
)

Struct(name='VkImageSubresourceLayers_', enabled=True,
var1=VarDef(name='aspectMask', type='VkImageAspectFlags'),
var2=VarDef(name='mipLevel', type='uint32_t'),
var3=VarDef(name='baseArrayLayer', type='uint32_t'),
var4=VarDef(name='layerCount', type='uint32_t')
)

Struct(name='VkImageSubresourceRange_', enabled=True, declareArray=True,
var1=VarDef(name='aspectMask', type='VkImageAspectFlags'),
var2=VarDef(name='baseMipLevel', type='uint32_t'),
var3=VarDef(name='levelCount', type='uint32_t'),
var4=VarDef(name='baseArrayLayer', type='uint32_t'),
var5=VarDef(name='layerCount', type='uint32_t')
)

Struct(name='VkImageSwapchainCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchain', type='VkSwapchainKHR')
)

Struct(name='VkImageViewASTCDecodeModeEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='decodeMode', type='VkFormat')
)

Struct(name='VkImageViewAddressPropertiesNVX_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceAddress', type='VkDeviceAddress'),
var4=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkImageViewCaptureDescriptorDataInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageView', type='VkImageView')
)

Struct(name='VkImageViewCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkImageViewCreateFlags'),
var4=VarDef(name='image', type='VkImage'),
var5=VarDef(name='viewType', type='VkImageViewType'),
var6=VarDef(name='format', type='VkFormat'),
var7=VarDef(name='components', type='VkComponentMapping'),
var8=VarDef(name='subresourceRange', type='VkImageSubresourceRange')
)

Struct(name='VkImageViewHandleInfoNVX_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageView', type='VkImageView'),
var4=VarDef(name='descriptorType', type='VkDescriptorType'),
var5=VarDef(name='sampler', type='VkSampler')
)

Struct(name='VkImageViewMinLodCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='minLod', type='float')
)

Struct(name='VkImageViewSlicedCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sliceOffset', type='uint32_t'),
var4=VarDef(name='sliceCount', type='uint32_t')
)

Struct(name='VkImageViewUsageCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='usage', type='VkImageUsageFlags')
)

Struct(name='VkImportFenceFdInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='fence', type='VkFence'),
var4=VarDef(name='flags', type='VkFenceImportFlags'),
var5=VarDef(name='handleType', type='VkExternalFenceHandleTypeFlagBits'),
var6=VarDef(name='fd', type='int')
)

Struct(name='VkImportFenceWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='fence', type='VkFence'),
var4=VarDef(name='flags', type='VkFenceImportFlags'),
var5=VarDef(name='handleType', type='VkExternalFenceHandleTypeFlagBits'),
var6=VarDef(name='handle', type='HANDLE'),
var7=VarDef(name='name', type='LPCWSTR')
)

Struct(name='VkImportMemoryFdInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits'),
var4=VarDef(name='fd', type='int')
)

Struct(name='VkImportMemoryHostPointerInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits'),
var4=VarDef(name='pHostPointer', type='void*')
)

Struct(name='VkImportMemoryWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits'),
var4=VarDef(name='handle', type='HANDLE'),
var5=VarDef(name='name', type='LPCWSTR')
)

Struct(name='VkImportMemoryWin32HandleInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagsNV'),
var4=VarDef(name='handle', type='HANDLE')
)

Struct(name='VkImportSemaphoreFdInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='flags', type='VkSemaphoreImportFlags'),
var5=VarDef(name='handleType', type='VkExternalSemaphoreHandleTypeFlagBits'),
var6=VarDef(name='fd', type='int')
)

Struct(name='VkImportSemaphoreWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='flags', type='VkSemaphoreImportFlags'),
var5=VarDef(name='handleType', type='VkExternalSemaphoreHandleTypeFlagBits'),
var6=VarDef(name='handle', type='HANDLE'),
var7=VarDef(name='name', type='LPCWSTR')
)

Struct(name='VkIndirectCommandsLayoutCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkIndirectCommandsLayoutUsageFlagsNV'),
var4=VarDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
var5=VarDef(name='tokenCount', type='uint32_t'),
var6=VarDef(name='pTokens', type='const VkIndirectCommandsLayoutTokenNV*'),
var7=VarDef(name='streamCount', type='uint32_t'),
var8=VarDef(name='pStreamStrides', type='const uint32_t*')
)

Struct(name='VkIndirectCommandsLayoutTokenNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='tokenType', type='VkIndirectCommandsTokenTypeNV'),
var4=VarDef(name='stream', type='uint32_t'),
var5=VarDef(name='offset', type='uint32_t'),
var6=VarDef(name='vertexBindingUnit', type='uint32_t'),
var7=VarDef(name='vertexDynamicStride', type='VkBool32'),
var8=VarDef(name='pushconstantPipelineLayout', type='VkPipelineLayout'),
var9=VarDef(name='pushconstantShaderStageFlags', type='VkShaderStageFlags'),
var10=VarDef(name='pushconstantOffset', type='uint32_t'),
var11=VarDef(name='pushconstantSize', type='uint32_t'),
var12=VarDef(name='indirectStateFlags', type='VkIndirectStateFlagsNV'),
var13=VarDef(name='indexTypeCount', type='uint32_t'),
var14=VarDef(name='pIndexTypes', type='const VkIndexType*'),
var15=VarDef(name='pIndexTypeValues', type='const uint32_t*')
)

Struct(name='VkIndirectCommandsStreamNV_', enabled=False,
var1=VarDef(name='buffer', type='VkBuffer'),
var2=VarDef(name='offset', type='VkDeviceSize')
)

Struct(name='VkInitializePerformanceApiInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pUserData', type='void*')
)

Struct(name='VkInputAttachmentAspectReference_', enabled=False,
var1=VarDef(name='subpass', type='uint32_t'),
var2=VarDef(name='inputAttachmentIndex', type='uint32_t'),
var3=VarDef(name='aspectMask', type='VkImageAspectFlags')
)

Struct(name='VkInstanceCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkInstanceCreateFlags'),
var4=VarDef(name='pApplicationInfo', type='const VkApplicationInfo*', wrapType='CVkApplicationInfoArray', wrapParams='1, instancecreateinfo->pApplicationInfo'),
var5=VarDef(name='enabledLayerCount', type='uint32_t'),
var6=VarDef(name='ppEnabledLayerNames', type='const char*const*', wrapType='CStringArray', wrapParams='instancecreateinfo->enabledLayerCount, (const char**)instancecreateinfo->ppEnabledLayerNames', count='enabledLayerCount'),
var7=VarDef(name='enabledExtensionCount', type='uint32_t'),
var8=VarDef(name='ppEnabledExtensionNames', type='const char*const*', wrapType='CStringArray', wrapParams='instancecreateinfo->enabledExtensionCount, (const char**)instancecreateinfo->ppEnabledExtensionNames', count='enabledExtensionCount')
)

Struct(name='VkLayerProperties_', enabled=True, declareArray=True,
var1=VarDef(name='layerName', type='char[256]'),
var2=VarDef(name='specVersion', type='uint32_t'),
var3=VarDef(name='implementationVersion', type='uint32_t'),
var4=VarDef(name='description', type='char[256]')
)

Struct(name='VkMacOSSurfaceCreateInfoMVK_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkMacOSSurfaceCreateFlagsMVK'),
var4=VarDef(name='pView', type='const void*')
)

Struct(name='VkMappedMemoryRange_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='memory', type='VkDeviceMemory'),
var4=VarDef(name='offset', type='VkDeviceSize'),
var5=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkMemoryAllocateFlagsInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkMemoryAllocateFlags'),
var4=VarDef(name='deviceMask', type='uint32_t')
)

Struct(name='VkMemoryAllocateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='allocationSize', type='VkDeviceSize'),
var4=VarDef(name='memoryTypeIndex', type='uint32_t')
)

Struct(name='VkMemoryBarrier_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcAccessMask', type='VkAccessFlags'),
var4=VarDef(name='dstAccessMask', type='VkAccessFlags')
)

Struct(name='VkMemoryBarrier2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcStageMask', type='VkPipelineStageFlags2'),
var4=VarDef(name='srcAccessMask', type='VkAccessFlags2'),
var5=VarDef(name='dstStageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='dstAccessMask', type='VkAccessFlags2')
)

Struct(name='VkMemoryBarrier2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcStageMask', type='VkPipelineStageFlags2'),
var4=VarDef(name='srcAccessMask', type='VkAccessFlags2'),
var5=VarDef(name='dstStageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='dstAccessMask', type='VkAccessFlags2')
)

Struct(name='VkMemoryDedicatedAllocateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='image', type='VkImage'),
var4=VarDef(name='buffer', type='VkBuffer')
)

Struct(name='VkMemoryDedicatedRequirements_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='prefersDedicatedAllocation', type='VkBool32'),
var4=VarDef(name='requiresDedicatedAllocation', type='VkBool32')
)

Struct(name='VkMemoryFdPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryTypeBits', type='uint32_t')
)

Struct(name='VkMemoryGetFdInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='memory', type='VkDeviceMemory'),
var4=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits')
)

Struct(name='VkMemoryGetRemoteAddressInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='memory', type='VkDeviceMemory'),
var4=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits')
)

Struct(name='VkMemoryGetWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='memory', type='VkDeviceMemory'),
var4=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits')
)

Struct(name='VkMemoryHeap_', enabled=True, declareArray=True,
var1=VarDef(name='size', type='VkDeviceSize'),
var2=VarDef(name='flags', type='VkMemoryHeapFlags')
)

Struct(name='VkMemoryHostPointerPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryTypeBits', type='uint32_t')
)

Struct(name='VkMemoryMapInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkMemoryMapFlags'),
var4=VarDef(name='memory', type='VkDeviceMemory'),
var5=VarDef(name='offset', type='VkDeviceSize'),
var6=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkMemoryOpaqueCaptureAddressAllocateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='opaqueCaptureAddress', type='uint64_t')
)

Struct(name='VkMemoryPriorityAllocateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='priority', type='float')
)

Struct(name='VkMemoryRequirements_', enabled=True,
var1=VarDef(name='size', type='VkDeviceSize'),
var2=VarDef(name='alignment', type='VkDeviceSize'),
var3=VarDef(name='memoryTypeBits', type='uint32_t')
)

Struct(name='VkMemoryRequirements2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*', wrapType='CVkGenericArgument'),
var3=VarDef(name='memoryRequirements', type='VkMemoryRequirements')
)

Struct(name='VkMemoryType_', enabled=True, declareArray=True,
var1=VarDef(name='propertyFlags', type='VkMemoryPropertyFlags'),
var2=VarDef(name='heapIndex', type='uint32_t')
)

Struct(name='VkMemoryUnmapInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkMemoryUnmapFlagsKHR'),
var4=VarDef(name='memory', type='VkDeviceMemory')
)

Struct(name='VkMemoryWin32HandlePropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryTypeBits', type='uint32_t')
)

Struct(name='VkMicromapBuildInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkMicromapTypeEXT'),
var4=VarDef(name='flags', type='VkBuildMicromapFlagsEXT'),
var5=VarDef(name='mode', type='VkBuildMicromapModeEXT'),
var6=VarDef(name='dstMicromap', type='VkMicromapEXT'),
var7=VarDef(name='usageCountsCount', type='uint32_t'),
var8=VarDef(name='pUsageCounts', type='const VkMicromapUsageEXT*'),
var9=VarDef(name='ppUsageCounts', type='const VkMicromapUsageEXT* const*'),
var10=VarDef(name='data', type='VkDeviceOrHostAddressConstKHR'),
var11=VarDef(name='scratchData', type='VkDeviceOrHostAddressKHR'),
var12=VarDef(name='triangleArray', type='VkDeviceOrHostAddressConstKHR'),
var13=VarDef(name='triangleArrayStride', type='VkDeviceSize')
)

Struct(name='VkMicromapBuildSizesInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='micromapSize', type='VkDeviceSize'),
var4=VarDef(name='buildScratchSize', type='VkDeviceSize'),
var5=VarDef(name='discardable', type='VkBool32')
)

Struct(name='VkMicromapCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='createFlags', type='VkMicromapCreateFlagsEXT'),
var4=VarDef(name='buffer', type='VkBuffer'),
var5=VarDef(name='offset', type='VkDeviceSize'),
var6=VarDef(name='size', type='VkDeviceSize'),
var7=VarDef(name='type', type='VkMicromapTypeEXT'),
var8=VarDef(name='deviceAddress', type='VkDeviceAddress')
)

Struct(name='VkMicromapTriangleEXT_', enabled=False,
var1=VarDef(name='dataOffset', type='uint32_t'),
var2=VarDef(name='subdivisionLevel', type='uint16_t'),
var3=VarDef(name='format', type='uint16_t')
)

Struct(name='VkMicromapUsageEXT_', enabled=False,
var1=VarDef(name='count', type='uint32_t'),
var2=VarDef(name='subdivisionLevel', type='uint32_t'),
var3=VarDef(name='format', type='uint32_t')
)

Struct(name='VkMicromapVersionInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pVersionData', type='const uint8_t*')
)

Struct(name='VkMultiDrawIndexedInfoEXT_', enabled=False,
var1=VarDef(name='firstIndex', type='uint32_t'),
var2=VarDef(name='indexCount', type='uint32_t'),
var3=VarDef(name='vertexOffset', type='int32_t')
)

Struct(name='VkMultiDrawInfoEXT_', enabled=False,
var1=VarDef(name='firstVertex', type='uint32_t'),
var2=VarDef(name='vertexCount', type='uint32_t')
)

Struct(name='VkMultisamplePropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxSampleLocationGridSize', type='VkExtent2D')
)

Struct(name='VkMultisampledRenderToSingleSampledInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='multisampledRenderToSingleSampledEnable', type='VkBool32'),
var4=VarDef(name='rasterizationSamples', type='VkSampleCountFlagBits')
)

Struct(name='VkMultiviewPerViewAttributesInfoNVX_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='perViewAttributes', type='VkBool32'),
var4=VarDef(name='perViewAttributesPositionXOnly', type='VkBool32')
)

Struct(name='VkMutableDescriptorTypeCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='mutableDescriptorTypeListCount', type='uint32_t'),
var4=VarDef(name='pMutableDescriptorTypeLists', type='const VkMutableDescriptorTypeListEXT*')
)

Struct(name='VkMutableDescriptorTypeCreateInfoVALVE_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='mutableDescriptorTypeListCount', type='uint32_t'),
var4=VarDef(name='pMutableDescriptorTypeLists', type='const VkMutableDescriptorTypeListEXT*')
)

Struct(name='VkMutableDescriptorTypeListEXT_', enabled=False,
var1=VarDef(name='descriptorTypeCount', type='uint32_t'),
var2=VarDef(name='pDescriptorTypes', type='const VkDescriptorType*')
)

Struct(name='VkMutableDescriptorTypeListVALVE_', enabled=False,
var1=VarDef(name='descriptorTypeCount', type='uint32_t'),
var2=VarDef(name='pDescriptorTypes', type='const VkDescriptorType*')
)

Struct(name='VkOffset2D_', enabled=True,
var1=VarDef(name='x', type='int32_t'),
var2=VarDef(name='y', type='int32_t')
)

Struct(name='VkOffset3D_', enabled=True, declareArray=True,
var1=VarDef(name='x', type='int32_t'),
var2=VarDef(name='y', type='int32_t'),
var3=VarDef(name='z', type='int32_t')
)

Struct(name='VkOpaqueCaptureDescriptorDataCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='opaqueCaptureDescriptorData', type='const void*')
)

Struct(name='VkOpticalFlowExecuteInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkOpticalFlowExecuteFlagsNV'),
var4=VarDef(name='regionCount', type='uint32_t'),
var5=VarDef(name='pRegions', type='const VkRect2D*')
)

Struct(name='VkOpticalFlowImageFormatInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='usage', type='VkOpticalFlowUsageFlagsNV')
)

Struct(name='VkOpticalFlowImageFormatPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='format', type='VkFormat')
)

Struct(name='VkOpticalFlowSessionCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='width', type='uint32_t'),
var4=VarDef(name='height', type='uint32_t'),
var5=VarDef(name='imageFormat', type='VkFormat'),
var6=VarDef(name='flowVectorFormat', type='VkFormat'),
var7=VarDef(name='costFormat', type='VkFormat'),
var8=VarDef(name='outputGridSize', type='VkOpticalFlowGridSizeFlagsNV'),
var9=VarDef(name='hintGridSize', type='VkOpticalFlowGridSizeFlagsNV'),
var10=VarDef(name='performanceLevel', type='VkOpticalFlowPerformanceLevelNV'),
var11=VarDef(name='flags', type='VkOpticalFlowSessionCreateFlagsNV')
)

Struct(name='VkOpticalFlowSessionCreatePrivateDataInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='id', type='uint32_t'),
var4=VarDef(name='size', type='uint32_t'),
var5=VarDef(name='pPrivateData', type='const void*')
)

Struct(name='VkPastPresentationTimingGOOGLE_', enabled=False,
var1=VarDef(name='presentID', type='uint32_t'),
var2=VarDef(name='desiredPresentTime', type='uint64_t'),
var3=VarDef(name='actualPresentTime', type='uint64_t'),
var4=VarDef(name='earliestPresentTime', type='uint64_t'),
var5=VarDef(name='presentMargin', type='uint64_t')
)

Struct(name='VkPerformanceConfigurationAcquireInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkPerformanceConfigurationTypeINTEL')
)

Struct(name='VkPerformanceCounterDescriptionKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkPerformanceCounterDescriptionFlagsKHR'),
var4=VarDef(name='name', type='char[256]'),
var5=VarDef(name='category', type='char[256]'),
var6=VarDef(name='description', type='char[256]')
)

Struct(name='VkPerformanceCounterKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='unit', type='VkPerformanceCounterUnitKHR'),
var4=VarDef(name='scope', type='VkPerformanceCounterScopeKHR'),
var5=VarDef(name='storage', type='VkPerformanceCounterStorageKHR'),
var6=VarDef(name='uuid', type='uint8_t[16]')
)

Struct(name='VkPerformanceMarkerInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='marker', type='uint64_t')
)

Struct(name='VkPerformanceOverrideInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkPerformanceOverrideTypeINTEL'),
var4=VarDef(name='enable', type='VkBool32'),
var5=VarDef(name='parameter', type='uint64_t')
)

Struct(name='VkPerformanceQueryReservationInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='maxPerformanceQueriesPerPool', type='uint32_t')
)

Struct(name='VkPerformanceQuerySubmitInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='counterPassIndex', type='uint32_t')
)

Struct(name='VkPerformanceStreamMarkerInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='marker', type='uint32_t')
)

Struct(name='VkPerformanceValueINTEL_', enabled=False,
var1=VarDef(name='type', type='VkPerformanceValueTypeINTEL'),
var2=VarDef(name='data', type='VkPerformanceValueDataINTEL')
)

Struct(name='VkPerformanceValueDataINTEL_', type='union', enabled=False,
var1=VarDef(name='value32', type='uint32_t'),
var2=VarDef(name='value64', type='uint64_t'),
var3=VarDef(name='valueFloat', type='float'),
var4=VarDef(name='valueString', type='const char*'),
)

Struct(name='VkPhysicalDevice16BitStorageFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='storageBuffer16BitAccess', type='VkBool32'),
var4=VarDef(name='uniformAndStorageBuffer16BitAccess', type='VkBool32'),
var5=VarDef(name='storagePushConstant16', type='VkBool32'),
var6=VarDef(name='storageInputOutput16', type='VkBool32')
)

Struct(name='VkPhysicalDevice4444FormatsFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='formatA4R4G4B4', type='VkBool32'),
var4=VarDef(name='formatA4B4G4R4', type='VkBool32')
)

Struct(name='VkPhysicalDevice8BitStorageFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='storageBuffer8BitAccess', type='VkBool32'),
var4=VarDef(name='uniformAndStorageBuffer8BitAccess', type='VkBool32'),
var5=VarDef(name='storagePushConstant8', type='VkBool32')
)

Struct(name='VkPhysicalDeviceASTCDecodeFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='decodeModeSharedExponent', type='VkBool32')
)

Struct(name='VkPhysicalDeviceAccelerationStructureFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='accelerationStructure', type='VkBool32'),
var4=VarDef(name='accelerationStructureCaptureReplay', type='VkBool32'),
var5=VarDef(name='accelerationStructureIndirectBuild', type='VkBool32'),
var6=VarDef(name='accelerationStructureHostCommands', type='VkBool32'),
var7=VarDef(name='descriptorBindingAccelerationStructureUpdateAfterBind', type='VkBool32')
)

Struct(name='VkPhysicalDeviceAccelerationStructurePropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxGeometryCount', type='uint64_t'),
var4=VarDef(name='maxInstanceCount', type='uint64_t'),
var5=VarDef(name='maxPrimitiveCount', type='uint64_t'),
var6=VarDef(name='maxPerStageDescriptorAccelerationStructures', type='uint32_t'),
var7=VarDef(name='maxPerStageDescriptorUpdateAfterBindAccelerationStructures', type='uint32_t'),
var8=VarDef(name='maxDescriptorSetAccelerationStructures', type='uint32_t'),
var9=VarDef(name='maxDescriptorSetUpdateAfterBindAccelerationStructures', type='uint32_t'),
var10=VarDef(name='minAccelerationStructureScratchOffsetAlignment', type='uint32_t')
)

Struct(name='VkPhysicalDeviceAddressBindingReportFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='reportAddressBinding', type='VkBool32')
)

Struct(name='VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='attachmentFeedbackLoopLayout', type='VkBool32')
)

Struct(name='VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='advancedBlendCoherentOperations', type='VkBool32')
)

Struct(name='VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='advancedBlendMaxColorAttachments', type='uint32_t'),
var4=VarDef(name='advancedBlendIndependentBlend', type='VkBool32'),
var5=VarDef(name='advancedBlendNonPremultipliedSrcColor', type='VkBool32'),
var6=VarDef(name='advancedBlendNonPremultipliedDstColor', type='VkBool32'),
var7=VarDef(name='advancedBlendCorrelatedOverlap', type='VkBool32'),
var8=VarDef(name='advancedBlendAllOperations', type='VkBool32')
)

Struct(name='VkPhysicalDeviceBorderColorSwizzleFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='borderColorSwizzle', type='VkBool32'),
var4=VarDef(name='borderColorSwizzleFromImage', type='VkBool32')
)

Struct(name='VkPhysicalDeviceBufferAddressFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='bufferDeviceAddress', type='VkBool32'),
var4=VarDef(name='bufferDeviceAddressCaptureReplay', type='VkBool32'),
var5=VarDef(name='bufferDeviceAddressMultiDevice', type='VkBool32')
)

Struct(name='VkPhysicalDeviceBufferDeviceAddressFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='bufferDeviceAddress', type='VkBool32'),
var4=VarDef(name='bufferDeviceAddressCaptureReplay', type='VkBool32'),
var5=VarDef(name='bufferDeviceAddressMultiDevice', type='VkBool32')
)

Struct(name='VkPhysicalDeviceBufferDeviceAddressFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='bufferDeviceAddress', type='VkBool32'),
var4=VarDef(name='bufferDeviceAddressCaptureReplay', type='VkBool32'),
var5=VarDef(name='bufferDeviceAddressMultiDevice', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCoherentMemoryFeaturesAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceCoherentMemory', type='VkBool32')
)

Struct(name='VkPhysicalDeviceColorWriteEnableFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='colorWriteEnable', type='VkBool32')
)

Struct(name='VkPhysicalDeviceComputeShaderDerivativesFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='computeDerivativeGroupQuads', type='VkBool32'),
var4=VarDef(name='computeDerivativeGroupLinear', type='VkBool32')
)

Struct(name='VkPhysicalDeviceConditionalRenderingFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='conditionalRendering', type='VkBool32'),
var4=VarDef(name='inheritedConditionalRendering', type='VkBool32')
)

Struct(name='VkPhysicalDeviceConservativeRasterizationPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='primitiveOverestimationSize', type='float'),
var4=VarDef(name='maxExtraPrimitiveOverestimationSize', type='float'),
var5=VarDef(name='extraPrimitiveOverestimationSizeGranularity', type='float'),
var6=VarDef(name='primitiveUnderestimation', type='VkBool32'),
var7=VarDef(name='conservativePointAndLineRasterization', type='VkBool32'),
var8=VarDef(name='degenerateTrianglesRasterized', type='VkBool32'),
var9=VarDef(name='degenerateLinesRasterized', type='VkBool32'),
var10=VarDef(name='fullyCoveredFragmentShaderInputVariable', type='VkBool32'),
var11=VarDef(name='conservativeRasterizationPostDepthCoverage', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCooperativeMatrixFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='cooperativeMatrix', type='VkBool32'),
var4=VarDef(name='cooperativeMatrixRobustBufferAccess', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCooperativeMatrixPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='cooperativeMatrixSupportedStages', type='VkShaderStageFlags')
)

Struct(name='VkPhysicalDeviceCopyMemoryIndirectFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='indirectCopy', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCopyMemoryIndirectPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='supportedQueues', type='VkQueueFlags')
)

Struct(name='VkPhysicalDeviceCornerSampledImageFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='cornerSampledImage', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCoverageReductionModeFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='coverageReductionMode', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCustomBorderColorFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='customBorderColors', type='VkBool32'),
var4=VarDef(name='customBorderColorWithoutFormat', type='VkBool32')
)

Struct(name='VkPhysicalDeviceCustomBorderColorPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxCustomBorderColorSamplers', type='uint32_t')
)

Struct(name='VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='dedicatedAllocationImageAliasing', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDepthClampZeroOneFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='depthClampZeroOne', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDepthClipControlFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='depthClipControl', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDepthClipEnableFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='depthClipEnable', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDepthStencilResolveProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='supportedDepthResolveModes', type='VkResolveModeFlags'),
var4=VarDef(name='supportedStencilResolveModes', type='VkResolveModeFlags'),
var5=VarDef(name='independentResolveNone', type='VkBool32'),
var6=VarDef(name='independentResolve', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='combinedImageSamplerDensityMapDescriptorSize', type='size_t')
)

Struct(name='VkPhysicalDeviceDescriptorBufferFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='descriptorBuffer', type='VkBool32'),
var4=VarDef(name='descriptorBufferCaptureReplay', type='VkBool32'),
var5=VarDef(name='descriptorBufferImageLayoutIgnored', type='VkBool32'),
var6=VarDef(name='descriptorBufferPushDescriptors', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDescriptorBufferPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='combinedImageSamplerDescriptorSingleArray', type='VkBool32'),
var4=VarDef(name='bufferlessPushDescriptors', type='VkBool32'),
var5=VarDef(name='allowSamplerImageViewPostSubmitCreation', type='VkBool32'),
var6=VarDef(name='descriptorBufferOffsetAlignment', type='VkDeviceSize'),
var7=VarDef(name='maxDescriptorBufferBindings', type='uint32_t'),
var8=VarDef(name='maxResourceDescriptorBufferBindings', type='uint32_t'),
var9=VarDef(name='maxSamplerDescriptorBufferBindings', type='uint32_t'),
var10=VarDef(name='maxEmbeddedImmutableSamplerBindings', type='uint32_t'),
var11=VarDef(name='maxEmbeddedImmutableSamplers', type='uint32_t'),
var12=VarDef(name='bufferCaptureReplayDescriptorDataSize', type='size_t'),
var13=VarDef(name='imageCaptureReplayDescriptorDataSize', type='size_t'),
var14=VarDef(name='imageViewCaptureReplayDescriptorDataSize', type='size_t'),
var15=VarDef(name='samplerCaptureReplayDescriptorDataSize', type='size_t'),
var16=VarDef(name='accelerationStructureCaptureReplayDescriptorDataSize', type='size_t'),
var17=VarDef(name='samplerDescriptorSize', type='size_t'),
var18=VarDef(name='combinedImageSamplerDescriptorSize', type='size_t'),
var19=VarDef(name='sampledImageDescriptorSize', type='size_t'),
var20=VarDef(name='storageImageDescriptorSize', type='size_t'),
var21=VarDef(name='uniformTexelBufferDescriptorSize', type='size_t'),
var22=VarDef(name='robustUniformTexelBufferDescriptorSize', type='size_t'),
var23=VarDef(name='storageTexelBufferDescriptorSize', type='size_t'),
var24=VarDef(name='robustStorageTexelBufferDescriptorSize', type='size_t'),
var25=VarDef(name='uniformBufferDescriptorSize', type='size_t'),
var26=VarDef(name='robustUniformBufferDescriptorSize', type='size_t'),
var27=VarDef(name='storageBufferDescriptorSize', type='size_t'),
var28=VarDef(name='robustStorageBufferDescriptorSize', type='size_t'),
var29=VarDef(name='inputAttachmentDescriptorSize', type='size_t'),
var30=VarDef(name='accelerationStructureDescriptorSize', type='size_t'),
var31=VarDef(name='maxSamplerDescriptorBufferRange', type='VkDeviceSize'),
var32=VarDef(name='maxResourceDescriptorBufferRange', type='VkDeviceSize'),
var33=VarDef(name='samplerDescriptorBufferAddressSpaceSize', type='VkDeviceSize'),
var34=VarDef(name='resourceDescriptorBufferAddressSpaceSize', type='VkDeviceSize'),
var35=VarDef(name='descriptorBufferAddressSpaceSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceDescriptorIndexingFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderInputAttachmentArrayDynamicIndexing', type='VkBool32'),
var4=VarDef(name='shaderUniformTexelBufferArrayDynamicIndexing', type='VkBool32'),
var5=VarDef(name='shaderStorageTexelBufferArrayDynamicIndexing', type='VkBool32'),
var6=VarDef(name='shaderUniformBufferArrayNonUniformIndexing', type='VkBool32'),
var7=VarDef(name='shaderSampledImageArrayNonUniformIndexing', type='VkBool32'),
var8=VarDef(name='shaderStorageBufferArrayNonUniformIndexing', type='VkBool32'),
var9=VarDef(name='shaderStorageImageArrayNonUniformIndexing', type='VkBool32'),
var10=VarDef(name='shaderInputAttachmentArrayNonUniformIndexing', type='VkBool32'),
var11=VarDef(name='shaderUniformTexelBufferArrayNonUniformIndexing', type='VkBool32'),
var12=VarDef(name='shaderStorageTexelBufferArrayNonUniformIndexing', type='VkBool32'),
var13=VarDef(name='descriptorBindingUniformBufferUpdateAfterBind', type='VkBool32'),
var14=VarDef(name='descriptorBindingSampledImageUpdateAfterBind', type='VkBool32'),
var15=VarDef(name='descriptorBindingStorageImageUpdateAfterBind', type='VkBool32'),
var16=VarDef(name='descriptorBindingStorageBufferUpdateAfterBind', type='VkBool32'),
var17=VarDef(name='descriptorBindingUniformTexelBufferUpdateAfterBind', type='VkBool32'),
var18=VarDef(name='descriptorBindingStorageTexelBufferUpdateAfterBind', type='VkBool32'),
var19=VarDef(name='descriptorBindingUpdateUnusedWhilePending', type='VkBool32'),
var20=VarDef(name='descriptorBindingPartiallyBound', type='VkBool32'),
var21=VarDef(name='descriptorBindingVariableDescriptorCount', type='VkBool32'),
var22=VarDef(name='runtimeDescriptorArray', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDescriptorIndexingProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxUpdateAfterBindDescriptorsInAllPools', type='uint32_t'),
var4=VarDef(name='shaderUniformBufferArrayNonUniformIndexingNative', type='VkBool32'),
var5=VarDef(name='shaderSampledImageArrayNonUniformIndexingNative', type='VkBool32'),
var6=VarDef(name='shaderStorageBufferArrayNonUniformIndexingNative', type='VkBool32'),
var7=VarDef(name='shaderStorageImageArrayNonUniformIndexingNative', type='VkBool32'),
var8=VarDef(name='shaderInputAttachmentArrayNonUniformIndexingNative', type='VkBool32'),
var9=VarDef(name='robustBufferAccessUpdateAfterBind', type='VkBool32'),
var10=VarDef(name='quadDivergentImplicitLod', type='VkBool32'),
var11=VarDef(name='maxPerStageDescriptorUpdateAfterBindSamplers', type='uint32_t'),
var12=VarDef(name='maxPerStageDescriptorUpdateAfterBindUniformBuffers', type='uint32_t'),
var13=VarDef(name='maxPerStageDescriptorUpdateAfterBindStorageBuffers', type='uint32_t'),
var14=VarDef(name='maxPerStageDescriptorUpdateAfterBindSampledImages', type='uint32_t'),
var15=VarDef(name='maxPerStageDescriptorUpdateAfterBindStorageImages', type='uint32_t'),
var16=VarDef(name='maxPerStageDescriptorUpdateAfterBindInputAttachments', type='uint32_t'),
var17=VarDef(name='maxPerStageUpdateAfterBindResources', type='uint32_t'),
var18=VarDef(name='maxDescriptorSetUpdateAfterBindSamplers', type='uint32_t'),
var19=VarDef(name='maxDescriptorSetUpdateAfterBindUniformBuffers', type='uint32_t'),
var20=VarDef(name='maxDescriptorSetUpdateAfterBindUniformBuffersDynamic', type='uint32_t'),
var21=VarDef(name='maxDescriptorSetUpdateAfterBindStorageBuffers', type='uint32_t'),
var22=VarDef(name='maxDescriptorSetUpdateAfterBindStorageBuffersDynamic', type='uint32_t'),
var23=VarDef(name='maxDescriptorSetUpdateAfterBindSampledImages', type='uint32_t'),
var24=VarDef(name='maxDescriptorSetUpdateAfterBindStorageImages', type='uint32_t'),
var25=VarDef(name='maxDescriptorSetUpdateAfterBindInputAttachments', type='uint32_t')
)

Struct(name='VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceGeneratedCommands', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxGraphicsShaderGroupCount', type='uint32_t'),
var4=VarDef(name='maxIndirectSequenceCount', type='uint32_t'),
var5=VarDef(name='maxIndirectCommandsTokenCount', type='uint32_t'),
var6=VarDef(name='maxIndirectCommandsStreamCount', type='uint32_t'),
var7=VarDef(name='maxIndirectCommandsTokenOffset', type='uint32_t'),
var8=VarDef(name='maxIndirectCommandsStreamStride', type='uint32_t'),
var9=VarDef(name='minSequencesCountBufferOffsetAlignment', type='uint32_t'),
var10=VarDef(name='minSequencesIndexBufferOffsetAlignment', type='uint32_t'),
var11=VarDef(name='minIndirectCommandsBufferOffsetAlignment', type='uint32_t')
)

Struct(name='VkPhysicalDeviceDeviceMemoryReportFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceMemoryReport', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDiagnosticsConfigFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='diagnosticsConfig', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDiscardRectanglePropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxDiscardRectangles', type='uint32_t')
)

Struct(name='VkPhysicalDeviceDisplacementMicromapFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='displacementMicromap', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDisplacementMicromapPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxDisplacementMicromapSubdivisionLevel', type='uint32_t')
)

Struct(name='VkPhysicalDeviceDriverProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='driverID', type='VkDriverId'),
var4=VarDef(name='driverName', type='char[256]'),
var5=VarDef(name='driverInfo', type='char[256]'),
var6=VarDef(name='conformanceVersion', type='VkConformanceVersion')
)

Struct(name='VkPhysicalDeviceDriverPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='driverID', type='VkDriverId'),
var4=VarDef(name='driverName', type='char[256]'),
var5=VarDef(name='driverInfo', type='char[256]'),
var6=VarDef(name='conformanceVersion', type='VkConformanceVersion')
)

Struct(name='VkPhysicalDeviceDrmPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='hasPrimary', type='VkBool32'),
var4=VarDef(name='hasRender', type='VkBool32'),
var5=VarDef(name='primaryMajor', type='int64_t'),
var6=VarDef(name='primaryMinor', type='int64_t'),
var7=VarDef(name='renderMajor', type='int64_t'),
var8=VarDef(name='renderMinor', type='int64_t')
)

Struct(name='VkPhysicalDeviceDynamicRenderingFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='dynamicRendering', type='VkBool32')
)

Struct(name='VkPhysicalDeviceDynamicRenderingFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='dynamicRendering', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExclusiveScissorFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='exclusiveScissor', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExtendedDynamicState2FeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='extendedDynamicState2', type='VkBool32'),
var4=VarDef(name='extendedDynamicState2LogicOp', type='VkBool32'),
var5=VarDef(name='extendedDynamicState2PatchControlPoints', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExtendedDynamicState3FeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='extendedDynamicState3TessellationDomainOrigin', type='VkBool32'),
var4=VarDef(name='extendedDynamicState3DepthClampEnable', type='VkBool32'),
var5=VarDef(name='extendedDynamicState3PolygonMode', type='VkBool32'),
var6=VarDef(name='extendedDynamicState3RasterizationSamples', type='VkBool32'),
var7=VarDef(name='extendedDynamicState3SampleMask', type='VkBool32'),
var8=VarDef(name='extendedDynamicState3AlphaToCoverageEnable', type='VkBool32'),
var9=VarDef(name='extendedDynamicState3AlphaToOneEnable', type='VkBool32'),
var10=VarDef(name='extendedDynamicState3LogicOpEnable', type='VkBool32'),
var11=VarDef(name='extendedDynamicState3ColorBlendEnable', type='VkBool32'),
var12=VarDef(name='extendedDynamicState3ColorBlendEquation', type='VkBool32'),
var13=VarDef(name='extendedDynamicState3ColorWriteMask', type='VkBool32'),
var14=VarDef(name='extendedDynamicState3RasterizationStream', type='VkBool32'),
var15=VarDef(name='extendedDynamicState3ConservativeRasterizationMode', type='VkBool32'),
var16=VarDef(name='extendedDynamicState3ExtraPrimitiveOverestimationSize', type='VkBool32'),
var17=VarDef(name='extendedDynamicState3DepthClipEnable', type='VkBool32'),
var18=VarDef(name='extendedDynamicState3SampleLocationsEnable', type='VkBool32'),
var19=VarDef(name='extendedDynamicState3ColorBlendAdvanced', type='VkBool32'),
var20=VarDef(name='extendedDynamicState3ProvokingVertexMode', type='VkBool32'),
var21=VarDef(name='extendedDynamicState3LineRasterizationMode', type='VkBool32'),
var22=VarDef(name='extendedDynamicState3LineStippleEnable', type='VkBool32'),
var23=VarDef(name='extendedDynamicState3DepthClipNegativeOneToOne', type='VkBool32'),
var24=VarDef(name='extendedDynamicState3ViewportWScalingEnable', type='VkBool32'),
var25=VarDef(name='extendedDynamicState3ViewportSwizzle', type='VkBool32'),
var26=VarDef(name='extendedDynamicState3CoverageToColorEnable', type='VkBool32'),
var27=VarDef(name='extendedDynamicState3CoverageToColorLocation', type='VkBool32'),
var28=VarDef(name='extendedDynamicState3CoverageModulationMode', type='VkBool32'),
var29=VarDef(name='extendedDynamicState3CoverageModulationTableEnable', type='VkBool32'),
var30=VarDef(name='extendedDynamicState3CoverageModulationTable', type='VkBool32'),
var31=VarDef(name='extendedDynamicState3CoverageReductionMode', type='VkBool32'),
var32=VarDef(name='extendedDynamicState3RepresentativeFragmentTestEnable', type='VkBool32'),
var33=VarDef(name='extendedDynamicState3ShadingRateImageEnable', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExtendedDynamicState3PropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='dynamicPrimitiveTopologyUnrestricted', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExtendedDynamicStateFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='extendedDynamicState', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExternalBufferInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkBufferCreateFlags'),
var4=VarDef(name='usage', type='VkBufferUsageFlags'),
var5=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits')
)

Struct(name='VkPhysicalDeviceExternalFenceInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalFenceHandleTypeFlagBits')
)

Struct(name='VkPhysicalDeviceExternalImageFormatInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalMemoryHandleTypeFlagBits')
)

Struct(name='VkPhysicalDeviceExternalMemoryHostPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minImportedHostPointerAlignment', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceExternalMemoryRDMAFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='externalMemoryRDMA', type='VkBool32')
)

Struct(name='VkPhysicalDeviceExternalSemaphoreInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='handleType', type='VkExternalSemaphoreHandleTypeFlagBits')
)

Struct(name='VkPhysicalDeviceFaultFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceFault', type='VkBool32'),
var4=VarDef(name='deviceFaultVendorBinary', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFeatures_', enabled=True, declareArray=True,
var1=VarDef(name='robustBufferAccess', type='VkBool32'),
var2=VarDef(name='fullDrawIndexUint32', type='VkBool32'),
var3=VarDef(name='imageCubeArray', type='VkBool32'),
var4=VarDef(name='independentBlend', type='VkBool32'),
var5=VarDef(name='geometryShader', type='VkBool32'),
var6=VarDef(name='tessellationShader', type='VkBool32'),
var7=VarDef(name='sampleRateShading', type='VkBool32'),
var8=VarDef(name='dualSrcBlend', type='VkBool32'),
var9=VarDef(name='logicOp', type='VkBool32'),
var10=VarDef(name='multiDrawIndirect', type='VkBool32'),
var11=VarDef(name='drawIndirectFirstInstance', type='VkBool32'),
var12=VarDef(name='depthClamp', type='VkBool32'),
var13=VarDef(name='depthBiasClamp', type='VkBool32'),
var14=VarDef(name='fillModeNonSolid', type='VkBool32'),
var15=VarDef(name='depthBounds', type='VkBool32'),
var16=VarDef(name='wideLines', type='VkBool32'),
var17=VarDef(name='largePoints', type='VkBool32'),
var18=VarDef(name='alphaToOne', type='VkBool32'),
var19=VarDef(name='multiViewport', type='VkBool32'),
var20=VarDef(name='samplerAnisotropy', type='VkBool32'),
var21=VarDef(name='textureCompressionETC2', type='VkBool32'),
var22=VarDef(name='textureCompressionASTC_LDR', type='VkBool32'),
var23=VarDef(name='textureCompressionBC', type='VkBool32'),
var24=VarDef(name='occlusionQueryPrecise', type='VkBool32'),
var25=VarDef(name='pipelineStatisticsQuery', type='VkBool32'),
var26=VarDef(name='vertexPipelineStoresAndAtomics', type='VkBool32'),
var27=VarDef(name='fragmentStoresAndAtomics', type='VkBool32'),
var28=VarDef(name='shaderTessellationAndGeometryPointSize', type='VkBool32'),
var29=VarDef(name='shaderImageGatherExtended', type='VkBool32'),
var30=VarDef(name='shaderStorageImageExtendedFormats', type='VkBool32'),
var31=VarDef(name='shaderStorageImageMultisample', type='VkBool32'),
var32=VarDef(name='shaderStorageImageReadWithoutFormat', type='VkBool32'),
var33=VarDef(name='shaderStorageImageWriteWithoutFormat', type='VkBool32'),
var34=VarDef(name='shaderUniformBufferArrayDynamicIndexing', type='VkBool32'),
var35=VarDef(name='shaderSampledImageArrayDynamicIndexing', type='VkBool32'),
var36=VarDef(name='shaderStorageBufferArrayDynamicIndexing', type='VkBool32'),
var37=VarDef(name='shaderStorageImageArrayDynamicIndexing', type='VkBool32'),
var38=VarDef(name='shaderClipDistance', type='VkBool32'),
var39=VarDef(name='shaderCullDistance', type='VkBool32'),
var40=VarDef(name='shaderFloat64', type='VkBool32'),
var41=VarDef(name='shaderInt64', type='VkBool32'),
var42=VarDef(name='shaderInt16', type='VkBool32'),
var43=VarDef(name='shaderResourceResidency', type='VkBool32'),
var44=VarDef(name='shaderResourceMinLod', type='VkBool32'),
var45=VarDef(name='sparseBinding', type='VkBool32'),
var46=VarDef(name='sparseResidencyBuffer', type='VkBool32'),
var47=VarDef(name='sparseResidencyImage2D', type='VkBool32'),
var48=VarDef(name='sparseResidencyImage3D', type='VkBool32'),
var49=VarDef(name='sparseResidency2Samples', type='VkBool32'),
var50=VarDef(name='sparseResidency4Samples', type='VkBool32'),
var51=VarDef(name='sparseResidency8Samples', type='VkBool32'),
var52=VarDef(name='sparseResidency16Samples', type='VkBool32'),
var53=VarDef(name='sparseResidencyAliased', type='VkBool32'),
var54=VarDef(name='variableMultisampleRate', type='VkBool32'),
var55=VarDef(name='inheritedQueries', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFeatures2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='features', type='VkPhysicalDeviceFeatures')
)

Struct(name='VkPhysicalDeviceFloat16Int8FeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderFloat16', type='VkBool32'),
var4=VarDef(name='shaderInt8', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFloatControlsProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='denormBehaviorIndependence', type='VkShaderFloatControlsIndependence'),
var4=VarDef(name='roundingModeIndependence', type='VkShaderFloatControlsIndependence'),
var5=VarDef(name='shaderSignedZeroInfNanPreserveFloat16', type='VkBool32'),
var6=VarDef(name='shaderSignedZeroInfNanPreserveFloat32', type='VkBool32'),
var7=VarDef(name='shaderSignedZeroInfNanPreserveFloat64', type='VkBool32'),
var8=VarDef(name='shaderDenormPreserveFloat16', type='VkBool32'),
var9=VarDef(name='shaderDenormPreserveFloat32', type='VkBool32'),
var10=VarDef(name='shaderDenormPreserveFloat64', type='VkBool32'),
var11=VarDef(name='shaderDenormFlushToZeroFloat16', type='VkBool32'),
var12=VarDef(name='shaderDenormFlushToZeroFloat32', type='VkBool32'),
var13=VarDef(name='shaderDenormFlushToZeroFloat64', type='VkBool32'),
var14=VarDef(name='shaderRoundingModeRTEFloat16', type='VkBool32'),
var15=VarDef(name='shaderRoundingModeRTEFloat32', type='VkBool32'),
var16=VarDef(name='shaderRoundingModeRTEFloat64', type='VkBool32'),
var17=VarDef(name='shaderRoundingModeRTZFloat16', type='VkBool32'),
var18=VarDef(name='shaderRoundingModeRTZFloat32', type='VkBool32'),
var19=VarDef(name='shaderRoundingModeRTZFloat64', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentDensityMap2FeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fragmentDensityMapDeferred', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentDensityMap2PropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='subsampledLoads', type='VkBool32'),
var4=VarDef(name='subsampledCoarseReconstructionEarlyAccess', type='VkBool32'),
var5=VarDef(name='maxSubsampledArrayLayers', type='uint32_t'),
var6=VarDef(name='maxDescriptorSetSubsampledSamplers', type='uint32_t')
)

Struct(name='VkPhysicalDeviceFragmentDensityMapFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fragmentDensityMap', type='VkBool32'),
var4=VarDef(name='fragmentDensityMapDynamic', type='VkBool32'),
var5=VarDef(name='fragmentDensityMapNonSubsampledImages', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentDensityMapPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minFragmentDensityTexelSize', type='VkExtent2D'),
var4=VarDef(name='maxFragmentDensityTexelSize', type='VkExtent2D'),
var5=VarDef(name='fragmentDensityInvocations', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fragmentShaderBarycentric', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fragmentShaderBarycentric', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='triStripVertexOrderIndependentOfProvokingVertex', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fragmentShaderSampleInterlock', type='VkBool32'),
var4=VarDef(name='fragmentShaderPixelInterlock', type='VkBool32'),
var5=VarDef(name='fragmentShaderShadingRateInterlock', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fragmentShadingRateEnums', type='VkBool32'),
var4=VarDef(name='supersampleFragmentShadingRates', type='VkBool32'),
var5=VarDef(name='noInvocationFragmentShadingRates', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxFragmentShadingRateInvocationCount', type='VkSampleCountFlagBits')
)

Struct(name='VkPhysicalDeviceFragmentShadingRateFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineFragmentShadingRate', type='VkBool32'),
var4=VarDef(name='primitiveFragmentShadingRate', type='VkBool32'),
var5=VarDef(name='attachmentFragmentShadingRate', type='VkBool32')
)

Struct(name='VkPhysicalDeviceFragmentShadingRateKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='sampleCounts', type='VkSampleCountFlags'),
var4=VarDef(name='fragmentSize', type='VkExtent2D')
)

Struct(name='VkPhysicalDeviceFragmentShadingRatePropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minFragmentShadingRateAttachmentTexelSize', type='VkExtent2D'),
var4=VarDef(name='maxFragmentShadingRateAttachmentTexelSize', type='VkExtent2D'),
var5=VarDef(name='maxFragmentShadingRateAttachmentTexelSizeAspectRatio', type='uint32_t'),
var6=VarDef(name='primitiveFragmentShadingRateWithMultipleViewports', type='VkBool32'),
var7=VarDef(name='layeredShadingRateAttachments', type='VkBool32'),
var8=VarDef(name='fragmentShadingRateNonTrivialCombinerOps', type='VkBool32'),
var9=VarDef(name='maxFragmentSize', type='VkExtent2D'),
var10=VarDef(name='maxFragmentSizeAspectRatio', type='uint32_t'),
var11=VarDef(name='maxFragmentShadingRateCoverageSamples', type='uint32_t'),
var12=VarDef(name='maxFragmentShadingRateRasterizationSamples', type='VkSampleCountFlagBits'),
var13=VarDef(name='fragmentShadingRateWithShaderDepthStencilWrites', type='VkBool32'),
var14=VarDef(name='fragmentShadingRateWithSampleMask', type='VkBool32'),
var15=VarDef(name='fragmentShadingRateWithShaderSampleMask', type='VkBool32'),
var16=VarDef(name='fragmentShadingRateWithConservativeRasterization', type='VkBool32'),
var17=VarDef(name='fragmentShadingRateWithFragmentShaderInterlock', type='VkBool32'),
var18=VarDef(name='fragmentShadingRateWithCustomSampleLocations', type='VkBool32'),
var19=VarDef(name='fragmentShadingRateStrictMultiplyCombiner', type='VkBool32')
)

Struct(name='VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='globalPriorityQuery', type='VkBool32')
)

Struct(name='VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='globalPriorityQuery', type='VkBool32')
)

Struct(name='VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='graphicsPipelineLibrary', type='VkBool32')
)

Struct(name='VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='graphicsPipelineLibraryFastLinking', type='VkBool32'),
var4=VarDef(name='graphicsPipelineLibraryIndependentInterpolationDecoration', type='VkBool32')
)

Struct(name='VkPhysicalDeviceGroupProperties_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*', wrapType='CVkGenericArgument'),
var3=VarDef(name='physicalDeviceCount', type='uint32_t'),
var4=VarDef(name='physicalDevices', type='VkPhysicalDevice[32]', wrapType='CVkPhysicalDevice::CSArray', wrapParams='physicaldevicegroupproperties->physicalDeviceCount, physicaldevicegroupproperties->physicalDevices', count='physicalDeviceCount'),
var5=VarDef(name='subsetAllocation', type='VkBool32')
)

Struct(name='VkPhysicalDeviceHostQueryResetFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='hostQueryReset', type='VkBool32')
)

Struct(name='VkPhysicalDeviceIDProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceUUID', type='uint8_t[16]', count='16'),
var4=VarDef(name='driverUUID', type='uint8_t[16]', count='16'),
var5=VarDef(name='deviceLUID', type='uint8_t[8]', count='8'),
var6=VarDef(name='deviceNodeMask', type='uint32_t'),
var7=VarDef(name='deviceLUIDValid', type='VkBool32')
)

Struct(name='VkPhysicalDeviceIDPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceUUID', type='uint8_t[16]'),
var4=VarDef(name='driverUUID', type='uint8_t[16]'),
var5=VarDef(name='deviceLUID', type='uint8_t[8]'),
var6=VarDef(name='deviceNodeMask', type='uint32_t'),
var7=VarDef(name='deviceLUIDValid', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImage2DViewOf3DFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='image2DViewOf3D', type='VkBool32'),
var4=VarDef(name='sampler2DViewOf3D', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImageCompressionControlFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageCompressionControl', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageCompressionControlSwapchain', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImageDrmFormatModifierInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='drmFormatModifier', type='uint64_t'),
var4=VarDef(name='sharingMode', type='VkSharingMode'),
var5=VarDef(name='queueFamilyIndexCount', type='uint32_t'),
var6=VarDef(name='pQueueFamilyIndices', type='const uint32_t*')
)

Struct(name='VkPhysicalDeviceImageFormatInfo2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='format', type='VkFormat'),
var4=VarDef(name='type', type='VkImageType'),
var5=VarDef(name='tiling', type='VkImageTiling'),
var6=VarDef(name='usage', type='VkImageUsageFlags'),
var7=VarDef(name='flags', type='VkImageCreateFlags')
)

Struct(name='VkPhysicalDeviceImageFormatInfo2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='format', type='VkFormat'),
var4=VarDef(name='type', type='VkImageType'),
var5=VarDef(name='tiling', type='VkImageTiling'),
var6=VarDef(name='usage', type='VkImageUsageFlags'),
var7=VarDef(name='flags', type='VkImageCreateFlags')
)

Struct(name='VkPhysicalDeviceImageRobustnessFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='robustImageAccess', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImageRobustnessFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='robustImageAccess', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageSlicedViewOf3D', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImageViewImageFormatInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageViewType', type='VkImageViewType')
)

Struct(name='VkPhysicalDeviceImageViewMinLodFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minLod', type='VkBool32')
)

Struct(name='VkPhysicalDeviceImagelessFramebufferFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imagelessFramebuffer', type='VkBool32')
)

Struct(name='VkPhysicalDeviceIndexTypeUint8FeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='indexTypeUint8', type='VkBool32')
)

Struct(name='VkPhysicalDeviceInheritedViewportScissorFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='inheritedViewportScissor2D', type='VkBool32')
)

Struct(name='VkPhysicalDeviceInlineUniformBlockFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='inlineUniformBlock', type='VkBool32'),
var4=VarDef(name='descriptorBindingInlineUniformBlockUpdateAfterBind', type='VkBool32')
)

Struct(name='VkPhysicalDeviceInlineUniformBlockFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='inlineUniformBlock', type='VkBool32'),
var4=VarDef(name='descriptorBindingInlineUniformBlockUpdateAfterBind', type='VkBool32')
)

Struct(name='VkPhysicalDeviceInlineUniformBlockProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxInlineUniformBlockSize', type='uint32_t'),
var4=VarDef(name='maxPerStageDescriptorInlineUniformBlocks', type='uint32_t'),
var5=VarDef(name='maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks', type='uint32_t'),
var6=VarDef(name='maxDescriptorSetInlineUniformBlocks', type='uint32_t'),
var7=VarDef(name='maxDescriptorSetUpdateAfterBindInlineUniformBlocks', type='uint32_t')
)

Struct(name='VkPhysicalDeviceInlineUniformBlockPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxInlineUniformBlockSize', type='uint32_t'),
var4=VarDef(name='maxPerStageDescriptorInlineUniformBlocks', type='uint32_t'),
var5=VarDef(name='maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks', type='uint32_t'),
var6=VarDef(name='maxDescriptorSetInlineUniformBlocks', type='uint32_t'),
var7=VarDef(name='maxDescriptorSetUpdateAfterBindInlineUniformBlocks', type='uint32_t')
)

Struct(name='VkPhysicalDeviceLegacyDitheringFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='legacyDithering', type='VkBool32')
)

Struct(name='VkPhysicalDeviceLimits_', enabled=False,
var1=VarDef(name='maxImageDimension1D', type='uint32_t'),
var2=VarDef(name='maxImageDimension2D', type='uint32_t'),
var3=VarDef(name='maxImageDimension3D', type='uint32_t'),
var4=VarDef(name='maxImageDimensionCube', type='uint32_t'),
var5=VarDef(name='maxImageArrayLayers', type='uint32_t'),
var6=VarDef(name='maxTexelBufferElements', type='uint32_t'),
var7=VarDef(name='maxUniformBufferRange', type='uint32_t'),
var8=VarDef(name='maxStorageBufferRange', type='uint32_t'),
var9=VarDef(name='maxPushConstantsSize', type='uint32_t'),
var10=VarDef(name='maxMemoryAllocationCount', type='uint32_t'),
var11=VarDef(name='maxSamplerAllocationCount', type='uint32_t'),
var12=VarDef(name='bufferImageGranularity', type='VkDeviceSize'),
var13=VarDef(name='sparseAddressSpaceSize', type='VkDeviceSize'),
var14=VarDef(name='maxBoundDescriptorSets', type='uint32_t'),
var15=VarDef(name='maxPerStageDescriptorSamplers', type='uint32_t'),
var16=VarDef(name='maxPerStageDescriptorUniformBuffers', type='uint32_t'),
var17=VarDef(name='maxPerStageDescriptorStorageBuffers', type='uint32_t'),
var18=VarDef(name='maxPerStageDescriptorSampledImages', type='uint32_t'),
var19=VarDef(name='maxPerStageDescriptorStorageImages', type='uint32_t'),
var20=VarDef(name='maxPerStageDescriptorInputAttachments', type='uint32_t'),
var21=VarDef(name='maxPerStageResources', type='uint32_t'),
var22=VarDef(name='maxDescriptorSetSamplers', type='uint32_t'),
var23=VarDef(name='maxDescriptorSetUniformBuffers', type='uint32_t'),
var24=VarDef(name='maxDescriptorSetUniformBuffersDynamic', type='uint32_t'),
var25=VarDef(name='maxDescriptorSetStorageBuffers', type='uint32_t'),
var26=VarDef(name='maxDescriptorSetStorageBuffersDynamic', type='uint32_t'),
var27=VarDef(name='maxDescriptorSetSampledImages', type='uint32_t'),
var28=VarDef(name='maxDescriptorSetStorageImages', type='uint32_t'),
var29=VarDef(name='maxDescriptorSetInputAttachments', type='uint32_t'),
var30=VarDef(name='maxVertexInputAttributes', type='uint32_t'),
var31=VarDef(name='maxVertexInputBindings', type='uint32_t'),
var32=VarDef(name='maxVertexInputAttributeOffset', type='uint32_t'),
var33=VarDef(name='maxVertexInputBindingStride', type='uint32_t'),
var34=VarDef(name='maxVertexOutputComponents', type='uint32_t'),
var35=VarDef(name='maxTessellationGenerationLevel', type='uint32_t'),
var36=VarDef(name='maxTessellationPatchSize', type='uint32_t'),
var37=VarDef(name='maxTessellationControlPerVertexInputComponents', type='uint32_t'),
var38=VarDef(name='maxTessellationControlPerVertexOutputComponents', type='uint32_t'),
var39=VarDef(name='maxTessellationControlPerPatchOutputComponents', type='uint32_t'),
var40=VarDef(name='maxTessellationControlTotalOutputComponents', type='uint32_t'),
var41=VarDef(name='maxTessellationEvaluationInputComponents', type='uint32_t'),
var42=VarDef(name='maxTessellationEvaluationOutputComponents', type='uint32_t'),
var43=VarDef(name='maxGeometryShaderInvocations', type='uint32_t'),
var44=VarDef(name='maxGeometryInputComponents', type='uint32_t'),
var45=VarDef(name='maxGeometryOutputComponents', type='uint32_t'),
var46=VarDef(name='maxGeometryOutputVertices', type='uint32_t'),
var47=VarDef(name='maxGeometryTotalOutputComponents', type='uint32_t'),
var48=VarDef(name='maxFragmentInputComponents', type='uint32_t'),
var49=VarDef(name='maxFragmentOutputAttachments', type='uint32_t'),
var50=VarDef(name='maxFragmentDualSrcAttachments', type='uint32_t'),
var51=VarDef(name='maxFragmentCombinedOutputResources', type='uint32_t'),
var52=VarDef(name='maxComputeSharedMemorySize', type='uint32_t'),
var53=VarDef(name='maxComputeWorkGroupCount', type='uint32_t[3]', count='3'),
var54=VarDef(name='maxComputeWorkGroupInvocations', type='uint32_t'),
var55=VarDef(name='maxComputeWorkGroupSize', type='uint32_t[3]', count='3'),
var56=VarDef(name='subPixelPrecisionBits', type='uint32_t'),
var57=VarDef(name='subTexelPrecisionBits', type='uint32_t'),
var58=VarDef(name='mipmapPrecisionBits', type='uint32_t'),
var59=VarDef(name='maxDrawIndexedIndexValue', type='uint32_t'),
var60=VarDef(name='maxDrawIndirectCount', type='uint32_t'),
var61=VarDef(name='maxSamplerLodBias', type='float'),
var62=VarDef(name='maxSamplerAnisotropy', type='float'),
var63=VarDef(name='maxViewports', type='uint32_t'),
var64=VarDef(name='maxViewportDimensions', type='uint32_t[2]', count='2'),
var65=VarDef(name='viewportBoundsRange', type='float[2]', count='2'),
var66=VarDef(name='viewportSubPixelBits', type='uint32_t'),
var67=VarDef(name='minMemoryMapAlignment', type='size_t'),
var68=VarDef(name='minTexelBufferOffsetAlignment', type='VkDeviceSize'),
var69=VarDef(name='minUniformBufferOffsetAlignment', type='VkDeviceSize'),
var70=VarDef(name='minStorageBufferOffsetAlignment', type='VkDeviceSize'),
var71=VarDef(name='minTexelOffset', type='int32_t'),
var72=VarDef(name='maxTexelOffset', type='uint32_t'),
var73=VarDef(name='minTexelGatherOffset', type='int32_t'),
var74=VarDef(name='maxTexelGatherOffset', type='uint32_t'),
var75=VarDef(name='minInterpolationOffset', type='float'),
var76=VarDef(name='maxInterpolationOffset', type='float'),
var77=VarDef(name='subPixelInterpolationOffsetBits', type='uint32_t'),
var78=VarDef(name='maxFramebufferWidth', type='uint32_t'),
var79=VarDef(name='maxFramebufferHeight', type='uint32_t'),
var80=VarDef(name='maxFramebufferLayers', type='uint32_t'),
var81=VarDef(name='framebufferColorSampleCounts', type='VkSampleCountFlags'),
var82=VarDef(name='framebufferDepthSampleCounts', type='VkSampleCountFlags'),
var83=VarDef(name='framebufferStencilSampleCounts', type='VkSampleCountFlags'),
var84=VarDef(name='framebufferNoAttachmentsSampleCounts', type='VkSampleCountFlags'),
var85=VarDef(name='maxColorAttachments', type='uint32_t'),
var86=VarDef(name='sampledImageColorSampleCounts', type='VkSampleCountFlags'),
var87=VarDef(name='sampledImageIntegerSampleCounts', type='VkSampleCountFlags'),
var88=VarDef(name='sampledImageDepthSampleCounts', type='VkSampleCountFlags'),
var89=VarDef(name='sampledImageStencilSampleCounts', type='VkSampleCountFlags'),
var90=VarDef(name='storageImageSampleCounts', type='VkSampleCountFlags'),
var91=VarDef(name='maxSampleMaskWords', type='uint32_t'),
var92=VarDef(name='timestampComputeAndGraphics', type='VkBool32'),
var93=VarDef(name='timestampPeriod', type='float'),
var94=VarDef(name='maxClipDistances', type='uint32_t'),
var95=VarDef(name='maxCullDistances', type='uint32_t'),
var96=VarDef(name='maxCombinedClipAndCullDistances', type='uint32_t'),
var97=VarDef(name='discreteQueuePriorities', type='uint32_t'),
var98=VarDef(name='pointSizeRange', type='float[2]', count='2'),
var99=VarDef(name='lineWidthRange', type='float[2]', count='2'),
var100=VarDef(name='pointSizeGranularity', type='float'),
var101=VarDef(name='lineWidthGranularity', type='float'),
var102=VarDef(name='strictLines', type='VkBool32'),
var103=VarDef(name='standardSampleLocations', type='VkBool32'),
var104=VarDef(name='optimalBufferCopyOffsetAlignment', type='VkDeviceSize'),
var105=VarDef(name='optimalBufferCopyRowPitchAlignment', type='VkDeviceSize'),
var106=VarDef(name='nonCoherentAtomSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceLineRasterizationFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rectangularLines', type='VkBool32'),
var4=VarDef(name='bresenhamLines', type='VkBool32'),
var5=VarDef(name='smoothLines', type='VkBool32'),
var6=VarDef(name='stippledRectangularLines', type='VkBool32'),
var7=VarDef(name='stippledBresenhamLines', type='VkBool32'),
var8=VarDef(name='stippledSmoothLines', type='VkBool32')
)

Struct(name='VkPhysicalDeviceLineRasterizationPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='lineSubPixelPrecisionBits', type='uint32_t')
)

Struct(name='VkPhysicalDeviceLinearColorAttachmentFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='linearColorAttachment', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMaintenance3Properties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxPerSetDescriptors', type='uint32_t'),
var4=VarDef(name='maxMemoryAllocationSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceMaintenance4Features_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maintenance4', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMaintenance4FeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maintenance4', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMaintenance4Properties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxBufferSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceMaintenance4PropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxBufferSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceMemoryBudgetPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='heapBudget', type='VkDeviceSize[16]'),
var4=VarDef(name='heapUsage', type='VkDeviceSize[16]')
)

Struct(name='VkPhysicalDeviceMemoryDecompressionFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryDecompression', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMemoryDecompressionPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='decompressionMethods', type='VkMemoryDecompressionMethodFlagsNV'),
var4=VarDef(name='maxDecompressionIndirectCount', type='uint64_t')
)

Struct(name='VkPhysicalDeviceMemoryPriorityFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryPriority', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMemoryProperties_', enabled=True,
var1=VarDef(name='memoryTypeCount', type='uint32_t'),
var2=VarDef(name='memoryTypes', type='VkMemoryType[32]', wrapType='CVkMemoryTypeArray', count='memoryTypeCount'),
var3=VarDef(name='memoryHeapCount', type='uint32_t'),
var4=VarDef(name='memoryHeaps', type='VkMemoryHeap[16]', wrapType='CVkMemoryHeapArray', count='memoryHeapCount')
)

Struct(name='VkPhysicalDeviceMemoryProperties2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryProperties', type='VkPhysicalDeviceMemoryProperties')
)

Struct(name='VkPhysicalDeviceMeshShaderFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='taskShader', type='VkBool32'),
var4=VarDef(name='meshShader', type='VkBool32'),
var5=VarDef(name='multiviewMeshShader', type='VkBool32'),
var6=VarDef(name='primitiveFragmentShadingRateMeshShader', type='VkBool32'),
var7=VarDef(name='meshShaderQueries', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMeshShaderFeaturesNV_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='taskShader', type='VkBool32'),
var4=VarDef(name='meshShader', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMeshShaderPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxTaskWorkGroupTotalCount', type='uint32_t'),
var4=VarDef(name='maxTaskWorkGroupCount', type='uint32_t[3]'),
var5=VarDef(name='maxTaskWorkGroupInvocations', type='uint32_t'),
var6=VarDef(name='maxTaskWorkGroupSize', type='uint32_t[3]'),
var7=VarDef(name='maxTaskPayloadSize', type='uint32_t'),
var8=VarDef(name='maxTaskSharedMemorySize', type='uint32_t'),
var9=VarDef(name='maxTaskPayloadAndSharedMemorySize', type='uint32_t'),
var10=VarDef(name='maxMeshWorkGroupTotalCount', type='uint32_t'),
var11=VarDef(name='maxMeshWorkGroupCount', type='uint32_t[3]'),
var12=VarDef(name='maxMeshWorkGroupInvocations', type='uint32_t'),
var13=VarDef(name='maxMeshWorkGroupSize', type='uint32_t[3]'),
var14=VarDef(name='maxMeshSharedMemorySize', type='uint32_t'),
var15=VarDef(name='maxMeshPayloadAndSharedMemorySize', type='uint32_t'),
var16=VarDef(name='maxMeshOutputMemorySize', type='uint32_t'),
var17=VarDef(name='maxMeshPayloadAndOutputMemorySize', type='uint32_t'),
var18=VarDef(name='maxMeshOutputComponents', type='uint32_t'),
var19=VarDef(name='maxMeshOutputVertices', type='uint32_t'),
var20=VarDef(name='maxMeshOutputPrimitives', type='uint32_t'),
var21=VarDef(name='maxMeshOutputLayers', type='uint32_t'),
var22=VarDef(name='maxMeshMultiviewViewCount', type='uint32_t'),
var23=VarDef(name='meshOutputPerVertexGranularity', type='uint32_t'),
var24=VarDef(name='meshOutputPerPrimitiveGranularity', type='uint32_t'),
var25=VarDef(name='maxPreferredTaskWorkGroupInvocations', type='uint32_t'),
var26=VarDef(name='maxPreferredMeshWorkGroupInvocations', type='uint32_t'),
var27=VarDef(name='prefersLocalInvocationVertexOutput', type='VkBool32'),
var28=VarDef(name='prefersLocalInvocationPrimitiveOutput', type='VkBool32'),
var29=VarDef(name='prefersCompactVertexOutput', type='VkBool32'),
var30=VarDef(name='prefersCompactPrimitiveOutput', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMeshShaderPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxDrawMeshTasksCount', type='uint32_t'),
var4=VarDef(name='maxTaskWorkGroupInvocations', type='uint32_t'),
var5=VarDef(name='maxTaskWorkGroupSize', type='uint32_t[3]'),
var6=VarDef(name='maxTaskTotalMemorySize', type='uint32_t'),
var7=VarDef(name='maxTaskOutputCount', type='uint32_t'),
var8=VarDef(name='maxMeshWorkGroupInvocations', type='uint32_t'),
var9=VarDef(name='maxMeshWorkGroupSize', type='uint32_t[3]'),
var10=VarDef(name='maxMeshTotalMemorySize', type='uint32_t'),
var11=VarDef(name='maxMeshOutputVertices', type='uint32_t'),
var12=VarDef(name='maxMeshOutputPrimitives', type='uint32_t'),
var13=VarDef(name='maxMeshMultiviewViewCount', type='uint32_t'),
var14=VarDef(name='meshOutputPerVertexGranularity', type='uint32_t'),
var15=VarDef(name='meshOutputPerPrimitiveGranularity', type='uint32_t')
)

Struct(name='VkPhysicalDeviceMultiDrawFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='multiDraw', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMultiDrawPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxMultiDrawCount', type='uint32_t')
)

Struct(name='VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='multisampledRenderToSingleSampled', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMultiviewFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='multiview', type='VkBool32'),
var4=VarDef(name='multiviewGeometryShader', type='VkBool32'),
var5=VarDef(name='multiviewTessellationShader', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='perViewPositionAllComponents', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMultiviewProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxMultiviewViewCount', type='uint32_t'),
var4=VarDef(name='maxMultiviewInstanceIndex', type='uint32_t')
)

Struct(name='VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='mutableDescriptorType', type='VkBool32')
)

Struct(name='VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='mutableDescriptorType', type='VkBool32')
)

Struct(name='VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='nonSeamlessCubeMap', type='VkBool32')
)

Struct(name='VkPhysicalDeviceOpacityMicromapFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='micromap', type='VkBool32'),
var4=VarDef(name='micromapCaptureReplay', type='VkBool32'),
var5=VarDef(name='micromapHostCommands', type='VkBool32')
)

Struct(name='VkPhysicalDeviceOpacityMicromapPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxOpacity2StateSubdivisionLevel', type='uint32_t'),
var4=VarDef(name='maxOpacity4StateSubdivisionLevel', type='uint32_t')
)

Struct(name='VkPhysicalDeviceOpticalFlowFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='opticalFlow', type='VkBool32')
)

Struct(name='VkPhysicalDeviceOpticalFlowPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='supportedOutputGridSizes', type='VkOpticalFlowGridSizeFlagsNV'),
var4=VarDef(name='supportedHintGridSizes', type='VkOpticalFlowGridSizeFlagsNV'),
var5=VarDef(name='hintSupported', type='VkBool32'),
var6=VarDef(name='costSupported', type='VkBool32'),
var7=VarDef(name='bidirectionalFlowSupported', type='VkBool32'),
var8=VarDef(name='globalFlowSupported', type='VkBool32'),
var9=VarDef(name='minWidth', type='uint32_t'),
var10=VarDef(name='minHeight', type='uint32_t'),
var11=VarDef(name='maxWidth', type='uint32_t'),
var12=VarDef(name='maxHeight', type='uint32_t'),
var13=VarDef(name='maxNumRegionsOfInterest', type='uint32_t')
)

Struct(name='VkPhysicalDevicePCIBusInfoPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pciDomain', type='uint32_t'),
var4=VarDef(name='pciBus', type='uint32_t'),
var5=VarDef(name='pciDevice', type='uint32_t'),
var6=VarDef(name='pciFunction', type='uint32_t')
)

Struct(name='VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pageableDeviceLocalMemory', type='VkBool32')
)

Struct(name='VkPhysicalDevicePerformanceQueryFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='performanceCounterQueryPools', type='VkBool32'),
var4=VarDef(name='performanceCounterMultipleQueryPools', type='VkBool32')
)

Struct(name='VkPhysicalDevicePerformanceQueryPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='allowCommandBufferQueryCopies', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineCreationCacheControlFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineCreationCacheControl', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineCreationCacheControl', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineExecutableInfo', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineLibraryGroupHandles', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelinePropertiesFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelinePropertiesIdentifier', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineProtectedAccessFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineProtectedAccess', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineRobustnessFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineRobustness', type='VkBool32')
)

Struct(name='VkPhysicalDevicePipelineRobustnessPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='defaultRobustnessStorageBuffers', type='VkPipelineRobustnessBufferBehaviorEXT'),
var4=VarDef(name='defaultRobustnessUniformBuffers', type='VkPipelineRobustnessBufferBehaviorEXT'),
var5=VarDef(name='defaultRobustnessVertexInputs', type='VkPipelineRobustnessBufferBehaviorEXT'),
var6=VarDef(name='defaultRobustnessImages', type='VkPipelineRobustnessImageBehaviorEXT')
)

Struct(name='VkPhysicalDevicePointClippingProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pointClippingBehavior', type='VkPointClippingBehavior')
)

Struct(name='VkPhysicalDevicePortabilitySubsetFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='constantAlphaColorBlendFactors', type='VkBool32'),
var4=VarDef(name='events', type='VkBool32'),
var5=VarDef(name='imageViewFormatReinterpretation', type='VkBool32'),
var6=VarDef(name='imageViewFormatSwizzle', type='VkBool32'),
var7=VarDef(name='imageView2DOn3DImage', type='VkBool32'),
var8=VarDef(name='multisampleArrayImage', type='VkBool32'),
var9=VarDef(name='mutableComparisonSamplers', type='VkBool32'),
var10=VarDef(name='pointPolygons', type='VkBool32'),
var11=VarDef(name='samplerMipLodBias', type='VkBool32'),
var12=VarDef(name='separateStencilMaskRef', type='VkBool32'),
var13=VarDef(name='shaderSampleRateInterpolationFunctions', type='VkBool32'),
var14=VarDef(name='tessellationIsolines', type='VkBool32'),
var15=VarDef(name='tessellationPointMode', type='VkBool32'),
var16=VarDef(name='triangleFans', type='VkBool32'),
var17=VarDef(name='vertexAttributeAccessBeyondStride', type='VkBool32')
)

Struct(name='VkPhysicalDevicePortabilitySubsetPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minVertexInputBindingStrideAlignment', type='uint32_t')
)

Struct(name='VkPhysicalDevicePresentBarrierFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentBarrier', type='VkBool32')
)

Struct(name='VkPhysicalDevicePresentIdFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentId', type='VkBool32')
)

Struct(name='VkPhysicalDevicePresentWaitFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentWait', type='VkBool32')
)

Struct(name='VkPhysicalDevicePresentationPropertiesANDROID_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sharedImage', type='VkBool32')
)

Struct(name='VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='primitiveTopologyListRestart', type='VkBool32'),
var4=VarDef(name='primitiveTopologyPatchListRestart', type='VkBool32')
)

Struct(name='VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='primitivesGeneratedQuery', type='VkBool32'),
var4=VarDef(name='primitivesGeneratedQueryWithRasterizerDiscard', type='VkBool32'),
var5=VarDef(name='primitivesGeneratedQueryWithNonZeroStreams', type='VkBool32')
)

Struct(name='VkPhysicalDevicePrivateDataFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='privateData', type='VkBool32')
)

Struct(name='VkPhysicalDevicePrivateDataFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='privateData', type='VkBool32')
)

Struct(name='VkPhysicalDeviceProperties_', enabled=False,
var1=VarDef(name='apiVersion', type='uint32_t'),
var2=VarDef(name='driverVersion', type='uint32_t'),
var3=VarDef(name='vendorID', type='uint32_t'),
var4=VarDef(name='deviceID', type='uint32_t'),
var5=VarDef(name='deviceType', type='VkPhysicalDeviceType'),
var6=VarDef(name='deviceName', type='char[256]'),
var7=VarDef(name='pipelineCacheUUID', type='uint8_t[16]', count='16'),
var8=VarDef(name='limits', type='VkPhysicalDeviceLimits'),
var9=VarDef(name='sparseProperties', type='VkPhysicalDeviceSparseProperties')
)

Struct(name='VkPhysicalDeviceProperties2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='properties', type='VkPhysicalDeviceProperties')
)

Struct(name='VkPhysicalDeviceProtectedMemoryFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='protectedMemory', type='VkBool32')
)

Struct(name='VkPhysicalDeviceProtectedMemoryProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='protectedNoFault', type='VkBool32')
)

Struct(name='VkPhysicalDeviceProvokingVertexFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='provokingVertexLast', type='VkBool32'),
var4=VarDef(name='transformFeedbackPreservesProvokingVertex', type='VkBool32')
)

Struct(name='VkPhysicalDeviceProvokingVertexPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='provokingVertexModePerPipeline', type='VkBool32'),
var4=VarDef(name='transformFeedbackPreservesTriangleFanProvokingVertex', type='VkBool32')
)

Struct(name='VkPhysicalDevicePushDescriptorPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxPushDescriptors', type='uint32_t')
)

Struct(name='VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='formatRgba10x6WithoutYCbCrSampler', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rasterizationOrderColorAttachmentAccess', type='VkBool32'),
var4=VarDef(name='rasterizationOrderDepthAttachmentAccess', type='VkBool32'),
var5=VarDef(name='rasterizationOrderStencilAttachmentAccess', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRayQueryFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rayQuery', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rayTracingInvocationReorder', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rayTracingInvocationReorderReorderingHint', type='VkRayTracingInvocationReorderModeNV')
)

Struct(name='VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rayTracingMaintenance1', type='VkBool32'),
var4=VarDef(name='rayTracingPipelineTraceRaysIndirect2', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRayTracingMotionBlurFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rayTracingMotionBlur', type='VkBool32'),
var4=VarDef(name='rayTracingMotionBlurPipelineTraceRaysIndirect', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRayTracingPipelineFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='rayTracingPipeline', type='VkBool32'),
var4=VarDef(name='rayTracingPipelineShaderGroupHandleCaptureReplay', type='VkBool32'),
var5=VarDef(name='rayTracingPipelineShaderGroupHandleCaptureReplayMixed', type='VkBool32'),
var6=VarDef(name='rayTracingPipelineTraceRaysIndirect', type='VkBool32'),
var7=VarDef(name='rayTraversalPrimitiveCulling', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRayTracingPipelinePropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderGroupHandleSize', type='uint32_t'),
var4=VarDef(name='maxRayRecursionDepth', type='uint32_t'),
var5=VarDef(name='maxShaderGroupStride', type='uint32_t'),
var6=VarDef(name='shaderGroupBaseAlignment', type='uint32_t'),
var7=VarDef(name='shaderGroupHandleCaptureReplaySize', type='uint32_t'),
var8=VarDef(name='maxRayDispatchInvocationCount', type='uint32_t'),
var9=VarDef(name='shaderGroupHandleAlignment', type='uint32_t'),
var10=VarDef(name='maxRayHitAttributeSize', type='uint32_t')
)

Struct(name='VkPhysicalDeviceRayTracingPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderGroupHandleSize', type='uint32_t'),
var4=VarDef(name='maxRecursionDepth', type='uint32_t'),
var5=VarDef(name='maxShaderGroupStride', type='uint32_t'),
var6=VarDef(name='shaderGroupBaseAlignment', type='uint32_t'),
var7=VarDef(name='maxGeometryCount', type='uint64_t'),
var8=VarDef(name='maxInstanceCount', type='uint64_t'),
var9=VarDef(name='maxTriangleCount', type='uint64_t'),
var10=VarDef(name='maxDescriptorSetAccelerationStructures', type='uint32_t')
)

Struct(name='VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='representativeFragmentTest', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRobustness2FeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='robustBufferAccess2', type='VkBool32'),
var4=VarDef(name='robustImageAccess2', type='VkBool32'),
var5=VarDef(name='nullDescriptor', type='VkBool32')
)

Struct(name='VkPhysicalDeviceRobustness2PropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='robustStorageBufferAccessSizeAlignment', type='VkDeviceSize'),
var4=VarDef(name='robustUniformBufferAccessSizeAlignment', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceSampleLocationsPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='sampleLocationSampleCounts', type='VkSampleCountFlags'),
var4=VarDef(name='maxSampleLocationGridSize', type='VkExtent2D'),
var5=VarDef(name='sampleLocationCoordinateRange', type='float[2]'),
var6=VarDef(name='sampleLocationSubPixelBits', type='uint32_t'),
var7=VarDef(name='variableSampleLocations', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSamplerFilterMinmaxProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='filterMinmaxSingleComponentFormats', type='VkBool32'),
var4=VarDef(name='filterMinmaxImageComponentMapping', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSamplerYcbcrConversionFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='samplerYcbcrConversion', type='VkBool32')
)

Struct(name='VkPhysicalDeviceScalarBlockLayoutFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='scalarBlockLayout', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='separateDepthStencilLayouts', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='separateDepthStencilLayouts', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderBufferFloat16Atomics', type='VkBool32'),
var4=VarDef(name='shaderBufferFloat16AtomicAdd', type='VkBool32'),
var5=VarDef(name='shaderBufferFloat16AtomicMinMax', type='VkBool32'),
var6=VarDef(name='shaderBufferFloat32AtomicMinMax', type='VkBool32'),
var7=VarDef(name='shaderBufferFloat64AtomicMinMax', type='VkBool32'),
var8=VarDef(name='shaderSharedFloat16Atomics', type='VkBool32'),
var9=VarDef(name='shaderSharedFloat16AtomicAdd', type='VkBool32'),
var10=VarDef(name='shaderSharedFloat16AtomicMinMax', type='VkBool32'),
var11=VarDef(name='shaderSharedFloat32AtomicMinMax', type='VkBool32'),
var12=VarDef(name='shaderSharedFloat64AtomicMinMax', type='VkBool32'),
var13=VarDef(name='shaderImageFloat32AtomicMinMax', type='VkBool32'),
var14=VarDef(name='sparseImageFloat32AtomicMinMax', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderAtomicFloatFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderBufferFloat32Atomics', type='VkBool32'),
var4=VarDef(name='shaderBufferFloat32AtomicAdd', type='VkBool32'),
var5=VarDef(name='shaderBufferFloat64Atomics', type='VkBool32'),
var6=VarDef(name='shaderBufferFloat64AtomicAdd', type='VkBool32'),
var7=VarDef(name='shaderSharedFloat32Atomics', type='VkBool32'),
var8=VarDef(name='shaderSharedFloat32AtomicAdd', type='VkBool32'),
var9=VarDef(name='shaderSharedFloat64Atomics', type='VkBool32'),
var10=VarDef(name='shaderSharedFloat64AtomicAdd', type='VkBool32'),
var11=VarDef(name='shaderImageFloat32Atomics', type='VkBool32'),
var12=VarDef(name='shaderImageFloat32AtomicAdd', type='VkBool32'),
var13=VarDef(name='sparseImageFloat32Atomics', type='VkBool32'),
var14=VarDef(name='sparseImageFloat32AtomicAdd', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderAtomicInt64Features_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderBufferInt64Atomics', type='VkBool32'),
var4=VarDef(name='shaderSharedInt64Atomics', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderClockFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderSubgroupClock', type='VkBool32'),
var4=VarDef(name='shaderDeviceClock', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderCoreProperties2AMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderCoreFeatures', type='VkShaderCorePropertiesFlagsAMD'),
var4=VarDef(name='activeComputeUnitCount', type='uint32_t')
)

Struct(name='VkPhysicalDeviceShaderCorePropertiesAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderEngineCount', type='uint32_t'),
var4=VarDef(name='shaderArraysPerEngineCount', type='uint32_t'),
var5=VarDef(name='computeUnitsPerShaderArray', type='uint32_t'),
var6=VarDef(name='simdPerComputeUnit', type='uint32_t'),
var7=VarDef(name='wavefrontsPerSimd', type='uint32_t'),
var8=VarDef(name='wavefrontSize', type='uint32_t'),
var9=VarDef(name='sgprsPerSimd', type='uint32_t'),
var10=VarDef(name='minSgprAllocation', type='uint32_t'),
var11=VarDef(name='maxSgprAllocation', type='uint32_t'),
var12=VarDef(name='sgprAllocationGranularity', type='uint32_t'),
var13=VarDef(name='vgprsPerSimd', type='uint32_t'),
var14=VarDef(name='minVgprAllocation', type='uint32_t'),
var15=VarDef(name='maxVgprAllocation', type='uint32_t'),
var16=VarDef(name='vgprAllocationGranularity', type='uint32_t')
)

Struct(name='VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderDemoteToHelperInvocation', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderDemoteToHelperInvocation', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderDrawParametersFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderDrawParameters', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderEarlyAndLateFragmentTests', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderFloat16Int8Features_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderFloat16', type='VkBool32'),
var4=VarDef(name='shaderInt8', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderImageInt64Atomics', type='VkBool32'),
var4=VarDef(name='sparseImageInt64Atomics', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderImageFootprintFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='imageFootprint', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderIntegerDotProductFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderIntegerDotProduct', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderIntegerDotProduct', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderIntegerDotProductProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='integerDotProduct8BitUnsignedAccelerated', type='VkBool32'),
var4=VarDef(name='integerDotProduct8BitSignedAccelerated', type='VkBool32'),
var5=VarDef(name='integerDotProduct8BitMixedSignednessAccelerated', type='VkBool32'),
var6=VarDef(name='integerDotProduct4x8BitPackedUnsignedAccelerated', type='VkBool32'),
var7=VarDef(name='integerDotProduct4x8BitPackedSignedAccelerated', type='VkBool32'),
var8=VarDef(name='integerDotProduct4x8BitPackedMixedSignednessAccelerated', type='VkBool32'),
var9=VarDef(name='integerDotProduct16BitUnsignedAccelerated', type='VkBool32'),
var10=VarDef(name='integerDotProduct16BitSignedAccelerated', type='VkBool32'),
var11=VarDef(name='integerDotProduct16BitMixedSignednessAccelerated', type='VkBool32'),
var12=VarDef(name='integerDotProduct32BitUnsignedAccelerated', type='VkBool32'),
var13=VarDef(name='integerDotProduct32BitSignedAccelerated', type='VkBool32'),
var14=VarDef(name='integerDotProduct32BitMixedSignednessAccelerated', type='VkBool32'),
var15=VarDef(name='integerDotProduct64BitUnsignedAccelerated', type='VkBool32'),
var16=VarDef(name='integerDotProduct64BitSignedAccelerated', type='VkBool32'),
var17=VarDef(name='integerDotProduct64BitMixedSignednessAccelerated', type='VkBool32'),
var18=VarDef(name='integerDotProductAccumulatingSaturating8BitUnsignedAccelerated', type='VkBool32'),
var19=VarDef(name='integerDotProductAccumulatingSaturating8BitSignedAccelerated', type='VkBool32'),
var20=VarDef(name='integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated', type='VkBool32'),
var21=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated', type='VkBool32'),
var22=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated', type='VkBool32'),
var23=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated', type='VkBool32'),
var24=VarDef(name='integerDotProductAccumulatingSaturating16BitUnsignedAccelerated', type='VkBool32'),
var25=VarDef(name='integerDotProductAccumulatingSaturating16BitSignedAccelerated', type='VkBool32'),
var26=VarDef(name='integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated', type='VkBool32'),
var27=VarDef(name='integerDotProductAccumulatingSaturating32BitUnsignedAccelerated', type='VkBool32'),
var28=VarDef(name='integerDotProductAccumulatingSaturating32BitSignedAccelerated', type='VkBool32'),
var29=VarDef(name='integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated', type='VkBool32'),
var30=VarDef(name='integerDotProductAccumulatingSaturating64BitUnsignedAccelerated', type='VkBool32'),
var31=VarDef(name='integerDotProductAccumulatingSaturating64BitSignedAccelerated', type='VkBool32'),
var32=VarDef(name='integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderIntegerDotProductPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='integerDotProduct8BitUnsignedAccelerated', type='VkBool32'),
var4=VarDef(name='integerDotProduct8BitSignedAccelerated', type='VkBool32'),
var5=VarDef(name='integerDotProduct8BitMixedSignednessAccelerated', type='VkBool32'),
var6=VarDef(name='integerDotProduct4x8BitPackedUnsignedAccelerated', type='VkBool32'),
var7=VarDef(name='integerDotProduct4x8BitPackedSignedAccelerated', type='VkBool32'),
var8=VarDef(name='integerDotProduct4x8BitPackedMixedSignednessAccelerated', type='VkBool32'),
var9=VarDef(name='integerDotProduct16BitUnsignedAccelerated', type='VkBool32'),
var10=VarDef(name='integerDotProduct16BitSignedAccelerated', type='VkBool32'),
var11=VarDef(name='integerDotProduct16BitMixedSignednessAccelerated', type='VkBool32'),
var12=VarDef(name='integerDotProduct32BitUnsignedAccelerated', type='VkBool32'),
var13=VarDef(name='integerDotProduct32BitSignedAccelerated', type='VkBool32'),
var14=VarDef(name='integerDotProduct32BitMixedSignednessAccelerated', type='VkBool32'),
var15=VarDef(name='integerDotProduct64BitUnsignedAccelerated', type='VkBool32'),
var16=VarDef(name='integerDotProduct64BitSignedAccelerated', type='VkBool32'),
var17=VarDef(name='integerDotProduct64BitMixedSignednessAccelerated', type='VkBool32'),
var18=VarDef(name='integerDotProductAccumulatingSaturating8BitUnsignedAccelerated', type='VkBool32'),
var19=VarDef(name='integerDotProductAccumulatingSaturating8BitSignedAccelerated', type='VkBool32'),
var20=VarDef(name='integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated', type='VkBool32'),
var21=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated', type='VkBool32'),
var22=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated', type='VkBool32'),
var23=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated', type='VkBool32'),
var24=VarDef(name='integerDotProductAccumulatingSaturating16BitUnsignedAccelerated', type='VkBool32'),
var25=VarDef(name='integerDotProductAccumulatingSaturating16BitSignedAccelerated', type='VkBool32'),
var26=VarDef(name='integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated', type='VkBool32'),
var27=VarDef(name='integerDotProductAccumulatingSaturating32BitUnsignedAccelerated', type='VkBool32'),
var28=VarDef(name='integerDotProductAccumulatingSaturating32BitSignedAccelerated', type='VkBool32'),
var29=VarDef(name='integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated', type='VkBool32'),
var30=VarDef(name='integerDotProductAccumulatingSaturating64BitUnsignedAccelerated', type='VkBool32'),
var31=VarDef(name='integerDotProductAccumulatingSaturating64BitSignedAccelerated', type='VkBool32'),
var32=VarDef(name='integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderIntegerFunctions2', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderModuleIdentifier', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderModuleIdentifierAlgorithmUUID', type='uint8_t[16]')
)

Struct(name='VkPhysicalDeviceShaderObjectFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderObject', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderObjectPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderBinaryUUID', type='uint8_t[16]'),
var4=VarDef(name='shaderBinaryVersion', type='uint32_t')
)

Struct(name='VkPhysicalDeviceShaderSMBuiltinsFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderSMBuiltins', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderSMBuiltinsPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderSMCount', type='uint32_t'),
var4=VarDef(name='shaderWarpsPerSM', type='uint32_t')
)

Struct(name='VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderSubgroupExtendedTypes', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderSubgroupExtendedTypes', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderSubgroupUniformControlFlow', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderTerminateInvocationFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderTerminateInvocation', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderTerminateInvocation', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderTileImageFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderTileImageColorReadAccess', type='VkBool32'),
var4=VarDef(name='shaderTileImageDepthReadAccess', type='VkBool32'),
var5=VarDef(name='shaderTileImageStencilReadAccess', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShaderTileImagePropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderTileImageCoherentReadAccelerated', type='VkBool32'),
var4=VarDef(name='shaderTileImageReadSampleFromPixelRateInvocation', type='VkBool32'),
var5=VarDef(name='shaderTileImageReadFromHelperInvocation', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShadingRateImageFeaturesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shadingRateImage', type='VkBool32'),
var4=VarDef(name='shadingRateCoarseSampleOrder', type='VkBool32')
)

Struct(name='VkPhysicalDeviceShadingRateImagePropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shadingRateTexelSize', type='VkExtent2D'),
var4=VarDef(name='shadingRatePaletteSize', type='uint32_t'),
var5=VarDef(name='shadingRateMaxCoarseSamples', type='uint32_t')
)

Struct(name='VkPhysicalDeviceSparseImageFormatInfo2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='format', type='VkFormat'),
var4=VarDef(name='type', type='VkImageType'),
var5=VarDef(name='samples', type='VkSampleCountFlagBits'),
var6=VarDef(name='usage', type='VkImageUsageFlags'),
var7=VarDef(name='tiling', type='VkImageTiling')
)

Struct(name='VkPhysicalDeviceSparseProperties_', enabled=False,
var1=VarDef(name='residencyStandard2DBlockShape', type='VkBool32'),
var2=VarDef(name='residencyStandard2DMultisampleBlockShape', type='VkBool32'),
var3=VarDef(name='residencyStandard3DBlockShape', type='VkBool32'),
var4=VarDef(name='residencyAlignedMipSize', type='VkBool32'),
var5=VarDef(name='residencyNonResidentStrict', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSubgroupProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='subgroupSize', type='uint32_t'),
var4=VarDef(name='supportedStages', type='VkShaderStageFlags'),
var5=VarDef(name='supportedOperations', type='VkSubgroupFeatureFlags'),
var6=VarDef(name='quadOperationsInAllStages', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSubgroupSizeControlFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='subgroupSizeControl', type='VkBool32'),
var4=VarDef(name='computeFullSubgroups', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSubgroupSizeControlFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='subgroupSizeControl', type='VkBool32'),
var4=VarDef(name='computeFullSubgroups', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSubgroupSizeControlProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minSubgroupSize', type='uint32_t'),
var4=VarDef(name='maxSubgroupSize', type='uint32_t'),
var5=VarDef(name='maxComputeWorkgroupSubgroups', type='uint32_t'),
var6=VarDef(name='requiredSubgroupSizeStages', type='VkShaderStageFlags')
)

Struct(name='VkPhysicalDeviceSubgroupSizeControlPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minSubgroupSize', type='uint32_t'),
var4=VarDef(name='maxSubgroupSize', type='uint32_t'),
var5=VarDef(name='maxComputeWorkgroupSubgroups', type='uint32_t'),
var6=VarDef(name='requiredSubgroupSizeStages', type='VkShaderStageFlags')
)

Struct(name='VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='subpassMergeFeedback', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSurfaceInfo2KHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='surface', type='VkSurfaceKHR')
)

Struct(name='VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='swapchainMaintenance1', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSynchronization2Features_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='synchronization2', type='VkBool32')
)

Struct(name='VkPhysicalDeviceSynchronization2FeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='synchronization2', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='texelBufferAlignment', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTexelBufferAlignmentProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='storageTexelBufferOffsetAlignmentBytes', type='VkDeviceSize'),
var4=VarDef(name='storageTexelBufferOffsetSingleTexelAlignment', type='VkBool32'),
var5=VarDef(name='uniformTexelBufferOffsetAlignmentBytes', type='VkDeviceSize'),
var6=VarDef(name='uniformTexelBufferOffsetSingleTexelAlignment', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='storageTexelBufferOffsetAlignmentBytes', type='VkDeviceSize'),
var4=VarDef(name='storageTexelBufferOffsetSingleTexelAlignment', type='VkBool32'),
var5=VarDef(name='uniformTexelBufferOffsetAlignmentBytes', type='VkDeviceSize'),
var6=VarDef(name='uniformTexelBufferOffsetSingleTexelAlignment', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTextureCompressionASTCHDRFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='textureCompressionASTC_HDR', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='textureCompressionASTC_HDR', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTimelineSemaphoreFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='timelineSemaphore', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTimelineSemaphoreProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxTimelineSemaphoreValueDifference', type='uint64_t')
)

Struct(name='VkPhysicalDeviceTimelineSemaphorePropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxTimelineSemaphoreValueDifference', type='uint64_t')
)

Struct(name='VkPhysicalDeviceToolProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='name', type='char[256]'),
var4=VarDef(name='version', type='char[256]'),
var5=VarDef(name='purposes', type='VkToolPurposeFlags'),
var6=VarDef(name='description', type='char[256]'),
var7=VarDef(name='layer', type='char[256]')
)

Struct(name='VkPhysicalDeviceTransformFeedbackFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='transformFeedback', type='VkBool32'),
var4=VarDef(name='geometryStreams', type='VkBool32')
)

Struct(name='VkPhysicalDeviceTransformFeedbackPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxTransformFeedbackStreams', type='uint32_t'),
var4=VarDef(name='maxTransformFeedbackBuffers', type='uint32_t'),
var5=VarDef(name='maxTransformFeedbackBufferSize', type='VkDeviceSize'),
var6=VarDef(name='maxTransformFeedbackStreamDataSize', type='uint32_t'),
var7=VarDef(name='maxTransformFeedbackBufferDataSize', type='uint32_t'),
var8=VarDef(name='maxTransformFeedbackBufferDataStride', type='uint32_t'),
var9=VarDef(name='transformFeedbackQueries', type='VkBool32'),
var10=VarDef(name='transformFeedbackStreamsLinesTriangles', type='VkBool32'),
var11=VarDef(name='transformFeedbackRasterizationStreamSelect', type='VkBool32'),
var12=VarDef(name='transformFeedbackDraw', type='VkBool32')
)

Struct(name='VkPhysicalDeviceUniformBufferStandardLayoutFeatures_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='uniformBufferStandardLayout', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVariablePointersFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='variablePointersStorageBuffer', type='VkBool32'),
var4=VarDef(name='variablePointers', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='vertexAttributeInstanceRateDivisor', type='VkBool32'),
var4=VarDef(name='vertexAttributeInstanceRateZeroDivisor', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='maxVertexAttribDivisor', type='uint32_t')
)

Struct(name='VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='vertexInputDynamicState', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVideoFormatInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageUsage', type='VkImageUsageFlags')
)

Struct(name='VkPhysicalDeviceVulkan11Features_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='storageBuffer16BitAccess', type='VkBool32'),
var4=VarDef(name='uniformAndStorageBuffer16BitAccess', type='VkBool32'),
var5=VarDef(name='storagePushConstant16', type='VkBool32'),
var6=VarDef(name='storageInputOutput16', type='VkBool32'),
var7=VarDef(name='multiview', type='VkBool32'),
var8=VarDef(name='multiviewGeometryShader', type='VkBool32'),
var9=VarDef(name='multiviewTessellationShader', type='VkBool32'),
var10=VarDef(name='variablePointersStorageBuffer', type='VkBool32'),
var11=VarDef(name='variablePointers', type='VkBool32'),
var12=VarDef(name='protectedMemory', type='VkBool32'),
var13=VarDef(name='samplerYcbcrConversion', type='VkBool32'),
var14=VarDef(name='shaderDrawParameters', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVulkan11Properties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceUUID', type='uint8_t[16]'),
var4=VarDef(name='driverUUID', type='uint8_t[16]'),
var5=VarDef(name='deviceLUID', type='uint8_t[8]'),
var6=VarDef(name='deviceNodeMask', type='uint32_t'),
var7=VarDef(name='deviceLUIDValid', type='VkBool32'),
var8=VarDef(name='subgroupSize', type='uint32_t'),
var9=VarDef(name='subgroupSupportedStages', type='VkShaderStageFlags'),
var10=VarDef(name='subgroupSupportedOperations', type='VkSubgroupFeatureFlags'),
var11=VarDef(name='subgroupQuadOperationsInAllStages', type='VkBool32'),
var12=VarDef(name='pointClippingBehavior', type='VkPointClippingBehavior'),
var13=VarDef(name='maxMultiviewViewCount', type='uint32_t'),
var14=VarDef(name='maxMultiviewInstanceIndex', type='uint32_t'),
var15=VarDef(name='protectedNoFault', type='VkBool32'),
var16=VarDef(name='maxPerSetDescriptors', type='uint32_t'),
var17=VarDef(name='maxMemoryAllocationSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceVulkan12Features_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='samplerMirrorClampToEdge', type='VkBool32'),
var4=VarDef(name='drawIndirectCount', type='VkBool32'),
var5=VarDef(name='storageBuffer8BitAccess', type='VkBool32'),
var6=VarDef(name='uniformAndStorageBuffer8BitAccess', type='VkBool32'),
var7=VarDef(name='storagePushConstant8', type='VkBool32'),
var8=VarDef(name='shaderBufferInt64Atomics', type='VkBool32'),
var9=VarDef(name='shaderSharedInt64Atomics', type='VkBool32'),
var10=VarDef(name='shaderFloat16', type='VkBool32'),
var11=VarDef(name='shaderInt8', type='VkBool32'),
var12=VarDef(name='descriptorIndexing', type='VkBool32'),
var13=VarDef(name='shaderInputAttachmentArrayDynamicIndexing', type='VkBool32'),
var14=VarDef(name='shaderUniformTexelBufferArrayDynamicIndexing', type='VkBool32'),
var15=VarDef(name='shaderStorageTexelBufferArrayDynamicIndexing', type='VkBool32'),
var16=VarDef(name='shaderUniformBufferArrayNonUniformIndexing', type='VkBool32'),
var17=VarDef(name='shaderSampledImageArrayNonUniformIndexing', type='VkBool32'),
var18=VarDef(name='shaderStorageBufferArrayNonUniformIndexing', type='VkBool32'),
var19=VarDef(name='shaderStorageImageArrayNonUniformIndexing', type='VkBool32'),
var20=VarDef(name='shaderInputAttachmentArrayNonUniformIndexing', type='VkBool32'),
var21=VarDef(name='shaderUniformTexelBufferArrayNonUniformIndexing', type='VkBool32'),
var22=VarDef(name='shaderStorageTexelBufferArrayNonUniformIndexing', type='VkBool32'),
var23=VarDef(name='descriptorBindingUniformBufferUpdateAfterBind', type='VkBool32'),
var24=VarDef(name='descriptorBindingSampledImageUpdateAfterBind', type='VkBool32'),
var25=VarDef(name='descriptorBindingStorageImageUpdateAfterBind', type='VkBool32'),
var26=VarDef(name='descriptorBindingStorageBufferUpdateAfterBind', type='VkBool32'),
var27=VarDef(name='descriptorBindingUniformTexelBufferUpdateAfterBind', type='VkBool32'),
var28=VarDef(name='descriptorBindingStorageTexelBufferUpdateAfterBind', type='VkBool32'),
var29=VarDef(name='descriptorBindingUpdateUnusedWhilePending', type='VkBool32'),
var30=VarDef(name='descriptorBindingPartiallyBound', type='VkBool32'),
var31=VarDef(name='descriptorBindingVariableDescriptorCount', type='VkBool32'),
var32=VarDef(name='runtimeDescriptorArray', type='VkBool32'),
var33=VarDef(name='samplerFilterMinmax', type='VkBool32'),
var34=VarDef(name='scalarBlockLayout', type='VkBool32'),
var35=VarDef(name='imagelessFramebuffer', type='VkBool32'),
var36=VarDef(name='uniformBufferStandardLayout', type='VkBool32'),
var37=VarDef(name='shaderSubgroupExtendedTypes', type='VkBool32'),
var38=VarDef(name='separateDepthStencilLayouts', type='VkBool32'),
var39=VarDef(name='hostQueryReset', type='VkBool32'),
var40=VarDef(name='timelineSemaphore', type='VkBool32'),
var41=VarDef(name='bufferDeviceAddress', type='VkBool32'),
var42=VarDef(name='bufferDeviceAddressCaptureReplay', type='VkBool32'),
var43=VarDef(name='bufferDeviceAddressMultiDevice', type='VkBool32'),
var44=VarDef(name='vulkanMemoryModel', type='VkBool32'),
var45=VarDef(name='vulkanMemoryModelDeviceScope', type='VkBool32'),
var46=VarDef(name='vulkanMemoryModelAvailabilityVisibilityChains', type='VkBool32'),
var47=VarDef(name='shaderOutputViewportIndex', type='VkBool32'),
var48=VarDef(name='shaderOutputLayer', type='VkBool32'),
var49=VarDef(name='subgroupBroadcastDynamicId', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVulkan12Properties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='driverID', type='VkDriverId'),
var4=VarDef(name='driverName', type='char[256]'),
var5=VarDef(name='driverInfo', type='char[256]'),
var6=VarDef(name='conformanceVersion', type='VkConformanceVersion'),
var7=VarDef(name='denormBehaviorIndependence', type='VkShaderFloatControlsIndependence'),
var8=VarDef(name='roundingModeIndependence', type='VkShaderFloatControlsIndependence'),
var9=VarDef(name='shaderSignedZeroInfNanPreserveFloat16', type='VkBool32'),
var10=VarDef(name='shaderSignedZeroInfNanPreserveFloat32', type='VkBool32'),
var11=VarDef(name='shaderSignedZeroInfNanPreserveFloat64', type='VkBool32'),
var12=VarDef(name='shaderDenormPreserveFloat16', type='VkBool32'),
var13=VarDef(name='shaderDenormPreserveFloat32', type='VkBool32'),
var14=VarDef(name='shaderDenormPreserveFloat64', type='VkBool32'),
var15=VarDef(name='shaderDenormFlushToZeroFloat16', type='VkBool32'),
var16=VarDef(name='shaderDenormFlushToZeroFloat32', type='VkBool32'),
var17=VarDef(name='shaderDenormFlushToZeroFloat64', type='VkBool32'),
var18=VarDef(name='shaderRoundingModeRTEFloat16', type='VkBool32'),
var19=VarDef(name='shaderRoundingModeRTEFloat32', type='VkBool32'),
var20=VarDef(name='shaderRoundingModeRTEFloat64', type='VkBool32'),
var21=VarDef(name='shaderRoundingModeRTZFloat16', type='VkBool32'),
var22=VarDef(name='shaderRoundingModeRTZFloat32', type='VkBool32'),
var23=VarDef(name='shaderRoundingModeRTZFloat64', type='VkBool32'),
var24=VarDef(name='maxUpdateAfterBindDescriptorsInAllPools', type='uint32_t'),
var25=VarDef(name='shaderUniformBufferArrayNonUniformIndexingNative', type='VkBool32'),
var26=VarDef(name='shaderSampledImageArrayNonUniformIndexingNative', type='VkBool32'),
var27=VarDef(name='shaderStorageBufferArrayNonUniformIndexingNative', type='VkBool32'),
var28=VarDef(name='shaderStorageImageArrayNonUniformIndexingNative', type='VkBool32'),
var29=VarDef(name='shaderInputAttachmentArrayNonUniformIndexingNative', type='VkBool32'),
var30=VarDef(name='robustBufferAccessUpdateAfterBind', type='VkBool32'),
var31=VarDef(name='quadDivergentImplicitLod', type='VkBool32'),
var32=VarDef(name='maxPerStageDescriptorUpdateAfterBindSamplers', type='uint32_t'),
var33=VarDef(name='maxPerStageDescriptorUpdateAfterBindUniformBuffers', type='uint32_t'),
var34=VarDef(name='maxPerStageDescriptorUpdateAfterBindStorageBuffers', type='uint32_t'),
var35=VarDef(name='maxPerStageDescriptorUpdateAfterBindSampledImages', type='uint32_t'),
var36=VarDef(name='maxPerStageDescriptorUpdateAfterBindStorageImages', type='uint32_t'),
var37=VarDef(name='maxPerStageDescriptorUpdateAfterBindInputAttachments', type='uint32_t'),
var38=VarDef(name='maxPerStageUpdateAfterBindResources', type='uint32_t'),
var39=VarDef(name='maxDescriptorSetUpdateAfterBindSamplers', type='uint32_t'),
var40=VarDef(name='maxDescriptorSetUpdateAfterBindUniformBuffers', type='uint32_t'),
var41=VarDef(name='maxDescriptorSetUpdateAfterBindUniformBuffersDynamic', type='uint32_t'),
var42=VarDef(name='maxDescriptorSetUpdateAfterBindStorageBuffers', type='uint32_t'),
var43=VarDef(name='maxDescriptorSetUpdateAfterBindStorageBuffersDynamic', type='uint32_t'),
var44=VarDef(name='maxDescriptorSetUpdateAfterBindSampledImages', type='uint32_t'),
var45=VarDef(name='maxDescriptorSetUpdateAfterBindStorageImages', type='uint32_t'),
var46=VarDef(name='maxDescriptorSetUpdateAfterBindInputAttachments', type='uint32_t'),
var47=VarDef(name='supportedDepthResolveModes', type='VkResolveModeFlags'),
var48=VarDef(name='supportedStencilResolveModes', type='VkResolveModeFlags'),
var49=VarDef(name='independentResolveNone', type='VkBool32'),
var50=VarDef(name='independentResolve', type='VkBool32'),
var51=VarDef(name='filterMinmaxSingleComponentFormats', type='VkBool32'),
var52=VarDef(name='filterMinmaxImageComponentMapping', type='VkBool32'),
var53=VarDef(name='maxTimelineSemaphoreValueDifference', type='uint64_t'),
var54=VarDef(name='framebufferIntegerColorSampleCounts', type='VkSampleCountFlags')
)

Struct(name='VkPhysicalDeviceVulkan13Features_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='robustImageAccess', type='VkBool32'),
var4=VarDef(name='inlineUniformBlock', type='VkBool32'),
var5=VarDef(name='descriptorBindingInlineUniformBlockUpdateAfterBind', type='VkBool32'),
var6=VarDef(name='pipelineCreationCacheControl', type='VkBool32'),
var7=VarDef(name='privateData', type='VkBool32'),
var8=VarDef(name='shaderDemoteToHelperInvocation', type='VkBool32'),
var9=VarDef(name='shaderTerminateInvocation', type='VkBool32'),
var10=VarDef(name='subgroupSizeControl', type='VkBool32'),
var11=VarDef(name='computeFullSubgroups', type='VkBool32'),
var12=VarDef(name='synchronization2', type='VkBool32'),
var13=VarDef(name='textureCompressionASTC_HDR', type='VkBool32'),
var14=VarDef(name='shaderZeroInitializeWorkgroupMemory', type='VkBool32'),
var15=VarDef(name='dynamicRendering', type='VkBool32'),
var16=VarDef(name='shaderIntegerDotProduct', type='VkBool32'),
var17=VarDef(name='maintenance4', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVulkan13Properties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minSubgroupSize', type='uint32_t'),
var4=VarDef(name='maxSubgroupSize', type='uint32_t'),
var5=VarDef(name='maxComputeWorkgroupSubgroups', type='uint32_t'),
var6=VarDef(name='requiredSubgroupSizeStages', type='VkShaderStageFlags'),
var7=VarDef(name='maxInlineUniformBlockSize', type='uint32_t'),
var8=VarDef(name='maxPerStageDescriptorInlineUniformBlocks', type='uint32_t'),
var9=VarDef(name='maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks', type='uint32_t'),
var10=VarDef(name='maxDescriptorSetInlineUniformBlocks', type='uint32_t'),
var11=VarDef(name='maxDescriptorSetUpdateAfterBindInlineUniformBlocks', type='uint32_t'),
var12=VarDef(name='maxInlineUniformTotalSize', type='uint32_t'),
var13=VarDef(name='integerDotProduct8BitUnsignedAccelerated', type='VkBool32'),
var14=VarDef(name='integerDotProduct8BitSignedAccelerated', type='VkBool32'),
var15=VarDef(name='integerDotProduct8BitMixedSignednessAccelerated', type='VkBool32'),
var16=VarDef(name='integerDotProduct4x8BitPackedUnsignedAccelerated', type='VkBool32'),
var17=VarDef(name='integerDotProduct4x8BitPackedSignedAccelerated', type='VkBool32'),
var18=VarDef(name='integerDotProduct4x8BitPackedMixedSignednessAccelerated', type='VkBool32'),
var19=VarDef(name='integerDotProduct16BitUnsignedAccelerated', type='VkBool32'),
var20=VarDef(name='integerDotProduct16BitSignedAccelerated', type='VkBool32'),
var21=VarDef(name='integerDotProduct16BitMixedSignednessAccelerated', type='VkBool32'),
var22=VarDef(name='integerDotProduct32BitUnsignedAccelerated', type='VkBool32'),
var23=VarDef(name='integerDotProduct32BitSignedAccelerated', type='VkBool32'),
var24=VarDef(name='integerDotProduct32BitMixedSignednessAccelerated', type='VkBool32'),
var25=VarDef(name='integerDotProduct64BitUnsignedAccelerated', type='VkBool32'),
var26=VarDef(name='integerDotProduct64BitSignedAccelerated', type='VkBool32'),
var27=VarDef(name='integerDotProduct64BitMixedSignednessAccelerated', type='VkBool32'),
var28=VarDef(name='integerDotProductAccumulatingSaturating8BitUnsignedAccelerated', type='VkBool32'),
var29=VarDef(name='integerDotProductAccumulatingSaturating8BitSignedAccelerated', type='VkBool32'),
var30=VarDef(name='integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated', type='VkBool32'),
var31=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated', type='VkBool32'),
var32=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated', type='VkBool32'),
var33=VarDef(name='integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated', type='VkBool32'),
var34=VarDef(name='integerDotProductAccumulatingSaturating16BitUnsignedAccelerated', type='VkBool32'),
var35=VarDef(name='integerDotProductAccumulatingSaturating16BitSignedAccelerated', type='VkBool32'),
var36=VarDef(name='integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated', type='VkBool32'),
var37=VarDef(name='integerDotProductAccumulatingSaturating32BitUnsignedAccelerated', type='VkBool32'),
var38=VarDef(name='integerDotProductAccumulatingSaturating32BitSignedAccelerated', type='VkBool32'),
var39=VarDef(name='integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated', type='VkBool32'),
var40=VarDef(name='integerDotProductAccumulatingSaturating64BitUnsignedAccelerated', type='VkBool32'),
var41=VarDef(name='integerDotProductAccumulatingSaturating64BitSignedAccelerated', type='VkBool32'),
var42=VarDef(name='integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated', type='VkBool32'),
var43=VarDef(name='storageTexelBufferOffsetAlignmentBytes', type='VkDeviceSize'),
var44=VarDef(name='storageTexelBufferOffsetSingleTexelAlignment', type='VkBool32'),
var45=VarDef(name='uniformTexelBufferOffsetAlignmentBytes', type='VkDeviceSize'),
var46=VarDef(name='uniformTexelBufferOffsetSingleTexelAlignment', type='VkBool32'),
var47=VarDef(name='maxBufferSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceVulkanMemoryModelFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='vulkanMemoryModel', type='VkBool32'),
var4=VarDef(name='vulkanMemoryModelDeviceScope', type='VkBool32'),
var5=VarDef(name='vulkanMemoryModelAvailabilityVisibilityChains', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVulkanSC10Features_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderAtomicInstructions', type='VkBool32')
)

Struct(name='VkPhysicalDeviceVulkanSC10Properties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='deviceNoDynamicHostAllocations', type='VkBool32'),
var4=VarDef(name='deviceDestroyFreesMemory', type='VkBool32'),
var5=VarDef(name='commandPoolMultipleCommandBuffersRecording', type='VkBool32'),
var6=VarDef(name='commandPoolResetCommandBuffer', type='VkBool32'),
var7=VarDef(name='commandBufferSimultaneousUse', type='VkBool32'),
var8=VarDef(name='secondaryCommandBufferNullOrImagelessFramebuffer', type='VkBool32'),
var9=VarDef(name='recycleDescriptorSetMemory', type='VkBool32'),
var10=VarDef(name='recyclePipelineMemory', type='VkBool32'),
var11=VarDef(name='maxRenderPassSubpasses', type='uint32_t'),
var12=VarDef(name='maxRenderPassDependencies', type='uint32_t'),
var13=VarDef(name='maxSubpassInputAttachments', type='uint32_t'),
var14=VarDef(name='maxSubpassPreserveAttachments', type='uint32_t'),
var15=VarDef(name='maxFramebufferAttachments', type='uint32_t'),
var16=VarDef(name='maxDescriptorSetLayoutBindings', type='uint32_t'),
var17=VarDef(name='maxQueryFaultCount', type='uint32_t'),
var18=VarDef(name='maxCallbackFaultCount', type='uint32_t'),
var19=VarDef(name='maxCommandPoolCommandBuffers', type='uint32_t'),
var20=VarDef(name='maxCommandBufferSize', type='VkDeviceSize')
)

Struct(name='VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='workgroupMemoryExplicitLayout', type='VkBool32'),
var4=VarDef(name='workgroupMemoryExplicitLayoutScalarBlockLayout', type='VkBool32'),
var5=VarDef(name='workgroupMemoryExplicitLayout8BitAccess', type='VkBool32'),
var6=VarDef(name='workgroupMemoryExplicitLayout16BitAccess', type='VkBool32')
)

Struct(name='VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='ycbcr2plane444Formats', type='VkBool32')
)

Struct(name='VkPhysicalDeviceYcbcrImageArraysFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='ycbcrImageArrays', type='VkBool32')
)

Struct(name='VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderZeroInitializeWorkgroupMemory', type='VkBool32')
)

Struct(name='VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='shaderZeroInitializeWorkgroupMemory', type='VkBool32')
)

Struct(name='VkPipelineCacheCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCacheCreateFlags'),
var4=VarDef(name='initialDataSize', type='size_t'),
var5=VarDef(name='pInitialData', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='pipelinecachecreateinfo->initialDataSize, (const uint8_t *)pipelinecachecreateinfo->pInitialData')
)

Struct(name='VkPipelineCacheCreateInfo_', enabled=True, version=1, custom=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCacheCreateFlags'),
var4=VarDef(name='initialDataSize', type='size_t'),
var5=VarDef(name='pInitialData', type='const void*', wrapType='CDeclaredBinaryResource', wrapParams='RESOURCE_DATA_RAW, pipelinecachecreateinfo->pInitialData, pipelinecachecreateinfo->initialDataSize')
)

Struct(name='VkPipelineCacheHeaderVersionOne_', enabled=False,
var1=VarDef(name='headerSize', type='uint32_t'),
var2=VarDef(name='headerVersion', type='VkPipelineCacheHeaderVersion'),
var3=VarDef(name='vendorID', type='uint32_t'),
var4=VarDef(name='deviceID', type='uint32_t'),
var5=VarDef(name='pipelineCacheUUID', type='uint8_t[16]')
)

Struct(name='VkPipelineCacheHeaderVersionSafetyCriticalOne_', enabled=False,
var1=VarDef(name='headerVersionOne', type='VkPipelineCacheHeaderVersionOne'),
var2=VarDef(name='validationVersion', type='VkPipelineCacheValidationVersion'),
var3=VarDef(name='implementationData', type='uint32_t'),
var4=VarDef(name='pipelineIndexCount', type='uint32_t'),
var5=VarDef(name='pipelineIndexStride', type='uint32_t'),
var6=VarDef(name='pipelineIndexOffset', type='uint64_t')
)

Struct(name='VkPipelineCacheSafetyCriticalIndexEntry_', enabled=False,
var1=VarDef(name='pipelineIdentifier', type='uint8_t[16]'),
var2=VarDef(name='pipelineMemorySize', type='uint64_t'),
var3=VarDef(name='jsonSize', type='uint64_t'),
var4=VarDef(name='jsonOffset', type='uint64_t'),
var5=VarDef(name='stageIndexCount', type='uint32_t'),
var6=VarDef(name='stageIndexStride', type='uint32_t'),
var7=VarDef(name='stageIndexOffset', type='uint64_t')
)

Struct(name='VkPipelineCacheStageValidationIndexEntry_', enabled=False,
var1=VarDef(name='codeSize', type='uint64_t'),
var2=VarDef(name='codeOffset', type='uint64_t')
)

Struct(name='VkPipelineColorBlendAdvancedStateCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcPremultiplied', type='VkBool32'),
var4=VarDef(name='dstPremultiplied', type='VkBool32'),
var5=VarDef(name='blendOverlap', type='VkBlendOverlapEXT')
)

Struct(name='VkPipelineColorBlendAttachmentState_', enabled=True, declareArray=True,
var1=VarDef(name='blendEnable', type='VkBool32'),
var2=VarDef(name='srcColorBlendFactor', type='VkBlendFactor'),
var3=VarDef(name='dstColorBlendFactor', type='VkBlendFactor'),
var4=VarDef(name='colorBlendOp', type='VkBlendOp'),
var5=VarDef(name='srcAlphaBlendFactor', type='VkBlendFactor'),
var6=VarDef(name='dstAlphaBlendFactor', type='VkBlendFactor'),
var7=VarDef(name='alphaBlendOp', type='VkBlendOp'),
var8=VarDef(name='colorWriteMask', type='VkColorComponentFlags')
)

Struct(name='VkPipelineColorBlendStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineColorBlendStateCreateFlags'),
var4=VarDef(name='logicOpEnable', type='VkBool32'),
var5=VarDef(name='logicOp', type='VkLogicOp'),
var6=VarDef(name='attachmentCount', type='uint32_t'),
var7=VarDef(name='pAttachments', type='const VkPipelineColorBlendAttachmentState*', wrapType='CVkPipelineColorBlendAttachmentStateArray', wrapParams='pipelinecolorblendstatecreateinfo->attachmentCount, pipelinecolorblendstatecreateinfo->pAttachments', count='attachmentCount'),
var8=VarDef(name='blendConstants', type='float[4]', count='4')
)

Struct(name='VkPipelineColorWriteCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='attachmentCount', type='uint32_t'),
var4=VarDef(name='pColorWriteEnables', type='const VkBool32*')
)

Struct(name='VkPipelineCompilerControlCreateInfoAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='compilerControlFlags', type='VkPipelineCompilerControlFlagsAMD')
)

Struct(name='VkPipelineCoverageModulationStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCoverageModulationStateCreateFlagsNV'),
var4=VarDef(name='coverageModulationMode', type='VkCoverageModulationModeNV'),
var5=VarDef(name='coverageModulationTableEnable', type='VkBool32'),
var6=VarDef(name='coverageModulationTableCount', type='uint32_t'),
var7=VarDef(name='pCoverageModulationTable', type='const float*')
)

Struct(name='VkPipelineCoverageReductionStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCoverageReductionStateCreateFlagsNV'),
var4=VarDef(name='coverageReductionMode', type='VkCoverageReductionModeNV')
)

Struct(name='VkPipelineCoverageToColorStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCoverageToColorStateCreateFlagsNV'),
var4=VarDef(name='coverageToColorEnable', type='VkBool32'),
var5=VarDef(name='coverageToColorLocation', type='uint32_t')
)

Struct(name='VkPipelineCreationFeedback_', enabled=True, declareArray=True,
var1=VarDef(name='flags', type='VkPipelineCreationFeedbackFlags'),
var2=VarDef(name='duration', type='uint64_t')
)

Struct(name='VkPipelineCreationFeedbackCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pPipelineCreationFeedback', type='VkPipelineCreationFeedback*', wrapType='CVkPipelineCreationFeedbackArray', wrapParams='1, pipelinecreationfeedbackcreateinfo->pPipelineCreationFeedback'),
var4=VarDef(name='pipelineStageCreationFeedbackCount', type='uint32_t'),
var5=VarDef(name='pPipelineStageCreationFeedbacks', type='VkPipelineCreationFeedback*', wrapType='CVkPipelineCreationFeedbackArray', wrapParams='pipelinecreationfeedbackcreateinfo->pipelineStageCreationFeedbackCount, pipelinecreationfeedbackcreateinfo->pPipelineStageCreationFeedbacks', count='pipelineStageCreationFeedbackCount')
)

Struct(name='VkPipelineCreationFeedbackCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pPipelineCreationFeedback', type='VkPipelineCreationFeedback*', wrapType='CVkPipelineCreationFeedbackArray', wrapParams='1, pipelinecreationfeedbackcreateinfoext->pPipelineCreationFeedback'),
var4=VarDef(name='pipelineStageCreationFeedbackCount', type='uint32_t'),
var5=VarDef(name='pPipelineStageCreationFeedbacks', type='VkPipelineCreationFeedback*', wrapType='CVkPipelineCreationFeedbackArray', wrapParams='pipelinecreationfeedbackcreateinfoext->pipelineStageCreationFeedbackCount, pipelinecreationfeedbackcreateinfoext->pPipelineStageCreationFeedbacks', count='pipelineStageCreationFeedbackCount')
)

Struct(name='VkPipelineCreationFeedbackEXT_', enabled=True, declareArray=True,
var1=VarDef(name='flags', type='VkPipelineCreationFeedbackFlags'),
var2=VarDef(name='duration', type='uint64_t')
)

Struct(name='VkPipelineDepthStencilStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineDepthStencilStateCreateFlags'),
var4=VarDef(name='depthTestEnable', type='VkBool32'),
var5=VarDef(name='depthWriteEnable', type='VkBool32'),
var6=VarDef(name='depthCompareOp', type='VkCompareOp'),
var7=VarDef(name='depthBoundsTestEnable', type='VkBool32'),
var8=VarDef(name='stencilTestEnable', type='VkBool32'),
var9=VarDef(name='front', type='VkStencilOpState'),
var10=VarDef(name='back', type='VkStencilOpState'),
var11=VarDef(name='minDepthBounds', type='float'),
var12=VarDef(name='maxDepthBounds', type='float')
)

Struct(name='VkPipelineDiscardRectangleStateCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineDiscardRectangleStateCreateFlagsEXT'),
var4=VarDef(name='discardRectangleMode', type='VkDiscardRectangleModeEXT'),
var5=VarDef(name='discardRectangleCount', type='uint32_t'),
var6=VarDef(name='pDiscardRectangles', type='const VkRect2D*', count='discardRectangleCount')
)

Struct(name='VkPipelineDynamicStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineDynamicStateCreateFlags'),
var4=VarDef(name='dynamicStateCount', type='uint32_t'),
var5=VarDef(name='pDynamicStates', type='const VkDynamicState*', wrapType='CVkDynamicState::CSArray', wrapParams='pipelinedynamicstatecreateinfo->dynamicStateCount, pipelinedynamicstatecreateinfo->pDynamicStates', count='dynamicStateCount')
)

Struct(name='VkPipelineExecutableInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipeline', type='VkPipeline'),
var4=VarDef(name='executableIndex', type='uint32_t')
)

Struct(name='VkPipelineExecutableInternalRepresentationKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='name', type='char[256]'),
var4=VarDef(name='description', type='char[256]'),
var5=VarDef(name='isText', type='VkBool32'),
var6=VarDef(name='dataSize', type='size_t'),
var7=VarDef(name='pData', type='void*')
)

Struct(name='VkPipelineExecutablePropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='stages', type='VkShaderStageFlags'),
var4=VarDef(name='name', type='char[256]'),
var5=VarDef(name='description', type='char[256]'),
var6=VarDef(name='subgroupSize', type='uint32_t')
)

Struct(name='VkPipelineExecutableStatisticKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='name', type='char[256]'),
var4=VarDef(name='description', type='char[256]'),
var5=VarDef(name='format', type='VkPipelineExecutableStatisticFormatKHR'),
var6=VarDef(name='value', type='VkPipelineExecutableStatisticValueKHR')
)

Struct(name='VkPipelineExecutableStatisticValueKHR_', type='union', enabled='False',
var1=VarDef(name='b32', type='VkBool32'),
var2=VarDef(name='i64', type='int64_t'),
var3=VarDef(name='u64', type='uint64_t'),
var4=VarDef(name='f64', type='double')
)

Struct(name='VkPipelineFragmentShadingRateEnumStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='shadingRateType', type='VkFragmentShadingRateTypeNV'),
var4=VarDef(name='shadingRate', type='VkFragmentShadingRateNV'),
var5=VarDef(name='combinerOps', type='VkFragmentShadingRateCombinerOpKHR[2]')
)

Struct(name='VkPipelineFragmentShadingRateStateCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='fragmentSize', type='VkExtent2D'),
var4=VarDef(name='combinerOps', type='VkFragmentShadingRateCombinerOpKHR[2]')
)

Struct(name='VkPipelineInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipeline', type='VkPipeline')
)

Struct(name='VkPipelineInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipeline', type='VkPipeline')
)

Struct(name='VkPipelineInputAssemblyStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineInputAssemblyStateCreateFlags'),
var4=VarDef(name='topology', type='VkPrimitiveTopology'),
var5=VarDef(name='primitiveRestartEnable', type='VkBool32')
)

Struct(name='VkPipelineLayoutCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineLayoutCreateFlags'),
var4=VarDef(name='setLayoutCount', type='uint32_t'),
var5=VarDef(name='pSetLayouts', type='const VkDescriptorSetLayout*', wrapType='CVkDescriptorSetLayout::CSArray', wrapParams='pipelinelayoutcreateinfo->setLayoutCount, pipelinelayoutcreateinfo->pSetLayouts', count='setLayoutCount'),
var6=VarDef(name='pushConstantRangeCount', type='uint32_t'),
var7=VarDef(name='pPushConstantRanges', type='const VkPushConstantRange*', wrapType='CVkPushConstantRangeArray', wrapParams='pipelinelayoutcreateinfo->pushConstantRangeCount, pipelinelayoutcreateinfo->pPushConstantRanges', count='pushConstantRangeCount')
)

Struct(name='VkPipelineLibraryCreateInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='libraryCount', type='uint32_t'),
var4=VarDef(name='pLibraries', type='const VkPipeline*', wrapType='CVkPipeline::CSArray', wrapParams='pipelinelibrarycreateinfokhr->libraryCount, pipelinelibrarycreateinfokhr->pLibraries', count='libraryCount')
)

Struct(name='VkPipelineMultisampleStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineMultisampleStateCreateFlags'),
var4=VarDef(name='rasterizationSamples', type='uint32_t'),
var5=VarDef(name='sampleShadingEnable', type='VkBool32'),
var6=VarDef(name='minSampleShading', type='float'),
var7=VarDef(name='pSampleMask', type='const VkSampleMask*', wrapType='Cuint32_t::CSArray', wrapParams='1, (const uint32_t*)pipelinemultisamplestatecreateinfo->pSampleMask'),
var8=VarDef(name='alphaToCoverageEnable', type='VkBool32'),
var9=VarDef(name='alphaToOneEnable', type='VkBool32')
)

Struct(name='VkPipelineOfflineCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pipelineIdentifier', type='uint8_t[16]'),
var4=VarDef(name='matchControl', type='VkPipelineMatchControl'),
var5=VarDef(name='poolEntrySize', type='VkDeviceSize')
)

Struct(name='VkPipelinePoolSize_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='poolEntrySize', type='VkDeviceSize'),
var4=VarDef(name='poolEntryCount', type='uint32_t')
)

Struct(name='VkPipelinePropertiesIdentifierEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='pipelineIdentifier', type='uint8_t[16]')
)

Struct(name='VkPipelineRasterizationConservativeStateCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineRasterizationConservativeStateCreateFlagsEXT'),
var4=VarDef(name='conservativeRasterizationMode', type='VkConservativeRasterizationModeEXT'),
var5=VarDef(name='extraPrimitiveOverestimationSize', type='float')
)

Struct(name='VkPipelineRasterizationDepthClipStateCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineRasterizationDepthClipStateCreateFlagsEXT'),
var4=VarDef(name='depthClipEnable', type='VkBool32')
)

Struct(name='VkPipelineRasterizationLineStateCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='lineRasterizationMode', type='VkLineRasterizationModeEXT'),
var4=VarDef(name='stippledLineEnable', type='VkBool32'),
var5=VarDef(name='lineStippleFactor', type='uint32_t'),
var6=VarDef(name='lineStipplePattern', type='uint16_t')
)

Struct(name='VkPipelineRasterizationProvokingVertexStateCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='provokingVertexMode', type='VkProvokingVertexModeEXT')
)

Struct(name='VkPipelineRasterizationStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineRasterizationStateCreateFlags'),
var4=VarDef(name='depthClampEnable', type='VkBool32'),
var5=VarDef(name='rasterizerDiscardEnable', type='VkBool32'),
var6=VarDef(name='polygonMode', type='VkPolygonMode'),
var7=VarDef(name='cullMode', type='VkCullModeFlags'),
var8=VarDef(name='frontFace', type='VkFrontFace'),
var9=VarDef(name='depthBiasEnable', type='VkBool32'),
var10=VarDef(name='depthBiasConstantFactor', type='float'),
var11=VarDef(name='depthBiasClamp', type='float'),
var12=VarDef(name='depthBiasSlopeFactor', type='float'),
var13=VarDef(name='lineWidth', type='float')
)

Struct(name='VkPipelineRasterizationStateRasterizationOrderAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='rasterizationOrder', type='VkRasterizationOrderAMD')
)

Struct(name='VkPipelineRasterizationStateStreamCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineRasterizationStateStreamCreateFlagsEXT'),
var4=VarDef(name='rasterizationStream', type='uint32_t')
)

Struct(name='VkPipelineRenderingCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='viewMask', type='uint32_t'),
var4=VarDef(name='colorAttachmentCount', type='uint32_t'),
var5=VarDef(name='pColorAttachmentFormats', type='const VkFormat*',wrapType='CVkFormat::CSArray', wrapParams='pipelinerenderingcreateinfo->colorAttachmentCount, pipelinerenderingcreateinfo->pColorAttachmentFormats', count='colorAttachmentCount'),
var6=VarDef(name='depthAttachmentFormat', type='VkFormat'),
var7=VarDef(name='stencilAttachmentFormat', type='VkFormat')
)

Struct(name='VkPipelineRenderingCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='viewMask', type='uint32_t'),
var4=VarDef(name='colorAttachmentCount', type='uint32_t'),
var5=VarDef(name='pColorAttachmentFormats', type='const VkFormat*'),
var6=VarDef(name='depthAttachmentFormat', type='VkFormat'),
var7=VarDef(name='stencilAttachmentFormat', type='VkFormat')
)

Struct(name='VkPipelineRepresentativeFragmentTestStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='representativeFragmentTestEnable', type='VkBool32')
)

Struct(name='VkPipelineRobustnessCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='storageBuffers', type='VkPipelineRobustnessBufferBehaviorEXT'),
var4=VarDef(name='uniformBuffers', type='VkPipelineRobustnessBufferBehaviorEXT'),
var5=VarDef(name='vertexInputs', type='VkPipelineRobustnessBufferBehaviorEXT'),
var6=VarDef(name='images', type='VkPipelineRobustnessImageBehaviorEXT')
)

Struct(name='VkPipelineSampleLocationsStateCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sampleLocationsEnable', type='VkBool32'),
var4=VarDef(name='sampleLocationsInfo', type='VkSampleLocationsInfoEXT')
)

Struct(name='VkPipelineShaderStageCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineShaderStageCreateFlags'),
var4=VarDef(name='stage', type='VkShaderStageFlagBits'),
var5=VarDef(name='module', type='VkShaderModule'),
var6=VarDef(name='pName', type='const char*', wrapType='Cchar::CSArray', wrapParams='pipelineshaderstagecreateinfo->pName, \'\\0\', 1'),
var7=VarDef(name='pSpecializationInfo', type='const VkSpecializationInfo*', wrapType='CVkSpecializationInfoArray', wrapParams='1,pipelineshaderstagecreateinfo->pSpecializationInfo')
)

Struct(name='VkPipelineShaderStageModuleIdentifierCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='identifierSize', type='uint32_t'),
var4=VarDef(name='pIdentifier', type='const uint8_t*')
)

Struct(name='VkPipelineShaderStageRequiredSubgroupSizeCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='requiredSubgroupSize', type='uint32_t')
)

Struct(name='VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='requiredSubgroupSize', type='uint32_t')
)

Struct(name='VkPipelineTessellationDomainOriginStateCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='domainOrigin', type='VkTessellationDomainOrigin')
)

Struct(name='VkPipelineTessellationStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineTessellationStateCreateFlags'),
var4=VarDef(name='patchControlPoints', type='uint32_t')
)

Struct(name='VkPipelineVertexInputDivisorStateCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='vertexBindingDivisorCount', type='uint32_t'),
var4=VarDef(name='pVertexBindingDivisors', type='const VkVertexInputBindingDivisorDescriptionEXT*', wrapType='CVkVertexInputBindingDivisorDescriptionEXTArray', wrapParams='pipelinevertexinputdivisorstatecreateinfoext->vertexBindingDivisorCount, pipelinevertexinputdivisorstatecreateinfoext->pVertexBindingDivisors')
)

Struct(name='VkPipelineVertexInputStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineVertexInputStateCreateFlags'),
var4=VarDef(name='vertexBindingDescriptionCount', type='uint32_t'),
var5=VarDef(name='pVertexBindingDescriptions', type='const VkVertexInputBindingDescription*', wrapType='CVkVertexInputBindingDescriptionArray', wrapParams='pipelinevertexinputstatecreateinfo->vertexBindingDescriptionCount, pipelinevertexinputstatecreateinfo->pVertexBindingDescriptions', count='vertexBindingDescriptionCount'),
var6=VarDef(name='vertexAttributeDescriptionCount', type='uint32_t'),
var7=VarDef(name='pVertexAttributeDescriptions', type='const VkVertexInputAttributeDescription*', wrapType='CVkVertexInputAttributeDescriptionArray', wrapParams='pipelinevertexinputstatecreateinfo->vertexAttributeDescriptionCount, pipelinevertexinputstatecreateinfo->pVertexAttributeDescriptions', count='vertexAttributeDescriptionCount')
)

Struct(name='VkPipelineViewportCoarseSampleOrderStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sampleOrderType', type='VkCoarseSampleOrderTypeNV'),
var4=VarDef(name='customSampleOrderCount', type='uint32_t'),
var5=VarDef(name='pCustomSampleOrders', type='const VkCoarseSampleOrderCustomNV*')
)

Struct(name='VkPipelineViewportDepthClipControlCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='negativeOneToOne', type='VkBool32')
)

Struct(name='VkPipelineViewportExclusiveScissorStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='exclusiveScissorCount', type='uint32_t'),
var4=VarDef(name='pExclusiveScissors', type='const VkRect2D*')
)

Struct(name='VkPipelineViewportShadingRateImageStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='shadingRateImageEnable', type='VkBool32'),
var4=VarDef(name='viewportCount', type='uint32_t'),
var5=VarDef(name='pShadingRatePalettes', type='const VkShadingRatePaletteNV*')
)

Struct(name='VkPipelineViewportStateCreateInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineViewportStateCreateFlags'),
var4=VarDef(name='viewportCount', type='uint32_t'),
var5=VarDef(name='pViewports', type='const VkViewport*', wrapType='CVkViewportArray', wrapParams='pipelineviewportstatecreateinfo->viewportCount, pipelineviewportstatecreateinfo->pViewports', count='viewportCount'),
var6=VarDef(name='scissorCount', type='uint32_t'),
var7=VarDef(name='pScissors', type='const VkRect2D*', wrapType='CVkRect2DArray', wrapParams='pipelineviewportstatecreateinfo->scissorCount, pipelineviewportstatecreateinfo->pScissors', count='scissorCount')
)

Struct(name='VkPipelineViewportSwizzleStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineViewportSwizzleStateCreateFlagsNV'),
var4=VarDef(name='viewportCount', type='uint32_t'),
var5=VarDef(name='pViewportSwizzles', type='const VkViewportSwizzleNV*', count='viewportCount')
)

Struct(name='VkPipelineViewportWScalingStateCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='viewportWScalingEnable', type='VkBool32'),
var4=VarDef(name='viewportCount', type='uint32_t'),
var5=VarDef(name='pViewportWScalings', type='const VkViewportWScalingNV*')
)

Struct(name='VkPresentIdKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchainCount', type='uint32_t'),
var4=VarDef(name='pPresentIds', type='const uint64_t*')
)

Struct(name='VkPresentInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphores', type='const VkSemaphore*', wrapType='CVkSemaphore::CSArray', wrapParams='presentinfokhr->waitSemaphoreCount, presentinfokhr->pWaitSemaphores', count='waitSemaphoreCount'),
var5=VarDef(name='swapchainCount', type='uint32_t'),
var6=VarDef(name='pSwapchains', type='const VkSwapchainKHR*', wrapType='CVkSwapchainKHR::CSArray', wrapParams='presentinfokhr->swapchainCount, presentinfokhr->pSwapchains', count='swapchainCount'),
var7=VarDef(name='pImageIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='presentinfokhr->swapchainCount, presentinfokhr->pImageIndices', count='swapchainCount'),
var8=VarDef(name='pResults', type='VkResult*', wrapType='CVkResult::CSArray', wrapParams='presentinfokhr->swapchainCount, presentinfokhr->pResults', count='swapchainCount')
)

Struct(name='VkPresentRegionKHR_', enabled=False,
var1=VarDef(name='rectangleCount', type='uint32_t'),
var2=VarDef(name='pRectangles', type='const VkRectLayerKHR*', count='rectangleCount')
)

Struct(name='VkPresentRegionsKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchainCount', type='uint32_t'),
var4=VarDef(name='pRegions', type='const VkPresentRegionKHR*', count='swapchainCount')
)

Struct(name='VkPresentTimeGOOGLE_', enabled=False,
var1=VarDef(name='presentID', type='uint32_t'),
var2=VarDef(name='desiredPresentTime', type='uint64_t')
)

Struct(name='VkPresentTimesInfoGOOGLE_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchainCount', type='uint32_t'),
var4=VarDef(name='pTimes', type='const VkPresentTimeGOOGLE*', count='swapchainCount')
)

Struct(name='VkPrivateDataSlotCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPrivateDataSlotCreateFlags')
)

Struct(name='VkProtectedSubmitInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='protectedSubmit', type='VkBool32')
)

Struct(name='VkPushConstantRange_', enabled=True, declareArray=True,
var1=VarDef(name='stageFlags', type='VkShaderStageFlags'),
var2=VarDef(name='offset', type='uint32_t'),
var3=VarDef(name='size', type='uint32_t')
)

Struct(name='VkQueryLowLatencySupportNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pQueriedLowLatencyData', type='void*')
)

Struct(name='VkQueryPoolCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkQueryPoolCreateFlags'),
var4=VarDef(name='queryType', type='VkQueryType'),
var5=VarDef(name='queryCount', type='uint32_t'),
var6=VarDef(name='pipelineStatistics', type='VkQueryPipelineStatisticFlags')
)

Struct(name='VkQueryPoolCreateInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='performanceCountersSampling', type='VkQueryPoolSamplingModeINTEL')
)

Struct(name='VkQueryPoolPerformanceCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='queueFamilyIndex', type='uint32_t'),
var4=VarDef(name='counterIndexCount', type='uint32_t'),
var5=VarDef(name='pCounterIndices', type='const uint32_t*')
)

Struct(name='VkQueryPoolPerformanceQueryCreateInfoINTEL_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='performanceCountersSampling', type='VkQueryPoolSamplingModeINTEL')
)

Struct(name='VkQueryPoolVideoEncodeFeedbackCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='encodeFeedbackFlags', type='VkVideoEncodeFeedbackFlagsKHR')
)

Struct(name='VkQueueFamilyCheckpointProperties2NV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='checkpointExecutionStageMask', type='VkPipelineStageFlags2')
)

Struct(name='VkQueueFamilyCheckpointPropertiesNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='checkpointExecutionStageMask', type='VkPipelineStageFlags')
)

Struct(name='VkQueueFamilyGlobalPriorityPropertiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='priorityCount', type='uint32_t'),
var4=VarDef(name='priorities', type='VkQueueGlobalPriorityKHR[16]')
)

Struct(name='VkQueueFamilyGlobalPriorityPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='priorityCount', type='uint32_t'),
var4=VarDef(name='priorities', type='VkQueueGlobalPriorityKHR[16]')
)

Struct(name='VkQueueFamilyProperties_', enabled=True, declareArray=True,
var1=VarDef(name='queueFlags', type='VkQueueFlags'),
var2=VarDef(name='queueCount', type='uint32_t'),
var3=VarDef(name='timestampValidBits', type='uint32_t'),
var4=VarDef(name='minImageTransferGranularity', type='VkExtent3D')
)

Struct(name='VkQueueFamilyProperties2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='queueFamilyProperties', type='VkQueueFamilyProperties')
)

Struct(name='VkQueueFamilyProperties2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='queueFamilyProperties', type='VkQueueFamilyProperties')
)

Struct(name='VkQueueFamilyQueryResultStatusPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='queryResultStatusSupport', type='VkBool32')
)

Struct(name='VkQueueFamilyVideoPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='videoCodecOperations', type='VkVideoCodecOperationFlagsKHR')
)

Struct(name='VkRayTracingPipelineCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCreateFlags'),
var4=VarDef(name='stageCount', type='uint32_t'),
var5=VarDef(name='pStages', type='const VkPipelineShaderStageCreateInfo*'),
var6=VarDef(name='groupCount', type='uint32_t'),
var7=VarDef(name='pGroups', type='const VkRayTracingShaderGroupCreateInfoKHR*'),
var8=VarDef(name='maxPipelineRayRecursionDepth', type='uint32_t'),
var9=VarDef(name='pLibraryInfo', type='const VkPipelineLibraryCreateInfoKHR*'),
var10=VarDef(name='pLibraryInterface', type='const VkRayTracingPipelineInterfaceCreateInfoKHR*'),
var11=VarDef(name='pDynamicState', type='const VkPipelineDynamicStateCreateInfo*'),
var12=VarDef(name='layout', type='VkPipelineLayout'),
var13=VarDef(name='basePipelineHandle', type='VkPipeline'),
var14=VarDef(name='basePipelineIndex', type='int32_t')
)

Struct(name='VkRayTracingPipelineCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkPipelineCreateFlags'),
var4=VarDef(name='stageCount', type='uint32_t'),
var5=VarDef(name='pStages', type='const VkPipelineShaderStageCreateInfo*'),
var6=VarDef(name='groupCount', type='uint32_t'),
var7=VarDef(name='pGroups', type='const VkRayTracingShaderGroupCreateInfoNV*'),
var8=VarDef(name='maxRecursionDepth', type='uint32_t'),
var9=VarDef(name='layout', type='VkPipelineLayout'),
var10=VarDef(name='basePipelineHandle', type='VkPipeline'),
var11=VarDef(name='basePipelineIndex', type='int32_t')
)

Struct(name='VkRayTracingPipelineInterfaceCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='maxPipelineRayPayloadSize', type='uint32_t'),
var4=VarDef(name='maxPipelineRayHitAttributeSize', type='uint32_t')
)

Struct(name='VkRayTracingShaderGroupCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkRayTracingShaderGroupTypeKHR'),
var4=VarDef(name='generalShader', type='uint32_t'),
var5=VarDef(name='closestHitShader', type='uint32_t'),
var6=VarDef(name='anyHitShader', type='uint32_t'),
var7=VarDef(name='intersectionShader', type='uint32_t'),
var8=VarDef(name='pShaderGroupCaptureReplayHandle', type='const void*')
)

Struct(name='VkRayTracingShaderGroupCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='type', type='VkRayTracingShaderGroupTypeKHR'),
var4=VarDef(name='generalShader', type='uint32_t'),
var5=VarDef(name='closestHitShader', type='uint32_t'),
var6=VarDef(name='anyHitShader', type='uint32_t'),
var7=VarDef(name='intersectionShader', type='uint32_t')
)

Struct(name='VkRect2D_', enabled=True, declareArray=True,
var1=VarDef(name='offset', type='VkOffset2D'),
var2=VarDef(name='extent', type='VkExtent2D')
)

Struct(name='VkRectLayerKHR_', enabled=False,
var1=VarDef(name='offset', type='VkOffset2D'),
var2=VarDef(name='extent', type='VkExtent2D'),
var3=VarDef(name='layer', type='uint32_t')
)

Struct(name='VkRefreshCycleDurationGOOGLE_', enabled=False,
var1=VarDef(name='refreshDuration', type='uint64_t')
)

Struct(name='VkRefreshObjectKHR_', enabled=False,
var1=VarDef(name='objectType', type='VkObjectType'),
var2=VarDef(name='objectHandle', type='uint64_t'),
var3=VarDef(name='flags', type='VkRefreshObjectFlagsKHR')
)

Struct(name='VkRefreshObjectListKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='objectCount', type='uint32_t'),
var4=VarDef(name='pObjects', type='const VkRefreshObjectKHR*')
)

Struct(name='VkReleaseSwapchainImagesInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchain', type='VkSwapchainKHR'),
var4=VarDef(name='imageIndexCount', type='uint32_t'),
var5=VarDef(name='pImageIndices', type='const uint32_t*')
)

Struct(name='VkRenderPassAttachmentBeginInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='attachmentCount', type='uint32_t'),
var4=VarDef(name='pAttachments', type='const VkImageView*', wrapType='CVkImageView::CSArray', wrapParams='renderpassattachmentbegininfo->attachmentCount, renderpassattachmentbegininfo->pAttachments', count='attachmentCount')
)

Struct(name='VkRenderPassBeginInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='renderPass', type='VkRenderPass'),
var4=VarDef(name='framebuffer', type='VkFramebuffer'),
var5=VarDef(name='renderArea', type='VkRect2D'),
var6=VarDef(name='clearValueCount', type='uint32_t'),
var7=VarDef(name='pClearValues', type='const VkClearValue*', wrapType='CVkClearValueArray', wrapParams='renderpassbegininfo->clearValueCount, renderpassbegininfo->pClearValues', count='clearValueCount')
)

Struct(name='VkRenderPassCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderPassCreateFlags'),
var4=VarDef(name='attachmentCount', type='uint32_t'),
var5=VarDef(name='pAttachments', type='const VkAttachmentDescription*', wrapType='CVkAttachmentDescriptionArray', wrapParams='renderpasscreateinfo->attachmentCount, renderpasscreateinfo->pAttachments', count='attachmentCount'),
var6=VarDef(name='subpassCount', type='uint32_t'),
var7=VarDef(name='pSubpasses', type='const VkSubpassDescription*', wrapType='CVkSubpassDescriptionArray', wrapParams='renderpasscreateinfo->subpassCount, renderpasscreateinfo->pSubpasses', count='subpassCount'),
var8=VarDef(name='dependencyCount', type='uint32_t'),
var9=VarDef(name='pDependencies', type='const VkSubpassDependency*', wrapType='CVkSubpassDependencyArray', wrapParams='renderpasscreateinfo->dependencyCount, renderpasscreateinfo->pDependencies', count='dependencyCount')
)

Struct(name='VkRenderPassCreateInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderPassCreateFlags'),
var4=VarDef(name='attachmentCount', type='uint32_t'),
var5=VarDef(name='pAttachments', type='const VkAttachmentDescription2*', wrapType='CVkAttachmentDescription2Array', wrapParams='renderpasscreateinfo2->attachmentCount, renderpasscreateinfo2->pAttachments', count='attachmentCount'),
var6=VarDef(name='subpassCount', type='uint32_t'),
var7=VarDef(name='pSubpasses', type='const VkSubpassDescription2*', wrapType='CVkSubpassDescription2Array', wrapParams='renderpasscreateinfo2->subpassCount, renderpasscreateinfo2->pSubpasses', count='subpassCount'),
var8=VarDef(name='dependencyCount', type='uint32_t'),
var9=VarDef(name='pDependencies', type='const VkSubpassDependency2*', wrapType='CVkSubpassDependency2Array', wrapParams='renderpasscreateinfo2->dependencyCount, renderpasscreateinfo2->pDependencies', count='dependencyCount'),
var10=VarDef(name='correlatedViewMaskCount', type='uint32_t'),
var11=VarDef(name='pCorrelatedViewMasks', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='renderpasscreateinfo2->correlatedViewMaskCount, renderpasscreateinfo2->pCorrelatedViewMasks', count='correlatedViewMaskCount')
)

Struct(name='VkRenderPassCreateInfo2KHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderPassCreateFlags'),
var4=VarDef(name='attachmentCount', type='uint32_t'),
var5=VarDef(name='pAttachments', type='const VkAttachmentDescription2*'),
var6=VarDef(name='subpassCount', type='uint32_t'),
var7=VarDef(name='pSubpasses', type='const VkSubpassDescription2*'),
var8=VarDef(name='dependencyCount', type='uint32_t'),
var9=VarDef(name='pDependencies', type='const VkSubpassDependency2*'),
var10=VarDef(name='correlatedViewMaskCount', type='uint32_t'),
var11=VarDef(name='pCorrelatedViewMasks', type='const uint32_t*')
)

Struct(name='VkRenderPassCreationControlEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='disallowMerging', type='VkBool32')
)

Struct(name='VkRenderPassCreationFeedbackCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pRenderPassFeedback', type='VkRenderPassCreationFeedbackInfoEXT*')
)

Struct(name='VkRenderPassCreationFeedbackInfoEXT_', enabled=False,
var1=VarDef(name='postMergeSubpassCount', type='uint32_t')
)

Struct(name='VkRenderPassFragmentDensityMapCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='fragmentDensityMapAttachment', type='VkAttachmentReference')
)

Struct(name='VkRenderPassInputAttachmentAspectCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='aspectReferenceCount', type='uint32_t'),
var4=VarDef(name='pAspectReferences', type='const VkInputAttachmentAspectReference*', count='aspectReferenceCount')
)

Struct(name='VkRenderPassMultiviewCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='subpassCount', type='uint32_t'),
var4=VarDef(name='pViewMasks', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='renderpassmultiviewcreateinfo->subpassCount, renderpassmultiviewcreateinfo->pViewMasks', count='subpassCount'),
var5=VarDef(name='dependencyCount', type='uint32_t'),
var6=VarDef(name='pViewOffsets', type='const int32_t*', wrapType='Cint32_t::CSArray', wrapParams='renderpassmultiviewcreateinfo->dependencyCount, renderpassmultiviewcreateinfo->pViewOffsets', count='dependencyCount'),
var7=VarDef(name='correlationMaskCount', type='uint32_t'),
var8=VarDef(name='pCorrelationMasks', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='renderpassmultiviewcreateinfo->correlationMaskCount, renderpassmultiviewcreateinfo->pCorrelationMasks', count='correlationMaskCount')
)

Struct(name='VkRenderPassSampleLocationsBeginInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='attachmentInitialSampleLocationsCount', type='uint32_t'),
var4=VarDef(name='pAttachmentInitialSampleLocations', type='const VkAttachmentSampleLocationsEXT*', count='attachmentInitialSampleLocationsCount'),
var5=VarDef(name='postSubpassSampleLocationsCount', type='uint32_t'),
var6=VarDef(name='pPostSubpassSampleLocations', type='const VkSubpassSampleLocationsEXT*', count='postSubpassSampleLocationsCount')
)

Struct(name='VkRenderPassSubpassFeedbackCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='pSubpassFeedback', type='VkRenderPassSubpassFeedbackInfoEXT*')
)

Struct(name='VkRenderPassSubpassFeedbackInfoEXT_', enabled=False,
var1=VarDef(name='subpassMergeStatus', type='VkSubpassMergeStatusEXT'),
var2=VarDef(name='description', type='char[256]'),
var3=VarDef(name='postMergeIndex', type='uint32_t')
)

Struct(name='VkRenderPassTransformBeginInfoQCOM_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='transform', type='VkSurfaceTransformFlagBitsKHR')
)

Struct(name='VkRenderingAttachmentInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageView', type='VkImageView'),
var4=VarDef(name='imageLayout', type='VkImageLayout'),
var5=VarDef(name='resolveMode', type='VkResolveModeFlagBits'),
var6=VarDef(name='resolveImageView', type='VkImageView'),
var7=VarDef(name='resolveImageLayout', type='VkImageLayout'),
var8=VarDef(name='loadOp', type='VkAttachmentLoadOp'),
var9=VarDef(name='storeOp', type='VkAttachmentStoreOp'),
var10=VarDef(name='clearValue', type='VkClearValue')
)

Struct(name='VkRenderingAttachmentInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageView', type='VkImageView'),
var4=VarDef(name='imageLayout', type='VkImageLayout'),
var5=VarDef(name='resolveMode', type='VkResolveModeFlagBits'),
var6=VarDef(name='resolveImageView', type='VkImageView'),
var7=VarDef(name='resolveImageLayout', type='VkImageLayout'),
var8=VarDef(name='loadOp', type='VkAttachmentLoadOp'),
var9=VarDef(name='storeOp', type='VkAttachmentStoreOp'),
var10=VarDef(name='clearValue', type='VkClearValue')
)

Struct(name='VkRenderingFragmentDensityMapAttachmentInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageView', type='VkImageView'),
var4=VarDef(name='imageLayout', type='VkImageLayout')
)

Struct(name='VkRenderingFragmentShadingRateAttachmentInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='imageView', type='VkImageView'),
var4=VarDef(name='imageLayout', type='VkImageLayout'),
var5=VarDef(name='shadingRateAttachmentTexelSize', type='VkExtent2D')
)

Struct(name='VkRenderingInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderingFlags'),
var4=VarDef(name='renderArea', type='VkRect2D'),
var5=VarDef(name='layerCount', type='uint32_t'),
var6=VarDef(name='viewMask', type='uint32_t'),
var7=VarDef(name='colorAttachmentCount', type='uint32_t'),
var8=VarDef(name='pColorAttachments', type='const VkRenderingAttachmentInfo*', wrapType='CVkRenderingAttachmentInfoArray', wrapParams='renderinginfo->colorAttachmentCount, renderinginfo->pColorAttachments', count='colorAttachmentCount'),
var9=VarDef(name='pDepthAttachment', type='const VkRenderingAttachmentInfo*', wrapType='CVkRenderingAttachmentInfoArray', wrapParams='1, renderinginfo->pDepthAttachment'),
var10=VarDef(name='pStencilAttachment', type='const VkRenderingAttachmentInfo*', wrapType='CVkRenderingAttachmentInfoArray', wrapParams='1, renderinginfo->pStencilAttachment')
)

Struct(name='VkRenderingInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkRenderingFlags'),
var4=VarDef(name='renderArea', type='VkRect2D'),
var5=VarDef(name='layerCount', type='uint32_t'),
var6=VarDef(name='viewMask', type='uint32_t'),
var7=VarDef(name='colorAttachmentCount', type='uint32_t'),
var8=VarDef(name='pColorAttachments', type='const VkRenderingAttachmentInfo*'),
var9=VarDef(name='pDepthAttachment', type='const VkRenderingAttachmentInfo*'),
var10=VarDef(name='pStencilAttachment', type='const VkRenderingAttachmentInfo*')
)

Struct(name='VkResolveImageInfo2_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcImage', type='VkImage'),
var4=VarDef(name='srcImageLayout', type='VkImageLayout'),
var5=VarDef(name='dstImage', type='VkImage'),
var6=VarDef(name='dstImageLayout', type='VkImageLayout'),
var7=VarDef(name='regionCount', type='uint32_t'),
var8=VarDef(name='pRegions', type='const VkImageResolve2*', wrapType='CVkImageResolve2Array', wrapParams='resolveimageinfo2->regionCount, resolveimageinfo2->pRegions', count='regionCount')
)

Struct(name='VkSRTDataNV_', enabled=False,
var1=VarDef(name='sx', type='float'),
var2=VarDef(name='a', type='float'),
var3=VarDef(name='b', type='float'),
var4=VarDef(name='pvx', type='float'),
var5=VarDef(name='sy', type='float'),
var6=VarDef(name='c', type='float'),
var7=VarDef(name='pvy', type='float'),
var8=VarDef(name='sz', type='float'),
var9=VarDef(name='pvz', type='float'),
var10=VarDef(name='qx', type='float'),
var11=VarDef(name='qy', type='float'),
var12=VarDef(name='qz', type='float'),
var13=VarDef(name='qw', type='float'),
var14=VarDef(name='tx', type='float'),
var15=VarDef(name='ty', type='float'),
var16=VarDef(name='tz', type='float')
)

Struct(name='VkSampleLocationEXT_', enabled=True, declareArray=True,
var1=VarDef(name='x', type='float'),
var2=VarDef(name='y', type='float')
)

Struct(name='VkSampleLocationsInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sampleLocationsPerPixel', type='VkSampleCountFlagBits'),
var4=VarDef(name='sampleLocationGridSize', type='VkExtent2D'),
var5=VarDef(name='sampleLocationsCount', type='uint32_t'),
var6=VarDef(name='pSampleLocations', type='const VkSampleLocationEXT*', wrapType='CVkSampleLocationEXTArray', wrapParams='samplelocationsinfoext->sampleLocationsCount, samplelocationsinfoext->pSampleLocations', count='sampleLocationsCount')
)

Struct(name='VkSamplerBorderColorComponentMappingCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='components', type='VkComponentMapping'),
var4=VarDef(name='srgb', type='VkBool32')
)

Struct(name='VkSamplerCaptureDescriptorDataInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='sampler', type='VkSampler')
)

Struct(name='VkSamplerCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkSamplerCreateFlags'),
var4=VarDef(name='magFilter', type='VkFilter'),
var5=VarDef(name='minFilter', type='VkFilter'),
var6=VarDef(name='mipmapMode', type='VkSamplerMipmapMode'),
var7=VarDef(name='addressModeU', type='VkSamplerAddressMode'),
var8=VarDef(name='addressModeV', type='VkSamplerAddressMode'),
var9=VarDef(name='addressModeW', type='VkSamplerAddressMode'),
var10=VarDef(name='mipLodBias', type='float'),
var11=VarDef(name='anisotropyEnable', type='VkBool32'),
var12=VarDef(name='maxAnisotropy', type='float'),
var13=VarDef(name='compareEnable', type='VkBool32'),
var14=VarDef(name='compareOp', type='VkCompareOp'),
var15=VarDef(name='minLod', type='float'),
var16=VarDef(name='maxLod', type='float'),
var17=VarDef(name='borderColor', type='VkBorderColor'),
var18=VarDef(name='unnormalizedCoordinates', type='VkBool32')
)

Struct(name='VkSamplerCustomBorderColorCreateInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='customBorderColor', type='VkClearColorValue'),
var4=VarDef(name='format', type='VkFormat')
)

Struct(name='VkSamplerReductionModeCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='reductionMode', type='VkSamplerReductionMode')
)

Struct(name='VkSamplerYcbcrConversionCreateInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='format', type='VkFormat'),
var4=VarDef(name='ycbcrModel', type='VkSamplerYcbcrModelConversion'),
var5=VarDef(name='ycbcrRange', type='VkSamplerYcbcrRange'),
var6=VarDef(name='components', type='VkComponentMapping'),
var7=VarDef(name='xChromaOffset', type='VkChromaLocation'),
var8=VarDef(name='yChromaOffset', type='VkChromaLocation'),
var9=VarDef(name='chromaFilter', type='VkFilter'),
var10=VarDef(name='forceExplicitReconstruction', type='VkBool32')
)

Struct(name='VkSamplerYcbcrConversionImageFormatProperties_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='combinedImageSamplerDescriptorCount', type='uint32_t')
)

Struct(name='VkSamplerYcbcrConversionInfo_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='conversion', type='VkSamplerYcbcrConversion')
)

Struct(name='VkSemaphoreCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkSemaphoreCreateFlags')
)

Struct(name='VkSemaphoreGetFdInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='handleType', type='VkExternalSemaphoreHandleTypeFlagBits')
)

Struct(name='VkSemaphoreGetWin32HandleInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='handleType', type='VkExternalSemaphoreHandleTypeFlagBits')
)

Struct(name='VkSemaphoreSignalInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='value', type='uint64_t')
)

Struct(name='VkSemaphoreSignalInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='value', type='uint64_t')
)

Struct(name='VkSemaphoreSubmitInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='value', type='uint64_t'),
var5=VarDef(name='stageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='deviceIndex', type='uint32_t')
)

Struct(name='VkSemaphoreSubmitInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphore', type='VkSemaphore'),
var4=VarDef(name='value', type='uint64_t'),
var5=VarDef(name='stageMask', type='VkPipelineStageFlags2'),
var6=VarDef(name='deviceIndex', type='uint32_t')
)

Struct(name='VkSemaphoreTypeCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='semaphoreType', type='VkSemaphoreType'),
var4=VarDef(name='initialValue', type='uint64_t')
)

Struct(name='VkSemaphoreWaitInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkSemaphoreWaitFlags'),
var4=VarDef(name='semaphoreCount', type='uint32_t'),
var5=VarDef(name='pSemaphores', type='const VkSemaphore*', wrapType='CVkSemaphore::CSArray', wrapParams='semaphorewaitinfo->semaphoreCount, semaphorewaitinfo->pSemaphores', count='semaphoreCount'),
var6=VarDef(name='pValues', type='const uint64_t*', wrapType='Cuint64_t::CSArray', wrapParams='semaphorewaitinfo->semaphoreCount, semaphorewaitinfo->pValues', count='semaphoreCount')
)

Struct(name='VkSetStateFlagsIndirectCommandNV_', enabled=False,
var1=VarDef(name='data', type='uint32_t')
)

Struct(name='VkShaderCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkShaderCreateFlagsEXT'),
var4=VarDef(name='stage', type='VkShaderStageFlagBits'),
var5=VarDef(name='nextStage', type='VkShaderStageFlags'),
var6=VarDef(name='codeType', type='VkShaderCodeTypeEXT'),
var7=VarDef(name='codeSize', type='size_t'),
var8=VarDef(name='pCode', type='const void*'),
var9=VarDef(name='pName', type='const char*'),
var10=VarDef(name='setLayoutCount', type='uint32_t'),
var11=VarDef(name='pSetLayouts', type='const VkDescriptorSetLayout*'),
var12=VarDef(name='pushConstantRangeCount', type='uint32_t'),
var13=VarDef(name='pPushConstantRanges', type='const VkPushConstantRange*'),
var14=VarDef(name='pSpecializationInfo', type='const VkSpecializationInfo*')
)

Struct(name='VkShaderModuleCreateInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkShaderModuleCreateFlags'),
var4=VarDef(name='codeSize', type='size_t'),
var5=VarDef(name='pCode', type='const uint32_t*', wrapType='CVulkanShader', wrapParams='shadermodulecreateinfo->codeSize, shadermodulecreateinfo->pCode')
)

Struct(name='VkShaderModuleIdentifierEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='identifierSize', type='uint32_t'),
var4=VarDef(name='identifier', type='uint8_t[32]')
)

Struct(name='VkShaderModuleValidationCacheCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='validationCache', type='VkValidationCacheEXT')
)

Struct(name='VkShaderRequiredSubgroupSizeCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='requiredSubgroupSize', type='uint32_t')
)

Struct(name='VkShaderResourceUsageAMD_', enabled=False,
var1=VarDef(name='numUsedVgprs', type='uint32_t'),
var2=VarDef(name='numUsedSgprs', type='uint32_t'),
var3=VarDef(name='ldsSizePerLocalWorkGroup', type='uint32_t'),
var4=VarDef(name='ldsUsageSizeInBytes', type='size_t'),
var5=VarDef(name='scratchMemUsageInBytes', type='size_t')
)

Struct(name='VkShaderStatisticsInfoAMD_', enabled=False,
var1=VarDef(name='shaderStageMask', type='VkShaderStageFlags'),
var2=VarDef(name='resourceUsage', type='VkShaderResourceUsageAMD'),
var3=VarDef(name='numPhysicalVgprs', type='uint32_t'),
var4=VarDef(name='numPhysicalSgprs', type='uint32_t'),
var5=VarDef(name='numAvailableVgprs', type='uint32_t'),
var6=VarDef(name='numAvailableSgprs', type='uint32_t'),
var7=VarDef(name='computeWorkGroupSize', type='uint32_t[3]', count='3')
)

Struct(name='VkShadingRatePaletteNV_', enabled=False,
var1=VarDef(name='shadingRatePaletteEntryCount', type='uint32_t'),
var2=VarDef(name='pShadingRatePaletteEntries', type='const VkShadingRatePaletteEntryNV*')
)

Struct(name='VkSharedPresentSurfaceCapabilitiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='sharedPresentSupportedUsageFlags', type='VkImageUsageFlags')
)

Struct(name='VkSparseBufferMemoryBindInfo_', enabled=True, declareArray=True,
var1=VarDef(name='buffer', type='VkBuffer'),
var2=VarDef(name='bindCount', type='uint32_t'),
var3=VarDef(name='pBinds', type='const VkSparseMemoryBind*', wrapType='CVkSparseMemoryBindArray', wrapParams='sparsebuffermemorybindinfo->bindCount, sparsebuffermemorybindinfo->pBinds', count='bindCount')
)

Struct(name='VkSparseImageFormatProperties_', enabled=False,
var1=VarDef(name='aspectMask', type='VkImageAspectFlags'),
var2=VarDef(name='imageGranularity', type='VkExtent3D'),
var3=VarDef(name='flags', type='VkSparseImageFormatFlags')
)

Struct(name='VkSparseImageFormatProperties2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='properties', type='VkSparseImageFormatProperties')
)

Struct(name='VkSparseImageMemoryBind_', enabled=True, declareArray=True,
var1=VarDef(name='subresource', type='VkImageSubresource'),
var2=VarDef(name='offset', type='VkOffset3D'),
var3=VarDef(name='extent', type='VkExtent3D'),
var4=VarDef(name='memory', type='VkDeviceMemory'),
var5=VarDef(name='memoryOffset', type='VkDeviceSize'),
var6=VarDef(name='flags', type='VkSparseMemoryBindFlags')
)

Struct(name='VkSparseImageMemoryBindInfo_', enabled=True, declareArray=True,
var1=VarDef(name='image', type='VkImage'),
var2=VarDef(name='bindCount', type='uint32_t'),
var3=VarDef(name='pBinds', type='const VkSparseImageMemoryBind*', wrapType='CVkSparseImageMemoryBindArray', wrapParams='sparseimagememorybindinfo->bindCount, sparseimagememorybindinfo->pBinds', count='bindCount')
)

Struct(name='VkSparseImageMemoryRequirements_', enabled=False,
var1=VarDef(name='formatProperties', type='VkSparseImageFormatProperties'),
var2=VarDef(name='imageMipTailFirstLod', type='uint32_t'),
var3=VarDef(name='imageMipTailSize', type='VkDeviceSize'),
var4=VarDef(name='imageMipTailOffset', type='VkDeviceSize'),
var5=VarDef(name='imageMipTailStride', type='VkDeviceSize')
)

Struct(name='VkSparseImageMemoryRequirements2_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryRequirements', type='VkSparseImageMemoryRequirements')
)

Struct(name='VkSparseImageOpaqueMemoryBindInfo_', enabled=True, declareArray=True,
var1=VarDef(name='image', type='VkImage'),
var2=VarDef(name='bindCount', type='uint32_t'),
var3=VarDef(name='pBinds', type='const VkSparseMemoryBind*', wrapType='CVkSparseMemoryBindArray', wrapParams='sparseimageopaquememorybindinfo->bindCount, sparseimageopaquememorybindinfo->pBinds', count='bindCount')
)

Struct(name='VkSparseMemoryBind_', enabled=True, declareArray=True,
var1=VarDef(name='resourceOffset', type='VkDeviceSize'),
var2=VarDef(name='size', type='VkDeviceSize'),
var3=VarDef(name='memory', type='VkDeviceMemory'),
var4=VarDef(name='memoryOffset', type='VkDeviceSize'),
var5=VarDef(name='flags', type='VkSparseMemoryBindFlags')
)

Struct(name='VkSpecializationInfo_', enabled=True, declareArray=True,
var1=VarDef(name='mapEntryCount', type='uint32_t'),
var2=VarDef(name='pMapEntries', type='const VkSpecializationMapEntry*', wrapType='CVkSpecializationMapEntryArray', wrapParams='specializationinfo->mapEntryCount,specializationinfo->pMapEntries', count='mapEntryCount'),
var3=VarDef(name='dataSize', type='size_t'),
var4=VarDef(name='pData', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='specializationinfo->dataSize, (const uint8_t *)specializationinfo->pData')
)

Struct(name='VkSpecializationMapEntry_', enabled=True, declareArray=True,
var1=VarDef(name='constantID', type='uint32_t'),
var2=VarDef(name='offset', type='uint32_t'),
var3=VarDef(name='size', type='size_t')
)

Struct(name='VkStencilOpState_', enabled=True,
var1=VarDef(name='failOp', type='VkStencilOp'),
var2=VarDef(name='passOp', type='VkStencilOp'),
var3=VarDef(name='depthFailOp', type='VkStencilOp'),
var4=VarDef(name='compareOp', type='VkCompareOp'),
var5=VarDef(name='compareMask', type='uint32_t'),
var6=VarDef(name='writeMask', type='uint32_t'),
var7=VarDef(name='reference', type='uint32_t')
)

#Struct(name='VkStreamDescriptorSurfaceCreateInfoGGP_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='flags', type='VkStreamDescriptorSurfaceCreateFlagsGGP'),
#var4=VarDef(name='streamDescriptor', type='GgpStreamDescriptor')
#)

Struct(name='VkStridedDeviceAddressRegionKHR_', enabled=False,
var1=VarDef(name='deviceAddress', type='VkDeviceAddress'),
var2=VarDef(name='stride', type='VkDeviceSize'),
var3=VarDef(name='size', type='VkDeviceSize')
)

Struct(name='VkSubmitInfo_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphores', type='const VkSemaphore*', wrapType='CVkSemaphore::CSArray', wrapParams='submitinfo->waitSemaphoreCount, submitinfo->pWaitSemaphores', count='waitSemaphoreCount'),
var5=VarDef(name='pWaitDstStageMask', type='const VkPipelineStageFlags*', wrapType='Cuint32_t::CSArray', wrapParams='submitinfo->waitSemaphoreCount, submitinfo->pWaitDstStageMask', count='waitSemaphoreCount'),
var6=VarDef(name='commandBufferCount', type='uint32_t'),
var7=VarDef(name='pCommandBuffers', type='const VkCommandBuffer*', wrapType='CVkCommandBuffer::CSArray', wrapParams='submitinfo->commandBufferCount, submitinfo->pCommandBuffers', count='commandBufferCount'),
var8=VarDef(name='signalSemaphoreCount', type='uint32_t'),
var9=VarDef(name='pSignalSemaphores', type='const VkSemaphore*', wrapType='CVkSemaphore::CSArray', wrapParams='submitinfo->signalSemaphoreCount, submitinfo->pSignalSemaphores', count='signalSemaphoreCount')
)

Struct(name='VkSubmitInfo2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkSubmitFlags'),
var4=VarDef(name='waitSemaphoreInfoCount', type='uint32_t'),
var5=VarDef(name='pWaitSemaphoreInfos', type='const VkSemaphoreSubmitInfo*', wrapType='CVkSemaphoreSubmitInfoArray', wrapParams='submitinfo2->waitSemaphoreInfoCount, submitinfo2->pWaitSemaphoreInfos', count='waitSemaphoreInfoCount'),
var6=VarDef(name='commandBufferInfoCount', type='uint32_t'),
var7=VarDef(name='pCommandBufferInfos', type='const VkCommandBufferSubmitInfo*', wrapType='CVkCommandBufferSubmitInfoArray', wrapParams='submitinfo2->commandBufferInfoCount, submitinfo2->pCommandBufferInfos', count='commandBufferInfoCount'),
var8=VarDef(name='signalSemaphoreInfoCount', type='uint32_t'),
var9=VarDef(name='pSignalSemaphoreInfos', type='const VkSemaphoreSubmitInfo*', wrapType='CVkSemaphoreSubmitInfoArray', wrapParams='submitinfo2->signalSemaphoreInfoCount, submitinfo2->pSignalSemaphoreInfos', count='signalSemaphoreInfoCount')
)

Struct(name='VkSubpassBeginInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='contents', type='VkSubpassContents')
)

Struct(name='VkSubpassDependency_', enabled=True, declareArray=True,
var1=VarDef(name='srcSubpass', type='uint32_t'),
var2=VarDef(name='dstSubpass', type='uint32_t'),
var3=VarDef(name='srcStageMask', type='VkPipelineStageFlags'),
var4=VarDef(name='dstStageMask', type='VkPipelineStageFlags'),
var5=VarDef(name='srcAccessMask', type='VkAccessFlags'),
var6=VarDef(name='dstAccessMask', type='VkAccessFlags'),
var7=VarDef(name='dependencyFlags', type='VkDependencyFlags')
)

Struct(name='VkSubpassDependency2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='srcSubpass', type='uint32_t'),
var4=VarDef(name='dstSubpass', type='uint32_t'),
var5=VarDef(name='srcStageMask', type='VkPipelineStageFlags'),
var6=VarDef(name='dstStageMask', type='VkPipelineStageFlags'),
var7=VarDef(name='srcAccessMask', type='VkAccessFlags'),
var8=VarDef(name='dstAccessMask', type='VkAccessFlags'),
var9=VarDef(name='dependencyFlags', type='VkDependencyFlags'),
var10=VarDef(name='viewOffset', type='int32_t')
)

Struct(name='VkSubpassDescription_', enabled=True, declareArray=True,
var1=VarDef(name='flags', type='VkSubpassDescriptionFlags'),
var2=VarDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
var3=VarDef(name='inputAttachmentCount', type='uint32_t'),
var4=VarDef(name='pInputAttachments', type='const VkAttachmentReference*', wrapType='CVkAttachmentReferenceArray', wrapParams='subpassdescription->inputAttachmentCount, subpassdescription->pInputAttachments', count='inputAttachmentCount'),
var5=VarDef(name='colorAttachmentCount', type='uint32_t'),
var6=VarDef(name='pColorAttachments', type='const VkAttachmentReference*', wrapType='CVkAttachmentReferenceArray', wrapParams='subpassdescription->colorAttachmentCount, subpassdescription->pColorAttachments', count='colorAttachmentCount'),
var7=VarDef(name='pResolveAttachments', type='const VkAttachmentReference*', wrapType='CVkAttachmentReferenceArray', wrapParams='subpassdescription->colorAttachmentCount, subpassdescription->pResolveAttachments', count='colorAttachmentCount'),
var8=VarDef(name='pDepthStencilAttachment', type='const VkAttachmentReference*', wrapType='CVkAttachmentReferenceArray', wrapParams='1, subpassdescription->pDepthStencilAttachment'),
var9=VarDef(name='preserveAttachmentCount', type='uint32_t'),
var10=VarDef(name='pPreserveAttachments', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='subpassdescription->preserveAttachmentCount, subpassdescription->pPreserveAttachments', count='preserveAttachmentCount')
)

Struct(name='VkSubpassDescription2_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkSubpassDescriptionFlags'),
var4=VarDef(name='pipelineBindPoint', type='VkPipelineBindPoint'),
var5=VarDef(name='viewMask', type='uint32_t'),
var6=VarDef(name='inputAttachmentCount', type='uint32_t'),
var7=VarDef(name='pInputAttachments', type='const VkAttachmentReference2*', wrapType='CVkAttachmentReference2Array', wrapParams='subpassdescription2->inputAttachmentCount, subpassdescription2->pInputAttachments', count='inputAttachmentCount'),
var8=VarDef(name='colorAttachmentCount', type='uint32_t'),
var9=VarDef(name='pColorAttachments', type='const VkAttachmentReference2*', wrapType='CVkAttachmentReference2Array', wrapParams='subpassdescription2->colorAttachmentCount, subpassdescription2->pColorAttachments', count='colorAttachmentCount'),
var10=VarDef(name='pResolveAttachments', type='const VkAttachmentReference2*', wrapType='CVkAttachmentReference2Array', wrapParams='subpassdescription2->colorAttachmentCount, subpassdescription2->pResolveAttachments', count='colorAttachmentCount'),
var11=VarDef(name='pDepthStencilAttachment', type='const VkAttachmentReference2*', wrapType='CVkAttachmentReference2Array', wrapParams='1, subpassdescription2->pDepthStencilAttachment'),
var12=VarDef(name='preserveAttachmentCount', type='uint32_t'),
var13=VarDef(name='pPreserveAttachments', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='subpassdescription2->preserveAttachmentCount, subpassdescription2->pPreserveAttachments', count='preserveAttachmentCount')
)

Struct(name='VkSubpassDescriptionDepthStencilResolve_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='depthResolveMode', type='VkResolveModeFlagBits'),
var4=VarDef(name='stencilResolveMode', type='VkResolveModeFlagBits'),
var5=VarDef(name='pDepthStencilResolveAttachment', type='const VkAttachmentReference2*', wrapType='CVkAttachmentReference2Array', wrapParams='1, subpassdescriptiondepthstencilresolve->pDepthStencilResolveAttachment')
)

Struct(name='VkSubpassEndInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*')
)

Struct(name='VkSubpassEndInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*')
)

Struct(name='VkSubpassResolvePerformanceQueryEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='optimal', type='VkBool32')
)

Struct(name='VkSubpassSampleLocationsEXT_', enabled=False,
var1=VarDef(name='subpassIndex', type='uint32_t'),
var2=VarDef(name='sampleLocationsInfo', type='VkSampleLocationsInfoEXT')
)

Struct(name='VkSubresourceLayout_', enabled=True,
var1=VarDef(name='offset', type='VkDeviceSize'),
var2=VarDef(name='size', type='VkDeviceSize'),
var3=VarDef(name='rowPitch', type='VkDeviceSize'),
var4=VarDef(name='arrayPitch', type='VkDeviceSize'),
var5=VarDef(name='depthPitch', type='VkDeviceSize')
)

Struct(name='VkSubresourceLayout2EXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='subresourceLayout', type='VkSubresourceLayout')
)

Struct(name='VkSurfaceCapabilities2EXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='minImageCount', type='uint32_t'),
var4=VarDef(name='maxImageCount', type='uint32_t'),
var5=VarDef(name='currentExtent', type='VkExtent2D'),
var6=VarDef(name='minImageExtent', type='VkExtent2D'),
var7=VarDef(name='maxImageExtent', type='VkExtent2D'),
var8=VarDef(name='maxImageArrayLayers', type='uint32_t'),
var9=VarDef(name='supportedTransforms', type='VkSurfaceTransformFlagsKHR'),
var10=VarDef(name='currentTransform', type='VkSurfaceTransformFlagBitsKHR'),
var11=VarDef(name='supportedCompositeAlpha', type='VkCompositeAlphaFlagsKHR'),
var12=VarDef(name='supportedUsageFlags', type='VkImageUsageFlags'),
var13=VarDef(name='supportedSurfaceCounters', type='VkSurfaceCounterFlagsEXT')
)

Struct(name='VkSurfaceCapabilities2KHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='surfaceCapabilities', type='VkSurfaceCapabilitiesKHR')
)

Struct(name='VkSurfaceCapabilitiesFullScreenExclusiveEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fullScreenExclusiveSupported', type='VkBool32')
)

Struct(name='VkSurfaceCapabilitiesKHR_', enabled=True,
var1=VarDef(name='minImageCount', type='uint32_t'),
var2=VarDef(name='maxImageCount', type='uint32_t'),
var3=VarDef(name='currentExtent', type='VkExtent2D'),
var4=VarDef(name='minImageExtent', type='VkExtent2D'),
var5=VarDef(name='maxImageExtent', type='VkExtent2D'),
var6=VarDef(name='maxImageArrayLayers', type='uint32_t'),
var7=VarDef(name='supportedTransforms', type='VkSurfaceTransformFlagsKHR'),
var8=VarDef(name='currentTransform', type='VkSurfaceTransformFlagBitsKHR'),
var9=VarDef(name='supportedCompositeAlpha', type='VkCompositeAlphaFlagsKHR'),
var10=VarDef(name='supportedUsageFlags', type='VkImageUsageFlags')
)

Struct(name='VkSurfaceCapabilitiesPresentBarrierNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentBarrierSupported', type='VkBool32')
)

Struct(name='VkSurfaceFormat2KHR_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='surfaceFormat', type='VkSurfaceFormatKHR')
)

Struct(name='VkSurfaceFormatKHR_', enabled=True, declareArray=True,
var1=VarDef(name='format', type='VkFormat'),
var2=VarDef(name='colorSpace', type='VkColorSpaceKHR')
)

Struct(name='VkSurfaceFullScreenExclusiveInfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='fullScreenExclusive', type='VkFullScreenExclusiveEXT')
)

Struct(name='VkSurfaceFullScreenExclusiveWin32InfoEXT_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='hmonitor', type='HMONITOR', wrapType='CVkHMONITOR')
)

Struct(name='VkSurfacePresentModeCompatibilityEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentModeCount', type='uint32_t'),
var4=VarDef(name='pPresentModes', type='VkPresentModeKHR*')
)

Struct(name='VkSurfacePresentModeEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentMode', type='VkPresentModeKHR')
)

Struct(name='VkSurfacePresentScalingCapabilitiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='supportedPresentScaling', type='VkPresentScalingFlagsEXT'),
var4=VarDef(name='supportedPresentGravityX', type='VkPresentGravityFlagsEXT'),
var5=VarDef(name='supportedPresentGravityY', type='VkPresentGravityFlagsEXT'),
var6=VarDef(name='minScaledImageExtent', type='VkExtent2D'),
var7=VarDef(name='maxScaledImageExtent', type='VkExtent2D')
)

Struct(name='VkSurfaceProtectedCapabilitiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='supportsProtected', type='VkBool32')
)

Struct(name='VkSwapchainCounterCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='surfaceCounters', type='VkSurfaceCounterFlagsEXT')
)

Struct(name='VkSwapchainCreateInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkSwapchainCreateFlagsKHR'),
var4=VarDef(name='surface', type='VkSurfaceKHR'),
var5=VarDef(name='minImageCount', type='uint32_t'),
var6=VarDef(name='imageFormat', type='VkFormat'),
var7=VarDef(name='imageColorSpace', type='VkColorSpaceKHR'),
var8=VarDef(name='imageExtent', type='VkExtent2D'),
var9=VarDef(name='imageArrayLayers', type='uint32_t'),
var10=VarDef(name='imageUsage', type='VkImageUsageFlags'),
var11=VarDef(name='imageSharingMode', type='VkSharingMode'),
var12=VarDef(name='queueFamilyIndexCount', type='uint32_t'),
var13=VarDef(name='pQueueFamilyIndices', type='const uint32_t*', wrapType='Cuint32_t::CSArray', wrapParams='swapchaincreateinfokhr->queueFamilyIndexCount, swapchaincreateinfokhr->pQueueFamilyIndices', count='queueFamilyIndexCount'),
var14=VarDef(name='preTransform', type='VkSurfaceTransformFlagBitsKHR'),
var15=VarDef(name='compositeAlpha', type='VkCompositeAlphaFlagBitsKHR'),
var16=VarDef(name='presentMode', type='VkPresentModeKHR'),
var17=VarDef(name='clipped', type='VkBool32'),
var18=VarDef(name='oldSwapchain', type='VkSwapchainKHR')
)

Struct(name='VkSwapchainDisplayNativeHdrCreateInfoAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='localDimmingEnable', type='VkBool32')
)

Struct(name='VkSwapchainPresentBarrierCreateInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='presentBarrierEnable', type='VkBool32')
)

Struct(name='VkSwapchainPresentFenceInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchainCount', type='uint32_t'),
var4=VarDef(name='pFences', type='const VkFence*')
)

Struct(name='VkSwapchainPresentModeInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='swapchainCount', type='uint32_t'),
var4=VarDef(name='pPresentModes', type='const VkPresentModeKHR*')
)

Struct(name='VkSwapchainPresentModesCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='presentModeCount', type='uint32_t'),
var4=VarDef(name='pPresentModes', type='const VkPresentModeKHR*')
)

Struct(name='VkSwapchainPresentScalingCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='scalingBehavior', type='VkPresentScalingFlagsEXT'),
var4=VarDef(name='presentGravityX', type='VkPresentGravityFlagsEXT'),
var5=VarDef(name='presentGravityY', type='VkPresentGravityFlagsEXT')
)

Struct(name='VkTextureLODGatherFormatPropertiesAMD_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='supportsTextureGatherLODBiasAMD', type='VkBool32')
)

Struct(name='VkTimelineSemaphoreSubmitInfo_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreValueCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphoreValues', type='const uint64_t*', wrapType='Cuint64_t::CSArray', wrapParams='timelinesemaphoresubmitinfo->waitSemaphoreValueCount, timelinesemaphoresubmitinfo->pWaitSemaphoreValues', count='waitSemaphoreValueCount'),
var5=VarDef(name='signalSemaphoreValueCount', type='uint32_t'),
var6=VarDef(name='pSignalSemaphoreValues', type='const uint64_t*', wrapType='Cuint64_t::CSArray', wrapParams='timelinesemaphoresubmitinfo->signalSemaphoreValueCount, timelinesemaphoresubmitinfo->pSignalSemaphoreValues', count='signalSemaphoreValueCount')
)

Struct(name='VkTimelineSemaphoreSubmitInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='waitSemaphoreValueCount', type='uint32_t'),
var4=VarDef(name='pWaitSemaphoreValues', type='const uint64_t*'),
var5=VarDef(name='signalSemaphoreValueCount', type='uint32_t'),
var6=VarDef(name='pSignalSemaphoreValues', type='const uint64_t*')
)

Struct(name='VkTraceRaysIndirectCommand2KHR_', enabled=False,
var1=VarDef(name='raygenShaderRecordAddress', type='VkDeviceAddress'),
var2=VarDef(name='raygenShaderRecordSize', type='VkDeviceSize'),
var3=VarDef(name='missShaderBindingTableAddress', type='VkDeviceAddress'),
var4=VarDef(name='missShaderBindingTableSize', type='VkDeviceSize'),
var5=VarDef(name='missShaderBindingTableStride', type='VkDeviceSize'),
var6=VarDef(name='hitShaderBindingTableAddress', type='VkDeviceAddress'),
var7=VarDef(name='hitShaderBindingTableSize', type='VkDeviceSize'),
var8=VarDef(name='hitShaderBindingTableStride', type='VkDeviceSize'),
var9=VarDef(name='callableShaderBindingTableAddress', type='VkDeviceAddress'),
var10=VarDef(name='callableShaderBindingTableSize', type='VkDeviceSize'),
var11=VarDef(name='callableShaderBindingTableStride', type='VkDeviceSize'),
var12=VarDef(name='width', type='uint32_t'),
var13=VarDef(name='height', type='uint32_t'),
var14=VarDef(name='depth', type='uint32_t')
)

Struct(name='VkTraceRaysIndirectCommandKHR_', enabled=False,
var1=VarDef(name='width', type='uint32_t'),
var2=VarDef(name='height', type='uint32_t'),
var3=VarDef(name='depth', type='uint32_t')
)

Struct(name='VkTransformMatrixKHR_', enabled=False,
var1=VarDef(name='matrix', type='float[3][4]')
)

Struct(name='VkValidationCacheCreateInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkValidationCacheCreateFlagsEXT'),
var4=VarDef(name='initialDataSize', type='size_t'),
var5=VarDef(name='pInitialData', type='const void*')
)

Struct(name='VkValidationFeaturesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='enabledValidationFeatureCount', type='uint32_t'),
var4=VarDef(name='pEnabledValidationFeatures', type='const VkValidationFeatureEnableEXT*'),
var5=VarDef(name='disabledValidationFeatureCount', type='uint32_t'),
var6=VarDef(name='pDisabledValidationFeatures', type='const VkValidationFeatureDisableEXT*')
)

Struct(name='VkValidationFlagsEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='disabledValidationCheckCount', type='uint32_t'),
var4=VarDef(name='pDisabledValidationChecks', type='VkValidationCheckEXT*', count='disabledValidationCheckCount')
)

Struct(name='VkVertexInputAttributeDescription_', enabled=True, declareArray=True,
var1=VarDef(name='location', type='uint32_t'),
var2=VarDef(name='binding', type='uint32_t'),
var3=VarDef(name='format', type='VkFormat'),
var4=VarDef(name='offset', type='uint32_t')
)

Struct(name='VkVertexInputAttributeDescription2EXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='location', type='uint32_t'),
var4=VarDef(name='binding', type='uint32_t'),
var5=VarDef(name='format', type='VkFormat'),
var6=VarDef(name='offset', type='uint32_t')
)

Struct(name='VkVertexInputBindingDescription_', enabled=True, declareArray=True,
var1=VarDef(name='binding', type='uint32_t'),
var2=VarDef(name='stride', type='uint32_t'),
var3=VarDef(name='inputRate', type='VkVertexInputRate')
)

Struct(name='VkVertexInputBindingDescription2EXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='binding', type='uint32_t'),
var4=VarDef(name='stride', type='uint32_t'),
var5=VarDef(name='inputRate', type='VkVertexInputRate'),
var6=VarDef(name='divisor', type='uint32_t')
)

Struct(name='VkVertexInputBindingDivisorDescriptionEXT_', enabled=True, declareArray=True,
var1=VarDef(name='binding', type='uint32_t'),
var2=VarDef(name='divisor', type='uint32_t')
)

Struct(name='VkVideoBeginCodingInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoBeginCodingFlagsKHR'),
var4=VarDef(name='videoSession', type='VkVideoSessionKHR'),
var5=VarDef(name='videoSessionParameters', type='VkVideoSessionParametersKHR'),
var6=VarDef(name='referenceSlotCount', type='uint32_t'),
var7=VarDef(name='pReferenceSlots', type='const VkVideoReferenceSlotInfoKHR*')
)

Struct(name='VkVideoCapabilitiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkVideoCapabilityFlagsKHR'),
var4=VarDef(name='minBitstreamBufferOffsetAlignment', type='VkDeviceSize'),
var5=VarDef(name='minBitstreamBufferSizeAlignment', type='VkDeviceSize'),
var6=VarDef(name='pictureAccessGranularity', type='VkExtent2D'),
var7=VarDef(name='minCodedExtent', type='VkExtent2D'),
var8=VarDef(name='maxCodedExtent', type='VkExtent2D'),
var9=VarDef(name='maxDpbSlots', type='uint32_t'),
var10=VarDef(name='maxActiveReferencePictures', type='uint32_t'),
var11=VarDef(name='stdHeaderVersion', type='VkExtensionProperties')
)

Struct(name='VkVideoCodingControlInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoCodingControlFlagsKHR')
)

Struct(name='VkVideoDecodeCapabilitiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkVideoDecodeCapabilityFlagsKHR')
)

#Struct(name='VkVideoDecodeH264CapabilitiesKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='void*'),
#var3=VarDef(name='maxLevelIdc', type='StdVideoH264LevelIdc'),
#var4=VarDef(name='fieldOffsetGranularity', type='VkOffset2D')
#)

#Struct(name='VkVideoDecodeH264DpbSlotInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pStdReferenceInfo', type='const StdVideoDecodeH264ReferenceInfo*')
#)

#Struct(name='VkVideoDecodeH264PictureInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pStdPictureInfo', type='const StdVideoDecodeH264PictureInfo*'),
#var4=VarDef(name='sliceCount', type='uint32_t'),
#var5=VarDef(name='pSliceOffsets', type='const uint32_t*')
#)

#Struct(name='VkVideoDecodeH264ProfileInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdProfileIdc', type='StdVideoH264ProfileIdc'),
#var4=VarDef(name='pictureLayout', type='VkVideoDecodeH264PictureLayoutFlagBitsKHR')
#)

#Struct(name='VkVideoDecodeH264SessionParametersAddInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdSPSCount', type='uint32_t'),
#var4=VarDef(name='pStdSPSs', type='const StdVideoH264SequenceParameterSet*'),
#var5=VarDef(name='stdPPSCount', type='uint32_t'),
#var6=VarDef(name='pStdPPSs', type='const StdVideoH264PictureParameterSet*')
#)

#Struct(name='VkVideoDecodeH264SessionParametersCreateInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='maxStdSPSCount', type='uint32_t'),
#var4=VarDef(name='maxStdPPSCount', type='uint32_t'),
#var5=VarDef(name='pParametersAddInfo', type='const VkVideoDecodeH264SessionParametersAddInfoKHR*')
#)

#Struct(name='VkVideoDecodeH265CapabilitiesKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='void*'),
#var3=VarDef(name='maxLevelIdc', type='StdVideoH265LevelIdc')
#)

#Struct(name='VkVideoDecodeH265DpbSlotInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pStdReferenceInfo', type='const StdVideoDecodeH265ReferenceInfo*')
#)

#Struct(name='VkVideoDecodeH265PictureInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pStdPictureInfo', type='StdVideoDecodeH265PictureInfo*'),
#var4=VarDef(name='sliceSegmentCount', type='uint32_t'),
#var5=VarDef(name='pSliceSegmentOffsets', type='const uint32_t*')
#)

#Struct(name='VkVideoDecodeH265ProfileInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdProfileIdc', type='StdVideoH265ProfileIdc')
#)

#Struct(name='VkVideoDecodeH265SessionParametersAddInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdVPSCount', type='uint32_t'),
#var4=VarDef(name='pStdVPSs', type='const StdVideoH265VideoParameterSet*'),
#var5=VarDef(name='stdSPSCount', type='uint32_t'),
#var6=VarDef(name='pStdSPSs', type='const StdVideoH265SequenceParameterSet*'),
#var7=VarDef(name='stdPPSCount', type='uint32_t'),
#var8=VarDef(name='pStdPPSs', type='const StdVideoH265PictureParameterSet*')
#)

#Struct(name='VkVideoDecodeH265SessionParametersCreateInfoKHR_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='maxStdVPSCount', type='uint32_t'),
#var4=VarDef(name='maxStdSPSCount', type='uint32_t'),
#var5=VarDef(name='maxStdPPSCount', type='uint32_t'),
#var6=VarDef(name='pParametersAddInfo', type='const VkVideoDecodeH265SessionParametersAddInfoKHR*')
#)

Struct(name='VkVideoDecodeInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoDecodeFlagsKHR'),
var4=VarDef(name='srcBuffer', type='VkBuffer'),
var5=VarDef(name='srcBufferOffset', type='VkDeviceSize'),
var6=VarDef(name='srcBufferRange', type='VkDeviceSize'),
var7=VarDef(name='dstPictureResource', type='VkVideoPictureResourceInfoKHR'),
var8=VarDef(name='pSetupReferenceSlot', type='const VkVideoReferenceSlotInfoKHR*'),
var9=VarDef(name='referenceSlotCount', type='uint32_t'),
var10=VarDef(name='pReferenceSlots', type='const VkVideoReferenceSlotInfoKHR*')
)

Struct(name='VkVideoDecodeUsageInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='videoUsageHints', type='VkVideoDecodeUsageFlagsKHR')
)

Struct(name='VkVideoEncodeCapabilitiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkVideoEncodeCapabilityFlagsKHR'),
var4=VarDef(name='rateControlModes', type='VkVideoEncodeRateControlModeFlagsKHR'),
var5=VarDef(name='maxRateControlLayers', type='uint32_t'),
var6=VarDef(name='maxQualityLevels', type='uint32_t'),
var7=VarDef(name='inputImageDataFillAlignment', type='VkExtent2D'),
var8=VarDef(name='supportedEncodeFeedbackFlags', type='VkVideoEncodeFeedbackFlagsKHR')
)

Struct(name='VkVideoEncodeH264CapabilitiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkVideoEncodeH264CapabilityFlagsEXT'),
var4=VarDef(name='maxPPictureL0ReferenceCount', type='uint32_t'),
var5=VarDef(name='maxBPictureL0ReferenceCount', type='uint32_t'),
var6=VarDef(name='maxL1ReferenceCount', type='uint32_t'),
var7=VarDef(name='motionVectorsOverPicBoundariesFlag', type='VkBool32'),
var8=VarDef(name='maxBytesPerPicDenom', type='uint32_t'),
var9=VarDef(name='maxBitsPerMbDenom', type='uint32_t'),
var10=VarDef(name='log2MaxMvLengthHorizontal', type='uint32_t'),
var11=VarDef(name='log2MaxMvLengthVertical', type='uint32_t')
)

#Struct(name='VkVideoEncodeH264DpbSlotInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pStdReferenceInfo', type='const StdVideoEncodeH264ReferenceInfo*')
#)

Struct(name='VkVideoEncodeH264FrameSizeEXT_', enabled=False,
var1=VarDef(name='frameISize', type='uint32_t'),
var2=VarDef(name='framePSize', type='uint32_t'),
var3=VarDef(name='frameBSize', type='uint32_t')
)

#Struct(name='VkVideoEncodeH264NaluSliceInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='mbCount', type='uint32_t'),
#var4=VarDef(name='pReferenceFinalLists', type='const VkVideoEncodeH264ReferenceListsInfoEXT*'),
#var5=VarDef(name='pSliceHeaderStd', type='const StdVideoEncodeH264SliceHeader*')
#)

#Struct(name='VkVideoEncodeH264ProfileInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdProfileIdc', type='StdVideoH264ProfileIdc')
#)

Struct(name='VkVideoEncodeH264QpEXT_', enabled=False,
var1=VarDef(name='qpI', type='int32_t'),
var2=VarDef(name='qpP', type='int32_t'),
var3=VarDef(name='qpB', type='int32_t')
)

Struct(name='VkVideoEncodeH264RateControlInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='gopFrameCount', type='uint32_t'),
var4=VarDef(name='idrPeriod', type='uint32_t'),
var5=VarDef(name='consecutiveBFrameCount', type='uint32_t'),
var6=VarDef(name='rateControlStructure', type='VkVideoEncodeH264RateControlStructureEXT'),
var7=VarDef(name='temporalLayerCount', type='uint32_t')
)

Struct(name='VkVideoEncodeH264RateControlLayerInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='temporalLayerId', type='uint32_t'),
var4=VarDef(name='useInitialRcQp', type='VkBool32'),
var5=VarDef(name='initialRcQp', type='VkVideoEncodeH264QpEXT'),
var6=VarDef(name='useMinQp', type='VkBool32'),
var7=VarDef(name='minQp', type='VkVideoEncodeH264QpEXT'),
var8=VarDef(name='useMaxQp', type='VkBool32'),
var9=VarDef(name='maxQp', type='VkVideoEncodeH264QpEXT'),
var10=VarDef(name='useMaxFrameSize', type='VkBool32'),
var11=VarDef(name='maxFrameSize', type='VkVideoEncodeH264FrameSizeEXT')
)

#Struct(name='VkVideoEncodeH264SessionParametersAddInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdSPSCount', type='uint32_t'),
#var4=VarDef(name='pStdSPSs', type='const StdVideoH264SequenceParameterSet*'),
#var5=VarDef(name='stdPPSCount', type='uint32_t'),
#var6=VarDef(name='pStdPPSs', type='const StdVideoH264PictureParameterSet*')
#)

#Struct(name='VkVideoEncodeH264SessionParametersCreateInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='maxStdSPSCount', type='uint32_t'),
#var4=VarDef(name='maxStdPPSCount', type='uint32_t'),
#var5=VarDef(name='pParametersAddInfo', type='const VkVideoEncodeH264SessionParametersAddInfoEXT*')
#)

#Struct(name='VkVideoEncodeH264VclFrameInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pReferenceFinalLists', type='const VkVideoEncodeH264ReferenceListsInfoEXT*'),
#var4=VarDef(name='naluSliceEntryCount', type='uint32_t'),
#var5=VarDef(name='pNaluSliceEntries', type='const VkVideoEncodeH264NaluSliceInfoEXT*'),
#var6=VarDef(name='pCurrentPictureInfo', type='const StdVideoEncodeH264PictureInfo*')
#)

Struct(name='VkVideoEncodeH265CapabilitiesEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='flags', type='VkVideoEncodeH265CapabilityFlagsEXT'),
var4=VarDef(name='ctbSizes', type='VkVideoEncodeH265CtbSizeFlagsEXT'),
var5=VarDef(name='transformBlockSizes', type='VkVideoEncodeH265TransformBlockSizeFlagsEXT'),
var6=VarDef(name='maxPPictureL0ReferenceCount', type='uint32_t'),
var7=VarDef(name='maxBPictureL0ReferenceCount', type='uint32_t'),
var8=VarDef(name='maxL1ReferenceCount', type='uint32_t'),
var9=VarDef(name='maxSubLayersCount', type='uint32_t'),
var10=VarDef(name='minLog2MinLumaCodingBlockSizeMinus3', type='uint32_t'),
var11=VarDef(name='maxLog2MinLumaCodingBlockSizeMinus3', type='uint32_t'),
var12=VarDef(name='minLog2MinLumaTransformBlockSizeMinus2', type='uint32_t'),
var13=VarDef(name='maxLog2MinLumaTransformBlockSizeMinus2', type='uint32_t'),
var14=VarDef(name='minMaxTransformHierarchyDepthInter', type='uint32_t'),
var15=VarDef(name='maxMaxTransformHierarchyDepthInter', type='uint32_t'),
var16=VarDef(name='minMaxTransformHierarchyDepthIntra', type='uint32_t'),
var17=VarDef(name='maxMaxTransformHierarchyDepthIntra', type='uint32_t'),
var18=VarDef(name='maxDiffCuQpDeltaDepth', type='uint32_t'),
var19=VarDef(name='minMaxNumMergeCand', type='uint32_t'),
var20=VarDef(name='maxMaxNumMergeCand', type='uint32_t')
)

#Struct(name='VkVideoEncodeH265DpbSlotInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pStdReferenceInfo', type='const StdVideoEncodeH265ReferenceInfo*')
#)

Struct(name='VkVideoEncodeH265FrameSizeEXT_', enabled=False,
var1=VarDef(name='frameISize', type='uint32_t'),
var2=VarDef(name='framePSize', type='uint32_t'),
var3=VarDef(name='frameBSize', type='uint32_t')
)

#Struct(name='VkVideoEncodeH265NaluSliceSegmentInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='ctbCount', type='uint32_t'),
#var4=VarDef(name='pReferenceFinalLists', type='const VkVideoEncodeH265ReferenceListsInfoEXT*'),
#var5=VarDef(name='pSliceSegmentHeaderStd', type='const StdVideoEncodeH265SliceSegmentHeader*')
#)

#Struct(name='VkVideoEncodeH265ProfileInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdProfileIdc', type='StdVideoH265ProfileIdc')
#)

Struct(name='VkVideoEncodeH265QpEXT_', enabled=False,
var1=VarDef(name='qpI', type='int32_t'),
var2=VarDef(name='qpP', type='int32_t'),
var3=VarDef(name='qpB', type='int32_t')
)

Struct(name='VkVideoEncodeH265RateControlInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='gopFrameCount', type='uint32_t'),
var4=VarDef(name='idrPeriod', type='uint32_t'),
var5=VarDef(name='consecutiveBFrameCount', type='uint32_t'),
var6=VarDef(name='rateControlStructure', type='VkVideoEncodeH265RateControlStructureEXT'),
var7=VarDef(name='subLayerCount', type='uint32_t')
)

Struct(name='VkVideoEncodeH265RateControlLayerInfoEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='temporalId', type='uint32_t'),
var4=VarDef(name='useInitialRcQp', type='VkBool32'),
var5=VarDef(name='initialRcQp', type='VkVideoEncodeH265QpEXT'),
var6=VarDef(name='useMinQp', type='VkBool32'),
var7=VarDef(name='minQp', type='VkVideoEncodeH265QpEXT'),
var8=VarDef(name='useMaxQp', type='VkBool32'),
var9=VarDef(name='maxQp', type='VkVideoEncodeH265QpEXT'),
var10=VarDef(name='useMaxFrameSize', type='VkBool32'),
var11=VarDef(name='maxFrameSize', type='VkVideoEncodeH265FrameSizeEXT')
)

#Struct(name='VkVideoEncodeH265SessionParametersAddInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='stdVPSCount', type='uint32_t'),
#var4=VarDef(name='pStdVPSs', type='const StdVideoH265VideoParameterSet*'),
#var5=VarDef(name='stdSPSCount', type='uint32_t'),
#var6=VarDef(name='pStdSPSs', type='const StdVideoH265SequenceParameterSet*'),
#var7=VarDef(name='stdPPSCount', type='uint32_t'),
#var8=VarDef(name='pStdPPSs', type='const StdVideoH265PictureParameterSet*')
#)

#Struct(name='VkVideoEncodeH265SessionParametersCreateInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='maxStdVPSCount', type='uint32_t'),
#var4=VarDef(name='maxStdSPSCount', type='uint32_t'),
#var5=VarDef(name='maxStdPPSCount', type='uint32_t'),
#var6=VarDef(name='pParametersAddInfo', type='const VkVideoEncodeH265SessionParametersAddInfoEXT*')
#)

#Struct(name='VkVideoEncodeH265VclFrameInfoEXT_', enabled=False,
#var1=VarDef(name='sType', type='VkStructureType'),
#var2=VarDef(name='pNext', type='const void*'),
#var3=VarDef(name='pReferenceFinalLists', type='const VkVideoEncodeH265ReferenceListsInfoEXT*'),
#var4=VarDef(name='naluSliceSegmentEntryCount', type='uint32_t'),
#var5=VarDef(name='pNaluSliceSegmentEntries', type='const VkVideoEncodeH265NaluSliceSegmentInfoEXT*'),
#var6=VarDef(name='pCurrentPictureInfo', type='const StdVideoEncodeH265PictureInfo*')
#)

Struct(name='VkVideoEncodeInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoEncodeFlagsKHR'),
var4=VarDef(name='qualityLevel', type='uint32_t'),
var5=VarDef(name='dstBuffer', type='VkBuffer'),
var6=VarDef(name='dstBufferOffset', type='VkDeviceSize'),
var7=VarDef(name='dstBufferRange', type='VkDeviceSize'),
var8=VarDef(name='srcPictureResource', type='VkVideoPictureResourceInfoKHR'),
var9=VarDef(name='pSetupReferenceSlot', type='const VkVideoReferenceSlotInfoKHR*'),
var10=VarDef(name='referenceSlotCount', type='uint32_t'),
var11=VarDef(name='pReferenceSlots', type='const VkVideoReferenceSlotInfoKHR*'),
var12=VarDef(name='precedingExternallyEncodedBytes', type='uint32_t')
)

Struct(name='VkVideoEncodeRateControlInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoEncodeRateControlFlagsKHR'),
var4=VarDef(name='rateControlMode', type='VkVideoEncodeRateControlModeFlagBitsKHR'),
var5=VarDef(name='layerCount', type='uint32_t'),
var6=VarDef(name='pLayers', type='const VkVideoEncodeRateControlLayerInfoKHR*')
)

Struct(name='VkVideoEncodeRateControlLayerInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='averageBitrate', type='uint64_t'),
var4=VarDef(name='maxBitrate', type='uint64_t'),
var5=VarDef(name='frameRateNumerator', type='uint32_t'),
var6=VarDef(name='frameRateDenominator', type='uint32_t'),
var7=VarDef(name='virtualBufferSizeInMs', type='uint32_t'),
var8=VarDef(name='initialVirtualBufferSizeInMs', type='uint32_t')
)

Struct(name='VkVideoEncodeUsageInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='videoUsageHints', type='VkVideoEncodeUsageFlagsKHR'),
var4=VarDef(name='videoContentHints', type='VkVideoEncodeContentFlagsKHR'),
var5=VarDef(name='tuningMode', type='VkVideoEncodeTuningModeKHR')
)

Struct(name='VkVideoEndCodingInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoEndCodingFlagsKHR')
)

Struct(name='VkVideoFormatPropertiesKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='format', type='VkFormat'),
var4=VarDef(name='componentMapping', type='VkComponentMapping'),
var5=VarDef(name='imageCreateFlags', type='VkImageCreateFlags'),
var6=VarDef(name='imageType', type='VkImageType'),
var7=VarDef(name='imageTiling', type='VkImageTiling'),
var8=VarDef(name='imageUsageFlags', type='VkImageUsageFlags')
)

Struct(name='VkVideoPictureResourceInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='codedOffset', type='VkOffset2D'),
var4=VarDef(name='codedExtent', type='VkExtent2D'),
var5=VarDef(name='baseArrayLayer', type='uint32_t'),
var6=VarDef(name='imageViewBinding', type='VkImageView')
)

Struct(name='VkVideoProfileInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='videoCodecOperation', type='VkVideoCodecOperationFlagBitsKHR'),
var4=VarDef(name='chromaSubsampling', type='VkVideoChromaSubsamplingFlagsKHR'),
var5=VarDef(name='lumaBitDepth', type='VkVideoComponentBitDepthFlagsKHR'),
var6=VarDef(name='chromaBitDepth', type='VkVideoComponentBitDepthFlagsKHR')
)

Struct(name='VkVideoProfileListInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='profileCount', type='uint32_t'),
var4=VarDef(name='pProfiles', type='const VkVideoProfileInfoKHR*')
)

Struct(name='VkVideoReferenceSlotInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='slotIndex', type='int32_t'),
var4=VarDef(name='pPictureResource', type='const VkVideoPictureResourceInfoKHR*')
)

Struct(name='VkVideoSessionCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='queueFamilyIndex', type='uint32_t'),
var4=VarDef(name='flags', type='VkVideoSessionCreateFlagsKHR'),
var5=VarDef(name='pVideoProfile', type='const VkVideoProfileInfoKHR*'),
var6=VarDef(name='pictureFormat', type='VkFormat'),
var7=VarDef(name='maxCodedExtent', type='VkExtent2D'),
var8=VarDef(name='referencePictureFormat', type='VkFormat'),
var9=VarDef(name='maxDpbSlots', type='uint32_t'),
var10=VarDef(name='maxActiveReferencePictures', type='uint32_t'),
var11=VarDef(name='pStdHeaderVersion', type='const VkExtensionProperties*')
)

Struct(name='VkVideoSessionMemoryRequirementsKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='memoryBindIndex', type='uint32_t'),
var4=VarDef(name='memoryRequirements', type='VkMemoryRequirements')
)

Struct(name='VkVideoSessionParametersCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkVideoSessionParametersCreateFlagsKHR'),
var4=VarDef(name='videoSessionParametersTemplate', type='VkVideoSessionParametersKHR'),
var5=VarDef(name='videoSession', type='VkVideoSessionKHR')
)

Struct(name='VkVideoSessionParametersUpdateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='updateSequenceCount', type='uint32_t')
)

Struct(name='VkViewport_', enabled=True, declareArray=True,
var1=VarDef(name='x', type='float'),
var2=VarDef(name='y', type='float'),
var3=VarDef(name='width', type='float'),
var4=VarDef(name='height', type='float'),
var5=VarDef(name='minDepth', type='float'),
var6=VarDef(name='maxDepth', type='float')
)

Struct(name='VkViewportSwizzleNV_', enabled=False,
var1=VarDef(name='x', type='VkViewportCoordinateSwizzleNV'),
var2=VarDef(name='y', type='VkViewportCoordinateSwizzleNV'),
var3=VarDef(name='z', type='VkViewportCoordinateSwizzleNV'),
var4=VarDef(name='w', type='VkViewportCoordinateSwizzleNV')
)

Struct(name='VkViewportWScalingNV_', enabled=False,
var1=VarDef(name='xcoeff', type='float'),
var2=VarDef(name='ycoeff', type='float')
)

Struct(name='VkWaylandSurfaceCreateInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkWaylandSurfaceCreateFlagsKHR'),
var4=VarDef(name='display', type='struct wl_display*'),
var5=VarDef(name='surface', type='struct wl_surface*')
)

Struct(name='VkWin32KeyedMutexAcquireReleaseInfoKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='acquireCount', type='uint32_t'),
var4=VarDef(name='pAcquireSyncs', type='const VkDeviceMemory*', count='acquireCount'),
var5=VarDef(name='pAcquireKeys', type='const uint64_t*', count='acquireCount'),
var6=VarDef(name='pAcquireTimeouts', type='const uint32_t*', count='acquireCount'),
var7=VarDef(name='releaseCount', type='uint32_t'),
var8=VarDef(name='pReleaseSyncs', type='const VkDeviceMemory*', count='releaseCount'),
var9=VarDef(name='pReleaseKeys', type='const uint64_t*', count='releaseCount')
)

Struct(name='VkWin32KeyedMutexAcquireReleaseInfoNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='acquireCount', type='uint32_t'),
var4=VarDef(name='pAcquireSyncs', type='const VkDeviceMemory*', count='acquireCount'),
var5=VarDef(name='pAcquireKeys', type='const uint64_t*', count='acquireCount'),
var6=VarDef(name='pAcquireTimeoutMilliseconds', type='const uint32_t*', count='acquireCount'),
var7=VarDef(name='releaseCount', type='uint32_t'),
var8=VarDef(name='pReleaseSyncs', type='const VkDeviceMemory*', count='releaseCount'),
var9=VarDef(name='pReleaseKeys', type='const uint64_t*', count='releaseCount')
)

Struct(name='VkWin32SurfaceCreateInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkWin32SurfaceCreateFlagsKHR'),
var4=VarDef(name='hinstance', type='HINSTANCE', wrapType='CVkHINSTANCE'),
var5=VarDef(name='hwnd', type='HWND', wrapType='CVkHWND')
)

Struct(name='VkWriteDescriptorSet_', enabled=True, declareArray=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='dstSet', type='VkDescriptorSet'),
var4=VarDef(name='dstBinding', type='uint32_t'),
var5=VarDef(name='dstArrayElement', type='uint32_t'),
var6=VarDef(name='descriptorCount', type='uint32_t'),
var7=VarDef(name='descriptorType', type='VkDescriptorType'),
var8=VarDef(name='pImageInfo', type='const VkDescriptorImageInfo*', wrapType='CVkDescriptorImageInfoArray', wrapParams='isImageDescriptor(writedescriptorset->descriptorType) ? writedescriptorset->descriptorCount : 0, isImageDescriptor(writedescriptorset->descriptorType) ? writedescriptorset->pImageInfo : nullptr, writedescriptorset->descriptorType', count='descriptorCount', logCondition='isImageDescriptor(c.descriptorType)'),
var9=VarDef(name='pBufferInfo', type='const VkDescriptorBufferInfo*', wrapType='CVkDescriptorBufferInfoArray', wrapParams='isBufferDescriptor(writedescriptorset->descriptorType) ? writedescriptorset->descriptorCount : 0, isBufferDescriptor(writedescriptorset->descriptorType) ? writedescriptorset->pBufferInfo : nullptr', count='descriptorCount', logCondition='isBufferDescriptor(c.descriptorType)'),
var10=VarDef(name='pTexelBufferView', type='const VkBufferView*', wrapType='CVkBufferView::CSArray', wrapParams='isTexelBufferDescriptor(writedescriptorset->descriptorType) ? writedescriptorset->descriptorCount : 0, isTexelBufferDescriptor(writedescriptorset->descriptorType) ? writedescriptorset->pTexelBufferView : nullptr', count='descriptorCount', logCondition='isTexelBufferDescriptor(c.descriptorType)')
)

Struct(name='VkWriteDescriptorSetAccelerationStructureKHR_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='accelerationStructureCount', type='uint32_t'),
var4=VarDef(name='pAccelerationStructures', type='const VkAccelerationStructureKHR*')
)

Struct(name='VkWriteDescriptorSetAccelerationStructureNV_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='accelerationStructureCount', type='uint32_t'),
var4=VarDef(name='pAccelerationStructures', type='const VkAccelerationStructureNV*')
)

Struct(name='VkWriteDescriptorSetInlineUniformBlock_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='dataSize', type='uint32_t'),
var4=VarDef(name='pData', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='(size_t)(writedescriptorsetinlineuniformblock->dataSize), (const uint8_t *)writedescriptorsetinlineuniformblock->pData')
)

Struct(name='VkWriteDescriptorSetInlineUniformBlockEXT_', enabled=False,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='dataSize', type='uint32_t'),
var4=VarDef(name='pData', type='const void*', wrapType='Cuint8_t::CSArray', wrapParams='(size_t)(writedescriptorsetinlineuniformblockext->dataSize), (const uint8_t *)writedescriptorsetinlineuniformblockext->pData')
)

Struct(name='VkXYColorEXT_', enabled=False,
var1=VarDef(name='x', type='float'),
var2=VarDef(name='y', type='float')
)

Struct(name='VkXcbSurfaceCreateInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkXcbSurfaceCreateFlagsKHR'),
var4=VarDef(name='connection', type='xcb_connection_t*', wrapType='Cxcb_connection_t'),
var5=VarDef(name='window', type='xcb_window_t', wrapType='CVkWindow')
)

Struct(name='VkXlibSurfaceCreateInfoKHR_', enabled=True,
var1=VarDef(name='sType', type='VkStructureType'),
var2=VarDef(name='pNext', type='const void*'),
var3=VarDef(name='flags', type='VkXlibSurfaceCreateFlagsKHR'),
var4=VarDef(name='dpy', type='Display*', wrapType='CVkDisplay'),
var5=VarDef(name='window', type='Window', wrapType='CVkWindow')
)

Struct(name='VkNegotiateLayerInterface_', enabled=False,
var1=VarDef(name='sType', type='VkNegotiateLayerStructType'),
var2=VarDef(name='pNext', type='void*'),
var3=VarDef(name='loaderLayerInterfaceVersion', type='uint32_t'),
var4=VarDef(name='pfnGetInstanceProcAddr', type='PFN_vkGetInstanceProcAddr'),
var5=VarDef(name='pfnGetDeviceProcAddr', type='PFN_vkGetDeviceProcAddr'),
var6=VarDef(name='pfnGetPhysicalDeviceProcAddr', type='PFN_GetPhysicalDeviceProcAddr')
)

Function(name='vkAllocationFunction', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void*'),
arg1=ArgDef(name='pUserData', type='void*'),
arg2=ArgDef(name='size', type='size_t'),
arg3=ArgDef(name='alignment', type='size_t'),
arg4=ArgDef(name='allocationScope', type='VkSystemAllocationScope')
)

Function(name='vkReallocationFunction', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void*'),
arg1=ArgDef(name='pUserData', type='void*'),
arg2=ArgDef(name='pOriginal', type='void*'),
arg3=ArgDef(name='size', type='size_t'),
arg4=ArgDef(name='alignment', type='size_t'),
arg5=ArgDef(name='allocationScope', type='VkSystemAllocationScope')
)

Function(name='vkFreeFunction', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='pUserData', type='void*'),
arg2=ArgDef(name='pMemory', type='void*')
)

Function(name='vkInternalAllocationNotification', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='pUserData', type='void*'),
arg2=ArgDef(name='size', type='size_t'),
arg3=ArgDef(name='allocationType', type='VkInternalAllocationType'),
arg4=ArgDef(name='allocationScope', type='VkSystemAllocationScope')
)

Function(name='vkInternalFreeNotification', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='pUserData', type='void*'),
arg2=ArgDef(name='size', type='size_t'),
arg3=ArgDef(name='allocationType', type='VkInternalAllocationType'),
arg4=ArgDef(name='allocationScope', type='VkSystemAllocationScope')
)

Function(name='vkDebugReportCallbackEXT', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='VkBool32'),
arg1=ArgDef(name='flags', type='VkDebugReportFlagsEXT'),
arg2=ArgDef(name='objectType', type='VkDebugReportObjectTypeEXT'),
arg3=ArgDef(name='object', type='uint64_t'),
arg4=ArgDef(name='location', type='size_t'),
arg5=ArgDef(name='messageCode', type='int32_t'),
arg6=ArgDef(name='pLayerPrefix', type='const char*'),
arg7=ArgDef(name='pMessage', type='const char*'),
arg8=ArgDef(name='pUserData', type='void*')
)

Function(name='vkDebugUtilsMessengerCallbackEXT', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='VkBool32'),
arg1=ArgDef(name='messageSeverity', type='VkDebugUtilsMessageSeverityFlagBitsEXT'),
arg2=ArgDef(name='messageType', type='VkDebugUtilsMessageTypeFlagsEXT'),
arg3=ArgDef(name='pCallbackData', type='const VkDebugUtilsMessengerCallbackDataEXT*'),
arg4=ArgDef(name='pUserData', type='void*')
)

Function(name='vkDeviceMemoryReportCallbackEXT', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='pCallbackData', type='const VkDeviceMemoryReportCallbackDataEXT*'),
arg2=ArgDef(name='pUserData', type='void*')
)

Function(name='vkGetInstanceProcAddrLUNARG', enabled=False, type=Param, level=GlobalLevel,
retV=RetDef(type='PFN_vkVoidFunction'),
arg1=ArgDef(name='instance', type='VkInstance'),
arg2=ArgDef(name='pName', type='const char*')
)

Function(name='vkFaultCallbackFunction', enabled=False, type=Param, level=PrototypeLevel,
retV=RetDef(type='void'),
arg1=ArgDef(name='unrecordedFaults', type='VkBool32'),
arg2=ArgDef(name='faultCount', type='uint32_t'),
arg3=ArgDef(name='pFaults', type='const VkFaultData*')
)
