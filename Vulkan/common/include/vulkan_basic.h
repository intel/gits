/*
** Copyright 2015-2023 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
** This header is generated from the Khronos Vulkan XML API Registry.
**
*/

#define VK_VERSION_1_0 1
#include "vk_platform.h"
#include "vulkanTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VK_MAKE_API_VERSION(variant, major, minor, patch)                                          \
  ((((uint32_t)(variant)) << 29) | (((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) |     \
   ((uint32_t)(patch)))

// Vulkan 1.0 version number
#define VK_API_VERSION_1_0                                                                         \
  VK_MAKE_API_VERSION(0, 1, 0, 0) // Patch version should always be set to 0

// Version of this file
#define VK_HEADER_VERSION 248

// Complete version of this file
#define VK_HEADER_VERSION_COMPLETE VK_MAKE_API_VERSION(0, 1, 3, VK_HEADER_VERSION)

// DEPRECATED: This define is deprecated. VK_API_VERSION_MAJOR should be used instead.
#define VK_VERSION_MAJOR(version) ((uint32_t)(version) >> 22U)

// DEPRECATED: This define is deprecated. VK_API_VERSION_MINOR should be used instead.
#define VK_VERSION_MINOR(version) (((uint32_t)(version) >> 12U) & 0x3FFU)

// DEPRECATED: This define is deprecated. VK_API_VERSION_PATCH should be used instead.
#define VK_VERSION_PATCH(version) ((uint32_t)(version)&0xFFFU)

#define VK_API_VERSION_VARIANT(version) ((uint32_t)(version) >> 29U)
#define VK_API_VERSION_MAJOR(version)   (((uint32_t)(version) >> 22U) & 0x7FU)
#define VK_API_VERSION_MINOR(version)   (((uint32_t)(version) >> 12U) & 0x3FFU)
#define VK_API_VERSION_PATCH(version)   ((uint32_t)(version)&0xFFFU)

#define VK_NULL_HANDLE 0

#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if !defined(VK_DEFINE_NON_DISPATCHABLE_HANDLE)
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__)) ||        \
    defined(_M_X64) || defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||              \
    defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T* object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif
#endif

typedef uint32_t VkFlags;
typedef uint32_t VkBool32;
typedef uint64_t VkDeviceSize;
typedef uint64_t VkDeviceAddress;
typedef uint32_t VkSampleMask;
typedef uint64_t VkFlags64;

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSemaphore)
VK_DEFINE_HANDLE(VkCommandBuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkFence)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceMemory)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkEvent)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkQueryPool)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBufferView)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImageView)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderModule)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineCache)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineLayout)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkRenderPass)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipeline)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorSetLayout)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSampler)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorPool)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorSet)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkFramebuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCommandPool)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkAccelerationStructureKHR)
typedef VkAccelerationStructureKHR VkAccelerationStructureNV;
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeferredOperationKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectCommandsLayoutNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPrivateDataSlotEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkVideoSessionKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkVideoSessionParametersKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkMicromapEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkOpticalFlowSessionNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPrivateDataSlot)

#define VK_LOD_CLAMP_NONE                1000.0f
#define VK_REMAINING_MIP_LEVELS          (~0U)
#define VK_REMAINING_ARRAY_LAYERS        (~0U)
#define VK_WHOLE_SIZE                    (~0ULL)
#define VK_ATTACHMENT_UNUSED             (~0U)
#define VK_TRUE                          1
#define VK_FALSE                         0
#define VK_QUEUE_FAMILY_IGNORED          (~0U)
#define VK_SUBPASS_EXTERNAL              (~0U)
#define VK_MAX_PHYSICAL_DEVICE_NAME_SIZE 256
#define VK_UUID_SIZE                     16
#define VK_MAX_MEMORY_TYPES              32
#define VK_MAX_MEMORY_HEAPS              16
#define VK_MAX_EXTENSION_NAME_SIZE       256
#define VK_MAX_DESCRIPTION_SIZE          256
#define VK_MAX_DRIVER_NAME_SIZE          256
#define VK_MAX_DRIVER_INFO_SIZE          256

#define VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO_INTEL static_cast<VkStructureType>(808600001)

#define VK_VERSION_1_1 1
// Vulkan 1.1 version number
#define VK_API_VERSION_1_1                                                                         \
  VK_MAKE_API_VERSION(0, 1, 1, 0) // Patch version should always be set to 0

#define VK_VERSION_1_2 1
// Vulkan 1.2 version number
#define VK_API_VERSION_1_2                                                                         \
  VK_MAKE_API_VERSION(0, 1, 2, 0) // Patch version should always be set to 0

#define VK_VERSION_1_3 1
// Vulkan 1.3 version number
#define VK_API_VERSION_1_3                                                                         \
  VK_MAKE_API_VERSION(0, 1, 3, 0) // Patch version should always be set to 0

VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSamplerYcbcrConversion)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorUpdateTemplate)

#define VK_MAX_DEVICE_GROUP_SIZE 32
#define VK_LUID_SIZE             8
#define VK_QUEUE_FAMILY_EXTERNAL (~0U - 1)

typedef VkFlags VkInstanceCreateFlags;
typedef VkFlags VkFormatFeatureFlags;
typedef VkFlags VkFormatFeatureFlags2;
typedef VkFlags VkImageUsageFlags;
typedef VkFlags VkImageCreateFlags;
typedef VkFlags VkSampleCountFlags;
typedef VkFlags VkQueueFlags;
typedef VkFlags VkMemoryPropertyFlags;
typedef VkFlags VkMemoryHeapFlags;
typedef VkFlags VkDeviceCreateFlags;
typedef VkFlags VkDeviceQueueCreateFlags;
typedef VkFlags VkPipelineStageFlags;
typedef VkFlags VkMemoryMapFlags;
typedef VkFlags VkImageAspectFlags;
typedef VkFlags VkSparseImageFormatFlags;
typedef VkFlags VkSparseMemoryBindFlags;
typedef VkFlags VkFenceCreateFlags;
typedef VkFlags VkSemaphoreCreateFlags;
typedef VkFlags VkEventCreateFlags;
typedef VkFlags VkQueryPoolCreateFlags;
typedef VkFlags VkQueryPipelineStatisticFlags;
typedef VkFlags VkQueryResultFlags;
typedef VkFlags VkBufferCreateFlags;
typedef VkFlags VkBufferUsageFlags;
typedef VkFlags VkBufferViewCreateFlags;
typedef VkFlags VkImageViewCreateFlags;
typedef VkFlags VkShaderModuleCreateFlags;
typedef VkFlags VkPipelineCacheCreateFlags;
typedef VkFlags VkPipelineCreateFlags;
typedef VkFlags VkPipelineShaderStageCreateFlags;
typedef VkFlags VkPipelineVertexInputStateCreateFlags;
typedef VkFlags VkPipelineInputAssemblyStateCreateFlags;
typedef VkFlags VkPipelineTessellationStateCreateFlags;
typedef VkFlags VkPipelineViewportStateCreateFlags;
typedef VkFlags VkPipelineRasterizationStateCreateFlags;
typedef VkFlags VkCullModeFlags;
typedef VkFlags VkPipelineMultisampleStateCreateFlags;
typedef VkFlags VkPipelineDepthStencilStateCreateFlags;
typedef VkFlags VkPipelineColorBlendStateCreateFlags;
typedef VkFlags VkColorComponentFlags;
typedef VkFlags VkPipelineDynamicStateCreateFlags;
typedef VkFlags VkPipelineLayoutCreateFlags;
typedef VkFlags VkShaderStageFlags;
typedef VkFlags VkSamplerCreateFlags;
typedef VkFlags VkDescriptorSetLayoutCreateFlags;
typedef VkFlags VkDescriptorPoolCreateFlags;
typedef VkFlags VkDescriptorPoolResetFlags;
typedef VkFlags VkFramebufferCreateFlags;
typedef VkFlags VkRenderPassCreateFlags;
typedef VkFlags VkAttachmentDescriptionFlags;
typedef VkFlags VkSubpassDescriptionFlags;
typedef VkFlags VkAccessFlags;
typedef VkFlags VkDependencyFlags;
typedef VkFlags VkCommandPoolCreateFlags;
typedef VkFlags VkCommandPoolResetFlags;
typedef VkFlags VkCommandBufferUsageFlags;
typedef VkFlags VkQueryControlFlags;
typedef VkFlags VkCommandBufferResetFlags;
typedef VkFlags VkStencilFaceFlags;
typedef VkFlags VkSubgroupFeatureFlags;
typedef VkFlags VkPeerMemoryFeatureFlags;
typedef VkFlags VkMemoryAllocateFlags;
typedef VkFlags VkCommandPoolTrimFlags;
typedef VkFlags VkDescriptorUpdateTemplateCreateFlags;
typedef VkFlags VkExternalMemoryHandleTypeFlags;
typedef VkFlags VkExternalMemoryFeatureFlags;
typedef VkFlags VkExternalFenceHandleTypeFlags;
typedef VkFlags VkExternalFenceFeatureFlags;
typedef VkFlags VkFenceImportFlags;
typedef VkFlags VkSemaphoreImportFlags;
typedef VkFlags VkExternalSemaphoreHandleTypeFlags;
typedef VkFlags VkExternalSemaphoreFeatureFlags;
typedef VkFlags VkSurfaceTransformFlagsKHR;
typedef VkFlags VkCompositeAlphaFlagsKHR;
typedef VkFlags VkSwapchainCreateFlagsKHR;
typedef VkFlags VkDeviceGroupPresentModeFlagsKHR;
typedef VkFlags VkDisplayPlaneAlphaFlagsKHR;
typedef VkFlags VkDisplayModeCreateFlagsKHR;
typedef VkFlags VkDisplaySurfaceCreateFlagsKHR;
typedef VkFlags VkDebugReportFlagsEXT;
typedef VkFlags VkExternalMemoryHandleTypeFlagsNV;
typedef VkFlags VkExternalMemoryFeatureFlagsNV;
typedef VkFlags VkIndirectCommandsLayoutUsageFlagsNVX;
typedef VkFlags VkObjectEntryUsageFlagsNVX;
typedef VkFlags VkSurfaceCounterFlagsEXT;
typedef VkFlags VkPipelineViewportSwizzleStateCreateFlagsNV;
typedef VkFlags VkPipelineDiscardRectangleStateCreateFlagsEXT;
typedef VkFlags VkPipelineRasterizationConservativeStateCreateFlagsEXT;
typedef VkFlags VkDebugUtilsMessengerCallbackDataFlagsEXT;
typedef VkFlags VkDebugUtilsMessengerCreateFlagsEXT;
typedef VkFlags VkDebugUtilsMessageSeverityFlagsEXT;
typedef VkFlags VkDebugUtilsMessageTypeFlagsEXT;
typedef VkFlags VkPipelineCoverageToColorStateCreateFlagsNV;
typedef VkFlags VkPipelineCoverageModulationStateCreateFlagsNV;
typedef VkFlags VkValidationCacheCreateFlagsEXT;
typedef VkFlags VkDescriptorBindingFlagsEXT;
typedef VkFlags VkBuildAccelerationStructureFlagsNV;
typedef VkFlags VkConditionalRenderingFlagsEXT;
typedef VkFlags VkHeadlessSurfaceCreateFlagsEXT;
typedef VkFlags VkIOSSurfaceCreateFlagsMVK;
typedef VkFlags VkMacOSSurfaceCreateFlagsMVK;
typedef VkFlags VkMetalSurfaceCreateFlagsEXT;
typedef VkFlags VkPipelineCoverageReductionStateCreateFlagsNV;
typedef VkFlags VkPipelineCreationFeedbackFlagsEXT;
typedef VkFlags VkPipelineRasterizationDepthClipStateCreateFlagsEXT;
typedef VkFlags VkPipelineRasterizationStateStreamCreateFlagsEXT;
typedef VkFlags VkStreamDescriptorSurfaceCreateFlagsGGP;
typedef VkFlags VkGeometryFlagsNV;
typedef VkFlags VkBuildAccelerationStructureFlagsKHR;
typedef VkFlags VkGeometryFlagsKHR;
typedef VkFlags VkAcquireProfilingLockFlagsKHR;
typedef VkFlags VkDeviceDiagnosticsConfigFlagsNV;
typedef VkFlags VkIndirectStateFlagsNV;
typedef VkFlags VkPerformanceCounterDescriptionFlagsKHR;
typedef VkFlags VkIndirectCommandsLayoutUsageFlagsNV;
typedef VkFlags VkResolveModeFlags;
typedef VkFlags VkShaderCorePropertiesFlagsAMD;
typedef VkFlags VkToolPurposeFlagsEXT;
typedef VkFlags VkPipelineCompilerControlFlagsAMD;
typedef VkFlags VkPrivateDataSlotCreateFlagsEXT;
typedef VkFlags VkSemaphoreWaitFlags;
typedef VkFlags VkGeometryInstanceFlagsKHR;
typedef VkFlags VkPipelineRasterizationDepthClipStateCreateFlagsEXT;
typedef VkFlags VkSemaphoreWaitFlags;
typedef VkFlags VkAccelerationStructureCreateFlagsKHR;
typedef VkFlags VkDescriptorBindingFlags;
typedef VkFlags VkDeviceMemoryReportFlagsEXT;
typedef VkFlags VkSubmitFlagsKHR;
typedef VkFlags VkVideoSessionCreateFlagsKHR;
typedef VkFlags VkVideoBeginCodingFlagsKHR;
typedef VkFlags VkVideoEndCodingFlagsKHR;
typedef VkFlags VkVideoCodingQualityPresetFlagsKHR;
typedef VkFlags VkVideoCapabilitiesFlagsKHR;
typedef VkFlags VkVideoCodingControlFlagsKHR;
typedef VkFlags VkVideoDecodeH264CreateFlagsEXT;
typedef VkFlags VkVideoDecodeH265CreateFlagsEXT;
typedef VkFlags VkVideoEncodeH264CapabilitiesFlagsEXT;
typedef VkFlags VkVideoEncodeH264InputModeFlagsEXT;
typedef VkFlags VkVideoEncodeH264OutputModeFlagsEXT;
typedef VkFlags VkVideoEncodeH264CreateFlagsEXT;
typedef VkFlags VkVideoEncodeRateControlFlagsKHR;
typedef VkFlags VkVideoChromaSubsamplingFlagsKHR;
typedef VkFlags VkVideoComponentBitDepthFlagsKHR;
typedef VkFlags VkVideoCodecOperationFlagsKHR;
typedef VkFlags VkVideoDecodeFlagsKHR;
typedef VkFlags VkVideoEncodeFlagsKHR;
typedef VkFlags VkImageCompressionFlagsEXT;
typedef VkFlags VkImageCompressionFixedRateFlagsEXT;
typedef VkFlags64 VkAccessFlags2;
typedef VkFlags64 VkAccessFlags2KHR;
typedef VkFlags64 VkPipelineStageFlags2;
typedef VkFlags64 VkPipelineStageFlags2KHR;
typedef VkFlags VkRenderingFlags;
typedef VkFlags VkSubmitFlags;
typedef VkFlags VkGraphicsPipelineLibraryFlagsEXT;
typedef VkFlags VkAccelerationStructureMotionInstanceFlagsNV;
typedef VkFlags VkPipelineCreationFeedbackFlags;
typedef VkFlags VkPrivateDataSlotCreateFlags;
typedef VkFlags VkAccelerationStructureMotionInfoFlagsNV;
typedef VkFlags VkVideoEncodeH265CapabilityFlagsEXT;
typedef VkFlags VkVideoEncodeH265InputModeFlagsEXT;
typedef VkFlags VkVideoEncodeH265OutputModeFlagsEXT;
typedef VkFlags VkVideoEncodeH265CtbSizeFlagsEXT;
typedef VkFlags VkVideoEncodeH265TransformBlockSizeFlagsEXT;
typedef VkFlags VkVideoCapabilitiesFlags;
typedef VkFlags VkVideoDecodeCapabilityFlagsKHR;
typedef VkFlags VkVideoEncodeCapabilityFlagsKHR;
typedef VkFlags VkVideoEncodeRateControlModeFlagsKHR;
typedef VkFlags VkToolPurposeFlags;
typedef VkFlags VkVideoEncodeH264CapabilityFlagsEXT;
typedef VkFlags VkVideoEncodeRateControlModeFlagsKHR;
typedef VkFlags VkVideoEncodeUsageFlagsKHR;
typedef VkFlags VkVideoEncodeContentFlagsKHR;
typedef VkFlags VkVideoSessionParametersCreateFlagsKHR;
typedef VkFlags VkVideoCapabilityFlagsKHR;
typedef VkFlags VkVideoDecodeUsageFlagsKHR;
typedef VkFlags VkBuildMicromapFlagsEXT;
typedef VkFlags VkVideoEncodeFeedbackFlagsKHR;
typedef VkFlags VkRefreshObjectFlagsKHR;
typedef VkFlags VkShaderCreateFlagsEXT;
typedef VkFlags64 VkMemoryDecompressionMethodFlagsNV;
typedef VkFlags VkDeviceAddressBindingFlagsEXT;
typedef VkFlags VkDirectDriverLoadingFlagsLUNARG;
typedef VkFlags VkMemoryUnmapFlagsKHR;
typedef VkFlags VkOpticalFlowExecuteFlagsNV;
typedef VkFlags VkOpticalFlowUsageFlagsNV;
typedef VkFlags VkOpticalFlowGridSizeFlagsNV;
typedef VkFlags VkOpticalFlowSessionCreateFlagsNV;
typedef VkFlags VkPresentScalingFlagsEXT;
typedef VkFlags VkPresentGravityFlagsEXT;
typedef VkFlags VkMicromapCreateFlagsEXT;

#define VK_KHR_surface 1
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)

#define VK_KHR_SURFACE_SPEC_VERSION      25
#define VK_KHR_SURFACE_EXTENSION_NAME    "VK_KHR_surface"
#define VK_COLORSPACE_SRGB_NONLINEAR_KHR VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

#define VK_KHR_swapchain 1
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSwapchainKHR)

#define VK_KHR_SWAPCHAIN_SPEC_VERSION   70
#define VK_KHR_SWAPCHAIN_EXTENSION_NAME "VK_KHR_swapchain"

#define VK_KHR_display 1
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDisplayKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDisplayModeKHR)

#define VK_KHR_DISPLAY_SPEC_VERSION   21
#define VK_KHR_DISPLAY_EXTENSION_NAME "VK_KHR_display"

#define VK_KHR_display_swapchain                1
#define VK_KHR_DISPLAY_SWAPCHAIN_SPEC_VERSION   9
#define VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME "VK_KHR_display_swapchain"

#define VK_KHR_sampler_mirror_clamp_to_edge                1
#define VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_SPEC_VERSION   1
#define VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME "VK_KHR_sampler_mirror_clamp_to_edge"

#define VK_KHR_multiview                1
#define VK_KHR_MULTIVIEW_SPEC_VERSION   1
#define VK_KHR_MULTIVIEW_EXTENSION_NAME "VK_KHR_multiview"

//typedef VkRenderPassMultiviewCreateInfo VkRenderPassMultiviewCreateInfoKHR;
//
//typedef VkPhysicalDeviceMultiviewFeatures VkPhysicalDeviceMultiviewFeaturesKHR;
//
//typedef VkPhysicalDeviceMultiviewProperties VkPhysicalDeviceMultiviewPropertiesKHR;

#define VK_KHR_get_physical_device_properties2               1
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_SPEC_VERSION 1
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME                                     \
  "VK_KHR_get_physical_device_properties2"

//typedef VkPhysicalDeviceFeatures2 VkPhysicalDeviceFeatures2KHR;
//
//typedef VkPhysicalDeviceProperties2 VkPhysicalDeviceProperties2KHR;
//
//typedef VkFormatProperties2 VkFormatProperties2KHR;
//
//typedef VkImageFormatProperties2 VkImageFormatProperties2KHR;
//
//typedef VkPhysicalDeviceImageFormatInfo2 VkPhysicalDeviceImageFormatInfo2KHR;
//
//typedef VkQueueFamilyProperties2 VkQueueFamilyProperties2KHR;
//
//typedef VkPhysicalDeviceMemoryProperties2 VkPhysicalDeviceMemoryProperties2KHR;
//
//typedef VkSparseImageFormatProperties2 VkSparseImageFormatProperties2KHR;
//
//typedef VkPhysicalDeviceSparseImageFormatInfo2 VkPhysicalDeviceSparseImageFormatInfo2KHR;

#define VK_KHR_device_group                1
#define VK_KHR_DEVICE_GROUP_SPEC_VERSION   3
#define VK_KHR_DEVICE_GROUP_EXTENSION_NAME "VK_KHR_device_group"

//typedef VkPeerMemoryFeatureFlags VkPeerMemoryFeatureFlagsKHR;
//
//typedef VkPeerMemoryFeatureFlagBits VkPeerMemoryFeatureFlagBitsKHR;
//
//typedef VkMemoryAllocateFlags VkMemoryAllocateFlagsKHR;
//
//typedef VkMemoryAllocateFlagBits VkMemoryAllocateFlagBitsKHR;
//
//typedef VkMemoryAllocateFlagsInfo VkMemoryAllocateFlagsInfoKHR;
//
//typedef VkDeviceGroupRenderPassBeginInfo VkDeviceGroupRenderPassBeginInfoKHR;
//
//typedef VkDeviceGroupCommandBufferBeginInfo VkDeviceGroupCommandBufferBeginInfoKHR;
//
//typedef VkDeviceGroupSubmitInfo VkDeviceGroupSubmitInfoKHR;
//
//typedef VkDeviceGroupBindSparseInfo VkDeviceGroupBindSparseInfoKHR;
//
//typedef VkBindBufferMemoryDeviceGroupInfo VkBindBufferMemoryDeviceGroupInfoKHR;
//
//typedef VkBindImageMemoryDeviceGroupInfo VkBindImageMemoryDeviceGroupInfoKHR;

#define VK_KHR_shader_draw_parameters                1
#define VK_KHR_SHADER_DRAW_PARAMETERS_SPEC_VERSION   1
#define VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME "VK_KHR_shader_draw_parameters"

#define VK_KHR_maintenance1                1
#define VK_KHR_MAINTENANCE1_SPEC_VERSION   2
#define VK_KHR_MAINTENANCE1_EXTENSION_NAME "VK_KHR_maintenance1"

//typedef VkCommandPoolTrimFlags VkCommandPoolTrimFlagsKHR;

#define VK_KHR_device_group_creation                1
#define VK_KHR_DEVICE_GROUP_CREATION_SPEC_VERSION   1
#define VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME "VK_KHR_device_group_creation"
#define VK_MAX_DEVICE_GROUP_SIZE_KHR                VK_MAX_DEVICE_GROUP_SIZE

//typedef VkPhysicalDeviceGroupProperties VkPhysicalDeviceGroupPropertiesKHR;
//
//typedef VkDeviceGroupDeviceCreateInfo VkDeviceGroupDeviceCreateInfoKHR;

#define VK_KHR_external_memory_capabilities                1
#define VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME "VK_KHR_external_memory_capabilities"
#define VK_LUID_SIZE_KHR                                   VK_LUID_SIZE

//typedef VkExternalMemoryHandleTypeFlags VkExternalMemoryHandleTypeFlagsKHR;
//
//typedef VkExternalMemoryHandleTypeFlagBits VkExternalMemoryHandleTypeFlagBitsKHR;
//
//typedef VkExternalMemoryFeatureFlags VkExternalMemoryFeatureFlagsKHR;
//
//typedef VkExternalMemoryFeatureFlagBits VkExternalMemoryFeatureFlagBitsKHR;
//
//typedef VkExternalMemoryProperties VkExternalMemoryPropertiesKHR;
//
//typedef VkPhysicalDeviceExternalImageFormatInfo VkPhysicalDeviceExternalImageFormatInfoKHR;
//
//typedef VkExternalImageFormatProperties VkExternalImageFormatPropertiesKHR;
//
//typedef VkPhysicalDeviceExternalBufferInfo VkPhysicalDeviceExternalBufferInfoKHR;
//
//typedef VkExternalBufferProperties VkExternalBufferPropertiesKHR;
//
//typedef VkPhysicalDeviceIDProperties VkPhysicalDeviceIDPropertiesKHR;

#define VK_KHR_external_memory                1
#define VK_KHR_EXTERNAL_MEMORY_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME "VK_KHR_external_memory"
#define VK_QUEUE_FAMILY_EXTERNAL_KHR          VK_QUEUE_FAMILY_EXTERNAL

//typedef VkExternalMemoryImageCreateInfo VkExternalMemoryImageCreateInfoKHR;
//
//typedef VkExternalMemoryBufferCreateInfo VkExternalMemoryBufferCreateInfoKHR;
//
//typedef VkExportMemoryAllocateInfo VkExportMemoryAllocateInfoKHR;

#define VK_KHR_external_memory_fd                1
#define VK_KHR_EXTERNAL_MEMORY_FD_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME "VK_KHR_external_memory_fd"

#define VK_KHR_external_semaphore_capabilities              1
#define VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_SPEC_VERSION 1
#define VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME                                      \
  "VK_KHR_external_semaphore_capabilities"

//typedef VkExternalSemaphoreHandleTypeFlags VkExternalSemaphoreHandleTypeFlagsKHR;
//
//typedef VkExternalSemaphoreHandleTypeFlagBits VkExternalSemaphoreHandleTypeFlagBitsKHR;
//
//typedef VkExternalSemaphoreFeatureFlags VkExternalSemaphoreFeatureFlagsKHR;
//
//typedef VkExternalSemaphoreFeatureFlagBits VkExternalSemaphoreFeatureFlagBitsKHR;
//
//typedef VkPhysicalDeviceExternalSemaphoreInfo VkPhysicalDeviceExternalSemaphoreInfoKHR;
//
//typedef VkExternalSemaphoreProperties VkExternalSemaphorePropertiesKHR;

#define VK_KHR_external_semaphore                1
#define VK_KHR_EXTERNAL_SEMAPHORE_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME "VK_KHR_external_semaphore"

//typedef VkSemaphoreImportFlags VkSemaphoreImportFlagsKHR;
//
//typedef VkSemaphoreImportFlagBits VkSemaphoreImportFlagBitsKHR;
//
//typedef VkExportSemaphoreCreateInfo VkExportSemaphoreCreateInfoKHR;

#define VK_KHR_external_semaphore_fd                1
#define VK_KHR_EXTERNAL_SEMAPHORE_FD_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME "VK_KHR_external_semaphore_fd"

#define VK_KHR_push_descriptor                1
#define VK_KHR_PUSH_DESCRIPTOR_SPEC_VERSION   2
#define VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME "VK_KHR_push_descriptor"

#define VK_KHR_16bit_storage                1
#define VK_KHR_16BIT_STORAGE_SPEC_VERSION   1
#define VK_KHR_16BIT_STORAGE_EXTENSION_NAME "VK_KHR_16bit_storage"

//typedef VkPhysicalDevice16BitStorageFeatures VkPhysicalDevice16BitStorageFeaturesKHR;

#define VK_KHR_incremental_present                1
#define VK_KHR_INCREMENTAL_PRESENT_SPEC_VERSION   1
#define VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME "VK_KHR_incremental_present"

#define VK_KHR_descriptor_update_template                1
#define VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_SPEC_VERSION   1
#define VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME "VK_KHR_descriptor_update_template"

//typedef VkDescriptorUpdateTemplate VkDescriptorUpdateTemplateKHR;
//
//typedef VkDescriptorUpdateTemplateType VkDescriptorUpdateTemplateTypeKHR;
//
//typedef VkDescriptorUpdateTemplateCreateFlags VkDescriptorUpdateTemplateCreateFlagsKHR;
//
//typedef VkDescriptorUpdateTemplateEntry VkDescriptorUpdateTemplateEntryKHR;
//
//typedef VkDescriptorUpdateTemplateCreateInfo VkDescriptorUpdateTemplateCreateInfoKHR;

#define VK_KHR_shared_presentable_image                1
#define VK_KHR_SHARED_PRESENTABLE_IMAGE_SPEC_VERSION   1
#define VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME "VK_KHR_shared_presentable_image"

#define VK_KHR_external_fence_capabilities                1
#define VK_KHR_EXTERNAL_FENCE_CAPABILITIES_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME "VK_KHR_external_fence_capabilities"

//typedef VkExternalFenceHandleTypeFlags VkExternalFenceHandleTypeFlagsKHR;
//
//typedef VkExternalFenceHandleTypeFlagBits VkExternalFenceHandleTypeFlagBitsKHR;
//
//typedef VkExternalFenceFeatureFlags VkExternalFenceFeatureFlagsKHR;
//
//typedef VkExternalFenceFeatureFlagBits VkExternalFenceFeatureFlagBitsKHR;
//
//typedef VkPhysicalDeviceExternalFenceInfo VkPhysicalDeviceExternalFenceInfoKHR;
//
//typedef VkExternalFenceProperties VkExternalFencePropertiesKHR;

#define VK_KHR_external_fence                1
#define VK_KHR_EXTERNAL_FENCE_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME "VK_KHR_external_fence"

//typedef VkFenceImportFlags VkFenceImportFlagsKHR;
//
//typedef VkFenceImportFlagBits VkFenceImportFlagBitsKHR;
//
//typedef VkExportFenceCreateInfo VkExportFenceCreateInfoKHR;

#define VK_KHR_external_fence_fd                1
#define VK_KHR_EXTERNAL_FENCE_FD_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME "VK_KHR_external_fence_fd"

#define VK_KHR_maintenance2                1
#define VK_KHR_MAINTENANCE2_SPEC_VERSION   1
#define VK_KHR_MAINTENANCE2_EXTENSION_NAME "VK_KHR_maintenance2"

//typedef VkPointClippingBehavior VkPointClippingBehaviorKHR;
//
//typedef VkTessellationDomainOrigin VkTessellationDomainOriginKHR;
//
//typedef VkPhysicalDevicePointClippingProperties VkPhysicalDevicePointClippingPropertiesKHR;
//
//typedef VkRenderPassInputAttachmentAspectCreateInfo VkRenderPassInputAttachmentAspectCreateInfoKHR;
//
//typedef VkInputAttachmentAspectReference VkInputAttachmentAspectReferenceKHR;
//
//typedef VkImageViewUsageCreateInfo VkImageViewUsageCreateInfoKHR;
//
//typedef VkPipelineTessellationDomainOriginStateCreateInfo VkPipelineTessellationDomainOriginStateCreateInfoKHR;

#define VK_KHR_get_surface_capabilities2                 1
#define VK_KHR_GET_SURFACE_CAPABILITIES_2_SPEC_VERSION   1
#define VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME "VK_KHR_get_surface_capabilities2"

#define VK_KHR_variable_pointers                1
#define VK_KHR_VARIABLE_POINTERS_SPEC_VERSION   1
#define VK_KHR_VARIABLE_POINTERS_EXTENSION_NAME "VK_KHR_variable_pointers"

//typedef VkPhysicalDeviceVariablePointerFeatures VkPhysicalDeviceVariablePointerFeaturesKHR;

#define VK_KHR_dedicated_allocation                1
#define VK_KHR_DEDICATED_ALLOCATION_SPEC_VERSION   3
#define VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME "VK_KHR_dedicated_allocation"

//typedef VkMemoryDedicatedRequirements VkMemoryDedicatedRequirementsKHR;
//
//typedef VkMemoryDedicatedAllocateInfo VkMemoryDedicatedAllocateInfoKHR;

#define VK_KHR_storage_buffer_storage_class                1
#define VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_SPEC_VERSION   1
#define VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME "VK_KHR_storage_buffer_storage_class"

#define VK_KHR_relaxed_block_layout                1
#define VK_KHR_RELAXED_BLOCK_LAYOUT_SPEC_VERSION   1
#define VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME "VK_KHR_relaxed_block_layout"

#define VK_KHR_get_memory_requirements2                 1
#define VK_KHR_GET_MEMORY_REQUIREMENTS_2_SPEC_VERSION   1
#define VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME "VK_KHR_get_memory_requirements2"

//typedef VkBufferMemoryRequirementsInfo2 VkBufferMemoryRequirementsInfo2KHR;
//
//typedef VkImageMemoryRequirementsInfo2 VkImageMemoryRequirementsInfo2KHR;
//
//typedef VkImageSparseMemoryRequirementsInfo2 VkImageSparseMemoryRequirementsInfo2KHR;
//
//typedef VkMemoryRequirements2 VkMemoryRequirements2KHR;
//
//typedef VkSparseImageMemoryRequirements2 VkSparseImageMemoryRequirements2KHR;

#define VK_KHR_image_format_list                1
#define VK_KHR_IMAGE_FORMAT_LIST_SPEC_VERSION   1
#define VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME "VK_KHR_image_format_list"

#define VK_KHR_sampler_ycbcr_conversion                1
#define VK_KHR_SAMPLER_YCBCR_CONVERSION_SPEC_VERSION   1
#define VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME "VK_KHR_sampler_ycbcr_conversion"

//typedef VkSamplerYcbcrConversion VkSamplerYcbcrConversionKHR;
//
//typedef VkSamplerYcbcrModelConversion VkSamplerYcbcrModelConversionKHR;
//
//typedef VkSamplerYcbcrRange VkSamplerYcbcrRangeKHR;
//
//typedef VkChromaLocation VkChromaLocationKHR;
//
//typedef VkSamplerYcbcrConversionCreateInfo VkSamplerYcbcrConversionCreateInfoKHR;
//
//typedef VkSamplerYcbcrConversionInfo VkSamplerYcbcrConversionInfoKHR;
//
//typedef VkBindImagePlaneMemoryInfo VkBindImagePlaneMemoryInfoKHR;
//
//typedef VkImagePlaneMemoryRequirementsInfo VkImagePlaneMemoryRequirementsInfoKHR;
//
//typedef VkPhysicalDeviceSamplerYcbcrConversionFeatures VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR;
//
//typedef VkSamplerYcbcrConversionImageFormatProperties VkSamplerYcbcrConversionImageFormatPropertiesKHR;

#define VK_KHR_bind_memory2                 1
#define VK_KHR_BIND_MEMORY_2_SPEC_VERSION   1
#define VK_KHR_BIND_MEMORY_2_EXTENSION_NAME "VK_KHR_bind_memory2"

//typedef VkBindBufferMemoryInfo VkBindBufferMemoryInfoKHR;
//
//typedef VkBindImageMemoryInfo VkBindImageMemoryInfoKHR;

#define VK_KHR_maintenance3                1
#define VK_KHR_MAINTENANCE3_SPEC_VERSION   1
#define VK_KHR_MAINTENANCE3_EXTENSION_NAME "VK_KHR_maintenance3"

//typedef VkPhysicalDeviceMaintenance3Properties VkPhysicalDeviceMaintenance3PropertiesKHR;
//
//typedef VkDescriptorSetLayoutSupport VkDescriptorSetLayoutSupportKHR;

#define VK_EXT_debug_report                1
#define VK_EXT_DEBUG_REPORT_SPEC_VERSION   9
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"

#define VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT                                             \
  VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT
#define VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT                                               \
  VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDebugReportCallbackEXT)

#define VK_NV_glsl_shader                1
#define VK_NV_GLSL_SHADER_SPEC_VERSION   1
#define VK_NV_GLSL_SHADER_EXTENSION_NAME "VK_NV_glsl_shader"

#define VK_EXT_depth_range_unrestricted                1
#define VK_EXT_DEPTH_RANGE_UNRESTRICTED_SPEC_VERSION   1
#define VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME "VK_EXT_depth_range_unrestricted"

#define VK_IMG_filter_cubic                1
#define VK_IMG_FILTER_CUBIC_SPEC_VERSION   1
#define VK_IMG_FILTER_CUBIC_EXTENSION_NAME "VK_IMG_filter_cubic"

#define VK_AMD_rasterization_order                1
#define VK_AMD_RASTERIZATION_ORDER_SPEC_VERSION   1
#define VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME "VK_AMD_rasterization_order"

#define VK_AMD_shader_trinary_minmax                1
#define VK_AMD_SHADER_TRINARY_MINMAX_SPEC_VERSION   1
#define VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME "VK_AMD_shader_trinary_minmax"

#define VK_AMD_shader_explicit_vertex_parameter              1
#define VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_SPEC_VERSION 1
#define VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_EXTENSION_NAME                                     \
  "VK_AMD_shader_explicit_vertex_parameter"

#define VK_EXT_debug_marker                1
#define VK_EXT_DEBUG_MARKER_SPEC_VERSION   4
#define VK_EXT_DEBUG_MARKER_EXTENSION_NAME "VK_EXT_debug_marker"

#define VK_AMD_gcn_shader                1
#define VK_AMD_GCN_SHADER_SPEC_VERSION   1
#define VK_AMD_GCN_SHADER_EXTENSION_NAME "VK_AMD_gcn_shader"

#define VK_NV_dedicated_allocation                1
#define VK_NV_DEDICATED_ALLOCATION_SPEC_VERSION   1
#define VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME "VK_NV_dedicated_allocation"

#define VK_AMD_draw_indirect_count                1
#define VK_AMD_DRAW_INDIRECT_COUNT_SPEC_VERSION   1
#define VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME "VK_AMD_draw_indirect_count"

#define VK_AMD_negative_viewport_height                1
#define VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_SPEC_VERSION   1
#define VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME "VK_AMD_negative_viewport_height"

#define VK_AMD_gpu_shader_half_float                1
#define VK_AMD_GPU_SHADER_HALF_FLOAT_SPEC_VERSION   1
#define VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME "VK_AMD_gpu_shader_half_float"

#define VK_AMD_shader_ballot                1
#define VK_AMD_SHADER_BALLOT_SPEC_VERSION   1
#define VK_AMD_SHADER_BALLOT_EXTENSION_NAME "VK_AMD_shader_ballot"

#define VK_AMD_texture_gather_bias_lod                1
#define VK_AMD_TEXTURE_GATHER_BIAS_LOD_SPEC_VERSION   1
#define VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME "VK_AMD_texture_gather_bias_lod"

#define VK_AMD_shader_info                1
#define VK_AMD_SHADER_INFO_SPEC_VERSION   1
#define VK_AMD_SHADER_INFO_EXTENSION_NAME "VK_AMD_shader_info"

#define VK_AMD_shader_image_load_store_lod                1
#define VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_SPEC_VERSION   1
#define VK_AMD_SHADER_IMAGE_LOAD_STORE_LOD_EXTENSION_NAME "VK_AMD_shader_image_load_store_lod"

#define VK_IMG_format_pvrtc                1
#define VK_IMG_FORMAT_PVRTC_SPEC_VERSION   1
#define VK_IMG_FORMAT_PVRTC_EXTENSION_NAME "VK_IMG_format_pvrtc"

#define VK_NV_external_memory_capabilities                1
#define VK_NV_EXTERNAL_MEMORY_CAPABILITIES_SPEC_VERSION   1
#define VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME "VK_NV_external_memory_capabilities"

#define VK_NV_external_memory                1
#define VK_NV_EXTERNAL_MEMORY_SPEC_VERSION   1
#define VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME "VK_NV_external_memory"

#define VK_EXT_validation_flags                1
#define VK_EXT_VALIDATION_FLAGS_SPEC_VERSION   1
#define VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME "VK_EXT_validation_flags"

#define VK_EXT_shader_subgroup_ballot                1
#define VK_EXT_SHADER_SUBGROUP_BALLOT_SPEC_VERSION   1
#define VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME "VK_EXT_shader_subgroup_ballot"

#define VK_EXT_shader_subgroup_vote                1
#define VK_EXT_SHADER_SUBGROUP_VOTE_SPEC_VERSION   1
#define VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME "VK_EXT_shader_subgroup_vote"

#define VK_NVX_device_generated_commands                1
#define VK_NVX_DEVICE_GENERATED_COMMANDS_SPEC_VERSION   3
#define VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME "VK_NVX_device_generated_commands"
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkObjectTableNVX)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectCommandsLayoutNVX)

#define VK_NV_clip_space_w_scaling                1
#define VK_NV_CLIP_SPACE_W_SCALING_SPEC_VERSION   1
#define VK_NV_CLIP_SPACE_W_SCALING_EXTENSION_NAME "VK_NV_clip_space_w_scaling"

#define VK_EXT_direct_mode_display                1
#define VK_EXT_DIRECT_MODE_DISPLAY_SPEC_VERSION   1
#define VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME "VK_EXT_direct_mode_display"

#define VK_EXT_display_surface_counter                1
#define VK_EXT_DISPLAY_SURFACE_COUNTER_SPEC_VERSION   1
#define VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME "VK_EXT_display_surface_counter"
#define VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES2_EXT   VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT

#define VK_EXT_display_control                1
#define VK_EXT_DISPLAY_CONTROL_SPEC_VERSION   1
#define VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME "VK_EXT_display_control"

#define VK_GOOGLE_display_timing                1
#define VK_GOOGLE_DISPLAY_TIMING_SPEC_VERSION   1
#define VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME "VK_GOOGLE_display_timing"

#define VK_NV_sample_mask_override_coverage                1
#define VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_SPEC_VERSION   1
#define VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_EXTENSION_NAME "VK_NV_sample_mask_override_coverage"

#define VK_NV_geometry_shader_passthrough                1
#define VK_NV_GEOMETRY_SHADER_PASSTHROUGH_SPEC_VERSION   1
#define VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME "VK_NV_geometry_shader_passthrough"

#define VK_NV_viewport_array2                1
#define VK_NV_VIEWPORT_ARRAY2_SPEC_VERSION   1
#define VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME "VK_NV_viewport_array2"

#define VK_NVX_multiview_per_view_attributes                1
#define VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_SPEC_VERSION   1
#define VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_EXTENSION_NAME "VK_NVX_multiview_per_view_attributes"

#define VK_NV_viewport_swizzle                1
#define VK_NV_VIEWPORT_SWIZZLE_SPEC_VERSION   1
#define VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME "VK_NV_viewport_swizzle"

#define VK_NV_external_memory_rdma 1
typedef void* VkRemoteAddressNV;
#define VK_NV_EXTERNAL_MEMORY_RDMA_SPEC_VERSION   1
#define VK_NV_EXTERNAL_MEMORY_RDMA_EXTENSION_NAME "VK_NV_external_memory_rdma"

#define VK_EXT_discard_rectangles                1
#define VK_EXT_DISCARD_RECTANGLES_SPEC_VERSION   1
#define VK_EXT_DISCARD_RECTANGLES_EXTENSION_NAME "VK_EXT_discard_rectangles"

#define VK_EXT_conservative_rasterization                1
#define VK_EXT_CONSERVATIVE_RASTERIZATION_SPEC_VERSION   1
#define VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME "VK_EXT_conservative_rasterization"

#define VK_EXT_swapchain_colorspace                 1
#define VK_EXT_SWAPCHAIN_COLOR_SPACE_SPEC_VERSION   3
#define VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME "VK_EXT_swapchain_colorspace"

#define VK_EXT_hdr_metadata                1
#define VK_EXT_HDR_METADATA_SPEC_VERSION   1
#define VK_EXT_HDR_METADATA_EXTENSION_NAME "VK_EXT_hdr_metadata"

#define VK_EXT_external_memory_dma_buf                1
#define VK_EXT_EXTERNAL_MEMORY_DMA_BUF_SPEC_VERSION   1
#define VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME "VK_EXT_external_memory_dma_buf"

#define VK_EXT_queue_family_foreign                1
#define VK_EXT_QUEUE_FAMILY_FOREIGN_SPEC_VERSION   1
#define VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME "VK_EXT_queue_family_foreign"
#define VK_QUEUE_FAMILY_FOREIGN_EXT                (~0U - 2)

#define VK_EXT_debug_utils                1
#define VK_EXT_DEBUG_UTILS_SPEC_VERSION   1
#define VK_EXT_DEBUG_UTILS_EXTENSION_NAME "VK_EXT_debug_utils"
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDebugUtilsMessengerEXT)

typedef VkFlags VkDebugUtilsMessengerCallbackDataFlagsEXT;

typedef VkFlags VkDebugUtilsMessengerCreateFlagsEXT;

#define VK_EXT_sampler_filter_minmax                1
#define VK_EXT_SAMPLER_FILTER_MINMAX_SPEC_VERSION   1
#define VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME "VK_EXT_sampler_filter_minmax"

#define VK_AMD_gpu_shader_int16                1
#define VK_AMD_GPU_SHADER_INT16_SPEC_VERSION   1
#define VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME "VK_AMD_gpu_shader_int16"

#define VK_AMD_mixed_attachment_samples                1
#define VK_AMD_MIXED_ATTACHMENT_SAMPLES_SPEC_VERSION   1
#define VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME "VK_AMD_mixed_attachment_samples"

#define VK_AMD_shader_fragment_mask                1
#define VK_AMD_SHADER_FRAGMENT_MASK_SPEC_VERSION   1
#define VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME "VK_AMD_shader_fragment_mask"

#define VK_EXT_shader_stencil_export                1
#define VK_EXT_SHADER_STENCIL_EXPORT_SPEC_VERSION   1
#define VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME "VK_EXT_shader_stencil_export"

#define VK_EXT_sample_locations                1
#define VK_EXT_SAMPLE_LOCATIONS_SPEC_VERSION   1
#define VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME "VK_EXT_sample_locations"

#define VK_EXT_blend_operation_advanced                1
#define VK_EXT_BLEND_OPERATION_ADVANCED_SPEC_VERSION   2
#define VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME "VK_EXT_blend_operation_advanced"

#define VK_NV_fragment_coverage_to_color                1
#define VK_NV_FRAGMENT_COVERAGE_TO_COLOR_SPEC_VERSION   1
#define VK_NV_FRAGMENT_COVERAGE_TO_COLOR_EXTENSION_NAME "VK_NV_fragment_coverage_to_color"

typedef VkFlags VkPipelineCoverageToColorStateCreateFlagsNV;

#define VK_NV_framebuffer_mixed_samples                1
#define VK_NV_FRAMEBUFFER_MIXED_SAMPLES_SPEC_VERSION   1
#define VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME "VK_NV_framebuffer_mixed_samples"

#define VK_NV_fill_rectangle                1
#define VK_NV_FILL_RECTANGLE_SPEC_VERSION   1
#define VK_NV_FILL_RECTANGLE_EXTENSION_NAME "VK_NV_fill_rectangle"

#define VK_EXT_post_depth_coverage                1
#define VK_EXT_POST_DEPTH_COVERAGE_SPEC_VERSION   1
#define VK_EXT_POST_DEPTH_COVERAGE_EXTENSION_NAME "VK_EXT_post_depth_coverage"

#define VK_EXT_validation_cache                1
#define VK_EXT_VALIDATION_CACHE_SPEC_VERSION   1
#define VK_EXT_VALIDATION_CACHE_EXTENSION_NAME "VK_EXT_validation_cache"
#define VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT                                           \
  VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkValidationCacheEXT)

#define VK_EXT_descriptor_indexing                1
#define VK_EXT_DESCRIPTOR_INDEXING_SPEC_VERSION   2
#define VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME "VK_EXT_descriptor_indexing"

#define VK_EXT_shader_viewport_index_layer                1
#define VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_SPEC_VERSION   1
#define VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME "VK_EXT_shader_viewport_index_layer"

#define VK_EXT_global_priority                1
#define VK_EXT_GLOBAL_PRIORITY_SPEC_VERSION   2
#define VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME "VK_EXT_global_priority"

#define VK_EXT_external_memory_host                1
#define VK_EXT_EXTERNAL_MEMORY_HOST_SPEC_VERSION   1
#define VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME "VK_EXT_external_memory_host"

#define VK_AMD_buffer_marker                1
#define VK_AMD_BUFFER_MARKER_SPEC_VERSION   1
#define VK_AMD_BUFFER_MARKER_EXTENSION_NAME "VK_AMD_buffer_marker"

#define VK_AMD_shader_core_properties                1
#define VK_AMD_SHADER_CORE_PROPERTIES_SPEC_VERSION   1
#define VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME "VK_AMD_shader_core_properties"

#define VK_EXT_vertex_attribute_divisor                1
#define VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_SPEC_VERSION   1
#define VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME "VK_EXT_vertex_attribute_divisor"

#define VK_NV_shader_subgroup_partitioned                1
#define VK_NV_SHADER_SUBGROUP_PARTITIONED_SPEC_VERSION   1
#define VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME "VK_NV_shader_subgroup_partitioned"

#define VK_KHR_buffer_device_address                1
#define VK_KHR_BUFFER_DEVICE_ADDRESS_SPEC_VERSION   1
#define VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME "VK_KHR_buffer_device_address"

#define VK_EXT_buffer_device_address                1
#define VK_EXT_BUFFER_DEVICE_ADDRESS_SPEC_VERSION   2
#define VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME "VK_EXT_buffer_device_address"

#define VK_KHR_synchronization2                 1
#define VK_KHR_SYNCHRONIZATION_2_SPEC_VERSION   1
#define VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME "VK_KHR_synchronization2"

#ifdef GITS_PLATFORM_X11
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_KHR_xlib_surface                1
#define VK_KHR_XLIB_SURFACE_SPEC_VERSION   6
#define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"

#define VK_EXT_acquire_xlib_display                1
#define VK_EXT_ACQUIRE_XLIB_DISPLAY_SPEC_VERSION   1
#define VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME "VK_EXT_acquire_xlib_display"
#endif

typedef VkFlags VkXlibSurfaceCreateFlagsKHR;

#ifdef GITS_PLATFORM_X11
#define VK_USE_PLATFORM_XCB_KHR
#define VK_KHR_xcb_surface                1
#define VK_KHR_XCB_SURFACE_SPEC_VERSION   6
#define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#endif

typedef VkFlags VkXcbSurfaceCreateFlagsKHR;

#ifdef GITS_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_KHR_win32_surface                1
#define VK_KHR_WIN32_SURFACE_SPEC_VERSION   6
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
#endif

typedef VkFlags VkWin32SurfaceCreateFlagsKHR;

#ifdef GITS_PLATFORM_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_KHR_wayland_surface                1
#define VK_KHR_WAYLAND_SURFACE_SPEC_VERSION   6
#define VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME "VK_KHR_wayland_surface"
#endif

typedef VkFlags VkWaylandSurfaceCreateFlagsKHR;

/****************************************************************************\
DEFINE: VkPerformanceConfigurationINTEL
*****************************************************************************/
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPerformanceConfigurationINTEL)

typedef VkFlags VkObjectMemoryUsageQueryFlags;

typedef void(VKAPI_PTR* PFN_vkVoidFunction)(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
