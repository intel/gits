#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_l0 import Argument, VarDef, ArgDef

callback_table = {}

def Callback(**kwargs):
  if kwargs['name'] in callback_table and kwargs['version']:
    callback_table[kwargs['name'] + '_V' + str(kwargs['version'])] = kwargs
  else:
    callback_table[kwargs['name']] = kwargs

def get_callbacks():
  return callback_table


Argument(name='ze_command_list_append_barrier_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var3=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var4=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_event_reset_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_command_list_append_image_copy_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phDstImage',type='ze_image_handle_t*',tag='inout'),
var3=VarDef(name='phSrcImage',type='ze_image_handle_t*',tag='inout'),
var4=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var5=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var6=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_image_copy_from_memory_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phDstImage',type='ze_image_handle_t*',tag='inout'),
var3=VarDef(name='psrcptr',type='const void**',tag='inout'),
var4=VarDef(name='ppDstRegion',type='const ze_image_region_t**',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_image_copy_from_memory_ext_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phDstImage',type='ze_image_handle_t*',tag='inout'),
var3=VarDef(name='psrcptr',type='const void**',tag='inout'),
var4=VarDef(name='ppDstRegion',type='const ze_image_region_t**',tag='inout'),
var5=VarDef(name='psrcRowPitch',type='uint32_t*',tag='inout'),
var6=VarDef(name='psrcSlicePitch',type='uint32_t*',tag='inout'),
var7=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var8=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var9=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_image_copy_region_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phDstImage',type='ze_image_handle_t*',tag='inout'),
var3=VarDef(name='phSrcImage',type='ze_image_handle_t*',tag='inout'),
var4=VarDef(name='ppDstRegion',type='const ze_image_region_t**',tag='inout'),
var5=VarDef(name='ppSrcRegion',type='const ze_image_region_t**',tag='inout'),
var6=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var7=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var8=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_image_copy_to_memory_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='void**',tag='inout'),
var3=VarDef(name='phSrcImage',type='ze_image_handle_t*',tag='inout'),
var4=VarDef(name='ppSrcRegion',type='const ze_image_region_t**',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_image_copy_to_memory_ext_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='void**',tag='inout'),
var3=VarDef(name='phSrcImage',type='ze_image_handle_t*',tag='inout'),
var4=VarDef(name='ppSrcRegion',type='const ze_image_region_t**',tag='inout'),
var5=VarDef(name='pdestRowPitch',type='uint32_t*',tag='inout'),
var6=VarDef(name='pdestSlicePitch',type='uint32_t*',tag='inout'),
var7=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var8=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var9=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_launch_cooperative_kernel_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var3=VarDef(name='ppLaunchFuncArgs',type='const ze_group_count_t**',tag='inout'),
var4=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var5=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var6=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_launch_kernel_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var3=VarDef(name='ppLaunchFuncArgs',type='const ze_group_count_t**',tag='inout'),
var4=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var5=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var6=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_launch_kernel_indirect_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var3=VarDef(name='ppLaunchArgumentsBuffer',type='const ze_group_count_t**',tag='inout'),
var4=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var5=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var6=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_launch_kernel_with_arguments_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var3=VarDef(name='pgroupCounts',type='const ze_group_count_t*',tag='inout'),
var4=VarDef(name='pgroupSizes',type='const ze_group_size_t*',tag='inout'),
var5=VarDef(name='ppArguments',type='void ***',tag='inout'),
var6=VarDef(name='ppNext',type='const void **',tag='inout',wrapType='CExtensionStructCore'),
var7=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var8=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var9=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_launch_kernel_with_parameters_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var3=VarDef(name='ppGroupCounts',type='const ze_group_count_t**',tag='inout'),
var4=VarDef(name='ppNext',type='const void **',tag='inout',wrapType='CExtensionStructCore'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_launch_multiple_kernels_indirect_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumKernels',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphKernels',type='ze_kernel_handle_t**',tag='inout'),
var4=VarDef(name='ppCountBuffer',type='const uint32_t**',tag='inout'),
var5=VarDef(name='ppLaunchArgumentsBuffer',type='const ze_group_count_t**',tag='inout'),
var6=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var7=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var8=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_mem_advise_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pptr',type='const void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
var5=VarDef(name='padvice',type='ze_memory_advice_t*',tag='inout'),
)

Argument(name='ze_command_list_append_memory_copy_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='void**',tag='inout'),
var3=VarDef(name='psrcptr',type='const void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_memory_copy_from_context_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='void**',tag='inout'),
var3=VarDef(name='phContextSrc',type='ze_context_handle_t*',tag='inout'),
var4=VarDef(name='psrcptr',type='const void**',tag='inout'),
var5=VarDef(name='psize',type='size_t*',tag='inout'),
var6=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var7=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var8=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_memory_copy_region_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='void**',tag='inout'),
var3=VarDef(name='pdstRegion',type='const ze_copy_region_t**',tag='inout'),
var4=VarDef(name='pdstPitch',type='uint32_t*',tag='inout'),
var5=VarDef(name='pdstSlicePitch',type='uint32_t*',tag='inout'),
var6=VarDef(name='psrcptr',type='const void**',tag='inout'),
var7=VarDef(name='psrcRegion',type='const ze_copy_region_t**',tag='inout'),
var8=VarDef(name='psrcPitch',type='uint32_t*',tag='inout'),
var9=VarDef(name='psrcSlicePitch',type='uint32_t*',tag='inout'),
var10=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var11=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var12=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_memory_fill_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='void**',tag='inout'),
var3=VarDef(name='ppattern',type='const void**',tag='inout'),
var4=VarDef(name='ppattern_size',type='size_t*',tag='inout'),
var5=VarDef(name='psize',type='size_t*',tag='inout'),
var6=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var7=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var8=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_memory_prefetch_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
)

Argument(name='ze_command_list_append_memory_ranges_barrier_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumRanges',type='uint32_t*',tag='inout'),
var3=VarDef(name='ppRangeSizes',type='const size_t**',tag='inout'),
var4=VarDef(name='ppRanges',type='const void***',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_query_kernel_timestamps_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumEvents',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphEvents',type='ze_event_handle_t**',tag='inout'),
var4=VarDef(name='pdstptr',type='void**',tag='inout'),
var5=VarDef(name='ppOffsets',type='const size_t**',tag='inout'),
var6=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var7=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var8=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_signal_event_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_command_list_append_signal_external_semaphore_ext_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumSemaphores',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphSemaphores',type='ze_external_semaphore_ext_handle_t**',tag='inout'),
var4=VarDef(name='psignalParams',type='ze_external_semaphore_signal_params_ext_t**',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_wait_external_semaphore_ext_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumSemaphores',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphSemaphores',type='ze_external_semaphore_ext_handle_t**',tag='inout'),
var4=VarDef(name='pwaitParams',type='ze_external_semaphore_wait_params_ext_t**',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_wait_on_events_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumEvents',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_append_write_global_timestamp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='uint64_t**',tag='inout'),
var3=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var4=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var5=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_close_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
)

Argument(name='ze_command_list_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_command_list_desc_t**',tag='inout'),
var4=VarDef(name='pphCommandList',type='ze_command_list_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_create_clone_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pphClonedCommandList',type='ze_command_list_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_create_immediate_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='paltdesc',type='const ze_command_queue_desc_t**',tag='inout'),
var4=VarDef(name='pphCommandList',type='ze_command_list_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_destroy_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
)

Argument(name='ze_command_list_get_context_handle_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pphContext',type='ze_context_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_get_device_handle_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pphDevice',type='ze_device_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_get_next_command_id_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_mutable_command_id_exp_desc_t**',tag='inout'),
var3=VarDef(name='ppCommandId',type='uint64_t**',tag='inout'),
)

Argument(name='ze_command_list_get_next_command_id_with_kernels_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_mutable_command_id_exp_desc_t**',tag='inout'),
var3=VarDef(name='pnumKernels',type='uint32_t*',tag='inout'),
var4=VarDef(name='pphKernels',type='ze_kernel_handle_t**',tag='inout'),
var5=VarDef(name='ppCommandId',type='uint64_t**',tag='inout'),
)

Argument(name='ze_command_list_get_ordinal_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='ppOrdinal',type='uint32_t**',tag='inout'),
)

Argument(name='ze_command_list_host_synchronize_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='ptimeout',type='uint64_t*',tag='inout'),
)

Argument(name='ze_command_list_immediate_append_command_lists_exp_params_t',enabled=False,
var1=VarDef(name='phCommandListImmediate',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumCommandLists',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphCommandLists',type='ze_command_list_handle_t**',tag='inout'),
var4=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var5=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var6=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_immediate_get_index_params_t',enabled=False,
var1=VarDef(name='phCommandListImmediate',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='ppIndex',type='uint32_t**',tag='inout'),
)

Argument(name='ze_command_list_is_immediate_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='ppIsImmediate',type='ze_bool_t**',tag='inout'),
)

Argument(name='ze_command_list_reset_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
)

Argument(name='ze_command_list_update_mutable_command_kernels_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pnumKernels',type='uint32_t*',tag='inout'),
var3=VarDef(name='ppCommandId',type='uint64_t**',tag='inout'),
var4=VarDef(name='pphKernels',type='ze_kernel_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_update_mutable_command_signal_event_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pcommandId',type='uint64_t*',tag='inout'),
var3=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_command_list_update_mutable_command_wait_events_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pcommandId',type='uint64_t*',tag='inout'),
var3=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var4=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_command_list_update_mutable_commands_exp_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_mutable_commands_exp_desc_t**',tag='inout'),
)

Argument(name='ze_command_queue_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_command_queue_desc_t**',tag='inout'),
var4=VarDef(name='pphCommandQueue',type='ze_command_queue_handle_t**',tag='inout'),
)

Argument(name='ze_command_queue_destroy_params_t',enabled=False,
var1=VarDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='inout'),
)

Argument(name='ze_command_queue_execute_command_lists_params_t',enabled=False,
var1=VarDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='inout'),
var2=VarDef(name='pnumCommandLists',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphCommandLists',type='ze_command_list_handle_t**',tag='inout'),
var4=VarDef(name='phFence',type='ze_fence_handle_t*',tag='inout'),
)

Argument(name='ze_command_queue_get_index_params_t',enabled=False,
var1=VarDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='inout'),
var2=VarDef(name='ppIndex',type='uint32_t**',tag='inout'),
)

Argument(name='ze_command_queue_get_ordinal_params_t',enabled=False,
var1=VarDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='inout'),
var2=VarDef(name='ppOrdinal',type='uint32_t**',tag='inout'),
)

Argument(name='ze_command_queue_synchronize_params_t',enabled=False,
var1=VarDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='inout'),
var2=VarDef(name='ptimeout',type='uint64_t*',tag='inout'),
)

Argument(name='ze_context_create_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_context_desc_t**',tag='inout'),
var3=VarDef(name='pphContext',type='ze_context_handle_t**',tag='inout'),
)

Argument(name='ze_context_create_ex_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_context_desc_t**',tag='inout'),
var3=VarDef(name='pnumDevices',type='uint32_t*',tag='inout'),
var4=VarDef(name='pphDevices',type='ze_device_handle_t**',tag='inout'),
var5=VarDef(name='pphContext',type='ze_context_handle_t**',tag='inout'),
)

Argument(name='ze_context_destroy_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
)

Argument(name='ze_context_evict_image_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
)

Argument(name='ze_context_evict_memory_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pptr',type='void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
)

Argument(name='ze_context_get_status_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
)

Argument(name='ze_context_make_image_resident_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
)

Argument(name='ze_context_make_memory_resident_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pptr',type='void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
)

Argument(name='ze_context_system_barrier_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
)

Argument(name='ze_device_can_access_peer_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='phPeerDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pvalue',type='ze_bool_t**',tag='inout'),
)

Argument(name='ze_device_get_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='pphDevices',type='ze_device_handle_t**',tag='inout'),
)

Argument(name='ze_device_get_aggregated_copy_offload_increment_value_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pincrementValue',type='uint32_t**',tag='inout'),
)

Argument(name='ze_device_get_cache_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppCacheProperties',type='ze_device_cache_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_command_queue_group_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppCommandQueueGroupProperties',type='ze_command_queue_group_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_compute_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppComputeProperties',type='ze_device_compute_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_external_memory_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppExternalMemoryProperties',type='ze_device_external_memory_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_fabric_vertex_exp_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pphVertex',type='ze_fabric_vertex_handle_t**',tag='inout'),
)

Argument(name='ze_device_get_global_timestamps_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='phostTimestamp',type='uint64_t**',tag='inout'),
var3=VarDef(name='pdeviceTimestamp',type='uint64_t**',tag='inout'),
)

Argument(name='ze_device_get_image_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppImageProperties',type='ze_device_image_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_memory_access_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppMemAccessProperties',type='ze_device_memory_access_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_memory_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppMemProperties',type='ze_device_memory_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_module_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppModuleProperties',type='ze_device_module_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_p2_p_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='phPeerDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='ppP2PProperties',type='ze_device_p2p_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppDeviceProperties',type='ze_device_properties_t**',tag='inout'),
)

Argument(name='ze_device_get_root_device_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pphRootDevice',type='ze_device_handle_t**',tag='inout'),
)

Argument(name='ze_device_get_status_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
)

Argument(name='ze_device_get_sub_devices_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='pphSubdevices',type='ze_device_handle_t**',tag='inout'),
)

Argument(name='ze_device_get_vector_width_properties_ext_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppVectorWidthProperties',type='ze_device_vector_width_properties_ext_t**',tag='inout'),
)

Argument(name='ze_device_import_external_semaphore_ext_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_external_semaphore_ext_desc_t**',tag='inout'),
var3=VarDef(name='pphSemaphore',type='ze_external_semaphore_ext_handle_t**',tag='inout'),
)

Argument(name='ze_device_pci_get_properties_ext_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='ppPciProperties',type='ze_pci_ext_properties_t**',tag='inout'),
)

Argument(name='ze_device_release_external_semaphore_ext_params_t',enabled=False,
var1=VarDef(name='phSemaphore',type='ze_external_semaphore_ext_handle_t*',tag='inout'),
)

Argument(name='ze_device_reserve_cache_ext_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pcacheLevel',type='size_t*',tag='inout'),
var3=VarDef(name='pcacheReservationSize',type='size_t*',tag='inout'),
)

Argument(name='ze_device_set_cache_advice_ext_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='void**',tag='inout'),
var3=VarDef(name='pregionSize',type='size_t*',tag='inout'),
var4=VarDef(name='pcacheRegion',type='ze_cache_ext_region_t*',tag='inout'),
)

Argument(name='ze_device_synchronize_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
)

Argument(name='ze_driver_get_params_t',enabled=False,
var1=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var2=VarDef(name='pphDrivers',type='ze_driver_handle_t**',tag='inout'),
)

Argument(name='ze_driver_get_api_version_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pversion',type='ze_api_version_t**',tag='inout'),
)

Argument(name='ze_driver_get_default_context_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
)

Argument(name='ze_driver_get_extension_function_address_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pname',type='const char**',tag='inout'),
var3=VarDef(name='pppFunctionAddress',type='void***',tag='inout'),
)

Argument(name='ze_driver_get_extension_properties_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppExtensionProperties',type='ze_driver_extension_properties_t**',tag='inout'),
)

Argument(name='ze_driver_get_ipc_properties_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppIpcProperties',type='ze_driver_ipc_properties_t**',tag='inout'),
)

Argument(name='ze_driver_get_last_error_description_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pppString',type='const char***',tag='inout'),
)

Argument(name='ze_driver_get_properties_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppDriverProperties',type='ze_driver_properties_t**',tag='inout'),
)

Argument(name='ze_driver_rtas_format_compatibility_check_exp_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='prtasFormatA',type='ze_rtas_format_exp_t*',tag='inout'),
var3=VarDef(name='prtasFormatB',type='ze_rtas_format_exp_t*',tag='inout'),
)

Argument(name='ze_driver_rtas_format_compatibility_check_ext_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='prtasFormatA',type='ze_rtas_format_ext_t*',tag='inout'),
var3=VarDef(name='prtasFormatB',type='ze_rtas_format_ext_t*',tag='inout'),
)

Argument(name='ze_event_counter_based_close_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_event_counter_based_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_event_counter_based_desc_t**',tag='inout'),
var4=VarDef(name='pphEvent',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_event_counter_based_get_device_address_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='pcompletionValue',type='uint64_t**',tag='inout'),
var3=VarDef(name='pdeviceAddress',type='uint64_t**',tag='inout'),
)

Argument(name='ze_event_counter_based_get_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='pphIpc',type='ze_ipc_event_counter_based_handle_t**',tag='inout'),
)

Argument(name='ze_event_counter_based_open_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phIpc',type='ze_ipc_event_counter_based_handle_t*',tag='inout'),
var3=VarDef(name='pphEvent',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_event_create_params_t',enabled=False,
var1=VarDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_event_desc_t**',tag='inout'),
var3=VarDef(name='pphEvent',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_event_destroy_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_event_get_event_pool_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='pphEventPool',type='ze_event_pool_handle_t**',tag='inout'),
)

Argument(name='ze_event_get_signal_scope_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='ppSignalScope',type='ze_event_scope_flags_t**',tag='inout'),
)

Argument(name='ze_event_get_wait_scope_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='ppWaitScope',type='ze_event_scope_flags_t**',tag='inout'),
)

Argument(name='ze_event_host_reset_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_event_host_signal_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_event_host_synchronize_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='ptimeout',type='uint64_t*',tag='inout'),
)

Argument(name='ze_event_pool_close_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='inout'),
)

Argument(name='ze_event_pool_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_event_pool_desc_t**',tag='inout'),
var3=VarDef(name='pnumDevices',type='uint32_t*',tag='inout'),
var4=VarDef(name='pphDevices',type='ze_device_handle_t**',tag='inout'),
var5=VarDef(name='pphEventPool',type='ze_event_pool_handle_t**',tag='inout'),
)

Argument(name='ze_event_pool_destroy_params_t',enabled=False,
var1=VarDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='inout'),
)

Argument(name='ze_event_pool_get_context_handle_params_t',enabled=False,
var1=VarDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='inout'),
var2=VarDef(name='pphContext',type='ze_context_handle_t**',tag='inout'),
)

Argument(name='ze_event_pool_get_flags_params_t',enabled=False,
var1=VarDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='inout'),
var2=VarDef(name='ppFlags',type='ze_event_pool_flags_t**',tag='inout'),
)

Argument(name='ze_event_pool_get_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='inout'),
var2=VarDef(name='pphIpc',type='ze_ipc_event_pool_handle_t**',tag='inout'),
)

Argument(name='ze_event_pool_open_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phIpc',type='ze_ipc_event_pool_handle_t*',tag='inout'),
var3=VarDef(name='pphEventPool',type='ze_event_pool_handle_t**',tag='inout'),
)

Argument(name='ze_event_pool_put_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phIpc',type='ze_ipc_event_pool_handle_t*',tag='inout'),
)

Argument(name='ze_event_query_kernel_timestamp_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='ze_kernel_timestamp_result_t**',tag='inout'),
)

Argument(name='ze_event_query_kernel_timestamps_ext_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var4=VarDef(name='ppResults',type='ze_event_query_kernel_timestamps_results_ext_properties_t**',tag='inout'),
)

Argument(name='ze_event_query_status_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
)

Argument(name='ze_event_query_timestamps_exp_params_t',enabled=False,
var1=VarDef(name='phEvent',type='ze_event_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var4=VarDef(name='ppTimestamps',type='ze_kernel_timestamp_result_t**',tag='inout'),
)

Argument(name='ze_fabric_edge_get_exp_params_t',enabled=False,
var1=VarDef(name='phVertexA',type='ze_fabric_vertex_handle_t*',tag='inout'),
var2=VarDef(name='phVertexB',type='ze_fabric_vertex_handle_t*',tag='inout'),
var3=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var4=VarDef(name='pphEdges',type='ze_fabric_edge_handle_t**',tag='inout'),
)

Argument(name='ze_fabric_edge_get_properties_exp_params_t',enabled=False,
var1=VarDef(name='phEdge',type='ze_fabric_edge_handle_t*',tag='inout'),
var2=VarDef(name='ppEdgeProperties',type='ze_fabric_edge_exp_properties_t**',tag='inout'),
)

Argument(name='ze_fabric_edge_get_vertices_exp_params_t',enabled=False,
var1=VarDef(name='phEdge',type='ze_fabric_edge_handle_t*',tag='inout'),
var2=VarDef(name='pphVertexA',type='ze_fabric_vertex_handle_t**',tag='inout'),
var3=VarDef(name='pphVertexB',type='ze_fabric_vertex_handle_t**',tag='inout'),
)

Argument(name='ze_fabric_vertex_get_device_exp_params_t',enabled=False,
var1=VarDef(name='phVertex',type='ze_fabric_vertex_handle_t*',tag='inout'),
var2=VarDef(name='pphDevice',type='ze_device_handle_t**',tag='inout'),
)

Argument(name='ze_fabric_vertex_get_exp_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='pphVertices',type='ze_fabric_vertex_handle_t**',tag='inout'),
)

Argument(name='ze_fabric_vertex_get_properties_exp_params_t',enabled=False,
var1=VarDef(name='phVertex',type='ze_fabric_vertex_handle_t*',tag='inout'),
var2=VarDef(name='ppVertexProperties',type='ze_fabric_vertex_exp_properties_t**',tag='inout'),
)

Argument(name='ze_fabric_vertex_get_sub_vertices_exp_params_t',enabled=False,
var1=VarDef(name='phVertex',type='ze_fabric_vertex_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='pphSubvertices',type='ze_fabric_vertex_handle_t**',tag='inout'),
)

Argument(name='ze_fence_create_params_t',enabled=False,
var1=VarDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_fence_desc_t**',tag='inout'),
var3=VarDef(name='pphFence',type='ze_fence_handle_t**',tag='inout'),
)

Argument(name='ze_fence_destroy_params_t',enabled=False,
var1=VarDef(name='phFence',type='ze_fence_handle_t*',tag='inout'),
)

Argument(name='ze_fence_host_synchronize_params_t',enabled=False,
var1=VarDef(name='phFence',type='ze_fence_handle_t*',tag='inout'),
var2=VarDef(name='ptimeout',type='uint64_t*',tag='inout'),
)

Argument(name='ze_fence_query_status_params_t',enabled=False,
var1=VarDef(name='phFence',type='ze_fence_handle_t*',tag='inout'),
)

Argument(name='ze_fence_reset_params_t',enabled=False,
var1=VarDef(name='phFence',type='ze_fence_handle_t*',tag='inout'),
)

Argument(name='ze_image_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_image_desc_t**',tag='inout'),
var4=VarDef(name='pphImage',type='ze_image_handle_t**',tag='inout'),
)

Argument(name='ze_image_destroy_params_t',enabled=False,
var1=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
)

Argument(name='ze_image_get_alloc_properties_ext_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
var3=VarDef(name='ppImageAllocProperties',type='ze_image_allocation_ext_properties_t**',tag='inout'),
)

Argument(name='ze_image_get_device_offset_exp_params_t',enabled=False,
var1=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
var2=VarDef(name='ppDeviceOffset',type='uint64_t**',tag='inout'),
)

Argument(name='ze_image_get_memory_properties_exp_params_t',enabled=False,
var1=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
var2=VarDef(name='ppMemoryProperties',type='ze_image_memory_properties_exp_t**',tag='inout'),
)

Argument(name='ze_image_get_properties_params_t',enabled=False,
var1=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_image_desc_t**',tag='inout'),
var3=VarDef(name='ppImageProperties',type='ze_image_properties_t**',tag='inout'),
)

Argument(name='ze_image_view_create_exp_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_image_desc_t**',tag='inout'),
var4=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
var5=VarDef(name='pphImageView',type='ze_image_handle_t**',tag='inout'),
)

Argument(name='ze_image_view_create_ext_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_image_desc_t**',tag='inout'),
var4=VarDef(name='phImage',type='ze_image_handle_t*',tag='inout'),
var5=VarDef(name='pphImageView',type='ze_image_handle_t**',tag='inout'),
)

Argument(name='ze_init_params_t',enabled=False,
var1=VarDef(name='pflags',type='ze_init_flags_t*',tag='inout'),
)

Argument(name='ze_init_drivers_params_t',enabled=False,
var1=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var2=VarDef(name='pphDrivers',type='ze_driver_handle_t**',tag='inout'),
var3=VarDef(name='pdesc',type='ze_init_driver_type_desc_t**',tag='inout'),
)

Argument(name='ze_kernel_create_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
var2=VarDef(name='pdesc',type='const ze_kernel_desc_t**',tag='inout'),
var3=VarDef(name='pphKernel',type='ze_kernel_handle_t**',tag='inout'),
)

Argument(name='ze_kernel_destroy_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
)

Argument(name='ze_kernel_get_allocation_properties_exp_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppAllocationProperties',type='ze_kernel_allocation_exp_properties_t**',tag='inout'),
)

Argument(name='ze_kernel_get_binary_exp_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppSize',type='size_t**',tag='inout'),
var3=VarDef(name='ppKernelBinary',type='uint8_t**',tag='inout'),
)

Argument(name='ze_kernel_get_indirect_access_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppFlags',type='ze_kernel_indirect_access_flags_t**',tag='inout'),
)

Argument(name='ze_kernel_get_name_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppSize',type='size_t**',tag='inout'),
var3=VarDef(name='ppName',type='char**',tag='inout'),
)

Argument(name='ze_kernel_get_properties_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppKernelProperties',type='ze_kernel_properties_t**',tag='inout'),
)

Argument(name='ze_kernel_get_source_attributes_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppSize',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppString',type='char***',tag='inout'),
)

Argument(name='ze_kernel_scheduling_hint_exp_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ppHint',type='ze_scheduling_hint_exp_desc_t**',tag='inout'),
)

Argument(name='ze_kernel_set_argument_value_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='pargIndex',type='uint32_t*',tag='inout'),
var3=VarDef(name='pargSize',type='size_t*',tag='inout'),
var4=VarDef(name='ppArgValue',type='const void**',tag='inout'),
)

Argument(name='ze_kernel_set_cache_config_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='pflags',type='ze_cache_config_flags_t*',tag='inout'),
)

Argument(name='ze_kernel_set_global_offset_exp_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='poffsetX',type='uint32_t*',tag='inout'),
var3=VarDef(name='poffsetY',type='uint32_t*',tag='inout'),
var4=VarDef(name='poffsetZ',type='uint32_t*',tag='inout'),
)

Argument(name='ze_kernel_set_group_size_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='pgroupSizeX',type='uint32_t*',tag='inout'),
var3=VarDef(name='pgroupSizeY',type='uint32_t*',tag='inout'),
var4=VarDef(name='pgroupSizeZ',type='uint32_t*',tag='inout'),
)

Argument(name='ze_kernel_set_indirect_access_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='pflags',type='ze_kernel_indirect_access_flags_t*',tag='inout'),
)

Argument(name='ze_kernel_suggest_group_size_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='pglobalSizeX',type='uint32_t*',tag='inout'),
var3=VarDef(name='pglobalSizeY',type='uint32_t*',tag='inout'),
var4=VarDef(name='pglobalSizeZ',type='uint32_t*',tag='inout'),
var5=VarDef(name='pgroupSizeX',type='uint32_t**',tag='inout'),
var6=VarDef(name='pgroupSizeY',type='uint32_t**',tag='inout'),
var7=VarDef(name='pgroupSizeZ',type='uint32_t**',tag='inout'),
)

Argument(name='ze_kernel_suggest_max_cooperative_group_count_params_t',enabled=False,
var1=VarDef(name='phKernel',type='ze_kernel_handle_t*',tag='inout'),
var2=VarDef(name='ptotalGroupCount',type='uint32_t**',tag='inout'),
)

Argument(name='ze_mem_alloc_device_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pdevice_desc',type='const ze_device_mem_alloc_desc_t**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='palignment',type='size_t*',tag='inout'),
var5=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var6=VarDef(name='ppptr',type='void***',tag='inout'),
)

Argument(name='ze_mem_alloc_host_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phost_desc',type='const ze_host_mem_alloc_desc_t**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='palignment',type='size_t*',tag='inout'),
var5=VarDef(name='ppptr',type='void***',tag='inout'),
)

Argument(name='ze_mem_alloc_shared_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pdevice_desc',type='const ze_device_mem_alloc_desc_t**',tag='inout'),
var3=VarDef(name='phost_desc',type='const ze_host_mem_alloc_desc_t**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
var5=VarDef(name='palignment',type='size_t*',tag='inout'),
var6=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var7=VarDef(name='ppptr',type='void***',tag='inout'),
)

Argument(name='ze_mem_close_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
)

Argument(name='ze_mem_free_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='void**',tag='inout'),
)

Argument(name='ze_mem_free_ext_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='ppMemFreeDesc',type='const ze_memory_free_ext_desc_t**',tag='inout'),
var3=VarDef(name='pptr',type='void**',tag='inout'),
)

Argument(name='ze_mem_get_address_range_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='ppBase',type='void***',tag='inout'),
var4=VarDef(name='ppSize',type='size_t**',tag='inout'),
)

Argument(name='ze_mem_get_alloc_properties_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='ppMemAllocProperties',type='ze_memory_allocation_properties_t**',tag='inout'),
var4=VarDef(name='pphDevice',type='ze_device_handle_t**',tag='inout'),
)

Argument(name='ze_mem_get_atomic_access_attribute_exp_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pptr',type='const void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
var5=VarDef(name='ppAttr',type='ze_memory_atomic_attr_exp_flags_t**',tag='inout'),
)

Argument(name='ze_mem_get_file_descriptor_from_ipc_handle_exp_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pipcHandle',type='ze_ipc_mem_handle_t*',tag='inout'),
var3=VarDef(name='ppHandle',type='uint64_t**',tag='inout'),
)

Argument(name='ze_mem_get_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='ppIpcHandle',type='ze_ipc_mem_handle_t**',tag='inout'),
)

Argument(name='ze_mem_get_ipc_handle_from_file_descriptor_exp_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phandle',type='uint64_t*',tag='inout'),
var3=VarDef(name='ppIpcHandle',type='ze_ipc_mem_handle_t**',tag='inout'),
)

Argument(name='ze_mem_get_ipc_handle_with_properties_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='ppNext',type='void**',tag='inout',wrapType='CExtensionStructCore'),
var4=VarDef(name='ppIpcHandle',type='ze_ipc_mem_handle_t**',tag='inout'),
)

Argument(name='ze_mem_get_pitch_for2d_image_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pimageWidth',type='size_t*',tag='inout'),
var4=VarDef(name='pimageHeight',type='size_t*',tag='inout'),
var5=VarDef(name='pelementSizeInBytes',type='unsigned int*',tag='inout'),
var6=VarDef(name='prowPitch',type='size_t **',tag='inout'),
)

Argument(name='ze_mem_open_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='phandle',type='ze_ipc_mem_handle_t*',tag='inout'),
var4=VarDef(name='pflags',type='ze_ipc_memory_flags_t*',tag='inout'),
var5=VarDef(name='ppptr',type='void***',tag='inout'),
)

Argument(name='ze_mem_put_ipc_handle_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phandle',type='ze_ipc_mem_handle_t*',tag='inout'),
)

Argument(name='ze_mem_set_atomic_access_attribute_exp_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pptr',type='const void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
var5=VarDef(name='pattr',type='ze_memory_atomic_attr_exp_flags_t*',tag='inout'),
)

Argument(name='ze_module_build_log_destroy_params_t',enabled=False,
var1=VarDef(name='phModuleBuildLog',type='ze_module_build_log_handle_t*',tag='inout'),
)

Argument(name='ze_module_build_log_get_string_params_t',enabled=False,
var1=VarDef(name='phModuleBuildLog',type='ze_module_build_log_handle_t*',tag='inout'),
var2=VarDef(name='ppSize',type='size_t**',tag='inout'),
var3=VarDef(name='ppBuildLog',type='char**',tag='inout'),
)

Argument(name='ze_module_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_module_desc_t**',tag='inout'),
var4=VarDef(name='pphModule',type='ze_module_handle_t**',tag='inout'),
var5=VarDef(name='pphBuildLog',type='ze_module_build_log_handle_t**',tag='inout'),
)

Argument(name='ze_module_destroy_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
)

Argument(name='ze_module_dynamic_link_params_t',enabled=False,
var1=VarDef(name='pnumModules',type='uint32_t*',tag='inout'),
var2=VarDef(name='pphModules',type='ze_module_handle_t**',tag='inout'),
var3=VarDef(name='pphLinkLog',type='ze_module_build_log_handle_t**',tag='inout'),
)

Argument(name='ze_module_get_function_pointer_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
var2=VarDef(name='ppFunctionName',type='const char**',tag='inout'),
var3=VarDef(name='ppfnFunction',type='void***',tag='inout'),
)

Argument(name='ze_module_get_global_pointer_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
var2=VarDef(name='ppGlobalName',type='const char**',tag='inout'),
var3=VarDef(name='ppSize',type='size_t**',tag='inout'),
var4=VarDef(name='ppptr',type='void***',tag='inout'),
)

Argument(name='ze_module_get_kernel_names_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
var2=VarDef(name='ppCount',type='uint32_t**',tag='inout'),
var3=VarDef(name='ppNames',type='const char***',tag='inout'),
)

Argument(name='ze_module_get_native_binary_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
var2=VarDef(name='ppSize',type='size_t**',tag='inout'),
var3=VarDef(name='ppModuleNativeBinary',type='uint8_t**',tag='inout'),
)

Argument(name='ze_module_get_properties_params_t',enabled=False,
var1=VarDef(name='phModule',type='ze_module_handle_t*',tag='inout'),
var2=VarDef(name='ppModuleProperties',type='ze_module_properties_t**',tag='inout'),
)

Argument(name='ze_module_inspect_linkage_ext_params_t',enabled=False,
var1=VarDef(name='ppInspectDesc',type='ze_linkage_inspection_ext_desc_t**',tag='inout'),
var2=VarDef(name='pnumModules',type='uint32_t*',tag='inout'),
var3=VarDef(name='pphModules',type='ze_module_handle_t**',tag='inout'),
var4=VarDef(name='pphLog',type='ze_module_build_log_handle_t**',tag='inout'),
)

Argument(name='ze_physical_mem_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='ze_physical_mem_desc_t**',tag='inout'),
var4=VarDef(name='pphPhysicalMemory',type='ze_physical_mem_handle_t**',tag='inout'),
)

Argument(name='ze_physical_mem_destroy_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phPhysicalMemory',type='ze_physical_mem_handle_t*',tag='inout'),
)

Argument(name='ze_physical_mem_get_properties_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phPhysicalMem',type='ze_physical_mem_handle_t*',tag='inout'),
var3=VarDef(name='ppMemProperties',type='ze_physical_mem_properties_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_build_exp_params_t',enabled=False,
var1=VarDef(name='phBuilder',type='ze_rtas_builder_exp_handle_t*',tag='inout'),
var2=VarDef(name='ppBuildOpDescriptor',type='const ze_rtas_builder_build_op_exp_desc_t**',tag='inout'),
var3=VarDef(name='ppScratchBuffer',type='void**',tag='inout'),
var4=VarDef(name='pscratchBufferSizeBytes',type='size_t*',tag='inout'),
var5=VarDef(name='ppRtasBuffer',type='void**',tag='inout'),
var6=VarDef(name='prtasBufferSizeBytes',type='size_t*',tag='inout'),
var7=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t*',tag='inout'),
var8=VarDef(name='ppBuildUserPtr',type='void**',tag='inout'),
var9=VarDef(name='ppBounds',type='ze_rtas_aabb_exp_t**',tag='inout'),
var10=VarDef(name='ppRtasBufferSizeBytes',type='size_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_build_ext_params_t',enabled=False,
var1=VarDef(name='phBuilder',type='ze_rtas_builder_ext_handle_t*',tag='inout'),
var2=VarDef(name='ppBuildOpDescriptor',type='const ze_rtas_builder_build_op_ext_desc_t**',tag='inout'),
var3=VarDef(name='ppScratchBuffer',type='void**',tag='inout'),
var4=VarDef(name='pscratchBufferSizeBytes',type='size_t*',tag='inout'),
var5=VarDef(name='ppRtasBuffer',type='void**',tag='inout'),
var6=VarDef(name='prtasBufferSizeBytes',type='size_t*',tag='inout'),
var7=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_ext_handle_t*',tag='inout'),
var8=VarDef(name='ppBuildUserPtr',type='void**',tag='inout'),
var9=VarDef(name='ppBounds',type='ze_rtas_aabb_ext_t**',tag='inout'),
var10=VarDef(name='ppRtasBufferSizeBytes',type='size_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_command_list_append_copy_ext_params_t',enabled=False,
var1=VarDef(name='phCommandList',type='ze_command_list_handle_t*',tag='inout'),
var2=VarDef(name='pdstptr',type='void**',tag='inout'),
var3=VarDef(name='psrcptr',type='const void**',tag='inout'),
var4=VarDef(name='psize',type='size_t*',tag='inout'),
var5=VarDef(name='phSignalEvent',type='ze_event_handle_t*',tag='inout'),
var6=VarDef(name='pnumWaitEvents',type='uint32_t*',tag='inout'),
var7=VarDef(name='pphWaitEvents',type='ze_event_handle_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_create_exp_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppDescriptor',type='const ze_rtas_builder_exp_desc_t**',tag='inout'),
var3=VarDef(name='pphBuilder',type='ze_rtas_builder_exp_handle_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_create_ext_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='ppDescriptor',type='const ze_rtas_builder_ext_desc_t**',tag='inout'),
var3=VarDef(name='pphBuilder',type='ze_rtas_builder_ext_handle_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_destroy_exp_params_t',enabled=False,
var1=VarDef(name='phBuilder',type='ze_rtas_builder_exp_handle_t*',tag='inout'),
)

Argument(name='ze_rtas_builder_destroy_ext_params_t',enabled=False,
var1=VarDef(name='phBuilder',type='ze_rtas_builder_ext_handle_t*',tag='inout'),
)

Argument(name='ze_rtas_builder_get_build_properties_exp_params_t',enabled=False,
var1=VarDef(name='phBuilder',type='ze_rtas_builder_exp_handle_t*',tag='inout'),
var2=VarDef(name='ppBuildOpDescriptor',type='const ze_rtas_builder_build_op_exp_desc_t**',tag='inout'),
var3=VarDef(name='ppProperties',type='ze_rtas_builder_exp_properties_t**',tag='inout'),
)

Argument(name='ze_rtas_builder_get_build_properties_ext_params_t',enabled=False,
var1=VarDef(name='phBuilder',type='ze_rtas_builder_ext_handle_t*',tag='inout'),
var2=VarDef(name='ppBuildOpDescriptor',type='const ze_rtas_builder_build_op_ext_desc_t**',tag='inout'),
var3=VarDef(name='ppProperties',type='ze_rtas_builder_ext_properties_t**',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_create_exp_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pphParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t**',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_create_ext_params_t',enabled=False,
var1=VarDef(name='phDriver',type='ze_driver_handle_t*',tag='inout'),
var2=VarDef(name='pphParallelOperation',type='ze_rtas_parallel_operation_ext_handle_t**',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_destroy_exp_params_t',enabled=False,
var1=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t*',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_destroy_ext_params_t',enabled=False,
var1=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_ext_handle_t*',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_get_properties_exp_params_t',enabled=False,
var1=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t*',tag='inout'),
var2=VarDef(name='ppProperties',type='ze_rtas_parallel_operation_exp_properties_t**',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_get_properties_ext_params_t',enabled=False,
var1=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_ext_handle_t*',tag='inout'),
var2=VarDef(name='ppProperties',type='ze_rtas_parallel_operation_ext_properties_t**',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_join_exp_params_t',enabled=False,
var1=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t*',tag='inout'),
)

Argument(name='ze_rtas_parallel_operation_join_ext_params_t',enabled=False,
var1=VarDef(name='phParallelOperation',type='ze_rtas_parallel_operation_ext_handle_t*',tag='inout'),
)

Argument(name='ze_sampler_create_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='pdesc',type='const ze_sampler_desc_t**',tag='inout'),
var4=VarDef(name='pphSampler',type='ze_sampler_handle_t**',tag='inout'),
)

Argument(name='ze_sampler_destroy_params_t',enabled=False,
var1=VarDef(name='phSampler',type='ze_sampler_handle_t*',tag='inout'),
)

Argument(name='ze_virtual_mem_free_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
)

Argument(name='ze_virtual_mem_get_access_attribute_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='paccess',type='ze_memory_access_attribute_t**',tag='inout'),
var5=VarDef(name='poutSize',type='size_t**',tag='inout'),
)

Argument(name='ze_virtual_mem_map_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='phPhysicalMemory',type='ze_physical_mem_handle_t*',tag='inout'),
var5=VarDef(name='poffset',type='size_t*',tag='inout'),
var6=VarDef(name='paccess',type='ze_memory_access_attribute_t*',tag='inout'),
)

Argument(name='ze_virtual_mem_query_page_size_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='phDevice',type='ze_device_handle_t*',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='ppagesize',type='size_t**',tag='inout'),
)

Argument(name='ze_virtual_mem_reserve_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='ppStart',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='ppptr',type='void***',tag='inout'),
)

Argument(name='ze_virtual_mem_set_access_attribute_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
var4=VarDef(name='paccess',type='ze_memory_access_attribute_t*',tag='inout'),
)

Argument(name='ze_virtual_mem_unmap_params_t',enabled=False,
var1=VarDef(name='phContext',type='ze_context_handle_t*',tag='inout'),
var2=VarDef(name='pptr',type='const void**',tag='inout'),
var3=VarDef(name='psize',type='size_t*',tag='inout'),
)


Callback(name='ze_pfnCommandListAppendBarrierCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_barrier_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendEventResetCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_event_reset_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendImageCopyCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_image_copy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendImageCopyFromMemoryCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_image_copy_from_memory_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendImageCopyFromMemoryExtCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_image_copy_from_memory_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendImageCopyRegionCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_image_copy_region_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendImageCopyToMemoryCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_image_copy_to_memory_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendImageCopyToMemoryExtCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_image_copy_to_memory_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendLaunchCooperativeKernelCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_launch_cooperative_kernel_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendLaunchKernelCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_launch_kernel_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendLaunchKernelIndirectCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_launch_kernel_indirect_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendLaunchKernelWithArgumentsCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_launch_kernel_with_arguments_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendLaunchKernelWithParametersCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_launch_kernel_with_parameters_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendLaunchMultipleKernelsIndirectCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_launch_multiple_kernels_indirect_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemAdviseCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_mem_advise_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemoryCopyCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_memory_copy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemoryCopyFromContextCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_memory_copy_from_context_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemoryCopyRegionCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_memory_copy_region_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemoryFillCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_memory_fill_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemoryPrefetchCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_memory_prefetch_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendMemoryRangesBarrierCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_memory_ranges_barrier_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendQueryKernelTimestampsCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_query_kernel_timestamps_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendSignalEventCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_signal_event_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendSignalExternalSemaphoreExtCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_signal_external_semaphore_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendWaitExternalSemaphoreExtCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_wait_external_semaphore_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendWaitOnEventsCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_wait_on_events_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListAppendWriteGlobalTimestampCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_append_write_global_timestamp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListCloseCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_close_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListCreateCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListCreateCloneExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_create_clone_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListCreateImmediateCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_create_immediate_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListDestroyCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListGetContextHandleCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_get_context_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListGetDeviceHandleCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_get_device_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListGetNextCommandIdExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_get_next_command_id_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListGetNextCommandIdWithKernelsExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_get_next_command_id_with_kernels_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListGetOrdinalCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_get_ordinal_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListHostSynchronizeCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_host_synchronize_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListImmediateAppendCommandListsExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_immediate_append_command_lists_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListImmediateGetIndexCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_immediate_get_index_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListIsImmediateCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_is_immediate_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListResetCb_t',component='ze_command_list_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_reset_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListUpdateMutableCommandKernelsExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_update_mutable_command_kernels_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListUpdateMutableCommandSignalEventExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_update_mutable_command_signal_event_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListUpdateMutableCommandWaitEventsExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_update_mutable_command_wait_events_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandListUpdateMutableCommandsExpCb_t',component='ze_command_list_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_list_update_mutable_commands_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandQueueCreateCb_t',component='ze_command_queue_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_queue_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandQueueDestroyCb_t',component='ze_command_queue_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_queue_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandQueueExecuteCommandListsCb_t',component='ze_command_queue_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_queue_execute_command_lists_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandQueueGetIndexCb_t',component='ze_command_queue_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_queue_get_index_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandQueueGetOrdinalCb_t',component='ze_command_queue_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_queue_get_ordinal_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnCommandQueueSynchronizeCb_t',component='ze_command_queue_callbacks_t',
arg1=ArgDef(name='params',type='ze_command_queue_synchronize_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextCreateCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextCreateExCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_create_ex_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextDestroyCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextEvictImageCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_evict_image_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextEvictMemoryCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_evict_memory_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextGetStatusCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_get_status_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextMakeImageResidentCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_make_image_resident_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextMakeMemoryResidentCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_make_memory_resident_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnContextSystemBarrierCb_t',component='ze_context_callbacks_t',
arg1=ArgDef(name='params',type='ze_context_system_barrier_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceCanAccessPeerCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_can_access_peer_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetAggregatedCopyOffloadIncrementValueCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_aggregated_copy_offload_increment_value_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetCachePropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_cache_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetCommandQueueGroupPropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_command_queue_group_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetComputePropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_compute_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetExternalMemoryPropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_external_memory_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetFabricVertexExpCb_t',component='ze_device_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_fabric_vertex_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetGlobalTimestampsCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_global_timestamps_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetImagePropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_image_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetMemoryAccessPropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_memory_access_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetMemoryPropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_memory_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetModulePropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_module_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetP2PPropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_p2_p_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetPropertiesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetRootDeviceCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_root_device_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetStatusCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_status_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetSubDevicesCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_sub_devices_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceGetVectorWidthPropertiesExtCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_get_vector_width_properties_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceImportExternalSemaphoreExtCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_import_external_semaphore_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDevicePciGetPropertiesExtCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_pci_get_properties_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceReleaseExternalSemaphoreExtCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_release_external_semaphore_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceReserveCacheExtCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_reserve_cache_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceSetCacheAdviceExtCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_set_cache_advice_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDeviceSynchronizeCb_t',component='ze_device_callbacks_t',
arg1=ArgDef(name='params',type='ze_device_synchronize_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetApiVersionCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_api_version_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetDefaultContextCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_default_context_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetExtensionFunctionAddressCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_extension_function_address_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetExtensionPropertiesCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_extension_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetIpcPropertiesCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_ipc_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetLastErrorDescriptionCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_last_error_description_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverGetPropertiesCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_get_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverRTASFormatCompatibilityCheckExpCb_t',component='ze_driver_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_rtas_format_compatibility_check_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnDriverRTASFormatCompatibilityCheckExtCb_t',component='ze_driver_callbacks_t',
arg1=ArgDef(name='params',type='ze_driver_rtas_format_compatibility_check_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventCounterBasedCloseIpcHandleCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_counter_based_close_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventCounterBasedCreateCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_counter_based_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventCounterBasedGetDeviceAddressCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_counter_based_get_device_address_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventCounterBasedGetIpcHandleCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_counter_based_get_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventCounterBasedOpenIpcHandleCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_counter_based_open_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventCreateCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventDestroyCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventGetEventPoolCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_get_event_pool_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventGetSignalScopeCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_get_signal_scope_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventGetWaitScopeCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_get_wait_scope_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventHostResetCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_host_reset_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventHostSignalCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_host_signal_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventHostSynchronizeCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_host_synchronize_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolCloseIpcHandleCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_close_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolCreateCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolDestroyCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolGetContextHandleCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_get_context_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolGetFlagsCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_get_flags_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolGetIpcHandleCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_get_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolOpenIpcHandleCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_open_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventPoolPutIpcHandleCb_t',component='ze_event_pool_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_pool_put_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventQueryKernelTimestampCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_query_kernel_timestamp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventQueryKernelTimestampsExtCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_query_kernel_timestamps_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventQueryStatusCb_t',component='ze_event_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_query_status_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnEventQueryTimestampsExpCb_t',component='ze_event_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_event_query_timestamps_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricEdgeGetExpCb_t',component='ze_fabric_edge_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_edge_get_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricEdgeGetPropertiesExpCb_t',component='ze_fabric_edge_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_edge_get_properties_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricEdgeGetVerticesExpCb_t',component='ze_fabric_edge_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_edge_get_vertices_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricVertexGetDeviceExpCb_t',component='ze_fabric_vertex_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_vertex_get_device_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricVertexGetExpCb_t',component='ze_fabric_vertex_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_vertex_get_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricVertexGetPropertiesExpCb_t',component='ze_fabric_vertex_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_vertex_get_properties_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFabricVertexGetSubVerticesExpCb_t',component='ze_fabric_vertex_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_fabric_vertex_get_sub_vertices_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFenceCreateCb_t',component='ze_fence_callbacks_t',
arg1=ArgDef(name='params',type='ze_fence_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFenceDestroyCb_t',component='ze_fence_callbacks_t',
arg1=ArgDef(name='params',type='ze_fence_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFenceHostSynchronizeCb_t',component='ze_fence_callbacks_t',
arg1=ArgDef(name='params',type='ze_fence_host_synchronize_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFenceQueryStatusCb_t',component='ze_fence_callbacks_t',
arg1=ArgDef(name='params',type='ze_fence_query_status_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnFenceResetCb_t',component='ze_fence_callbacks_t',
arg1=ArgDef(name='params',type='ze_fence_reset_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageCreateCb_t',component='ze_image_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageDestroyCb_t',component='ze_image_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageGetAllocPropertiesExtCb_t',component='ze_image_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_get_alloc_properties_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageGetDeviceOffsetExpCb_t',component='ze_image_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_get_device_offset_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageGetMemoryPropertiesExpCb_t',component='ze_image_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_get_memory_properties_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageGetPropertiesCb_t',component='ze_image_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_get_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageViewCreateExpCb_t',component='ze_image_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_view_create_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnImageViewCreateExtCb_t',component='ze_image_callbacks_t',
arg1=ArgDef(name='params',type='ze_image_view_create_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnInitCb_t',component='ze_global_callbacks_t',
arg1=ArgDef(name='params',type='ze_init_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnInitDriversCb_t',component='ze_global_callbacks_t',
arg1=ArgDef(name='params',type='ze_init_drivers_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelCreateCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelDestroyCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelGetAllocationPropertiesExpCb_t',component='ze_kernel_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_get_allocation_properties_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelGetBinaryExpCb_t',component='ze_kernel_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_get_binary_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelGetIndirectAccessCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_get_indirect_access_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelGetNameCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_get_name_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelGetPropertiesCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_get_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelGetSourceAttributesCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_get_source_attributes_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSchedulingHintExpCb_t',component='ze_kernel_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_scheduling_hint_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSetArgumentValueCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_set_argument_value_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSetCacheConfigCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_set_cache_config_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSetGlobalOffsetExpCb_t',component='ze_kernel_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_set_global_offset_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSetGroupSizeCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_set_group_size_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSetIndirectAccessCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_set_indirect_access_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSuggestGroupSizeCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_suggest_group_size_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnKernelSuggestMaxCooperativeGroupCountCb_t',component='ze_kernel_callbacks_t',
arg1=ArgDef(name='params',type='ze_kernel_suggest_max_cooperative_group_count_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemAllocDeviceCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_alloc_device_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemAllocHostCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_alloc_host_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemAllocSharedCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_alloc_shared_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemCloseIpcHandleCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_close_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemFreeCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_free_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemFreeExtCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_free_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetAddressRangeCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_address_range_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetAllocPropertiesCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_alloc_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetAtomicAccessAttributeExpCb_t',component='ze_mem_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_atomic_access_attribute_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetFileDescriptorFromIpcHandleExpCb_t',component='ze_mem_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_file_descriptor_from_ipc_handle_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetIpcHandleCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetIpcHandleFromFileDescriptorExpCb_t',component='ze_mem_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_ipc_handle_from_file_descriptor_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetIpcHandleWithPropertiesCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_ipc_handle_with_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemGetPitchFor2dImageCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_get_pitch_for2d_image_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemOpenIpcHandleCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_open_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemPutIpcHandleCb_t',component='ze_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_put_ipc_handle_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnMemSetAtomicAccessAttributeExpCb_t',component='ze_mem_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_mem_set_atomic_access_attribute_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleBuildLogDestroyCb_t',component='ze_module_build_log_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_build_log_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleBuildLogGetStringCb_t',component='ze_module_build_log_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_build_log_get_string_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleCreateCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleDestroyCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleDynamicLinkCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_dynamic_link_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleGetFunctionPointerCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_get_function_pointer_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleGetGlobalPointerCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_get_global_pointer_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleGetKernelNamesCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_get_kernel_names_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleGetNativeBinaryCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_get_native_binary_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleGetPropertiesCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_get_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnModuleInspectLinkageExtCb_t',component='ze_module_callbacks_t',
arg1=ArgDef(name='params',type='ze_module_inspect_linkage_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnPhysicalMemCreateCb_t',component='ze_physical_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_physical_mem_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnPhysicalMemDestroyCb_t',component='ze_physical_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_physical_mem_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnPhysicalMemGetPropertiesCb_t',component='ze_physical_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_physical_mem_get_properties_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderBuildExpCb_t',component='ze_rtas_builder_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_build_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderBuildExtCb_t',component='ze_rtas_builder_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_build_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderCommandListAppendCopyExtCb_t',component='ze_rtas_builder_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_command_list_append_copy_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderCreateExpCb_t',component='ze_rtas_builder_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_create_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderCreateExtCb_t',component='ze_rtas_builder_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_create_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderDestroyExpCb_t',component='ze_rtas_builder_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_destroy_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderDestroyExtCb_t',component='ze_rtas_builder_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_destroy_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderGetBuildPropertiesExpCb_t',component='ze_rtas_builder_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_get_build_properties_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASBuilderGetBuildPropertiesExtCb_t',component='ze_rtas_builder_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_builder_get_build_properties_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationCreateExpCb_t',component='ze_rtas_parallel_operation_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_create_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationCreateExtCb_t',component='ze_rtas_parallel_operation_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_create_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationDestroyExpCb_t',component='ze_rtas_parallel_operation_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_destroy_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationDestroyExtCb_t',component='ze_rtas_parallel_operation_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_destroy_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationGetPropertiesExpCb_t',component='ze_rtas_parallel_operation_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_get_properties_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationGetPropertiesExtCb_t',component='ze_rtas_parallel_operation_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_get_properties_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationJoinExpCb_t',component='ze_rtas_parallel_operation_exp_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_join_exp_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnRTASParallelOperationJoinExtCb_t',component='ze_rtas_parallel_operation_callbacks_t',
arg1=ArgDef(name='params',type='ze_rtas_parallel_operation_join_ext_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnSamplerCreateCb_t',component='ze_sampler_callbacks_t',
arg1=ArgDef(name='params',type='ze_sampler_create_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnSamplerDestroyCb_t',component='ze_sampler_callbacks_t',
arg1=ArgDef(name='params',type='ze_sampler_destroy_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemFreeCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_free_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemGetAccessAttributeCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_get_access_attribute_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemMapCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_map_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemQueryPageSizeCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_query_page_size_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemReserveCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_reserve_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemSetAccessAttributeCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_set_access_attribute_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

Callback(name='ze_pfnVirtualMemUnmapCb_t',component='ze_virtual_mem_callbacks_t',
arg1=ArgDef(name='params',type='ze_virtual_mem_unmap_params_t*'),
arg2=ArgDef(name='result',type='ze_result_t'),
arg3=ArgDef(name='pTracerUserData',type='void*'),
arg4=ArgDef(name='ppTracerInstanceUserData',type='void**'),
)

