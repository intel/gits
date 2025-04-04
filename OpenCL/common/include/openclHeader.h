// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "platform.h"
#include "CL/cl_platform.h"

#if defined(GITS_PLATFORM_WINDOWS) && !defined(BUILD_FOR_CCODE)
#include <d3d11.h>
#include <d3d10.h>
#include <d3d9.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cl_platform_id* cl_platform_id;
typedef struct _cl_device_id* cl_device_id;
typedef struct _cl_context* cl_context;
typedef struct _cl_command_queue* cl_command_queue;
typedef struct _cl_mem* cl_mem;
typedef struct _cl_program* cl_program;
typedef struct _cl_kernel* cl_kernel;
typedef struct _cl_event* cl_event;
typedef struct _cl_sampler* cl_sampler;

typedef cl_uint
    cl_bool; /* WARNING!  Unlike cl_ types in cl_platform.h, cl_bool is not guaranteed to be the same size as the bool in kernels. */
typedef cl_ulong cl_bitfield;
typedef cl_bitfield cl_device_type;
typedef cl_uint cl_platform_info;
typedef cl_uint cl_device_info;
typedef cl_bitfield cl_device_fp_config;
typedef cl_uint cl_device_mem_cache_type;
typedef cl_uint cl_device_local_mem_type;
typedef cl_bitfield cl_device_exec_capabilities;
typedef cl_bitfield cl_device_svm_capabilities;
typedef cl_bitfield cl_command_queue_properties;
typedef intptr_t cl_device_partition_property;
typedef cl_ulong cl_device_partition_property_ext;
typedef cl_bitfield cl_device_affinity_domain;

typedef intptr_t cl_context_properties;
typedef cl_uint cl_context_info;
typedef cl_bitfield cl_queue_properties;
typedef cl_uint cl_command_queue_info;
typedef cl_uint cl_channel_order;
typedef cl_uint cl_channel_type;
typedef cl_bitfield cl_mem_flags;
typedef cl_bitfield cl_mem_properties_intel;
typedef cl_bitfield cl_svm_mem_flags;
typedef cl_uint cl_mem_object_type;
typedef cl_uint cl_mem_info;
typedef cl_bitfield cl_mem_migration_flags;
typedef cl_bitfield cl_mem_migration_flags_intel;
typedef cl_uint cl_image_info;
typedef cl_uint cl_buffer_create_type;
typedef cl_uint cl_addressing_mode;
typedef cl_uint cl_filter_mode;
typedef cl_uint cl_sampler_info;
typedef cl_bitfield cl_map_flags;
typedef intptr_t cl_pipe_properties;
typedef cl_uint cl_pipe_info;
typedef cl_uint cl_program_info;
typedef cl_uint cl_program_build_info;
typedef cl_uint cl_program_binary_type;
typedef cl_int cl_build_status;
typedef cl_uint cl_kernel_info;
typedef cl_uint cl_kernel_arg_info;
typedef cl_uint cl_kernel_arg_address_qualifier;
typedef cl_uint cl_kernel_arg_access_qualifier;
typedef cl_bitfield cl_kernel_arg_type_qualifier;
typedef cl_uint cl_kernel_work_group_info;
typedef cl_uint cl_kernel_sub_group_info;
typedef cl_uint cl_event_info;
typedef cl_uint cl_command_type;
typedef cl_uint cl_profiling_info;
typedef cl_bitfield cl_sampler_properties;
typedef cl_uint cl_kernel_exec_info;
typedef cl_bitfield cl_device_unified_shared_memory_capabilities_intel;
typedef cl_bitfield cl_mem_properties_intel;
typedef cl_bitfield cl_mem_alloc_flags_intel;
typedef cl_uint cl_mem_info_intel;
typedef cl_uint cl_unified_shared_memory_type_intel;
typedef cl_uint cl_mem_advice_intel;

typedef cl_uint cl_gl_context_info;
typedef cl_uint cl_gl_texture_info;
typedef cl_uint cl_gl_object_type;
typedef cl_uint cl_gl_platform_info;

typedef cl_uint cl_d3d11_device_source_khr;
typedef cl_uint cl_d3d11_device_set_khr;
typedef cl_uint cl_d3d10_device_source_khr;
typedef cl_uint cl_d3d10_device_set_khr;
typedef cl_uint cl_dx9_device_source_intel;
typedef cl_uint cl_dx9_device_set_intel;
typedef cl_uint cl_dx9_media_adapter_type_khr;
typedef cl_uint cl_dx9_media_adapter_set_khr;

typedef cl_uint cl_d3d11_device_source_nv;
typedef cl_uint cl_d3d11_device_set_nv;

typedef cl_uint cl_resource_memory_scope;
typedef cl_uint cl_resource_barrier_type;

typedef cl_uint cl_va_api_device_source_intel;
typedef cl_uint cl_va_api_device_set_intel;
typedef unsigned int VASurfaceID;

typedef struct __GLsync* cl_GLsync;

typedef struct _cl_image_format {
  cl_channel_order image_channel_order;
  cl_channel_type image_channel_data_type;
} cl_image_format;

typedef struct _cl_image_desc {
  cl_mem_object_type image_type;
  size_t image_width;
  size_t image_height;
  size_t image_depth;
  size_t image_array_size;
  size_t image_row_pitch;
  size_t image_slice_pitch;
  cl_uint num_mip_levels;
  cl_uint num_samples;
#ifdef __GNUC__
  __extension__ /* Prevents warnings about anonymous union in -pedantic builds */
#endif
      union {
    cl_mem buffer;
    cl_mem mem_object;
  };
} cl_image_desc;

typedef struct _cl_buffer_region {
  size_t origin;
  size_t size;
} cl_buffer_region;

typedef struct _cl_resource_barrier_descriptor_intel {
  void* svm_allocation_pointer;
  cl_mem mem_object;
  cl_resource_barrier_type type;
  cl_resource_memory_scope scope;
} cl_resource_barrier_descriptor_intel;

#include "openclHeaderAuto.h"

/* OpenCL Version */
#define CL_VERSION_1_0 1
#define CL_VERSION_1_1 1
#define CL_VERSION_1_2 1
#define CL_VERSION_2_0 1
#define CL_VERSION_2_1 1
#define CL_VERSION_2_2 1
#define CL_VERSION_3_0 1

/* cl_bool */
#define CL_BLOCKING     CL_TRUE
#define CL_NON_BLOCKING CL_FALSE

/* clDeviceGetInfo selectors */
#define CL_DEVICE_PARENT_DEVICE_EXT    0x4054
#define CL_DEVICE_PARTITION_TYPES_EXT  0x4055
#define CL_DEVICE_AFFINITY_DOMAINS_EXT 0x4056
#define CL_DEVICE_REFERENCE_COUNT_EXT  0x4057
#define CL_DEVICE_PARTITION_STYLE_EXT  0x4058

#if !defined(GITS_PLATFORM_WINDOWS) || defined(BUILD_FOR_CCODE)
typedef unsigned int UINT;

typedef void ID3D11Buffer;
typedef void ID3D11Texture2D;
typedef void ID3D11Texture3D;

typedef void ID3D10Buffer;
typedef void ID3D10Texture2D;
typedef void ID3D10Texture3D;

typedef void IDirect3DSurface9;
typedef void* HANDLE;
#endif

#ifdef __cplusplus
}
#endif
