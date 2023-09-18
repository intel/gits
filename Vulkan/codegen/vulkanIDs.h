// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// clang-format off
ID_VK_ACQUIRE_NEXT_IMAGE_KHR,
ID_VK_ALLOCATE_COMMAND_BUFFERS,
ID_VK_ALLOCATE_DESCRIPTOR_SETS,
ID_VK_ALLOCATE_MEMORY,
ID_VK_BEGIN_COMMAND_BUFFER,
ID_VK_BIND_BUFFER_MEMORY,
ID_VK_BIND_IMAGE_MEMORY,
ID_VK_CMD_BEGIN_RENDER_PASS,
ID_VK_CMD_BIND_DESCRIPTOR_SETS,
ID_VK_CMD_BIND_INDEX_BUFFER,
ID_VK_CMD_BIND_PIPELINE,
ID_VK_CMD_BIND_VERTEX_BUFFERS,
ID_VK_CMD_CLEAR_ATTACHMENTS,
ID_VK_CMD_CLEAR_COLOR_IMAGE,
ID_VK_CMD_CLEAR_DEPTH_STENCIL_IMAGE,
ID_VK_CMD_COPY_BUFFER,
ID_VK_CMD_COPY_BUFFER_TO_IMAGE,
ID_VK_CMD_DISPATCH,
ID_VK_CMD_DRAW,
ID_VK_CMD_DRAW_INDEXED,
ID_VK_CMD_END_RENDER_PASS,
ID_VK_CMD_PIPELINE_BARRIER,
ID_VK_CMD_SET_BLEND_CONSTANTS,
ID_VK_CMD_SET_DEPTH_BIAS,
ID_VK_CMD_SET_DEPTH_BOUNDS,
ID_VK_CMD_SET_LINE_WIDTH,
ID_VK_CMD_SET_SCISSOR,
ID_VK_CMD_SET_STENCIL_COMPARE_MASK,
ID_VK_CMD_SET_STENCIL_REFERENCE,
ID_VK_CMD_SET_STENCIL_WRITE_MASK,
ID_VK_CMD_SET_VIEWPORT,
ID_VK_CREATE_BUFFER,
ID_VK_CREATE_BUFFER_VIEW,
ID_VK_CREATE_COMMAND_POOL,
ID_VK_CREATE_COMPUTE_PIPELINES,
ID_VK_CREATE_DESCRIPTOR_POOL,
ID_VK_CREATE_DESCRIPTOR_SET_LAYOUT,
ID_VK_CREATE_DEVICE,
ID_VK_CREATE_FENCE,
ID_VK_CREATE_FRAMEBUFFER,
ID_VK_CREATE_GRAPHICS_PIPELINES,
ID_VK_CREATE_IMAGE,
ID_VK_CREATE_IMAGE_VIEW,
ID_VK_CREATE_INSTANCE,
ID_VK_CREATE_PIPELINE_CACHE,
ID_VK_CREATE_PIPELINE_LAYOUT,
ID_VK_CREATE_RENDER_PASS,
ID_VK_CREATE_SAMPLER,
ID_VK_CREATE_SEMAPHORE,
ID_VK_CREATE_SHADER_MODULE,
ID_VK_CREATE_SWAPCHAIN_KHR,
ID_VK_CREATE_WIN32SURFACE_KHR,
ID_VK_DESTROY_BUFFER,
ID_VK_DESTROY_BUFFER_VIEW,
ID_VK_DESTROY_COMMAND_POOL,
ID_VK_DESTROY_DESCRIPTOR_POOL,
ID_VK_DESTROY_DESCRIPTOR_SET_LAYOUT,
ID_VK_DESTROY_DEVICE,
ID_VK_DESTROY_FENCE,
ID_VK_DESTROY_FRAMEBUFFER,
ID_VK_DESTROY_IMAGE,
ID_VK_DESTROY_IMAGE_VIEW,
ID_VK_DESTROY_INSTANCE,
ID_VK_DESTROY_PIPELINE,
ID_VK_DESTROY_PIPELINE_CACHE,
ID_VK_DESTROY_PIPELINE_LAYOUT,
ID_VK_DESTROY_RENDER_PASS,
ID_VK_DESTROY_SAMPLER,
ID_VK_DESTROY_SEMAPHORE,
ID_VK_DESTROY_SHADER_MODULE,
ID_VK_DESTROY_SURFACE_KHR,
ID_VK_DESTROY_SWAPCHAIN_KHR,
ID_VK_DEVICE_WAIT_IDLE,
ID_VK_END_COMMAND_BUFFER,
ID_VK_ENUMERATE_DEVICE_EXTENSION_PROPERTIES,
ID_VK_ENUMERATE_INSTANCE_EXTENSION_PROPERTIES,
ID_VK_ENUMERATE_PHYSICAL_DEVICES,
ID_VK_FLUSH_MAPPED_MEMORY_RANGES,
ID_VK_FREE_COMMAND_BUFFERS,
ID_VK_FREE_DESCRIPTOR_SETS,
ID_VK_FREE_MEMORY,
ID_VK_GET_DEVICE_QUEUE,
ID_VK_GET_SWAPCHAIN_IMAGES_KHR,
ID_VK_MAP_MEMORY,
ID_VK_QUEUE_PRESENT_KHR,
ID_VK_QUEUE_SUBMIT,
ID_VK_QUEUE_WAIT_IDLE,
ID_VK_RESET_COMMAND_BUFFER,
ID_VK_RESET_FENCES,
ID_VK_UNMAP_MEMORY,
ID_VK_UPDATE_DESCRIPTOR_SETS,
ID_VK_WAIT_FOR_FENCES,
ID_VK_CMD_BEGIN_QUERY,
ID_VK_CMD_COPY_IMAGE,
ID_VK_CMD_END_QUERY,
ID_VK_CMD_RESET_QUERY_POOL,
ID_VK_CREATE_QUERY_POOL,
ID_VK_CMD_NEXT_SUBPASS,
ID_VK_CMD_UPDATE_BUFFER,
ID_VK_CMD_BLIT_IMAGE,
ID_VK_DESTROY_QUERY_POOL,
ID_VK_ENUMERATE_INSTANCE_LAYER_PROPERTIES,
ID_VK_GET_FENCE_STATUS,
ID_VK_CMD_RESOLVE_IMAGE,
ID_VK_CMD_EXECUTE_COMMANDS,
ID_VK_CMD_PUSH_CONSTANTS,
ID_VK_CMD_COPY_QUERY_POOL_RESULTS,
ID_VK_CMD_SET_EVENT,
ID_VK_CMD_WAIT_EVENTS,
ID_VK_CREATE_EVENT,
ID_VK_DESTROY_EVENT,
ID_VK_SET_EVENT,
ID_VK_RESET_DESCRIPTOR_POOL,
ID_VK_GET_IMAGE_SUBRESOURCE_LAYOUT,
ID_VK_CMD_COPY_IMAGE_TO_BUFFER,
ID_VK_CMD_DRAW_INDEXED_INDIRECT,
ID_VK_CMD_WRITE_TIMESTAMP,
ID_VK_GET_QUERY_POOL_RESULTS,
ID_VK_CMD_DRAW_INDIRECT,
ID_VK_RESET_COMMAND_POOL,
ID_VK_GET_BUFFER_MEMORY_REQUIREMENTS,
ID_VK_GET_IMAGE_MEMORY_REQUIREMENTS,
ID_VK_CREATE_DESCRIPTOR_UPDATE_TEMPLATE_KHR,
ID_VK_DESTROY_DESCRIPTOR_UPDATE_TEMPLATE_KHR,
ID_VK_CMD_DEBUG_MARKER_BEGIN_EXT,
ID_VK_CMD_DEBUG_MARKER_END_EXT,
ID_VK_CMD_DEBUG_MARKER_INSERT_EXT,
ID_VK_DEBUG_MARKER_SET_OBJECT_NAME_EXT,
ID_VK_DEBUG_MARKER_SET_OBJECT_TAG_EXT,
ID_VK_UPDATE_DESCRIPTOR_SET_WITH_TEMPLATE_KHR,
ID_VK_CMD_DISPATCH_INDIRECT,
ID_VK_CMD_BEGIN_RENDER_PASS2KHR,
ID_VK_CMD_END_RENDER_PASS2KHR,
ID_VK_CMD_NEXT_SUBPASS2KHR,
ID_VK_CREATE_DESCRIPTOR_UPDATE_TEMPLATE,
ID_VK_CREATE_RENDER_PASS2KHR,
ID_VK_DESTROY_DESCRIPTOR_UPDATE_TEMPLATE,
ID_VK_GET_BUFFER_MEMORY_REQUIREMENTS2,
ID_VK_GET_BUFFER_MEMORY_REQUIREMENTS2KHR,
ID_VK_GET_DEVICE_QUEUE2,
ID_VK_GET_IMAGE_MEMORY_REQUIREMENTS2,
ID_VK_GET_IMAGE_MEMORY_REQUIREMENTS2KHR,
ID_VK_UPDATE_DESCRIPTOR_SET_WITH_TEMPLATE,
ID_VK_ACQUIRE_NEXT_IMAGE2KHR,
ID_VK_BIND_BUFFER_MEMORY2,
ID_VK_BIND_BUFFER_MEMORY2KHR,
ID_VK_BIND_IMAGE_MEMORY2,
ID_VK_BIND_IMAGE_MEMORY2KHR,
ID_VK_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES,
ID_VK_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES2,
ID_VK_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES2KHR,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES2KHR,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES_KHR,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_FORMATS2KHR,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_FORMATS_KHR,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES_KHR,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_SUPPORT_KHR,
ID_VK_CMD_PUSH_DESCRIPTOR_SET_KHR,
ID_VK_CREATE_XCB_SURFACE_KHR,
ID_VK_CREATE_PIPELINE_CACHE_V1,
ID_VK_CMD_RESET_EVENT,
ID_VK_RESET_EVENT,
ID_VK_GET_EVENT_STATUS,
ID_VK_QUEUE_BIND_SPARSE,
ID_VK_CMD_DRAW_MESH_TASKS_INDIRECT_COUNT_NV,
ID_VK_CMD_DRAW_MESH_TASKS_INDIRECT_NV,
ID_VK_CMD_DRAW_MESH_TASKS_NV,
ID_VK_ENUMERATE_PHYSICAL_DEVICE_GROUPS,
ID_VK_ENUMERATE_PHYSICAL_DEVICE_GROUPS_KHR,
ID_VK_CMD_DRAW_INDEXED_INDIRECT_COUNT_KHR,
ID_VK_CMD_DRAW_INDIRECT_COUNT_KHR,
ID_VK_CMD_FILL_BUFFER,
ID_VK_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES2EXT,
ID_VK_PASS_PHYSICAL_DEVICE_MEMORY_PROPERTIES_GITS,
ID_VK_CMD_BEGIN_RENDER_PASS2,
ID_VK_CMD_END_RENDER_PASS2,
ID_VK_CMD_NEXT_SUBPASS2,
ID_VK_CREATE_RENDER_PASS2,
ID_VK_GET_SEMAPHORE_COUNTER_VALUE,
ID_VK_GET_SEMAPHORE_COUNTER_VALUE_KHR,
ID_VK_SIGNAL_SEMAPHORE,
ID_VK_SIGNAL_SEMAPHORE_KHR,
ID_VK_WAIT_SEMAPHORES,
ID_VK_WAIT_SEMAPHORES_KHR,
ID_VK_CMD_BEGIN_CONDITIONAL_RENDERING_EXT,
ID_VK_CMD_END_CONDITIONAL_RENDERING_EXT,
ID_VK_CMD_BIND_VERTEX_BUFFERS2EXT,
ID_VK_RESET_QUERY_POOL,
ID_VK_RESET_QUERY_POOL_EXT,
ID_VK_GET_BUFFER_DEVICE_ADDRESS_UNIFIED_GITS,
ID_VK_CMD_BEGIN_TRANSFORM_FEEDBACK_EXT,
ID_VK_CMD_BIND_TRANSFORM_FEEDBACK_BUFFERS_EXT,
ID_VK_CMD_END_TRANSFORM_FEEDBACK_EXT,
ID_VK_CREATE_XLIB_SURFACE_KHR,
ID_VK_CMD_BEGIN_RENDERING,
ID_VK_CMD_END_RENDERING,
ID_VK_CMD_PIPELINE_BARRIER2,
ID_VK_CMD_SET_EVENT2,
ID_VK_QUEUE_SUBMIT2,
ID_VK_CMD_BIND_VERTEX_BUFFERS2,
ID_VK_CMD_COPY_BUFFER2,
ID_VK_CMD_COPY_BUFFER_TO_IMAGE2,
ID_VK_CMD_SET_CULL_MODE,
ID_VK_CMD_SET_FRONT_FACE,
ID_VK_CMD_SET_SCISSOR_WITH_COUNT,
ID_VK_CMD_SET_VIEWPORT_WITH_COUNT,
ID_VK_CMD_RESOLVE_IMAGE2,
ID_VK_CMD_PIPELINE_BARRIER2KHR,
ID_VK_CMD_PIPELINE_BARRIER2UNIFIED_GITS,
ID_VK_CMD_BLIT_IMAGE2,
ID_VK_CMD_COPY_IMAGE2,
ID_VK_CMD_COPY_IMAGE2KHR,
ID_VK_CMD_COPY_IMAGE_TO_BUFFER2,
ID_VK_CMD_COPY_IMAGE_TO_BUFFER2KHR,
ID_VK_CMD_SET_CULL_MODE_EXT,
ID_VK_CMD_SET_DEPTH_BIAS_ENABLE,
ID_VK_CMD_SET_DEPTH_BIAS_ENABLE_EXT,
ID_VK_CMD_SET_DEPTH_BOUNDS_TEST_ENABLE,
ID_VK_CMD_SET_DEPTH_BOUNDS_TEST_ENABLE_EXT,
ID_VK_CMD_SET_DEPTH_COMPARE_OP,
ID_VK_CMD_SET_DEPTH_COMPARE_OP_EXT,
ID_VK_CMD_SET_DEPTH_TEST_ENABLE,
ID_VK_CMD_SET_DEPTH_TEST_ENABLE_EXT,
ID_VK_CMD_SET_DEPTH_WRITE_ENABLE,
ID_VK_CMD_SET_DEPTH_WRITE_ENABLE_EXT,
ID_VK_CMD_SET_STENCIL_OP,
ID_VK_CMD_SET_STENCIL_OP_EXT,
ID_VK_CMD_SET_STENCIL_TEST_ENABLE,
ID_VK_CMD_SET_STENCIL_TEST_ENABLE_EXT,
ID_VK_CMD_DRAW_MESH_TASKS_EXT,
ID_VK_CMD_DRAW_MESH_TASKS_INDIRECT_COUNT_EXT,
ID_VK_CMD_DRAW_MESH_TASKS_INDIRECT_EXT,
ID_VK_CMD_SET_EVENT2KHR,
ID_VK_CMD_INITIALIZE_BUFFER_INTEL,
ID_VK_CMD_INITIALIZE_IMAGE_INTEL,
ID_VK_CMD_DRAW_INDEXED_INDIRECT_COUNT,
ID_VK_CMD_BUILD_ACCELERATION_STRUCTURES_KHR,
ID_VK_CMD_COPY_ACCELERATION_STRUCTURE_KHR,
ID_VK_CMD_TRACE_RAYS_KHR,
ID_VK_COPY_ACCELERATION_STRUCTURE_KHR,
ID_VK_CREATE_ACCELERATION_STRUCTURE_KHR,
ID_VK_CREATE_DEFERRED_OPERATION_KHR,
ID_VK_CREATE_RAY_TRACING_PIPELINES_KHR,
ID_VK_DEFERRED_OPERATION_JOIN_KHR,
ID_VK_DESTROY_ACCELERATION_STRUCTURE_KHR,
ID_VK_DESTROY_DEFERRED_OPERATION_KHR,
ID_VK_GET_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_UNIFIED_GITS,
ID_VK_GET_DEFERRED_OPERATION_RESULT_KHR,
