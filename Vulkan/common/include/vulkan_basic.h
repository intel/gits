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

typedef uint32_t VkBool32;
typedef uint64_t VkDeviceSize;
typedef uint64_t VkDeviceAddress;
typedef uint32_t VkSampleMask;

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
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkAccelerationStructureNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeferredOperationKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectCommandsLayoutNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPrivateDataSlotEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkVideoSessionKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkVideoSessionParametersKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkMicromapEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkOpticalFlowSessionNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPrivateDataSlot)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectCommandsLayoutEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectExecutionSetEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCudaFunctionNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCudaModuleNV)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineBinaryKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkObjectTableNVX)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkIndirectCommandsLayoutNVX)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSwapchainKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDisplayKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDisplayModeKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSamplerYcbcrConversion)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorUpdateTemplate)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDebugReportCallbackEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDebugUtilsMessengerEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkValidationCacheEXT)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPerformanceConfigurationINTEL)

#define VK_MAX_PHYSICAL_DEVICE_NAME_SIZE          256
#define VK_UUID_SIZE                              16
#define VK_LUID_SIZE                              8
#define VK_MAX_EXTENSION_NAME_SIZE                256
#define VK_MAX_DESCRIPTION_SIZE                   256
#define VK_MAX_MEMORY_TYPES                       32
#define VK_MAX_MEMORY_HEAPS                       16
#define VK_LOD_CLAMP_NONE                         1000.0f
#define VK_REMAINING_MIP_LEVELS                   (~0U)
#define VK_REMAINING_ARRAY_LAYERS                 (~0U)
#define VK_REMAINING_3D_SLICES_EXT                (~0U)
#define VK_WHOLE_SIZE                             (~0ULL)
#define VK_ATTACHMENT_UNUSED                      (~0U)
#define VK_TRUE                                   1
#define VK_FALSE                                  0
#define VK_QUEUE_FAMILY_IGNORED                   (~0U)
#define VK_QUEUE_FAMILY_EXTERNAL                  (~1U)
#define VK_QUEUE_FAMILY_FOREIGN_EXT               (~2U)
#define VK_SUBPASS_EXTERNAL                       (~0U)
#define VK_MAX_DEVICE_GROUP_SIZE                  32
#define VK_MAX_DRIVER_NAME_SIZE                   256
#define VK_MAX_DRIVER_INFO_SIZE                   256
#define VK_SHADER_UNUSED_KHR                      (~0U)
#define VK_MAX_GLOBAL_PRIORITY_SIZE_KHR           16
#define VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT  32
#define VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR       32
#define VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR 7

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

#define VK_KHR_surface 1

#define VK_KHR_SURFACE_SPEC_VERSION      25
#define VK_KHR_SURFACE_EXTENSION_NAME    "VK_KHR_surface"
#define VK_COLORSPACE_SRGB_NONLINEAR_KHR VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

#define VK_KHR_swapchain 1

#define VK_KHR_SWAPCHAIN_SPEC_VERSION   70
#define VK_KHR_SWAPCHAIN_EXTENSION_NAME "VK_KHR_swapchain"

#define VK_KHR_display 1

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

#define VK_KHR_get_physical_device_properties2               1
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_SPEC_VERSION 1
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME                                     \
  "VK_KHR_get_physical_device_properties2"

#define VK_KHR_device_group                1
#define VK_KHR_DEVICE_GROUP_SPEC_VERSION   3
#define VK_KHR_DEVICE_GROUP_EXTENSION_NAME "VK_KHR_device_group"

#define VK_KHR_shader_draw_parameters                1
#define VK_KHR_SHADER_DRAW_PARAMETERS_SPEC_VERSION   1
#define VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME "VK_KHR_shader_draw_parameters"

#define VK_KHR_maintenance1                1
#define VK_KHR_MAINTENANCE1_SPEC_VERSION   2
#define VK_KHR_MAINTENANCE1_EXTENSION_NAME "VK_KHR_maintenance1"

#define VK_KHR_device_group_creation                1
#define VK_KHR_DEVICE_GROUP_CREATION_SPEC_VERSION   1
#define VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME "VK_KHR_device_group_creation"
#define VK_MAX_DEVICE_GROUP_SIZE_KHR                VK_MAX_DEVICE_GROUP_SIZE

#define VK_KHR_external_memory_capabilities                1
#define VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME "VK_KHR_external_memory_capabilities"
#define VK_LUID_SIZE_KHR                                   VK_LUID_SIZE

#define VK_KHR_external_memory                1
#define VK_KHR_EXTERNAL_MEMORY_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME "VK_KHR_external_memory"
#define VK_QUEUE_FAMILY_EXTERNAL_KHR          VK_QUEUE_FAMILY_EXTERNAL

#define VK_KHR_external_memory_fd                1
#define VK_KHR_EXTERNAL_MEMORY_FD_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME "VK_KHR_external_memory_fd"

#define VK_KHR_external_semaphore_capabilities              1
#define VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_SPEC_VERSION 1
#define VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME                                      \
  "VK_KHR_external_semaphore_capabilities"

#define VK_KHR_external_semaphore                1
#define VK_KHR_EXTERNAL_SEMAPHORE_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME "VK_KHR_external_semaphore"

#define VK_KHR_external_semaphore_fd                1
#define VK_KHR_EXTERNAL_SEMAPHORE_FD_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME "VK_KHR_external_semaphore_fd"

#define VK_KHR_push_descriptor                1
#define VK_KHR_PUSH_DESCRIPTOR_SPEC_VERSION   2
#define VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME "VK_KHR_push_descriptor"

#define VK_KHR_16bit_storage                1
#define VK_KHR_16BIT_STORAGE_SPEC_VERSION   1
#define VK_KHR_16BIT_STORAGE_EXTENSION_NAME "VK_KHR_16bit_storage"

#define VK_KHR_incremental_present                1
#define VK_KHR_INCREMENTAL_PRESENT_SPEC_VERSION   1
#define VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME "VK_KHR_incremental_present"

#define VK_KHR_descriptor_update_template                1
#define VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_SPEC_VERSION   1
#define VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME "VK_KHR_descriptor_update_template"

#define VK_KHR_shared_presentable_image                1
#define VK_KHR_SHARED_PRESENTABLE_IMAGE_SPEC_VERSION   1
#define VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME "VK_KHR_shared_presentable_image"

#define VK_KHR_external_fence_capabilities                1
#define VK_KHR_EXTERNAL_FENCE_CAPABILITIES_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME "VK_KHR_external_fence_capabilities"

#define VK_KHR_external_fence                1
#define VK_KHR_EXTERNAL_FENCE_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME "VK_KHR_external_fence"

#define VK_KHR_external_fence_fd                1
#define VK_KHR_EXTERNAL_FENCE_FD_SPEC_VERSION   1
#define VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME "VK_KHR_external_fence_fd"

#define VK_KHR_maintenance2                1
#define VK_KHR_MAINTENANCE2_SPEC_VERSION   1
#define VK_KHR_MAINTENANCE2_EXTENSION_NAME "VK_KHR_maintenance2"

#define VK_KHR_get_surface_capabilities2                 1
#define VK_KHR_GET_SURFACE_CAPABILITIES_2_SPEC_VERSION   1
#define VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME "VK_KHR_get_surface_capabilities2"

#define VK_KHR_variable_pointers                1
#define VK_KHR_VARIABLE_POINTERS_SPEC_VERSION   1
#define VK_KHR_VARIABLE_POINTERS_EXTENSION_NAME "VK_KHR_variable_pointers"

#define VK_KHR_dedicated_allocation                1
#define VK_KHR_DEDICATED_ALLOCATION_SPEC_VERSION   3
#define VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME "VK_KHR_dedicated_allocation"

#define VK_KHR_storage_buffer_storage_class                1
#define VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_SPEC_VERSION   1
#define VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME "VK_KHR_storage_buffer_storage_class"

#define VK_KHR_relaxed_block_layout                1
#define VK_KHR_RELAXED_BLOCK_LAYOUT_SPEC_VERSION   1
#define VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME "VK_KHR_relaxed_block_layout"

#define VK_KHR_get_memory_requirements2                 1
#define VK_KHR_GET_MEMORY_REQUIREMENTS_2_SPEC_VERSION   1
#define VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME "VK_KHR_get_memory_requirements2"

#define VK_KHR_image_format_list                1
#define VK_KHR_IMAGE_FORMAT_LIST_SPEC_VERSION   1
#define VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME "VK_KHR_image_format_list"

#define VK_KHR_sampler_ycbcr_conversion                1
#define VK_KHR_SAMPLER_YCBCR_CONVERSION_SPEC_VERSION   1
#define VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME "VK_KHR_sampler_ycbcr_conversion"

#define VK_KHR_bind_memory2                 1
#define VK_KHR_BIND_MEMORY_2_SPEC_VERSION   1
#define VK_KHR_BIND_MEMORY_2_EXTENSION_NAME "VK_KHR_bind_memory2"

#define VK_KHR_maintenance3                1
#define VK_KHR_MAINTENANCE3_SPEC_VERSION   1
#define VK_KHR_MAINTENANCE3_EXTENSION_NAME "VK_KHR_maintenance3"

#define VK_EXT_debug_report                1
#define VK_EXT_DEBUG_REPORT_SPEC_VERSION   9
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"

#define VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT                                             \
  VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT
#define VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT                                               \
  VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT

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

#define VK_EXT_debug_utils                1
#define VK_EXT_DEBUG_UTILS_SPEC_VERSION   1
#define VK_EXT_DEBUG_UTILS_EXTENSION_NAME "VK_EXT_debug_utils"

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

#define VK_KHR_acceleration_structure                1
#define VK_KHR_ACCELERATION_STRUCTURE_SPEC_VERSION   11
#define VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME "VK_KHR_acceleration_structure"

#define VK_KHR_8bit_storage                1
#define VK_KHR_8BIT_STORAGE_SPEC_VERSION   1
#define VK_KHR_8BIT_STORAGE_EXTENSION_NAME "VK_KHR_8bit_storage"

#define VK_KHR_16bit_storage                1
#define VK_KHR_16BIT_STORAGE_SPEC_VERSION   1
#define VK_KHR_16BIT_STORAGE_EXTENSION_NAME "VK_KHR_16bit_storage"

#ifdef GITS_PLATFORM_X11
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_KHR_xlib_surface                1
#define VK_KHR_XLIB_SURFACE_SPEC_VERSION   6
#define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"

#define VK_EXT_acquire_xlib_display                1
#define VK_EXT_ACQUIRE_XLIB_DISPLAY_SPEC_VERSION   1
#define VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME "VK_EXT_acquire_xlib_display"
#endif

#ifdef GITS_PLATFORM_X11
#define VK_USE_PLATFORM_XCB_KHR
#define VK_KHR_xcb_surface                1
#define VK_KHR_XCB_SURFACE_SPEC_VERSION   6
#define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#endif

#ifdef GITS_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_KHR_win32_surface                1
#define VK_KHR_WIN32_SURFACE_SPEC_VERSION   6
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
#endif

#ifdef GITS_PLATFORM_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_KHR_wayland_surface                1
#define VK_KHR_WAYLAND_SURFACE_SPEC_VERSION   6
#define VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME "VK_KHR_wayland_surface"
#endif

typedef void(VKAPI_PTR* PFN_vkVoidFunction)(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
