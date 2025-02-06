#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

Creator=1
Retain=2
Release=4
Info=8
Build=16,
Set=32,
Enqueue=64,
NDRange=128

arguments_table = {}
enums_table = {}
functions_table = {}

def Argument(**kwargs):
  arguments_table[kwargs['name']] = kwargs

def Enum(**kwargs):
  enums_table[kwargs['name']] = kwargs

def Function(**kwargs):
  if kwargs['name'] in functions_table and kwargs['version']:
    functions_table[kwargs['name'] + '_V' + str(kwargs['version'])] = kwargs
  else:
    functions_table[kwargs['name']] = kwargs

def ArgDef(**kwargs):
  return kwargs

def RetDef(**kwargs):
  return kwargs

def VarDef(**kwargs):
  return kwargs

def GetArguments():
  return arguments_table

def GetEnums():
  return enums_table

def GetFunctions():
  return functions_table

Function(name='clBuildProgram',enabled=True,availableFrom='1.0',extension=False,type=Build,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='device_list',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='options',tag='in',type='const char*',wrapType='CBuildOptions',wrapParams='{name}, SD().GetProgramState(program, EXCEPTION_MESSAGE).HasHeaders()'),
arg5=ArgDef(name='pfn_notify',tag='in',type='CallbackProgram'),
arg6=ArgDef(name='user_data',tag='out',type='void*',wrapType='CCLUserData')
)

Function(name='clCreateBuffer',enabled=True,availableFrom='1.0',extension=False,type=Creator,stateTrack=True,recExecWrap=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='size',tag='in',type='size_t'),
arg4=ArgDef(name='host_ptr',tag='in',type='void*',wrapType='CAsyncBinaryData',wrapParams='{name} ? size : 0, {name}'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateBufferWithPropertiesINTEL',enabled=True,availableFrom='1.0',extension=True,type=Creator,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='properties',tag='in',type='cl_mem_properties_intel*',wrapParams='{name}, 0, 2'),
arg3=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg4=ArgDef(name='size',tag='in',type='size_t'),
arg5=ArgDef(name='host_ptr',tag='in',type='void*',wrapType='CAsyncBinaryData',wrapParams='{name} ? size : 0, {name}'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateCommandQueue',enabled=True,availableFrom='1.0',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_command_queue'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='properties',tag='in',type='cl_command_queue_properties'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

# XXX: arg2.wrapParams: should the last parameter be changed to 2?
Function(name='clCreateContext',enabled=True,availableFrom='1.0',extension=False,type=Creator,runWrap=True,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_context'),
arg1=ArgDef(name='properties',tag='in',type='const cl_context_properties*',wrapParams='{name}, 0, 2'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='devices',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='pfn_notify',tag='in',type='CallbackContext'),
arg5=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

# XXX: arg2.wrapParams: should the last parameter be changed to 2?
Function(name='clCreateContextFromType',enabled=True,availableFrom='1.0',extension=False,type=Creator,runWrap=True,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_context'),
arg1=ArgDef(name='properties',tag='in',type='const cl_context_properties*',wrapParams='{name}, 0, 2'),
arg2=ArgDef(name='device_type',tag='in',type='cl_device_type'),
arg3=ArgDef(name='pfn_notify',tag='in',type='CallbackContext'),
arg4=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateEventFromGLsyncKHR',enabled=True,availableFrom='1.1',extension=True,type=Creator,
retV=RetDef(type='cl_event'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='sync',tag='in',type='cl_GLsync'),
arg3=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromGLBuffer',enabled=True,availableFrom='1.0',extension=True,type=Creator,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='bufobj',tag='in',type='cl_GLuint'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromGLRenderbuffer',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clCreateFromGLBuffer')

Function(name='clCreateFromGLTexture',enabled=True,availableFrom='1.2',extension=False,type=Creator,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='target',tag='in',type='cl_GLenum'),
arg4=ArgDef(name='miplevel',tag='in',type='cl_GLint'),
arg5=ArgDef(name='texture',tag='in',type='cl_GLuint'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromGLTexture2D',enabled=True,availableFrom='1.1.DEPRECATED',extension=True,inheritFrom='clCreateFromGLTexture')

Function(name='clCreateFromGLTexture3D',enabled=True,availableFrom='1.1.DEPRECATED',extension=True,inheritFrom='clCreateFromGLTexture')

Function(name='clCreateImage',enabled=True,availableFrom='1.2',extension=False,type=Creator,stateTrack=True,recExecWrap=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='image_format',tag='in',type='const cl_image_format*'),
arg4=ArgDef(name='image_desc',tag='in',type='const cl_image_desc*'),
arg5=ArgDef(name='host_ptr',tag='in',type='void*',wrapType='CAsyncBinaryData',wrapParams='*image_format, *image_desc, {name}'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateImageWithPropertiesINTEL',enabled=True,availableFrom='1.2',extension=True,type=Creator,stateTrack=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='properties',tag='in',type='cl_mem_properties_intel*',wrapParams='{name}, 0, 2'),
arg3=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg4=ArgDef(name='image_format',tag='in',type='const cl_image_format*'),
arg5=ArgDef(name='image_desc',tag='in',type='const cl_image_desc*'),
arg6=ArgDef(name='host_ptr',tag='in',type='void*',wrapType='CAsyncBinaryData',wrapParams='*image_format, *image_desc, {name}'),
arg7=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateImage2D',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=Creator,stateTrack=True,recExecWrap=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='image_format',tag='in',type='const cl_image_format*'),
arg4=ArgDef(name='image_width',tag='in',type='size_t'),
arg5=ArgDef(name='image_height',tag='in',type='size_t'),
arg6=ArgDef(name='image_row_pitch',tag='in',type='size_t'),
arg7=ArgDef(name='host_ptr',tag='in',type='void*',wrapType='CAsyncBinaryData',wrapParams='*image_format, image_width, image_height, image_row_pitch, {name}'),
arg8=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateImage3D',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=Creator,stateTrack=True,recExecWrap=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='image_format',tag='in',type='const cl_image_format*'),
arg4=ArgDef(name='image_width',tag='in',type='size_t'),
arg5=ArgDef(name='image_height',tag='in',type='size_t'),
arg6=ArgDef(name='image_depth',tag='in',type='size_t'),
arg7=ArgDef(name='image_row_pitch',tag='in',type='size_t'),
arg8=ArgDef(name='image_slice_pitch',tag='in',type='size_t'),
arg9=ArgDef(name='host_ptr',tag='in',type='void*',wrapType='CAsyncBinaryData',wrapParams='*image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, {name}'),
arg10=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateKernel',enabled=True,availableFrom='1.0',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_kernel'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='kernel_name',tag='in',type='const char*'),
arg3=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateKernelsInProgram',enabled=True,availableFrom='1.0',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='num_kernels',tag='in',type='cl_uint'),
arg3=ArgDef(name='kernels',tag='out',type='cl_kernel*',wrapType='Ccl_kernel::CSMapArray',wrapParams='num_kernels, {name}'),
arg4=ArgDef(name='num_kernels_ret',tag='out',type='cl_uint*')
)

Function(name='clCreateProgramWithBinary',enabled=True,availableFrom='1.0',extension=False,type=Creator,stateTrack=True,passNullToken=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='device_list',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='lengths',tag='in',type='const size_t*',wrapParams='num_devices, {name}'),
arg5=ArgDef(name='binaries',tag='in',type='const unsigned char**',wrapType='CBinariesArray',wrapParams='num_devices, {name}, lengths'),
arg6=ArgDef(name='binary_status',tag='out',type='cl_int*',wrapType='CCLResult::CSArray',wrapParams='num_devices, {name}'),
arg7=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateProgramWithBinary',enabled=True,availableFrom='1.0',extension=False,type=Creator,version=1,stateTrack=True,runWrap=True,recWrap=True,passToken=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='device_list',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='lengths',tag='in',type='const size_t*',wrapParams='num_devices, {name}'),
arg5=ArgDef(name='binaries',tag='in',type='const unsigned char**',wrapType='CBinariesArray_V1',wrapParams='num_devices, {name}, lengths'),
arg6=ArgDef(name='binary_status',tag='out',type='cl_int*',wrapType='CCLResult::CSArray',wrapParams='num_devices, {name}'),
arg7=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateProgramWithSource',enabled=True,availableFrom='1.0',extension=False,type=Creator,passToken=True,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='count',tag='in',type='cl_uint',wrapParams='1'),
arg3=ArgDef(name='strings',tag='in',type='const char**',wrapType='CProgramSource', wrapParams='count, {name}, lengths'),
arg4=ArgDef(name='lengths',tag='in',type='const size_t*',wrapParams='1, _strings.Length()'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateSampler',enabled=True,availableFrom='1.0',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_sampler'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='normalized_coords',tag='in',type='cl_bool'),
arg3=ArgDef(name='addressing_mode',tag='in',type='cl_addressing_mode'),
arg4=ArgDef(name='filter_mode',tag='in',type='cl_filter_mode'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateSubBuffer',enabled=True,availableFrom='1.1',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='buffer_create_type',tag='in',type='cl_buffer_create_type'),
arg4=ArgDef(name='buffer_create_info',tag='in',type='const void*',wrapType='Ccl_buffer_region::CSArray',wrapParams='{name} ? 1 : 0, static_cast<const cl_buffer_region*>({name})'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateSubDevices',enabled=True,availableFrom='1.2',extension=False,type=Creator,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='in_device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='properties',tag='in',type='const cl_device_partition_property*',wrapParams='{name}, 0, 2'),
arg3=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg4=ArgDef(name='out_devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries, {name}'),
arg5=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

Function(name='clCreateSubDevicesEXT',enabled=True,availableFrom='1.1.DEPRECATED',extension=True,type=Creator,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='in_device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='properties',tag='in',type='const cl_device_partition_property_ext*',wrapParams='{name}, 0, 2'),
arg3=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg4=ArgDef(name='out_devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries, {name}'),
arg5=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

Function(name='clCreateUserEvent',enabled=True,availableFrom='1.1',extension=False,stateTrack=True,type=Creator,
retV=RetDef(type='cl_event'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueAcquireGLObjects',enabled=True,availableFrom='1.0',extension=True,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_objects',tag='in',type='cl_uint'),
arg3=ArgDef(name='mem_objects',tag='in',type='const cl_mem*',wrapParams='num_objects, {name}'),
arg4=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg5=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg6=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueBarrier',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=Enqueue,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue')
)

Function(name='clEnqueueCopyBuffer',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='src_buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='dst_buffer',tag='in',type='cl_mem'),
arg4=ArgDef(name='src_offset',tag='in',type='size_t'),
arg5=ArgDef(name='dst_offset',tag='in',type='size_t'),
arg6=ArgDef(name='cb',tag='in',type='size_t'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueCopyBufferRect',enabled=True,availableFrom='1.1',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='src_buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='dst_buffer',tag='in',type='cl_mem'),
arg4=ArgDef(name='src_origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='dst_origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg7=ArgDef(name='src_row_pitch',tag='in',type='size_t'),
arg8=ArgDef(name='src_slice_pitch',tag='in',type='size_t'),
arg9=ArgDef(name='dst_row_pitch',tag='in',type='size_t'),
arg10=ArgDef(name='dst_slice_pitch',tag='in',type='size_t'),
arg11=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg12=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg13=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueCopyBufferToImage',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='src_buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='dst_image',tag='in',type='cl_mem'),
arg4=ArgDef(name='src_offset',tag='in',type='size_t'),
arg5=ArgDef(name='dst_origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueCopyImage',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='src_image',tag='in',type='cl_mem'),
arg3=ArgDef(name='dst_image',tag='in',type='cl_mem'),
arg4=ArgDef(name='src_origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='dst_origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueCopyImageToBuffer',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='src_image',tag='in',type='cl_mem'),
arg3=ArgDef(name='dst_buffer',tag='in',type='cl_mem'),
arg4=ArgDef(name='src_origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='dst_offset',tag='in',type='size_t'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMapBuffer',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='void*'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_map',tag='in',type='cl_bool'),
arg4=ArgDef(name='map_flags',tag='in',type='cl_map_flags'),
arg5=ArgDef(name='offset',tag='in',type='size_t'),
arg6=ArgDef(name='cb',tag='in',type='size_t'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True),
arg10=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueMapImage',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='void*'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='image',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_map',tag='in',type='cl_bool'),
arg4=ArgDef(name='map_flags',tag='in',type='cl_map_flags'),
arg5=ArgDef(name='origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg7=ArgDef(name='image_row_pitch',tag='out',type='size_t*'),
arg8=ArgDef(name='image_slice_pitch',tag='out',type='size_t*'),
arg9=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg10=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg11=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True),
arg12=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueMarker',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=Enqueue,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueNDRangeKernel',enabled=True,availableFrom='1.0',extension=False,type=NDRange,stateTrack=True,recExecWrap=True,runWrap=True,recWrap=True,
         ccodeWrap='static size_t counter = 1;\n  stream.select(CCodeOStream::GITS_FRAMES_CPP);\n  stream.Indent() << "// kernel call #" << counter << std::endl;\n  counter++;\n  gits::CFunction::Write(stream);',
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg3=ArgDef(name='work_dim',tag='in',type='cl_uint'),
arg4=ArgDef(name='global_work_offset',tag='in',type='const size_t*',wrapParams='work_dim, {name}'),
arg5=ArgDef(name='global_work_size',tag='in',type='const size_t*',wrapParams='work_dim, {name}'),
arg6=ArgDef(name='local_work_size',tag='in',type='const size_t*',wrapParams='work_dim, {name}'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueNDCountKernelINTEL',enabled=True,availableFrom='1.2',extension=True,type=NDRange,stateTrack=True,recExecWrap=True,runWrap=True,recWrap=True,
         ccodeWrap='static size_t counter = 1;\n  stream.select(CCodeOStream::GITS_FRAMES_CPP);\n  stream.Indent() << "// kernel call #" << counter << std::endl;\n  counter++;\n  gits::CFunction::Write(stream);',
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg3=ArgDef(name='workDim',tag='in',type='cl_uint'),
arg4=ArgDef(name='globalWorkOffset',tag='in',type='const size_t*',wrapParams='workDim, {name}'),
arg5=ArgDef(name='workGroupCount',tag='in',type='const size_t*',wrapParams='workDim, {name}'),
arg6=ArgDef(name='localWorkSize',tag='in',type='const size_t*',wrapParams='workDim, {name}'),
arg7=ArgDef(name='numEventsInWaitList',tag='in',type='cl_uint'),
arg8=ArgDef(name='eventWaitList',tag='in',type='const cl_event*',wrapParams='numEventsInWaitList, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clGetKernelMaxConcurrentWorkGroupCountINTEL',enabled=True,availableFrom='1.2',extension=True,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg3=ArgDef(name='workDim',tag='in',type='cl_uint'),
arg4=ArgDef(name='globalWorkOffset',tag='in',type='const size_t*',wrapParams='workDim, {name}'),
arg5=ArgDef(name='localWorkSize',tag='in',type='const size_t*',wrapParams='workDim, {name}'),
arg6=ArgDef(name='suggestedWorkGroupCount',tag='out',type='size_t *',wrapParams='workDim, {name}')
)

Function(name='clGetDeviceGlobalVariablePointerINTEL',enabled=True,availableFrom='2.0',extension=True,type=Info,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='program',tag='in',type='cl_program'),
arg3=ArgDef(name='global_variable_name',tag='in',type='const char*'),
arg4=ArgDef(name='global_variable_size_ret',tag='out',type='size_t*'),
arg5=ArgDef(name='global_variable_pointer_ret',tag='out',type='void**',wrapType='CCLMappedPtr::CSMapArray',wrapParams='1, {name}')
)

Function(name='clGetDeviceFunctionPointerINTEL',enabled=True,availableFrom='1.2',extension=True,type=Info,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='program',tag='in',type='cl_program'),
arg3=ArgDef(name='function_name',tag='in',type='const char*'),
arg4=ArgDef(name='function_pointer_ret',tag='out',type='cl_ulong*',wrapType='CCLMappedPtr',wrapParams='{name}')
)

Function(name='clEnqueueNativeKernel',enabled=False,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='user_func',tag='in',type='CallbackFunc'),
arg3=ArgDef(name='args',tag='in',type='void*'),
arg4=ArgDef(name='cb_args',tag='in',type='size_t'),
arg5=ArgDef(name='num_mem_objects',tag='in',type='cl_uint'),
arg6=ArgDef(name='mem_list',tag='in',type='const cl_mem*'),
arg7=ArgDef(name='args_mem_loc',tag='in',type='const void**'),
arg8=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg9=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg10=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueReadBuffer',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_read',tag='in',type='cl_bool'),
arg4=ArgDef(name='offset',tag='in',type='size_t'),
arg5=ArgDef(name='cb',tag='in',type='size_t'),
arg6=ArgDef(name='ptr',tag='out',type='void*',wrapType='CAsyncBinaryData',wrapParams='cb, {name}, true'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueReadBufferRect',enabled=True,availableFrom='1.1',extension=False,type=Enqueue,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_read',tag='in',type='cl_bool'),
arg4=ArgDef(name='buffer_offset',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='host_offset',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg7=ArgDef(name='buffer_row_pitch',tag='in',type='size_t'),
arg8=ArgDef(name='buffer_slice_pitch',tag='in',type='size_t'),
arg9=ArgDef(name='host_row_pitch',tag='in',type='size_t'),
arg10=ArgDef(name='host_slice_pitch',tag='in',type='size_t'),
arg11=ArgDef(name='ptr',tag='out',type='void*',wrapType='CAsyncBinaryData',wrapParams='CountBufferRectSize(region, host_row_pitch, host_slice_pitch), {name}, true'),
arg12=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg13=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg14=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueReadImage',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='image',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_read',tag='in',type='cl_bool'),
arg4=ArgDef(name='origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='row_pitch',tag='in',type='size_t'),
arg7=ArgDef(name='slice_pitch',tag='in',type='size_t'),
arg8=ArgDef(name='ptr',tag='out',type='void*',wrapType='CAsyncBinaryData',wrapParams='{name} ? CountImageSize(SD().GetMemState(image, EXCEPTION_MESSAGE).image_format, region, row_pitch, slice_pitch) : 0, {name}, true'),
arg9=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg10=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg11=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueReleaseGLObjects',enabled=True,availableFrom='1.0',extension=True,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_objects',tag='in',type='cl_uint'),
arg3=ArgDef(name='mem_objects',tag='in',type='const cl_mem*',wrapParams='num_objects, {name}'),
arg4=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg5=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg6=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueTask',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg3=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg4=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg5=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueUnmapMemObject',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,recExecWrap=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='memobj',tag='in',type='cl_mem'),
arg3=ArgDef(name='mapped_ptr',tag='in',type='void*',wrapType='CCLMappedPtr',wrapParams='{name}, false'),
arg4=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg5=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg6=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueWaitForEvents',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_events',tag='in',type='cl_uint'),
arg3=ArgDef(name='event_list',tag='in',type='const cl_event*',wrapParams='num_events, {name}')
)

Function(name='clEnqueueWriteBuffer',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_write',tag='in',type='cl_bool'),
arg4=ArgDef(name='offset',tag='in',type='size_t'),
arg5=ArgDef(name='cb',tag='in',type='size_t'),
arg6=ArgDef(name='ptr',tag='in',type='const void*',wrapType='CAsyncBinaryData',wrapParams='cb, {name}'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueWriteBufferRect',enabled=True,availableFrom='1.1',extension=False,type=Enqueue,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_write',tag='in',type='cl_bool'),
arg4=ArgDef(name='buffer_offset',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='host_offset',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg7=ArgDef(name='buffer_row_pitch',tag='in',type='size_t'),
arg8=ArgDef(name='buffer_slice_pitch',tag='in',type='size_t'),
arg9=ArgDef(name='host_row_pitch',tag='in',type='size_t'),
arg10=ArgDef(name='host_slice_pitch',tag='in',type='size_t'),
arg11=ArgDef(name='ptr',tag='in',type='const void*',wrapType='CAsyncBinaryData',wrapParams='CountBufferRectSize(region, host_row_pitch, host_slice_pitch), {name}'),
arg12=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg13=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg14=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueWriteImage',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,stateTrack=True,passToken=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='image',tag='in',type='cl_mem'),
arg3=ArgDef(name='blocking_write',tag='in',type='cl_bool'),
arg4=ArgDef(name='origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='input_row_pitch',tag='in',type='size_t'),
arg7=ArgDef(name='input_slice_pitch',tag='in',type='size_t'),
arg8=ArgDef(name='ptr',tag='in',type='const void*',wrapType='CAsyncBinaryData',wrapParams='{name} ? CountImageSize(SD().GetMemState(image, EXCEPTION_MESSAGE).image_format, region, input_row_pitch, input_slice_pitch) : 0, {name}'),
arg9=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg10=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg11=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clFinish',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue')
)

Function(name='clFlush',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue')
)

Function(name='clGetCommandQueueInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='param_name',tag='in',type='cl_command_queue_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetContextInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,runWrap=True,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='param_name',tag='in',type='cl_context_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CGetContextInfoOutArgument',wrapParams='param_value_size, {name}, param_name'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetDeviceIDs',enabled=True,availableFrom='1.0',extension=False,type=Info,stateTrack=True,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='device_type',tag='in',type='cl_device_type'),
arg3=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg4=ArgDef(name='devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries, {name}'),
arg5=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

#Function(name='clGetDeviceImageInfoQCOM',enabled=True,availableFrom='1.0',extension=False,type=None,
#retV=RetDef(type='cl_int'),
#arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
#arg2=ArgDef(name='image_width',tag='in',type='size_t'),
#arg3=ArgDef(name='image_height',tag='in',type='size_t'),
#arg4=ArgDef(name='image_format',tag='in',type='const cl_image_format*'),
#arg5=ArgDef(name='param_name',tag='in',type='cl_image_pitch_info_qcom'),
#arg6=ArgDef(name='param_value_size',tag='in',type='size_t'),
#arg7=ArgDef(name='param_value',tag='out',type='void*'),
#arg8=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
#)

Function(name='clGetDeviceInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,runWrap=True,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='param_name',tag='in',type='cl_device_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetEventInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='event',tag='in',type='cl_event'),
arg2=ArgDef(name='param_name',tag='in',type='cl_event_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetEventProfilingInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='event',tag='in',type='cl_event'),
arg2=ArgDef(name='param_name',tag='in',type='cl_profiling_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetExtensionFunctionAddress',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=None,recWrap=True,recExecWrap=True,
retV=RetDef(type='void*',wrapType='CvoidPtr'),
arg1=ArgDef(name='function_name',tag='in',type='const char*')
)

Function(name='clGetGLContextInfoKHR',enabled=True,availableFrom='1.0',extension=True,type=Info,runWrap=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='properties',tag='in',type='const cl_context_properties*',wrapParams='{name}, 0, 2'),
arg2=ArgDef(name='param_name',tag='in',type='cl_gl_context_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetGLObjectInfo',enabled=True,availableFrom='1.0',extension=True,type=Info,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='memobj',tag='in',type='cl_mem'),
arg2=ArgDef(name='gl_object_type',tag='out',type='cl_gl_object_type*'),
arg3=ArgDef(name='gl_object_name',tag='out',type='cl_GLuint*')
)

Function(name='clGetGLTextureInfo',enabled=True,availableFrom='1.0',extension=True,type=Info,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='memobj',tag='in',type='cl_mem'),
arg2=ArgDef(name='param_name',tag='in',type='cl_gl_texture_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetImageInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='image',tag='in',type='cl_mem'),
arg2=ArgDef(name='param_name',tag='in',type='cl_image_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetKernelInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='param_name',tag='in',type='cl_kernel_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetKernelSubGroupInfo',enabled=True,availableFrom='2.1',extension=False,type=Info,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='in_kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='in_device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='param_name',tag='in',type='cl_kernel_sub_group_info'),
arg4=ArgDef(name='input_value_size',tag='in',type='size_t'),
arg5=ArgDef(name='input_value',tag='in',type='const void*',wrapType='CBinaryData',wrapParams='input_value_size, {name}'),
arg6=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg7=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg8=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetKernelSubGroupInfoKHR',enabled=True,availableFrom='2.0',extension=True,inheritFrom='clGetKernelSubGroupInfo')

Function(name='clGetKernelWorkGroupInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='param_name',tag='in',type='cl_kernel_work_group_info'),
arg4=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg5=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg6=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetMemObjectInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='memobj',tag='in',type='cl_mem'),
arg2=ArgDef(name='param_name',tag='in',type='cl_mem_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetPlatformIDs',enabled=True,availableFrom='1.0',extension=False,type=Info,runWrap=True,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg2=ArgDef(name='platforms',tag='out',type='cl_platform_id*',wrapType='Ccl_platform_id::CSMapArray',wrapParams='num_entries, {name}'),
arg3=ArgDef(name='num_platforms',tag='out',type='cl_uint*')
)

Function(name='clGetPlatformInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='param_name',tag='in',type='cl_platform_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetProgramBuildInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='param_name',tag='in',type='cl_program_build_info'),
arg4=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg5=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg6=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetProgramInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,runWrap=True,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='param_name',tag='in',type='cl_program_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetSamplerInfo',enabled=True,availableFrom='1.0',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='sampler',tag='in',type='cl_sampler'),
arg2=ArgDef(name='param_name',tag='in',type='cl_sampler_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clGetSupportedImageFormats',enabled=True,availableFrom='1.0',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='image_type',tag='in',type='cl_mem_object_type'),
arg4=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg5=ArgDef(name='image_formats',tag='out',type='cl_image_format*',wrapParams='num_entries, {name}'),
arg6=ArgDef(name='num_image_formats',tag='out',type='cl_uint*')
)

#Function(name='clIcdGetPlatformIDsKHR',enabled=True,availableFrom='1.0',extension=True,type=None,
#retV=RetDef(type='cl_int'),
#arg1=ArgDef(name='num_entries',tag='in',type='cl_uint'),
#arg2=ArgDef(name='platforms',tag='out',type='cl_platform_id*'),
#arg3=ArgDef(name='num_platforms',tag='out',type='cl_uint*')
#)
#
#Function(name='clLogMessagesToStderrAPPLE',enabled=True,availableFrom='1.0',extension=True,type=None,
#retV=RetDef(type='void'),
#arg1=ArgDef(name='errstr',tag='in',type='const char*'),
#arg2=ArgDef(name='private_info',tag='in',type='const void*'),
#arg3=ArgDef(name='cb',tag='in',type='size_t'),
#arg4=ArgDef(name='user_data',tag='out',type='void*')
#)
#
#Function(name='clLogMessagesToStdoutAPPLE',enabled=True,availableFrom='1.0',extension=True,type=None,
#retV=RetDef(type='void'),
#arg1=ArgDef(name='errstr',tag='in',type='const char*'),
#arg2=ArgDef(name='private_info',tag='in',type='const void*'),
#arg3=ArgDef(name='cb',tag='in',type='size_t'),
#arg4=ArgDef(name='user_data',tag='out',type='void*')
#)
#
#Function(name='clLogMessagesToSystemLogAPPLE',enabled=True,availableFrom='1.0',extension=True,type=None,
#retV=RetDef(type='void'),
#arg1=ArgDef(name='errstr',tag='in',type='const char*'),
#arg2=ArgDef(name='private_info',tag='in',type='const void*'),
#arg3=ArgDef(name='cb',tag='in',type='size_t'),
#arg4=ArgDef(name='user_data',tag='out',type='void*')
#)

Function(name='clReleaseCommandQueue',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue',removeMapping=True)
)

Function(name='clReleaseContext',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context',removeMapping=True)
)

Function(name='clReleaseDevice',enabled=True,availableFrom='1.2',extension=False,type=Release,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id')
)

Function(name='clReleaseDeviceEXT',enabled=True,availableFrom='1.1.DEPRECATED',extension=True,inheritFrom='clReleaseDevice')

Function(name='clReleaseEvent',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='event',tag='in',type='cl_event',removeMapping=True)
)

Function(name='clReleaseKernel',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel',removeMapping=True)
)

Function(name='clReleaseMemObject',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='memobj',tag='in',type='cl_mem',removeMapping=True)
)

Function(name='clReleaseProgram',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program',removeMapping=True)
)

Function(name='clReleaseSampler',enabled=True,availableFrom='1.0',extension=False,type=Release,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='sampler',tag='in',type='cl_sampler',removeMapping=True)
)

Function(name='clRetainCommandQueue',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue')
)

Function(name='clRetainContext',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context')
)

Function(name='clRetainDevice',enabled=True,availableFrom='1.2',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id')
)

Function(name='clRetainDeviceEXT',enabled=True,availableFrom='1.1.DEPRECATED',extension=True,inheritFrom='clRetainDevice')

Function(name='clRetainEvent',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='event',tag='in',type='cl_event')
)

Function(name='clRetainKernel',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel')
)

Function(name='clRetainMemObject',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='memobj',tag='in',type='cl_mem')
)

Function(name='clRetainProgram',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program')
)

Function(name='clRetainSampler',enabled=True,availableFrom='1.0',extension=False,type=Retain,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='sampler',tag='in',type='cl_sampler')
)

Function(name='clSetEventCallback',enabled=True,availableFrom='1.1',extension=False,type=Set,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='event',tag='in',type='cl_event'),
arg2=ArgDef(name='command_exec_callback_type',tag='in',type='cl_int'),
arg3=ArgDef(name='pfn_notify',tag='in',type='CallbackEvent'),
arg4=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData')
)

Function(name='clSetCommandQueueProperty',enabled=True,availableFrom='1.0.DEPRECATED',extension=False,type=Set,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='properties',tag='in',type='cl_command_queue_properties'),
arg3=ArgDef(name='enable',tag='in',type='cl_bool'),
arg4=ArgDef(name='old_properties',tag='in',type='cl_command_queue_properties*')
)

Function(name='clSetKernelArg',enabled=True,availableFrom='1.0',extension=False,type=Set,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='arg_index',tag='in',type='cl_uint'),
arg3=ArgDef(name='arg_size',tag='in',type='size_t'),
arg4=ArgDef(name='arg_value',tag='in',type='const void*',wrapType='CKernelArgValue',wrapParams='{name} ? arg_size : 0, {name}')
)

Function(name='clSetKernelArg',enabled=True,availableFrom='1.0',extension=False,type=Set,stateTrack=True,version=1,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='arg_index',tag='in',type='cl_uint'),
arg3=ArgDef(name='arg_size',tag='in',type='size_t'),
arg4=ArgDef(name='arg_value',tag='in',type='const void*',wrapType='CKernelArgValue_V1',wrapParams='{name} ? arg_size : 0, {name}')
)

#Function(name='clSetMemObjectDestructorAPPLE',enabled=True,availableFrom='1.0',extension=True,type=Set,
#retV=RetDef(type='cl_int'),
#arg1=ArgDef(name='memobj',tag='in',type='cl_mem'),
#arg2=ArgDef(name='pfn_notify',tag='in',type='CallbackMem'),
#arg3=ArgDef(name='user_data',tag='in',type='void*')
#)
#
Function(name='clSetMemObjectDestructorCallback',enabled=True,availableFrom='1.1',extension=False,type=Set,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='memobj',tag='in',type='cl_mem'),
arg2=ArgDef(name='pfn_notify',tag='in',type='CallbackMem'),
arg3=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData')
)

Function(name='clSetUserEventStatus',enabled=True,availableFrom='1.1',extension=False,type=Set,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='event',tag='in',type='cl_event'),
arg2=ArgDef(name='execution_status',tag='in',type='cl_int')
)

#Function(name='clTerminateContextKHR',enabled=True,availableFrom='1.0',extension=True,type=None,
#retV=RetDef(type='cl_int'),
#arg1=ArgDef(name='context',tag='in',type='cl_context')
#)

Function(name='clUnloadCompiler',enabled=True,availableFrom='1.1.DEPRECATED',extension=False,type=None,
retV=RetDef(type='cl_int')
)

Function(name='clWaitForEvents',enabled=True,availableFrom='1.0',extension=False,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='num_events',tag='in',type='cl_uint'),
arg2=ArgDef(name='event_list',tag='in',type='const cl_event*',wrapParams='num_events, {name}')
)

Function(name='nanf',enabled=True,availableFrom='1.0',extension=False,type=None,
retV=RetDef(type='float'),
arg1=ArgDef(name='arg0',tag='in',type='const char*')
)

#------------------------------------- D3D10 -----------------------------------

Function(name='clGetDeviceIDsFromD3D10KHR',enabled=True,availableFrom='1.0',extension=True,type=Info,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='d3d_device_source',tag='in',type='cl_d3d10_device_source_khr'),
arg3=ArgDef(name='d3d_object',tag='in',type='void*',wrapType='CvoidPtr'),
arg4=ArgDef(name='d3d_device_set',tag='in',type='cl_d3d10_device_set_khr'),
arg5=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg6=ArgDef(name='devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries,{name}'),
arg7=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

Function(name='clCreateFromD3D10BufferKHR',enabled=True,availableFrom='1.0',extension=True,type=Creator,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='ID3D10Buffer*',wrapType='CvoidPtr'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromD3D10Texture2DKHR',enabled=True,availableFrom='1.0',extension=True,type=Creator,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='ID3D10Texture2D*',wrapType='CvoidPtr'),
arg4=ArgDef(name='subresource',tag='in',type='UINT'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromD3D10Texture3DKHR',enabled=True,availableFrom='1.0',extension=True,type=Creator,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='ID3D10Texture3D*',wrapType='CvoidPtr'),
arg4=ArgDef(name='subresource',tag='in',type='UINT'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueAcquireD3D10ObjectsKHR',enabled=True,availableFrom='1.0',extension=True,type=Enqueue,platform='Windows',stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_objects',tag='in',type='cl_uint'),
arg3=ArgDef(name='mem_objects',tag='in',type='const cl_mem*',wrapParams='num_objects,{name}'),
arg4=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg5=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list,{name}'),
arg6=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

# no multiple inheritance
Function(name='clEnqueueReleaseD3D10ObjectsKHR',enabled=True,availableFrom='1.0',extension=True,type=Enqueue,platform='Windows',stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_objects',tag='in',type='cl_uint'),
arg3=ArgDef(name='mem_objects',tag='in',type='const cl_mem*',wrapParams='num_objects,{name}'),
arg4=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg5=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list,{name}'),
arg6=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

#------------------------------------- D3D11 -----------------------------------

#------------- KHR -------------
Function(name='clGetDeviceIDsFromD3D11KHR',enabled=True,availableFrom='1.2',extension=True,type=Info,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='d3d_device_source',tag='in',type='cl_d3d11_device_source_khr'),
arg3=ArgDef(name='d3d_object',tag='in',type='void*',wrapType='CvoidPtr'),
arg4=ArgDef(name='d3d_device_set',tag='in',type='cl_d3d11_device_set_khr'),
arg5=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg6=ArgDef(name='devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries,{name}'),
arg7=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

Function(name='clCreateFromD3D11BufferKHR',enabled=True,availableFrom='1.2',extension=True,type=Creator,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='ID3D11Buffer*',wrapType='CvoidPtr'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromD3D11Texture2DKHR',enabled=True,availableFrom='1.2',extension=True,type=Creator,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='ID3D11Texture2D*',wrapType='CvoidPtr'),
arg4=ArgDef(name='subresource',tag='in',type='UINT'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateFromD3D11Texture3DKHR',enabled=True,availableFrom='1.2',extension=True,type=Creator,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='ID3D11Texture3D*',wrapType='CvoidPtr'),
arg4=ArgDef(name='subresource',tag='in',type='UINT'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueAcquireD3D11ObjectsKHR',enabled=True,availableFrom='1.2',extension=True,inheritFrom='clEnqueueAcquireD3D10ObjectsKHR')

Function(name='clEnqueueReleaseD3D11ObjectsKHR',enabled=True,availableFrom='1.2',extension=True,inheritFrom='clEnqueueReleaseD3D10ObjectsKHR')

#------------- NV --------------

Function(name='clGetDeviceIDsFromD3D11NV',enabled=True,availableFrom='1.0',extension=True,type=Info,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='d3d_device_source',tag='in',type='cl_d3d11_device_source_nv'),
arg3=ArgDef(name='d3d_object',tag='in',type='void*',wrapType='CvoidPtr'),
arg4=ArgDef(name='d3d_device_set',tag='in',type='cl_d3d11_device_set_nv'),
arg5=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg6=ArgDef(name='devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries,{name}'),
arg7=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

Function(name='clCreateFromD3D11BufferNV',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clCreateFromD3D11BufferKHR')

Function(name='clCreateFromD3D11Texture2DNV',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clCreateFromD3D11Texture2DKHR')

Function(name='clCreateFromD3D11Texture3DNV',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clCreateFromD3D11Texture3DKHR')

Function(name='clEnqueueAcquireD3D11ObjectsNV',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clEnqueueAcquireD3D10ObjectsKHR')

Function(name='clEnqueueReleaseD3D11ObjectsNV',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clEnqueueReleaseD3D10ObjectsKHR')

#-------------------------------------- DX9 ------------------------------------

Function(name='clGetDeviceIDsFromDX9INTEL',enabled=True,availableFrom='1.0',extension=True,type=Info,platform='Windows',recWrap=True,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='dx9_device_source',tag='in',type='cl_dx9_device_source_intel'),
arg3=ArgDef(name='dx9_object',tag='in',type='void*',wrapType='CvoidPtr'),
arg4=ArgDef(name='dx9_device_set',tag='in',type='cl_dx9_device_set_intel'),
arg5=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg6=ArgDef(name='devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries,{name}'),
arg7=ArgDef(name='num_devices',tag='out',type='cl_uint*')
)

Function(name='clCreateFromDX9MediaSurfaceINTEL',enabled=True,availableFrom='1.0',extension=True,type=Creator,platform='Windows',stateTrack=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='resource',tag='in',type='IDirect3DSurface9*',wrapType='CvoidPtr'),
arg4=ArgDef(name='shared_handle',tag='in',type='void*',wrapType='CvoidPtr'),
arg5=ArgDef(name='plane',tag='in',type='UINT'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueAcquireDX9ObjectsINTEL',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clEnqueueAcquireD3D10ObjectsKHR')

Function(name='clEnqueueReleaseDX9ObjectsINTEL',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clEnqueueReleaseD3D10ObjectsKHR')

Function(name='clGetDeviceIDsFromDX9MediaAdapterKHR',enabled=True,availableFrom='1.0',extension=True,type=Info,platform='Windows',recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='num_media_adapters',tag='in',type='cl_uint'),
arg3=ArgDef(name='media_adapters_type',tag='in',type='cl_dx9_media_adapter_type_khr*'),
arg4=ArgDef(name='media_adapters',tag='in',type='void*',wrapType='CvoidPtr'),
arg5=ArgDef(name='media_adapter_set',tag='in',type='cl_dx9_media_adapter_set_khr'),
arg6=ArgDef(name='num_entries',tag='in',type='cl_uint'),
arg7=ArgDef(name='devices',tag='out',type='cl_device_id*',wrapType='Ccl_device_id::CSMapArray',wrapParams='num_entries,{name}'),
arg8=ArgDef(name='num_devices',tag='out',type='cl_int*')
)

Function(name='clCreateFromDX9MediaSurfaceKHR',enabled=True,availableFrom='1.0',extension=True,type=Creator,platform='Windows',stateTrack=True,recWrap=True,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='adapter_type',tag='in',type='cl_dx9_media_adapter_type_khr'),
arg4=ArgDef(name='surface_info',tag='in',type='void*',wrapType='CvoidPtr'),
arg5=ArgDef(name='plane',tag='in',type='cl_uint'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueAcquireDX9MediaSurfacesKHR',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clEnqueueAcquireD3D10ObjectsKHR')

Function(name='clEnqueueReleaseDX9MediaSurfacesKHR',enabled=True,availableFrom='1.0',extension=True,inheritFrom='clEnqueueReleaseD3D10ObjectsKHR')

# ------------------------------------------ OCL1.2 ------------------------------------------

Function(name='clCompileProgram',enabled=True,availableFrom='1.2',extension=False,type=None,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='device_list',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='options',tag='in',type='const char*'),
arg5=ArgDef(name='num_input_headers',tag='in',type='cl_uint'),
arg6=ArgDef(name='input_headers',tag='in',type='const cl_program*',wrapParams='num_input_headers, {name}'),
arg7=ArgDef(name='header_include_names',tag='in',type='const char**',wrapType='CStringArray',wrapParams='num_input_headers, {name}'),
arg8=ArgDef(name='pfn_notify',tag='in',type='CallbackProgram'),
arg9=ArgDef(name='user_data',tag='out',type='void*',wrapType='CCLUserData')
)

Function(name='clCreateProgramWithBuiltInKernels',enabled=True,availableFrom='1.2',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='device_list',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='kernel_names',tag='in',type='const char*'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueBarrierWithWaitList',enabled=True,availableFrom='1.2',extension=False,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg3=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg4=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

# pattern probably is not too big so we can save it using CBinaryData
# this saves us some work in state tracking
Function(name='clEnqueueFillBuffer',enabled=True,availableFrom='1.2',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='buffer',tag='in',type='cl_mem'),
arg3=ArgDef(name='pattern',tag='in',type='const void*',wrapType='CBinaryData',wrapParams='pattern_size, {name}'),
arg4=ArgDef(name='pattern_size',tag='in',type='size_t'),
arg5=ArgDef(name='offset',tag='in',type='size_t'),
arg6=ArgDef(name='cb',tag='in',type='size_t'),
arg7=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg8=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg9=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueFillImage',enabled=True,availableFrom='1.2',extension=False,type=Enqueue,stateTrack=True,passToken=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='image',tag='in',type='cl_mem'),
arg3=ArgDef(name='fill_color',tag='in',type='const void*',wrapType='Ccl_float::CSArray',wrapParams='4, static_cast<const cl_float*>({name})'),
arg4=ArgDef(name='origin',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg5=ArgDef(name='region',tag='in',type='const size_t*',wrapParams='3, {name}'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMarkerWithWaitList',enabled=True,availableFrom='1.2',extension=False,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg3=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg4=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMigrateMemObjects',enabled=True,availableFrom='1.2',extension=False,type=Enqueue,stateTrack=True,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_mem_objects',tag='in',type='cl_uint'),
arg3=ArgDef(name='mem_objects',tag='in',type='const cl_mem*',wrapParams='num_mem_objects, {name}'),
arg4=ArgDef(name='flags',tag='in',type='cl_mem_migration_flags'),
arg5=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg6=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg7=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clGetExtensionFunctionAddressForPlatform',enabled=True,availableFrom='1.2',extension=False,type=None,recWrap=True,recExecWrap=True,stateTrack=True,
retV=RetDef(type='void*',wrapType='CvoidPtr'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id'),
arg2=ArgDef(name='function_name',tag='in',type='const char*')
)

Function(name='clAddCommentINTEL',enabled=True,availableFrom='2.0',extension=True,type=None,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='comment',tag='in',type='const char*')
)

Function(name='clEnqueueResourcesBarrierINTEL',enabled=True,availableFrom='1.2',extension=True,type=Enqueue,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='resource_count',tag='in',type='cl_uint'),
arg3=ArgDef(name='barrier_descriptors',tag='in',type='const cl_resource_barrier_descriptor_intel*'),
arg4=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg5=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg6=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clGetKernelArgInfo',enabled=True,availableFrom='1.2',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='arg_indx',tag='in',type='cl_uint'),
arg3=ArgDef(name='param_name',tag='in',type='cl_kernel_arg_info'),
arg4=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg5=ArgDef(name='param_value',tag='out',type='void*',wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg6=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clLinkProgram',enabled=True,availableFrom='1.2',extension=False,type=None,stateTrack=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='num_devices',tag='in',type='cl_uint'),
arg3=ArgDef(name='device_list',tag='in',type='const cl_device_id*',wrapParams='num_devices, {name}'),
arg4=ArgDef(name='options',tag='in',type='const char*'),
arg5=ArgDef(name='num_input_programs',tag='in',type='cl_uint'),
arg6=ArgDef(name='input_programs',tag='in',type='const cl_program*',wrapParams='num_input_programs, {name}'),
arg7=ArgDef(name='pfn_notify',tag='in',type='CallbackProgram'),
arg8=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData'),
arg9=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clUnloadPlatformCompiler',enabled=True,availableFrom='1.2',extension=False,type=None,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='platform',tag='in',type='cl_platform_id')
)

Function(name='clCreateCommandQueueWithPropertiesINTEL',enabled=True,availableFrom='1.2',extension=True,inheritFrom='clCreateCommandQueueWithProperties')

# ------------------------------------------ OCL2.0 ------------------------------------------

Function(name='clCreateCommandQueueWithProperties',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Creator,
retV=RetDef(type='cl_command_queue'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='properties',tag='in',type='const cl_queue_properties*',wrapParams='{name}, 0, 2'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreatePipe',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Creator,
retV=RetDef(type='cl_mem'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_mem_flags'),
arg3=ArgDef(name='pipe_packet_size',tag='in',type='cl_uint'),
arg4=ArgDef(name='pipe_max_packets',tag='in',type='cl_uint'),
arg5=ArgDef(name='properties',tag='in',type='const cl_pipe_properties*',wrapParams='{name}, 0, 2'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateSamplerWithProperties',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Creator,
retV=RetDef(type='cl_sampler'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='sampler_properties',tag='in',type='const cl_sampler_properties*',wrapParams='{name}, 0, 2'),
arg3=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clEnqueueSVMFree',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,recExecWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_svm_pointers',tag='in',type='cl_uint'),
arg3=ArgDef(name='svm_pointers',tag='in',type='void**',wrapType='CCLMappedPtr::CSArray',wrapParams='num_svm_pointers, {name}',removeMapping=True),
arg4=ArgDef(name='pfn_free_func',tag='in',type='CallbackSVM'),
arg5=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMMap',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,runWrap=True,recExecWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='blocking_map',tag='in',type='cl_bool'),
arg3=ArgDef(name='flags',tag='in',type='cl_map_flags'),
arg4=ArgDef(name='svm_ptr',tag='in',type='void*',wrapType='CCLMappedPtr'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMMemFill',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='svm_ptr',tag='in',type='void*',wrapType='CSVMPtr',wrapParams='{name}, size, SD().CheckIfSVMAllocExists({name})'),
arg3=ArgDef(name='pattern',tag='in',type='const void*',wrapType='CBinaryData',wrapParams='pattern_size, {name}'),
arg4=ArgDef(name='pattern_size',tag='in',type='size_t'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMMemFill',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,version=1,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='svm_ptr',tag='in',type='void*',wrapType='CSVMPtr_V1',wrapParams='{name}, size'),
arg3=ArgDef(name='pattern',tag='in',type='const void*',wrapType='CBinaryData',wrapParams='pattern_size, {name}'),
arg4=ArgDef(name='pattern_size',tag='in',type='size_t'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMemsetINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='dst_ptr',tag='in',type='void*',wrapType='CUSMPtr',wrapParams='{name}, size'),
arg3=ArgDef(name='value',tag='in',type='cl_int'),
arg4=ArgDef(name='size',tag='in',type='size_t'),
arg5=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg6=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg7=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMMemcpy',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='blocking_copy',tag='in',type='cl_bool'),
arg3=ArgDef(name='dst_ptr',tag='in',type='void*',wrapType='CSVMPtr',wrapParams='{name}, size, SD().CheckIfSVMAllocExists({name})'),
arg4=ArgDef(name='src_ptr',tag='in',type='const void*',wrapType='CSVMPtr',wrapParams='const_cast<void*>({name}), size, SD().CheckIfSVMAllocExists(const_cast<void*>({name}))'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMMemcpy',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,runWrap=True,version=1,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='blocking_copy',tag='in',type='cl_bool'),
arg3=ArgDef(name='dst_ptr',tag='in',type='void*',wrapType='CSVMPtr_V1',wrapParams='{name}, size'),
arg4=ArgDef(name='src_ptr',tag='in',type='const void*',wrapType='CSVMPtr_V1',wrapParams='const_cast<void*>({name}), size'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMUnmap',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Enqueue,recWrap=True,recExecWrap=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='svm_ptr',tag='in',type='void*',wrapType='CCLMappedPtr'),
arg3=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg4=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg5=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueSVMMigrateMem',enabled=True,availableFrom='2.1',extension=False,stateTrack=True,type=Enqueue,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='num_svm_pointers',tag='in',type='cl_uint'),
arg3=ArgDef(name='svm_pointers',tag='in',type='const void**',wrapType='CCLMappedPtr::CSArray',wrapParams='num_svm_pointers, const_cast<void**>({name})',removeMapping=True),
arg4=ArgDef(name='sizes',tag='in',type='const size_t*'),
arg5=ArgDef(name='flags',tag='in',type='cl_mem_migration_flags'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueVerifyMemoryINTEL',enabled=True,availableFrom='2.0',extension=True,type=Enqueue,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='allocationPtr',tag='in',type='const void*',wrapType='CAsyncBinaryData',wrapParams='sizeOfComparison, {name}'),
arg3=ArgDef(name='expectedData',tag='in',type='const void*',wrapType='CAsyncBinaryData',wrapParams='sizeOfComparison, {name}'),
arg4=ArgDef(name='sizeOfComparison',tag='in',type='size_t',),
arg5=ArgDef(name='comparisonMode',tag='in',type='cl_uint')
)

Function(name='clGetPipeInfo',enabled=True,availableFrom='2.0',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='pipe',tag='in',type='cl_mem'),
arg2=ArgDef(name='param_name',tag='in',type='cl_pipe_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='out',type='void*',wrapType='Ccl_uint::CSArray',wrapParams='param_value_size ? 1 : 0, static_cast<unsigned int*>({name})'),
arg5=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clSVMAlloc',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Creator,runWrap=True,recWrap=True,
retV=RetDef(type='void*',wrapType='CCLMappedPtr'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='flags',tag='in',type='cl_svm_mem_flags'),
arg3=ArgDef(name='size',tag='in',type='size_t'),
arg4=ArgDef(name='alignment',tag='in',type='cl_uint')
)

Function(name='clSVMFree',enabled=True,availableFrom='2.0',extension=False,stateTrack=True,type=Release,recExecWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='svm_pointer',tag='out',type='void*',wrapType='CCLMappedPtr',removeMapping=True)
)

Function(name='clSetKernelArgSVMPointer',enabled=True,availableFrom='2.0',extension=False,type=Set,stateTrack=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='arg_index',tag='in',type='cl_uint'),
arg3=ArgDef(name='arg_value',tag='in',type='const void*',wrapType="CCLMappedPtr",wrapParams='const_cast<void*>({name})')
)

Function(name='clSetKernelExecInfo',enabled=True,availableFrom='2.0',extension=False,type=Info,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='param_name',tag='in',type='cl_kernel_exec_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='in',type='const void*',wrapType='CCLKernelExecInfo',wrapParams='param_name, param_value, param_value_size')
)

Function(name='clSetKernelExecInfo',enabled=True,availableFrom='2.0',extension=False,type=Info,stateTrack=True,version=1,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='param_name',tag='in',type='cl_kernel_exec_info'),
arg3=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg4=ArgDef(name='param_value',tag='in',type='const void*',wrapType='CCLKernelExecInfo_V1',wrapParams='param_name, param_value, param_value_size')
)

Function(name='clHostMemAllocINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Creator,runWrap=True,recWrap=True,
retV=RetDef(type='void*',wrapType='CCLMappedPtr'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='properties',tag='in',type='cl_mem_properties_intel*', wrapParams='{name}, 0, 2'),
arg3=ArgDef(name='size',tag='in',type='size_t'),
arg4=ArgDef(name='alignment',tag='in',type='cl_uint'),
arg5=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clDeviceMemAllocINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Creator,runWrap=True,recWrap=True,
retV=RetDef(type='void*',wrapType='CCLMappedPtr'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='properties',tag='in',type='cl_mem_properties_intel*', wrapParams='{name}, 0, 2'),
arg4=ArgDef(name='size',tag='in',type='size_t'),
arg5=ArgDef(name='alignment',tag='in',type='cl_uint'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clSharedMemAllocINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Creator,runWrap=True,recWrap=True,
retV=RetDef(type='void*',wrapType='CCLMappedPtr'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='device',tag='in',type='cl_device_id'),
arg3=ArgDef(name='properties',tag='in',type='cl_mem_properties_intel*', wrapParams='{name}, 0, 2'),
arg4=ArgDef(name='size',tag='in',type='size_t'),
arg5=ArgDef(name='alignment',tag='in',type='cl_uint'),
arg6=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clMemFreeINTEL',enabled=True,availableFrom='2.0',extension=True,type=Release,stateTrack=True,recExecWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='ptr',tag='in',type='void*',wrapType='CCLMappedPtr',removeMapping=True)
)

Function(name='clMemBlockingFreeINTEL',enabled=True,availableFrom='2.0',extension=True,type=Release,stateTrack=True,recExecWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='ptr',tag='in',type='void*',wrapType='CCLMappedPtr',removeMapping=True)
)

Function(name='clGetMemAllocInfoINTEL',enabled=True,availableFrom='2.0',extension=True,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='ptr',tag='in',type='const void*',wrapType="CUSMPtr"),
arg3=ArgDef(name='param_name',tag='in',type='cl_mem_info_intel', wrapType='Ccl_mem_info_intel'),
arg4=ArgDef(name='param_value_size',tag='in',type='size_t'),
arg5=ArgDef(name='param_value',tag='out',type='void*', wrapType='CBinaryData',wrapParams='param_value_size, {name}'),
arg6=ArgDef(name='param_value_size_ret',tag='out',type='size_t*')
)

Function(name='clSetKernelArgMemPointerINTEL',enabled=True,availableFrom='2.0',extension=True,type=Set,stateTrack=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='arg_index',tag='in',type='cl_uint'),
arg3=ArgDef(name='arg_value',tag='in',type='const void*',wrapType="CUSMPtr")
)


Function(name='clEnqueueMemFillINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Enqueue,recWrap=True,recExecWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='dst_ptr',tag='in',type='void*',wrapType='CUSMPtr',wrapParams='{name}, size'),
arg3=ArgDef(name='pattern',tag='in',type='const void*',wrapType='CBinaryData',wrapParams='pattern_size, {name}'),
arg4=ArgDef(name='pattern_size',tag='in',type='size_t'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMemcpyINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Enqueue,recWrap=True,recExecWrap=True,runWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='blocking',tag='in',type='cl_bool'),
arg3=ArgDef(name='dst_ptr',tag='in',type='void*',wrapType='CUSMPtr',wrapParams='{name}, size'),
arg4=ArgDef(name='src_ptr',tag='in',type='const void*',wrapType='CUSMPtr',wrapParams='const_cast<void*>({name}), size'),
arg5=ArgDef(name='size',tag='in',type='size_t'),
arg6=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg7=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg8=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMigrateMemINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='ptr',tag='in',type='const void*',wrapType='CUSMPtr',wrapParams='const_cast<void*>({name}), size'),
arg3=ArgDef(name='size',tag='in',type='size_t'),
arg4=ArgDef(name='flags',tag='in',type='cl_mem_migration_flags_intel'),
arg5=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg6=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg7=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clEnqueueMemAdviseINTEL',enabled=True,availableFrom='2.0',extension=True,stateTrack=True,type=Enqueue,recWrap=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='ptr',tag='in',type='const void*',wrapType='CUSMPtr',wrapParams='const_cast<void*>({name}), size'),
arg3=ArgDef(name='size',tag='in',type='size_t'),
arg4=ArgDef(name='advice',tag='in',type='cl_mem_advice_intel'),
arg5=ArgDef(name='num_events_in_wait_list',tag='in',type='cl_uint'),
arg6=ArgDef(name='event_wait_list',tag='in',type='const cl_event*',wrapParams='num_events_in_wait_list, {name}'),
arg7=ArgDef(name='event',tag='out',type='cl_event*',wrapType='Ccl_event::CSMapArray',removeMapping=True)
)

Function(name='clGetKernelSuggestedLocalWorkSizeINTEL',enabled=True,availableFrom='1.0',extension=True,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='command_queue',tag='in',type='cl_command_queue'),
arg2=ArgDef(name='kernel',tag='in',type='cl_kernel'),
arg3=ArgDef(name='work_dim',tag='in',type='cl_uint'),
arg4=ArgDef(name='global_work_offset',tag='in',type='const size_t*',wrapParams='work_dim, {name}'),
arg5=ArgDef(name='global_work_size',tag='in',type='const size_t*',wrapParams='work_dim, {name}'),
arg6=ArgDef(name='suggested_local_work_size',tag='out',type='size_t*',wrapParams='work_dim, {name}')
)

Function(name='clGitsIndirectAllocationOffsets',enabled=True,availableFrom='1.0',extension=True,type=Info,recExecWrap=True,stateTrack=True,runWrap=True,recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='pAlloc',tag='in',type='void*',wrapType='CCLMappedPtr'),
arg2=ArgDef(name='numOffsets',tag='in',type='uint32_t'),
arg3=ArgDef(name='pOffsets',tag='out',type='size_t*',wrapParams='numOffsets, {name}')
)

Function(name='clSetProgramSpecializationConstant',enabled=True,availableFrom='2.2',extension=True,type=Set,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='program',tag='in',type='cl_program'),
arg2=ArgDef(name='spec_id',tag='in',type='cl_uint'),
arg3=ArgDef(name='spec_size',tag='in',type='size_t'),
arg4=ArgDef(name='spec_value',tag='in',type='const void*',wrapType='CBinaryData',wrapParams='spec_size, {name}')
)
# ------------------------------------------ OCL2.1 ------------------------------------------

Function(name='clCreateProgramWithIL',enabled=True,availableFrom='2.1',extension=False,type=Creator,passToken=True,stateTrack=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='il',tag='in',type='const void*',wrapType='CProgramSource', wrapParams='{name}, length'),
arg3=ArgDef(name='length',tag='in',type='size_t'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clCreateProgramWithILKHR',enabled=True,availableFrom='2.0',extension=True,type=Creator,passToken=True,stateTrack=True,
retV=RetDef(type='cl_program'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='il',tag='in',type='const void*',wrapType='CProgramSource', wrapParams='{name}, length'),
arg3=ArgDef(name='length',tag='in',type='size_t'),
arg4=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clGetDeviceAndHostTimer',enabled=True,availableFrom='2.1',extension=False,type=Creator,passToken=True,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='device_timestamp',tag='out',type='cl_ulong*'),
arg3=ArgDef(name='host_timestamp',tag='out',type='cl_ulong*')
)

Function(name='clGetHostTimer',enabled=True,availableFrom='2.1',extension=False,type=Info,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='device',tag='in',type='cl_device_id'),
arg2=ArgDef(name='host_timestamp',tag='out',type='cl_ulong*')
)

Function(name='clCloneKernel',enabled=True,availableFrom='2.1',extension=False,type=Creator,stateTrack=True,
retV=RetDef(type='cl_kernel'),
arg1=ArgDef(name='source_kernel',tag='in',type='cl_kernel'),
arg2=ArgDef(name='errcode_ret',tag='out',type='cl_int*')
)

Function(name='clSetContextDestructorCallback',enabled=False,availableFrom='3.0',extension=False,type=Set,
retV=RetDef(type='cl_int'),
arg1=ArgDef(name='context',tag='in',type='cl_context'),
arg2=ArgDef(name='pfn_notify',tag='in',type='CallbackContext'),
arg3=ArgDef(name='user_data',tag='in',type='void*',wrapType='CCLUserData')
)

# ------------------------------------------ enums ------------------------------------------

Enum(name='CLResult', type='cl_int',
var1=VarDef(name='CL_SUCCESS', value='0'),
var2=VarDef(name='CL_DEVICE_NOT_FOUND', value='-1'),
var3=VarDef(name='CL_DEVICE_NOT_AVAILABLE', value='-2'),
var4=VarDef(name='CL_COMPILER_NOT_AVAILABLE', value='-3'),
var5=VarDef(name='CL_MEM_OBJECT_ALLOCATION_FAILURE', value='-4'),
var6=VarDef(name='CL_OUT_OF_RESOURCES', value='-5'),
var7=VarDef(name='CL_OUT_OF_HOST_MEMORY', value='-6'),
var8=VarDef(name='CL_PROFILING_INFO_NOT_AVAILABLE', value='-7'),
var9=VarDef(name='CL_MEM_COPY_OVERLAP', value='-8'),
var10=VarDef(name='CL_IMAGE_FORMAT_MISMATCH', value='-9'),
var11=VarDef(name='CL_IMAGE_FORMAT_NOT_SUPPORTED', value='-10'),
var12=VarDef(name='CL_BUILD_PROGRAM_FAILURE', value='-11'),
var13=VarDef(name='CL_MAP_FAILURE', value='-12'),
var14=VarDef(name='CL_MISALIGNED_SUB_BUFFER_OFFSET', value='-13'),
var15=VarDef(name='CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST', value='-14'),
var16=VarDef(name='CL_COMPILE_PROGRAM_FAILURE', value='-15'),
var17=VarDef(name='CL_LINKER_NOT_AVAILABLE', value='-16'),
var18=VarDef(name='CL_LINK_PROGRAM_FAILURE', value='-17'),
var19=VarDef(name='CL_DEVICE_PARTITION_FAILED', value='-18'),
var20=VarDef(name='CL_KERNEL_ARG_INFO_NOT_AVAILABLE', value='-19'),
var21=VarDef(name='CL_INVALID_VALUE', value='-30'),
var22=VarDef(name='CL_INVALID_DEVICE_TYPE', value='-31'),
var23=VarDef(name='CL_INVALID_PLATFORM', value='-32'),
var24=VarDef(name='CL_INVALID_DEVICE', value='-33'),
var25=VarDef(name='CL_INVALID_CONTEXT', value='-34'),
var26=VarDef(name='CL_INVALID_QUEUE_PROPERTIES', value='-35'),
var27=VarDef(name='CL_INVALID_COMMAND_QUEUE', value='-36'),
var28=VarDef(name='CL_INVALID_HOST_PTR', value='-37'),
var29=VarDef(name='CL_INVALID_MEM_OBJECT', value='-38'),
var30=VarDef(name='CL_INVALID_IMAGE_FORMAT_DESCRIPTOR', value='-39'),
var31=VarDef(name='CL_INVALID_IMAGE_SIZE', value='-40'),
var32=VarDef(name='CL_INVALID_SAMPLER', value='-41'),
var33=VarDef(name='CL_INVALID_BINARY', value='-42'),
var34=VarDef(name='CL_INVALID_BUILD_OPTIONS', value='-43'),
var35=VarDef(name='CL_INVALID_PROGRAM', value='-44'),
var36=VarDef(name='CL_INVALID_PROGRAM_EXECUTABLE', value='-45'),
var37=VarDef(name='CL_INVALID_KERNEL_NAME', value='-46'),
var38=VarDef(name='CL_INVALID_KERNEL_DEFINITION', value='-47'),
var39=VarDef(name='CL_INVALID_KERNEL', value='-48'),
var40=VarDef(name='CL_INVALID_ARG_INDEX', value='-49'),
var41=VarDef(name='CL_INVALID_ARG_VALUE', value='-50'),
var42=VarDef(name='CL_INVALID_ARG_SIZE', value='-51'),
var43=VarDef(name='CL_INVALID_KERNEL_ARGS', value='-52'),
var44=VarDef(name='CL_INVALID_WORK_DIMENSION', value='-53'),
var45=VarDef(name='CL_INVALID_WORK_GROUP_SIZE', value='-54'),
var46=VarDef(name='CL_INVALID_WORK_ITEM_SIZE', value='-55'),
var47=VarDef(name='CL_INVALID_GLOBAL_OFFSET', value='-56'),
var48=VarDef(name='CL_INVALID_EVENT_WAIT_LIST', value='-57'),
var49=VarDef(name='CL_INVALID_EVENT', value='-58'),
var50=VarDef(name='CL_INVALID_OPERATION', value='-59'),
var51=VarDef(name='CL_INVALID_GL_OBJECT', value='-60'),
var52=VarDef(name='CL_INVALID_BUFFER_SIZE', value='-61'),
var53=VarDef(name='CL_INVALID_MIP_LEVEL', value='-62'),
var54=VarDef(name='CL_INVALID_GLOBAL_WORK_SIZE', value='-63'),
var55=VarDef(name='CL_INVALID_PROPERTY', value='-64'),
var56=VarDef(name='CL_INVALID_IMAGE_DESCRIPTOR', value='-65'),
var57=VarDef(name='CL_INVALID_COMPILER_OPTIONS', value='-66'),
var58=VarDef(name='CL_INVALID_LINKER_OPTIONS', value='-67'),
var59=VarDef(name='CL_INVALID_DEVICE_PARTITION_COUNT', value='-68'),
var60=VarDef(name='CL_INVALID_PIPE_SIZE', value='-69'),
var61=VarDef(name='CL_INVALID_DEVICE_QUEUE', value='-70'),
var62=VarDef(name='CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR', value='-1000'),
var63=VarDef(name='CL_INVALID_D3D10_DEVICE_KHR', value='-1002'),
var64=VarDef(name='CL_INVALID_D3D10_RESOURCE_KHR', value='-1003'),
var65=VarDef(name='CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR', value='-1004'),
var66=VarDef(name='CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR', value='-1005'),
var67=VarDef(name='CL_INVALID_D3D11_DEVICE_KHR', value='-1006'),
var68=VarDef(name='CL_INVALID_D3D11_RESOURCE_KHR', value='-1007'),
var69=VarDef(name='CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR', value='-1008'),
var70=VarDef(name='CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR', value='-1009'),
var71=VarDef(name='CL_INVALID_DX9_MEDIA_ADAPTER_KHR', value='-1010'),
var72=VarDef(name='CL_INVALID_DX9_MEDIA_SURFACE_KHR', value='-1011'),
var73=VarDef(name='CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR', value='-1012'),
var74=VarDef(name='CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR', value='-1013')
)

Enum(name='cl_bool',
var1=VarDef(name='CL_FALSE', value='0'),
var2=VarDef(name='CL_TRUE', value='1'),
)

Enum(name='cl_platform_info',
var1=VarDef(name='CL_PLATFORM_PROFILE', value='0x0900'),
var2=VarDef(name='CL_PLATFORM_VERSION', value='0x0901'),
var3=VarDef(name='CL_PLATFORM_NAME', value='0x0902'),
var4=VarDef(name='CL_PLATFORM_VENDOR', value='0x0903'),
var5=VarDef(name='CL_PLATFORM_EXTENSIONS', value='0x0904'),
var6=VarDef(name='CL_PLATFORM_HOST_TIMER_RESOLUTION', value='0x0905')
)

Enum(name='cl_device_type',bitfield=True,
var1=VarDef(name='CL_DEVICE_TYPE_DEFAULT', value='(1 << 0)'),
var2=VarDef(name='CL_DEVICE_TYPE_CPU', value='(1 << 1)'),
var3=VarDef(name='CL_DEVICE_TYPE_GPU', value='(1 << 2)'),
var4=VarDef(name='CL_DEVICE_TYPE_ACCELERATOR', value='(1 << 3)'),
var5=VarDef(name='CL_DEVICE_TYPE_CUSTOM', value='(1 << 4)'),
var6=VarDef(name='CL_DEVICE_TYPE_ALL', value='0xFFFFFFFF')
)

Enum(name='cl_device_info',
var1=VarDef(name='CL_DEVICE_TYPE', value='0x1000'),
var2=VarDef(name='CL_DEVICE_VENDOR_ID', value='0x1001'),
var3=VarDef(name='CL_DEVICE_MAX_COMPUTE_UNITS', value='0x1002'),
var4=VarDef(name='CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS', value='0x1003'),
var5=VarDef(name='CL_DEVICE_MAX_WORK_GROUP_SIZE', value='0x1004'),
var6=VarDef(name='CL_DEVICE_MAX_WORK_ITEM_SIZES', value='0x1005'),
var7=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR', value='0x1006'),
var8=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT', value='0x1007'),
var9=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT', value='0x1008'),
var10=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG', value='0x1009'),
var11=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT', value='0x100A'),
var12=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE', value='0x100B'),
var13=VarDef(name='CL_DEVICE_MAX_CLOCK_FREQUENCY', value='0x100C'),
var14=VarDef(name='CL_DEVICE_ADDRESS_BITS', value='0x100D'),
var15=VarDef(name='CL_DEVICE_MAX_READ_IMAGE_ARGS', value='0x100E'),
var16=VarDef(name='CL_DEVICE_MAX_WRITE_IMAGE_ARGS', value='0x100F'),
var17=VarDef(name='CL_DEVICE_MAX_MEM_ALLOC_SIZE', value='0x1010'),
var18=VarDef(name='CL_DEVICE_IMAGE2D_MAX_WIDTH', value='0x1011'),
var19=VarDef(name='CL_DEVICE_IMAGE2D_MAX_HEIGHT', value='0x1012'),
var20=VarDef(name='CL_DEVICE_IMAGE3D_MAX_WIDTH', value='0x1013'),
var21=VarDef(name='CL_DEVICE_IMAGE3D_MAX_HEIGHT', value='0x1014'),
var22=VarDef(name='CL_DEVICE_IMAGE3D_MAX_DEPTH', value='0x1015'),
var23=VarDef(name='CL_DEVICE_IMAGE_SUPPORT', value='0x1016'),
var24=VarDef(name='CL_DEVICE_MAX_PARAMETER_SIZE', value='0x1017'),
var25=VarDef(name='CL_DEVICE_MAX_SAMPLERS', value='0x1018'),
var26=VarDef(name='CL_DEVICE_MEM_BASE_ADDR_ALIGN', value='0x1019'),
var27=VarDef(name='CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE', value='0x101A'),
var28=VarDef(name='CL_DEVICE_SINGLE_FP_CONFIG', value='0x101B'),
var29=VarDef(name='CL_DEVICE_GLOBAL_MEM_CACHE_TYPE', value='0x101C'),
var30=VarDef(name='CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE', value='0x101D'),
var31=VarDef(name='CL_DEVICE_GLOBAL_MEM_CACHE_SIZE', value='0x101E'),
var32=VarDef(name='CL_DEVICE_GLOBAL_MEM_SIZE', value='0x101F'),
var33=VarDef(name='CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE', value='0x1020'),
var34=VarDef(name='CL_DEVICE_MAX_CONSTANT_ARGS', value='0x1021'),
var35=VarDef(name='CL_DEVICE_LOCAL_MEM_TYPE', value='0x1022'),
var36=VarDef(name='CL_DEVICE_LOCAL_MEM_SIZE', value='0x1023'),
var37=VarDef(name='CL_DEVICE_ERROR_CORRECTION_SUPPORT', value='0x1024'),
var38=VarDef(name='CL_DEVICE_PROFILING_TIMER_RESOLUTION', value='0x1025'),
var39=VarDef(name='CL_DEVICE_ENDIAN_LITTLE', value='0x1026'),
var40=VarDef(name='CL_DEVICE_AVAILABLE', value='0x1027'),
var41=VarDef(name='CL_DEVICE_COMPILER_AVAILABLE', value='0x1028'),
var42=VarDef(name='CL_DEVICE_EXECUTION_CAPABILITIES', value='0x1029'),
#var43=VarDef(name='CL_DEVICE_QUEUE_PROPERTIES', value='0x102A'), # deprecated
var44=VarDef(name='CL_DEVICE_QUEUE_ON_HOST_PROPERTIES', value='0x102A'),
var45=VarDef(name='CL_DEVICE_NAME', value='0x102B'),
var46=VarDef(name='CL_DEVICE_VENDOR', value='0x102C'),
var47=VarDef(name='CL_DRIVER_VERSION', value='0x102D'),
var48=VarDef(name='CL_DEVICE_PROFILE', value='0x102E'),
var49=VarDef(name='CL_DEVICE_VERSION', value='0x102F'),
var50=VarDef(name='CL_DEVICE_EXTENSIONS', value='0x1030'),
var51=VarDef(name='CL_DEVICE_PLATFORM', value='0x1031'),
var52=VarDef(name='CL_DEVICE_DOUBLE_FP_CONFIG', value='0x1032'),
var53=VarDef(name='CL_DEVICE_HALF_FP_CONFIG', value='0x1033'),
var54=VarDef(name='CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF', value='0x1034'),
var55=VarDef(name='CL_DEVICE_HOST_UNIFIED_MEMORY', value='0x1035'),
var56=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR', value='0x1036'),
var57=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT', value='0x1037'),
var58=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_INT', value='0x1038'),
var59=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG', value='0x1039'),
var60=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT', value='0x103A'),
var61=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE', value='0x103B'),
var62=VarDef(name='CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF', value='0x103C'),
var63=VarDef(name='CL_DEVICE_OPENCL_C_VERSION', value='0x103D'),
var64=VarDef(name='CL_DEVICE_LINKER_AVAILABLE', value='0x103E'),
var65=VarDef(name='CL_DEVICE_BUILT_IN_KERNELS', value='0x103F'),
var66=VarDef(name='CL_DEVICE_IMAGE_MAX_BUFFER_SIZE', value='0x1040'),
var67=VarDef(name='CL_DEVICE_IMAGE_MAX_ARRAY_SIZE', value='0x1041'),
var68=VarDef(name='CL_DEVICE_PARENT_DEVICE', value='0x1042'),
var69=VarDef(name='CL_DEVICE_PARTITION_MAX_SUB_DEVICES', value='0x1043'),
var70=VarDef(name='CL_DEVICE_PARTITION_PROPERTIES', value='0x1044'),
var71=VarDef(name='CL_DEVICE_PARTITION_AFFINITY_DOMAIN', value='0x1045'),
var72=VarDef(name='CL_DEVICE_PARTITION_TYPE', value='0x1046'),
var73=VarDef(name='CL_DEVICE_REFERENCE_COUNT', value='0x1047'),
var74=VarDef(name='CL_DEVICE_PREFERRED_INTEROP_USER_SYNC', value='0x1048'),
var75=VarDef(name='CL_DEVICE_PRINTF_BUFFER_SIZE', value='0x1049'),
var76=VarDef(name='CL_DEVICE_IMAGE_PITCH_ALIGNMENT', value='0x104A'),
var77=VarDef(name='CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT', value='0x104B'),
var78=VarDef(name='CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS', value='0x104C'),
var79=VarDef(name='CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE', value='0x104D'),
var80=VarDef(name='CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES', value='0x104E'),
var81=VarDef(name='CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE', value='0x104F'),
var82=VarDef(name='CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE', value='0x1050'),
var83=VarDef(name='CL_DEVICE_MAX_ON_DEVICE_QUEUES', value='0x1051'),
var84=VarDef(name='CL_DEVICE_MAX_ON_DEVICE_EVENTS', value='0x1052'),
var85=VarDef(name='CL_DEVICE_SVM_CAPABILITIES', value='0x1053'),
var86=VarDef(name='CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE', value='0x1054'),
var87=VarDef(name='CL_DEVICE_MAX_PIPE_ARGS', value='0x1055'),
var88=VarDef(name='CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS', value='0x1056'),
var89=VarDef(name='CL_DEVICE_PIPE_MAX_PACKET_SIZE', value='0x1057'),
var90=VarDef(name='CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT', value='0x1058'),
var91=VarDef(name='CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT', value='0x1059'),
var92=VarDef(name='CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT', value='0x105A'),
var93=VarDef(name='CL_DEVICE_IL_VERSION', value='0x105B'),
var94=VarDef(name='CL_DEVICE_MAX_NUM_SUB_GROUPS', value='0x105C'),
var95=VarDef(name='CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS', value='0x105D'),
# cl_amd_device_attribute_query
var96=VarDef(name='CL_DEVICE_PROFILING_TIMER_OFFSET_AMD', value='0x4036'),
var97=VarDef(name='CL_DEVICE_TOPOLOGY_AMD', value='0x4037'),
var98=VarDef(name='CL_DEVICE_BOARD_NAME_AMD', value='0x4038'),
var99=VarDef(name='CL_DEVICE_GLOBAL_FREE_MEMORY_AMD', value='0x4039'),
var100=VarDef(name='CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD', value='0x4040'),
var101=VarDef(name='CL_DEVICE_SIMD_WIDTH_AMD', value='0x4041'),
var102=VarDef(name='CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD', value='0x4042'),
var103=VarDef(name='CL_DEVICE_WAVEFRONT_WIDTH_AMD', value='0x4043'),
var104=VarDef(name='CL_DEVICE_GLOBAL_MEM_CHANNELS_AMD', value='0x4044'),
var105=VarDef(name='CL_DEVICE_GLOBAL_MEM_CHANNEL_BANKS_AMD', value='0x4045'),
var106=VarDef(name='CL_DEVICE_GLOBAL_MEM_CHANNEL_BANK_WIDTH_AMD', value='0x4046'),
var107=VarDef(name='CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD', value='0x4047'),
var108=VarDef(name='CL_DEVICE_LOCAL_MEM_BANKS_AMD', value='0x4048'),
var109=VarDef(name='CL_DEVICE_THREAD_TRACE_SUPPORTED_AMD', value='0x4049'),
var110=VarDef(name='CL_DEVICE_GFXIP_MAJOR_AMD', value='0x404A'),
var111=VarDef(name='CL_DEVICE_GFXIP_MINOR_AMD', value='0x404B'),
var112=VarDef(name='CL_DEVICE_AVAILABLE_ASYNC_QUEUES_AMD', value='0x404C'),
# cl_nv_device_attribute_query
var113=VarDef(name='CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV', value='0x4000'),
var114=VarDef(name='CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV', value='0x4001'),
var115=VarDef(name='CL_DEVICE_REGISTERS_PER_BLOCK_NV', value='0x4002'),
var116=VarDef(name='CL_DEVICE_WARP_SIZE_NV', value='0x4003'),
var117=VarDef(name='CL_DEVICE_GPU_OVERLAP_NV', value='0x4004'),
var118=VarDef(name='CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV', value='0x4005'),
var119=VarDef(name='CL_DEVICE_INTEGRATED_MEMORY_NV', value='0x4006'),
# cl_intel_command_queue_properties
var120=VarDef(name='CL_DEVICE_NUM_TILES_INTEL', value='0x10000'),
var121=VarDef(name='CL_DEVICE_NUM_QUEUE_FAMILIES_INTEL', value='0x10009'),
# cl_device_unified_shared_memory_capabilities_intel
var122=VarDef(name='CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL', value='0x4190'),
var123=VarDef(name='CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL', value='0x4191'),
var124=VarDef(name='CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL', value='0x4192'),
var125=VarDef(name='CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL', value='0x4193'),
var126=VarDef(name='CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL', value='0x4194'),
#cl_intel_required_subgroup_size
var127=VarDef(name='CL_DEVICE_SUB_GROUP_SIZES_INTEL', value='0x4108')
)

Enum(name='cl_device_fp_config',bitfield=True,
var1=VarDef(name='CL_FP_DENORM', value='(1 << 0)'),
var2=VarDef(name='CL_FP_INF_NAN', value='(1 << 1)'),
var3=VarDef(name='CL_FP_ROUND_TO_NEAREST', value='(1 << 2)'),
var4=VarDef(name='CL_FP_ROUND_TO_ZERO', value='(1 << 3)'),
var5=VarDef(name='CL_FP_ROUND_TO_INF', value='(1 << 4)'),
var6=VarDef(name='CL_FP_FMA', value='(1 << 5)'),
var7=VarDef(name='CL_FP_SOFT_FLOAT', value='(1 << 6)'),
var8=VarDef(name='CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT', value='(1 << 7)')
)

Enum(name='cl_device_mem_cache_type',
var1=VarDef(name='CL_NONE', value='0x0'),
var2=VarDef(name='CL_READ_ONLY_CACHE', value='0x1'),
var3=VarDef(name='CL_READ_WRITE_CACHE', value='0x2')
)

Enum(name='cl_device_local_mem_type',
var1=VarDef(name='CL_LOCAL', value='0x1'),
var2=VarDef(name='CL_GLOBAL', value='0x2')
)

Enum(name='cl_device_exec_capabilities',bitfield=True,
var1=VarDef(name='CL_EXEC_KERNEL', value='(1 << 0)'),
var2=VarDef(name='CL_EXEC_NATIVE_KERNEL', value='(1 << 1)')
)

Enum(name='cl_command_queue_properties',bitfield=True,
var1=VarDef(name='CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE', value='(1 << 0)'),
var2=VarDef(name='CL_QUEUE_PROFILING_ENABLE', value='(1 << 1)'),
var3=VarDef(name='CL_QUEUE_ON_DEVICE', value='(1 << 2)'),
var4=VarDef(name='CL_QUEUE_ON_DEVICE_DEFAULT', value='(1 << 3)')
)

Enum(name='cl_context_info',
var1=VarDef(name='CL_CONTEXT_REFERENCE_COUNT', value='0x1080'),
var2=VarDef(name='CL_CONTEXT_DEVICES', value='0x1081'),
var3=VarDef(name='CL_CONTEXT_PROPERTIES', value='0x1082'),
var4=VarDef(name='CL_CONTEXT_NUM_DEVICES', value='0x1083'),
var5=VarDef(name='CL_CONTEXT_ADAPTER_D3D9_KHR', value='0x2025'),
var6=VarDef(name='CL_CONTEXT_ADAPTER_D3D9EX_KHR', value='0x2026'),
var7=VarDef(name='CL_CONTEXT_ADAPTER_DXVA_KHR', value='0x2027'),
var8=VarDef(name='CL_CONTEXT_D3D9_DEVICE_INTEL', value='0x4026'),
var9=VarDef(name='CL_CONTEXT_D3D9EX_DEVICE_INTEL', value='0x4072'),
var10=VarDef(name='CL_CONTEXT_DXVA_DEVICE_INTEL', value='0x4073'),
var11=VarDef(name='CL_CONTEXT_D3D10_DEVICE_KHR', value='0x4014'),
var12=VarDef(name='CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR', value='0x402C'),
var13=VarDef(name='CL_CONTEXT_D3D11_DEVICE_KHR', value='0x401D'),
var14=VarDef(name='CL_CONTEXT_D3D11_PREFER_SHARED_RESOURCES_KHR', value='0x402D')
)

Enum(name='cl_gl_context_info',
var1=VarDef(name='CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR', value='0x2006'),
var2=VarDef(name='CL_DEVICES_FOR_GL_CONTEXT_KHR', value='0x2007')
)

Enum(name='cl_context_properties', custom_ccode=True,
var1=VarDef(name='CL_CONTEXT_PLATFORM', value='0x1084'),
var2=VarDef(name='CL_CONTEXT_INTEROP_USER_SYNC', value='0x1085'),
var3=VarDef(name='CL_GL_CONTEXT_KHR', value='0x2008'),
var4=VarDef(name='CL_EGL_DISPLAY_KHR', value='0x2009'),
var5=VarDef(name='CL_GLX_DISPLAY_KHR', value='0x200A'),
var6=VarDef(name='CL_WGL_HDC_KHR', value='0x200B'),
var7=VarDef(name='CL_CGL_SHAREGROUP_KHR', value='0x200C'),
# cl_intel_context_properties
var8=VarDef(name='CL_CONTEXT_FLAGS_INTEL', value='0x10003'),
var9=VarDef(name='CL_CONTEXT_ALLOW_ONLY_MASKED_QUEUE_INTEL', value='0x10004'),
var10=VarDef(name='CL_CONTEXT_ALLOW_ONLY_SINGLE_TILE_QUEUES_INTEL', value='0x10005'),
var11=VarDef(name='CL_CONTEXT_ALLOW_ALL_KIND_OF_QUEUES_INTEL', value='0x10006'),
)

Enum(name='cl_mem_properties_intel',
var1=VarDef(name='CL_MEM_FLAGS_INTEL', value='0x10001'),
var2=VarDef(name='CL_MEM_FLAGS', value='0x1101'),
var3=VarDef(name='CL_MEM_ALLOC_FLAGS_INTEL', value='0x4195'),
var4=VarDef(name='CL_MEM_DEVICE_ID_INTEL', value='0x10011')
)

Enum(name='cl_unified_shared_memory_type_intel',
var1=VarDef(name='CL_MEM_TYPE_UNKNOWN_INTEL', value='0x4196'),
var2=VarDef(name='CL_MEM_TYPE_HOST_INTEL', value='0x4197'),
var3=VarDef(name='CL_MEM_TYPE_DEVICE_INTEL', value='0x4198'),
var4=VarDef(name='CL_MEM_TYPE_SHARED_INTEL', value='0x4199')
)

Enum(name='cl_mem_advice_intel',
var1=VarDef(name='CL_MEM_ADVICE_TBD0_INTEL', value='0x4208'),
var2=VarDef(name='CL_MEM_ADVICE_TBD1_INTEL', value='0x4209'),
var3=VarDef(name='CL_MEM_ADVICE_TBD2_INTEL', value='0x420A'),
var4=VarDef(name='CL_MEM_ADVICE_TBD3_INTEL', value='0x420B'),
var5=VarDef(name='CL_MEM_ADVICE_TBD4_INTEL', value='0x420C'),
var6=VarDef(name='CL_MEM_ADVICE_TBD5_INTEL', value='0x420D'),
var7=VarDef(name='CL_MEM_ADVICE_TBD6_INTEL', value='0x420E'),
var8=VarDef(name='CL_MEM_ADVICE_TBD7_INTEL', value='0x420F')
)

Enum(name='cl_device_partition_property', custom_tostring=True,
var1=VarDef(name='CL_DEVICE_PARTITION_EQUALLY', value='0x1086'),
var2=VarDef(name='CL_DEVICE_PARTITION_BY_COUNTS', value='0x1087'),
var3=VarDef(name='CL_DEVICE_PARTITION_BY_COUNTS_LIST_END', value='0x0'),
var4=VarDef(name='CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN', value='0x1088')
)

Enum(name='cl_device_partition_property_ext', custom_tostring=True,
var1=VarDef(name='CL_DEVICE_PARTITION_EQUALLY_EXT', value='0x4050'),
var2=VarDef(name='CL_DEVICE_PARTITION_BY_COUNTS_EXT', value='0x4051'),
var3=VarDef(name='CL_DEVICE_PARTITION_BY_COUNTS_LIST_END_EXT', value='0x0'),
var4=VarDef(name='CL_DEVICE_PARTITION_BY_NAMES_EXT', value='0x4052'),
var5=VarDef(name='CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT', value='0x4053')
# -Wc++11-narrowing
#var6=VarDef(name='CL_DEVICE_PARTITION_BY_NAMES_LIST_END_EXT', value='-1'),
)

Enum(name='cl_device_affinity_domain',bitfield=True,
var1=VarDef(name='CL_DEVICE_AFFINITY_DOMAIN_NUMA', value='(1 << 0)'),
var2=VarDef(name='CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE', value='(1 << 1)'),
var3=VarDef(name='CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE', value='(1 << 2)'),
var4=VarDef(name='CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE', value='(1 << 3)'),
var5=VarDef(name='CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE', value='(1 << 4)'),
var6=VarDef(name='CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE', value='(1 << 5)')
)

# there is no typedef for cl_affinity_domain_ext
Enum(name='cl_affinity_domain_ext', type='cl_ulong', custom_argument=True,
var1=VarDef(name='CL_AFFINITY_DOMAIN_L4_CACHE_EXT', value='0x1'),
var2=VarDef(name='CL_AFFINITY_DOMAIN_L3_CACHE_EXT', value='0x2'),
var3=VarDef(name='CL_AFFINITY_DOMAIN_L2_CACHE_EXT', value='0x3'),
var4=VarDef(name='CL_AFFINITY_DOMAIN_L1_CACHE_EXT', value='0x4'),
var5=VarDef(name='CL_AFFINITY_DOMAIN_NUMA_EXT', value='0x10'),
var6=VarDef(name='CL_AFFINITY_DOMAIN_NEXT_FISSIONABLE_EXT', value='0x100')
)

Enum(name='cl_device_svm_capabilities',bitfield=True,
var1=VarDef(name='CL_DEVICE_SVM_COARSE_GRAIN_BUFFER', value='(1 << 0)'),
var2=VarDef(name='CL_DEVICE_SVM_FINE_GRAIN_BUFFER', value='(1 << 1)'),
var3=VarDef(name='CL_DEVICE_SVM_FINE_GRAIN_SYSTEM', value='(1 << 2)'),
var4=VarDef(name='CL_DEVICE_SVM_ATOMICS', value='(1 << 3)')
)

Enum(name='cl_command_queue_info',
var1=VarDef(name='CL_QUEUE_CONTEXT', value='0x1090'),
var2=VarDef(name='CL_QUEUE_DEVICE', value='0x1091'),
var3=VarDef(name='CL_QUEUE_REFERENCE_COUNT', value='0x1092'),
var4=VarDef(name='CL_QUEUE_PROPERTIES', value='0x1093'),
var5=VarDef(name='CL_QUEUE_SIZE', value='0x1094'),
var6=VarDef(name='CL_QUEUE_DEVICE_DEFAULT', value='0x1095'),
# cl_intel_command_queue_properties
var7=VarDef(name='CL_QUEUE_TILE_ID_INTEL', value='0x10000'),
var8=VarDef(name='CL_QUEUE_TILE_ID_MASK_INTEL', value='0x10007'),
var9=VarDef(name='CL_QUEUE_FAMILY_INTEL', value='0x10008'),
)

Enum(name='cl_queue_properties',
var1=VarDef(name='CL_QUEUE_PROPERTIES', value='0x1093'),
var2=VarDef(name='CL_QUEUE_SIZE', value='0x1094'),
# cl_intel_command_queue_properties
var3=VarDef(name='CL_QUEUE_TILE_ID_INTEL', value='0x10000'),
var4=VarDef(name='CL_QUEUE_TILE_ID_MASK_INTEL', value='0x10007'),
var5=VarDef(name='CL_QUEUE_FAMILY_INTEL', value='0x10008'),
)

Enum(name='cl_mem_flags',bitfield=True,
var1=VarDef(name='CL_MEM_READ_WRITE', value='(1 << 0)'),
var2=VarDef(name='CL_MEM_WRITE_ONLY', value='(1 << 1)'),
var3=VarDef(name='CL_MEM_READ_ONLY', value='(1 << 2)'),
var4=VarDef(name='CL_MEM_USE_HOST_PTR', value='(1 << 3)'),
var5=VarDef(name='CL_MEM_ALLOC_HOST_PTR', value='(1 << 4)'),
var6=VarDef(name='CL_MEM_COPY_HOST_PTR', value='(1 << 5)'),
var7=VarDef(name='CL_MEM_HOST_WRITE_ONLY', value='(1 << 7)'),
var8=VarDef(name='CL_MEM_HOST_READ_ONLY', value='(1 << 8)'),
var9=VarDef(name='CL_MEM_HOST_NO_ACCESS', value='(1 << 9)'),
var10=VarDef(name='CL_MEM_KERNEL_READ_AND_WRITE', value='(1 << 12)')
)

Enum(name='cl_svm_mem_flags',bitfield=True,
var1=VarDef(name='CL_MEM_READ_WRITE', value='(1 << 0)'),
var2=VarDef(name='CL_MEM_WRITE_ONLY', value='(1 << 1)'),
var3=VarDef(name='CL_MEM_READ_ONLY', value='(1 << 2)'),
var4=VarDef(name='CL_MEM_SVM_FINE_GRAIN_BUFFER', value='(1 << 10)'),
var5=VarDef(name='CL_MEM_SVM_ATOMICS', value='(1 << 11)')
)

Enum(name='cl_mem_alloc_flags_intel',bitfield=True,
var1=VarDef(name='CL_MEM_ALLOC_WRITE_COMBINED_INTEL', value='(1 << 0)'),
var2=VarDef(name='CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL', value='(1 << 1)'),
var3=VarDef(name='CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL', value='(1 << 2)')
)

Enum(name='cl_mem_migration_flags',bitfield=True,
var1=VarDef(name='CL_MIGRATE_MEM_OBJECT_HOST', value='(1 << 0)'),
var2=VarDef(name='CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED', value='(1 << 1)')
)

Enum(name='cl_mem_migration_flags_intel',bitfield=True,
var1=VarDef(name='CL_MIGRATE_MEM_OBJECT_HOST_INTEL', value='(1 << 0)'),
var2=VarDef(name='CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED_INTEL', value='(1 << 1)')
)

Enum(name='cl_device_unified_shared_memory_capabilities_intel',bitfield=True,
var1=VarDef(name='CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL', value='(1 << 0)'),
var2=VarDef(name='CL_UNIFIED_SHARED_MEMORY_ATOMIC_ACCESS_INTEL', value='(1 << 1)'),
var3=VarDef(name='CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ACCESS_INTEL', value='(1 << 2)'),
var4=VarDef(name='CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ATOMIC_ACCESS_INTEL', value='(1 << 3)')
)

Enum(name='cl_channel_order',
var1=VarDef(name='CL_R', value='0x10B0'),
var2=VarDef(name='CL_A', value='0x10B1'),
var3=VarDef(name='CL_RG', value='0x10B2'),
var4=VarDef(name='CL_RA', value='0x10B3'),
var5=VarDef(name='CL_RGB', value='0x10B4'),
var6=VarDef(name='CL_RGBA', value='0x10B5'),
var7=VarDef(name='CL_BGRA', value='0x10B6'),
var8=VarDef(name='CL_ARGB', value='0x10B7'),
var9=VarDef(name='CL_INTENSITY', value='0x10B8'),
var10=VarDef(name='CL_LUMINANCE', value='0x10B9'),
var11=VarDef(name='CL_Rx', value='0x10BA'),
var12=VarDef(name='CL_RGx', value='0x10BB'),
var13=VarDef(name='CL_RGBx', value='0x10BC'),
var14=VarDef(name='CL_DEPTH', value='0x10BD'),
var15=VarDef(name='CL_DEPTH_STENCIL', value='0x10BE'),
var16=VarDef(name='CL_sRGB', value='0x10BF'),
var17=VarDef(name='CL_sRGBx', value='0x10C0'),
var18=VarDef(name='CL_sRGBA', value='0x10C1'),
var19=VarDef(name='CL_sBGRA', value='0x10C2'),
var20=VarDef(name='CL_ABGR', value='0x10C3')
)

Enum(name='cl_channel_type',
var1=VarDef(name='CL_SNORM_INT8', value='0x10D0'),
var2=VarDef(name='CL_SNORM_INT16', value='0x10D1'),
var3=VarDef(name='CL_UNORM_INT8', value='0x10D2'),
var4=VarDef(name='CL_UNORM_INT16', value='0x10D3'),
var5=VarDef(name='CL_UNORM_SHORT_565', value='0x10D4'),
var6=VarDef(name='CL_UNORM_SHORT_555', value='0x10D5'),
var7=VarDef(name='CL_UNORM_INT_101010', value='0x10D6'),
var8=VarDef(name='CL_SIGNED_INT8', value='0x10D7'),
var9=VarDef(name='CL_SIGNED_INT16', value='0x10D8'),
var10=VarDef(name='CL_SIGNED_INT32', value='0x10D9'),
var11=VarDef(name='CL_UNSIGNED_INT8', value='0x10DA'),
var12=VarDef(name='CL_UNSIGNED_INT16', value='0x10DB'),
var13=VarDef(name='CL_UNSIGNED_INT32', value='0x10DC'),
var14=VarDef(name='CL_HALF_FLOAT', value='0x10DD'),
var15=VarDef(name='CL_FLOAT', value='0x10DE'),
var16=VarDef(name='CL_UNORM_INT24', value='0x10DF'),
var17=VarDef(name='CL_UNORM_INT_101010_2', value='0x10E0')
)

Enum(name='cl_mem_object_type',
var1=VarDef(name='CL_MEM_OBJECT_BUFFER', value='0x10F0'),
var2=VarDef(name='CL_MEM_OBJECT_IMAGE2D', value='0x10F1'),
var3=VarDef(name='CL_MEM_OBJECT_IMAGE3D', value='0x10F2'),
var4=VarDef(name='CL_MEM_OBJECT_IMAGE2D_ARRAY', value='0x10F3'),
var5=VarDef(name='CL_MEM_OBJECT_IMAGE1D', value='0x10F4'),
var6=VarDef(name='CL_MEM_OBJECT_IMAGE1D_ARRAY', value='0x10F5'),
var7=VarDef(name='CL_MEM_OBJECT_IMAGE1D_BUFFER', value='0x10F6'),
var8=VarDef(name='CL_MEM_OBJECT_PIPE', value='0x10F7')
)

Enum(name='cl_mem_info',
var1=VarDef(name='CL_MEM_TYPE', value='0x1100'),
var2=VarDef(name='CL_MEM_FLAGS', value='0x1101'),
var3=VarDef(name='CL_MEM_SIZE', value='0x1102'),
var4=VarDef(name='CL_MEM_HOST_PTR', value='0x1103'),
var5=VarDef(name='CL_MEM_MAP_COUNT', value='0x1104'),
var6=VarDef(name='CL_MEM_REFERENCE_COUNT', value='0x1105'),
var7=VarDef(name='CL_MEM_CONTEXT', value='0x1106'),
var8=VarDef(name='CL_MEM_ASSOCIATED_MEMOBJECT', value='0x1107'),
var9=VarDef(name='CL_MEM_OFFSET', value='0x1108'),
var10=VarDef(name='CL_MEM_USES_SVM_POINTER', value='0x1109'),
var11=VarDef(name='CL_MEM_DX9_MEDIA_ADAPTER_TYPE_KHR', value='0x2028'),
var12=VarDef(name='CL_MEM_DX9_MEDIA_SURFACE_INFO_KHR', value='0x2029'),
var13=VarDef(name='CL_MEM_DX9_RESOURCE_INTEL', value='0x4027'),
var14=VarDef(name='CL_MEM_DX9_SHARED_HANDLE_INTEL', value='0x4074'),
var15=VarDef(name='CL_MEM_D3D10_RESOURCE_KHR', value='0x4015'),
var16=VarDef(name='CL_MEM_D3D11_RESOURCE_KHR', value='0x401E')
)

Enum(name='cl_image_info',
var1=VarDef(name='CL_IMAGE_FORMAT', value='0x1110'),
var2=VarDef(name='CL_IMAGE_ELEMENT_SIZE', value='0x1111'),
var3=VarDef(name='CL_IMAGE_ROW_PITCH', value='0x1112'),
var4=VarDef(name='CL_IMAGE_SLICE_PITCH', value='0x1113'),
var5=VarDef(name='CL_IMAGE_WIDTH', value='0x1114'),
var6=VarDef(name='CL_IMAGE_HEIGHT', value='0x1115'),
var7=VarDef(name='CL_IMAGE_DEPTH', value='0x1116'),
var8=VarDef(name='CL_IMAGE_ARRAY_SIZE', value='0x1117'),
var9=VarDef(name='CL_IMAGE_BUFFER', value='0x1118'),
var10=VarDef(name='CL_IMAGE_NUM_MIP_LEVELS', value='0x1119'),
var11=VarDef(name='CL_IMAGE_NUM_SAMPLES', value='0x111A'),
var12=VarDef(name='CL_IMAGE_DX9_MEDIA_PLANE_KHR', value='0x202A'),
var13=VarDef(name='CL_IMAGE_DX9_PLANE_INTEL', value='0x4075'),
var14=VarDef(name='CL_IMAGE_D3D10_SUBRESOURCE_KHR', value='0x4016'),
var15=VarDef(name='CL_IMAGE_D3D11_SUBRESOURCE_KHR', value='0x401F')
)

Enum(name='cl_pipe_info',
var1=VarDef(name='CL_PIPE_PACKET_SIZE', value='0x1120'),
var2=VarDef(name='CL_PIPE_MAX_PACKETS', value='0x1121')
)

Enum(name='cl_addressing_mode',
var1=VarDef(name='CL_ADDRESS_NONE', value='0x1130'),
var2=VarDef(name='CL_ADDRESS_CLAMP_TO_EDGE', value='0x1131'),
var3=VarDef(name='CL_ADDRESS_CLAMP', value='0x1132'),
var4=VarDef(name='CL_ADDRESS_REPEAT', value='0x1133'),
var5=VarDef(name='CL_ADDRESS_MIRRORED_REPEAT', value='0x1134')
)

Enum(name='cl_filter_mode',
var1=VarDef(name='CL_FILTER_NEAREST', value='0x1140'),
var2=VarDef(name='CL_FILTER_LINEAR', value='0x1141')
)

Enum(name='cl_sampler_info',
var1=VarDef(name='CL_SAMPLER_REFERENCE_COUNT', value='0x1150'),
var2=VarDef(name='CL_SAMPLER_CONTEXT', value='0x1151'),
var3=VarDef(name='CL_SAMPLER_NORMALIZED_COORDS', value='0x1152'),
var4=VarDef(name='CL_SAMPLER_ADDRESSING_MODE', value='0x1153'),
var5=VarDef(name='CL_SAMPLER_FILTER_MODE', value='0x1154'),
var6=VarDef(name='CL_SAMPLER_MIP_FILTER_MODE', value='0x1155'),
var7=VarDef(name='CL_SAMPLER_LOD_MIN', value='0x1156'),
var8=VarDef(name='CL_SAMPLER_LOD_MAX', value='0x1157')
)

Enum(name='cl_sampler_properties',
var1=VarDef(name='CL_SAMPLER_NORMALIZED_COORDS', value='0x1152'),
var2=VarDef(name='CL_SAMPLER_ADDRESSING_MODE', value='0x1153'),
var3=VarDef(name='CL_SAMPLER_FILTER_MODE', value='0x1154'),
var4=VarDef(name='CL_SAMPLER_MIP_FILTER_MODE', value='0x1155'),
var5=VarDef(name='CL_SAMPLER_LOD_MIN', value='0x1156'),
var6=VarDef(name='CL_SAMPLER_LOD_MAX', value='0x1157')
)

Enum(name='cl_map_flags',bitfield=True,
var1=VarDef(name='CL_MAP_READ', value='(1 << 0)'),
var2=VarDef(name='CL_MAP_WRITE', value='(1 << 1)'),
var3=VarDef(name='CL_MAP_WRITE_INVALIDATE_REGION', value='(1 << 2)')
)

Enum(name='cl_program_info',
var1=VarDef(name='CL_PROGRAM_REFERENCE_COUNT', value='0x1160'),
var2=VarDef(name='CL_PROGRAM_CONTEXT', value='0x1161'),
var3=VarDef(name='CL_PROGRAM_NUM_DEVICES', value='0x1162'),
var4=VarDef(name='CL_PROGRAM_DEVICES', value='0x1163'),
var5=VarDef(name='CL_PROGRAM_SOURCE', value='0x1164'),
var6=VarDef(name='CL_PROGRAM_BINARY_SIZES', value='0x1165'),
var7=VarDef(name='CL_PROGRAM_BINARIES', value='0x1166'),
var8=VarDef(name='CL_PROGRAM_NUM_KERNELS', value='0x1167'),
var9=VarDef(name='CL_PROGRAM_KERNEL_NAMES', value='0x1168'),
var10=VarDef(name='CL_PROGRAM_IL', value='0x1169')
)

Enum(name='cl_program_build_info',
var1=VarDef(name='CL_PROGRAM_BUILD_STATUS', value='0x1181'),
var2=VarDef(name='CL_PROGRAM_BUILD_OPTIONS', value='0x1182'),
var3=VarDef(name='CL_PROGRAM_BUILD_LOG', value='0x1183'),
var4=VarDef(name='CL_PROGRAM_BINARY_TYPE', value='0x1184'),
var5=VarDef(name='CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE', value='0x1185')
)

Enum(name='cl_program_binary_type',
var1=VarDef(name='CL_PROGRAM_BINARY_TYPE_NONE', value='0x0'),
var2=VarDef(name='CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT', value='0x1'),
var3=VarDef(name='CL_PROGRAM_BINARY_TYPE_LIBRARY', value='0x2'),
var4=VarDef(name='CL_PROGRAM_BINARY_TYPE_EXECUTABLE', value='0x4')
)

Enum(name='cl_build_status',
var1=VarDef(name='CL_BUILD_SUCCESS', value='0'),
var2=VarDef(name='CL_BUILD_NONE', value='-1'),
var3=VarDef(name='CL_BUILD_ERROR', value='-2'),
var4=VarDef(name='CL_BUILD_IN_PROGRESS', value='-3')
)

Enum(name='cl_kernel_info',
var1=VarDef(name='CL_KERNEL_FUNCTION_NAME', value='0x1190'),
var2=VarDef(name='CL_KERNEL_NUM_ARGS', value='0x1191'),
var3=VarDef(name='CL_KERNEL_REFERENCE_COUNT', value='0x1192'),
var4=VarDef(name='CL_KERNEL_CONTEXT', value='0x1193'),
var5=VarDef(name='CL_KERNEL_PROGRAM', value='0x1194'),
var6=VarDef(name='CL_KERNEL_ATTRIBUTES', value='0x1195'),
var7=VarDef(name='CL_KERNEL_MAX_NUM_SUB_GROUPS', value='0x11B9'),
var8=VarDef(name='CL_KERNEL_COMPILE_NUM_SUB_GROUPS', value='0x11BA')
)

Enum(name='cl_kernel_arg_info',
var1=VarDef(name='CL_KERNEL_ARG_ADDRESS_QUALIFIER', value='0x1196'),
var2=VarDef(name='CL_KERNEL_ARG_ACCESS_QUALIFIER', value='0x1197'),
var3=VarDef(name='CL_KERNEL_ARG_TYPE_NAME', value='0x1198'),
var4=VarDef(name='CL_KERNEL_ARG_TYPE_QUALIFIER', value='0x1199'),
var5=VarDef(name='CL_KERNEL_ARG_NAME', value='0x119A')
)

Enum(name='cl_kernel_arg_address_qualifier',
var1=VarDef(name='CL_KERNEL_ARG_ADDRESS_GLOBAL', value='0x119B'),
var2=VarDef(name='CL_KERNEL_ARG_ADDRESS_LOCAL', value='0x119C'),
var3=VarDef(name='CL_KERNEL_ARG_ADDRESS_CONSTANT', value='0x119D'),
var4=VarDef(name='CL_KERNEL_ARG_ADDRESS_PRIVATE', value='0x119E')
)

Enum(name='cl_kernel_arg_access_qualifier',
var1=VarDef(name='CL_KERNEL_ARG_ACCESS_READ_ONLY', value='0x11A0'),
var2=VarDef(name='CL_KERNEL_ARG_ACCESS_WRITE_ONLY', value='0x11A1'),
var3=VarDef(name='CL_KERNEL_ARG_ACCESS_READ_WRITE', value='0x11A2'),
var4=VarDef(name='CL_KERNEL_ARG_ACCESS_NONE', value='0x11A3')
)

Enum(name='cl_kernel_arg_type_qualifier',bitfield=True,
var1=VarDef(name='CL_KERNEL_ARG_TYPE_NONE', value='0'),
var2=VarDef(name='CL_KERNEL_ARG_TYPE_CONST', value='(1 << 0)'),
var3=VarDef(name='CL_KERNEL_ARG_TYPE_RESTRICT', value='(1 << 1)'),
var4=VarDef(name='CL_KERNEL_ARG_TYPE_VOLATILE', value='(1 << 2)'),
var5=VarDef(name='CL_KERNEL_ARG_TYPE_PIPE', value='(1 << 3)')
)

Enum(name='cl_kernel_work_group_info',
var1=VarDef(name='CL_KERNEL_WORK_GROUP_SIZE', value='0x11B0'),
var2=VarDef(name='CL_KERNEL_COMPILE_WORK_GROUP_SIZE', value='0x11B1'),
var3=VarDef(name='CL_KERNEL_LOCAL_MEM_SIZE', value='0x11B2'),
var4=VarDef(name='CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE', value='0x11B3'),
var5=VarDef(name='CL_KERNEL_PRIVATE_MEM_SIZE', value='0x11B4'),
var6=VarDef(name='CL_KERNEL_GLOBAL_WORK_SIZE', value='0x11B5'),
var7=VarDef(name='CL_KERNEL_SPILL_MEM_SIZE_INTEL', value='0x4109'),
var8=VarDef(name='CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL', value='0x410A')
)

Enum(name='cl_kernel_sub_group_info',
var1=VarDef(name='CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE', value='0x2033'),
var2=VarDef(name='CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE', value='0x2034'),
var3=VarDef(name='CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT', value='0x11B8'),
var4=VarDef(name='CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL', value='0x410A')
)

Enum(name='cl_kernel_exec_info',
var1=VarDef(name='CL_KERNEL_EXEC_INFO_SVM_PTRS', value='0x11B6'),
var2=VarDef(name='CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM', value='0x11B7'),
var3=VarDef(name='CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL', value='0x4200'),
var4=VarDef(name='CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL', value='0x4201'),
var5=VarDef(name='CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL', value='0x4202'),
var6=VarDef(name='CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL', value='0x4203'),
var7=VarDef(name='CL_KERNEL_EXEC_INFO_THREAD_ARBITRATION_POLICY_INTEL', value='0x10025'),
var8=VarDef(name='CL_KERNEL_EXEC_INFO_KERNEL_TYPE_INTEL', value='0x1000C')
)

Enum(name='cl_mem_info_intel',
var1=VarDef(name='CL_MEM_ALLOC_TYPE_INTEL', value='0x419A'),
var2=VarDef(name='CL_MEM_ALLOC_BASE_PTR_INTEL', value='0x419B'),
var3=VarDef(name='CL_MEM_ALLOC_SIZE_INTEL', value='0x419C'),
var4=VarDef(name='CL_MEM_ALLOC_DEVICE_INTEL', value='0x419D'),
var5=VarDef(name='CL_MEM_ALLOC_INFO_TBD0_INTEL', value='0x419E'),
var6=VarDef(name='CL_MEM_ALLOC_INFO_TBD1_INTEL', value='0x419F')
)

Enum(name='cl_event_info',
var1=VarDef(name='CL_EVENT_COMMAND_QUEUE', value='0x11D0'),
var2=VarDef(name='CL_EVENT_COMMAND_TYPE', value='0x11D1'),
var3=VarDef(name='CL_EVENT_REFERENCE_COUNT', value='0x11D2'),
var4=VarDef(name='CL_EVENT_COMMAND_EXECUTION_STATUS', value='0x11D3'),
var5=VarDef(name='CL_EVENT_CONTEXT', value='0x11D4')
)

Enum(name='cl_command_type',
var1=VarDef(name='CL_COMMAND_NDRANGE_KERNEL', value='0x11F0'),
var2=VarDef(name='CL_COMMAND_TASK', value='0x11F1'),
var3=VarDef(name='CL_COMMAND_NATIVE_KERNEL', value='0x11F2'),
var4=VarDef(name='CL_COMMAND_READ_BUFFER', value='0x11F3'),
var5=VarDef(name='CL_COMMAND_WRITE_BUFFER', value='0x11F4'),
var6=VarDef(name='CL_COMMAND_COPY_BUFFER', value='0x11F5'),
var7=VarDef(name='CL_COMMAND_READ_IMAGE', value='0x11F6'),
var8=VarDef(name='CL_COMMAND_WRITE_IMAGE', value='0x11F7'),
var9=VarDef(name='CL_COMMAND_COPY_IMAGE', value='0x11F8'),
var10=VarDef(name='CL_COMMAND_COPY_IMAGE_TO_BUFFER', value='0x11F9'),
var11=VarDef(name='CL_COMMAND_COPY_BUFFER_TO_IMAGE', value='0x11FA'),
var12=VarDef(name='CL_COMMAND_MAP_BUFFER', value='0x11FB'),
var13=VarDef(name='CL_COMMAND_MAP_IMAGE', value='0x11FC'),
var14=VarDef(name='CL_COMMAND_UNMAP_MEM_OBJECT', value='0x11FD'),
var15=VarDef(name='CL_COMMAND_MARKER', value='0x11FE'),
var16=VarDef(name='CL_COMMAND_ACQUIRE_GL_OBJECTS', value='0x11FF'),
var17=VarDef(name='CL_COMMAND_RELEASE_GL_OBJECTS', value='0x1200'),
var18=VarDef(name='CL_COMMAND_READ_BUFFER_RECT', value='0x1201'),
var19=VarDef(name='CL_COMMAND_WRITE_BUFFER_RECT', value='0x1202'),
var20=VarDef(name='CL_COMMAND_COPY_BUFFER_RECT', value='0x1203'),
var21=VarDef(name='CL_COMMAND_USER', value='0x1204'),
var22=VarDef(name='CL_COMMAND_BARRIER', value='0x1205'),
var23=VarDef(name='CL_COMMAND_MIGRATE_MEM_OBJECTS', value='0x1206'),
var24=VarDef(name='CL_COMMAND_FILL_BUFFER', value='0x1207'),
var25=VarDef(name='CL_COMMAND_FILL_IMAGE', value='0x1208'),
var26=VarDef(name='CL_COMMAND_SVM_FREE', value='0x1209'),
var27=VarDef(name='CL_COMMAND_SVM_MEMCPY', value='0x120A'),
var28=VarDef(name='CL_COMMAND_SVM_MEMFILL', value='0x120B'),
var29=VarDef(name='CL_COMMAND_SVM_MAP', value='0x120C'),
var30=VarDef(name='CL_COMMAND_SVM_UNMAP', value='0x120D'),
var31=VarDef(name='CL_COMMAND_ACQUIRE_DX9_MEDIA_SURFACES_KHR', value='0x202B'),
var32=VarDef(name='CL_COMMAND_RELEASE_DX9_MEDIA_SURFACES_KHR', value='0x202C'),
var33=VarDef(name='CL_COMMAND_ACQUIRE_DX9_OBJECTS_INTEL', value='0x402A'),
var34=VarDef(name='CL_COMMAND_RELEASE_DX9_OBJECTS_INTEL', value='0x402B'),
var35=VarDef(name='CL_COMMAND_ACQUIRE_D3D10_OBJECTS_KHR', value='0x4017'),
var36=VarDef(name='CL_COMMAND_RELEASE_D3D10_OBJECTS_KHR', value='0x4018'),
var37=VarDef(name='CL_COMMAND_ACQUIRE_D3D11_OBJECTS_KHR', value='0x4020'),
var38=VarDef(name='CL_COMMAND_RELEASE_D3D11_OBJECTS_KHR', value='0x4021'),
var39=VarDef(name='CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR', value='0x200D'),
var40=VarDef(name='CL_COMMAND_MEMSET_INTEL', value='0x4204'),
var41=VarDef(name='CL_COMMAND_MEMCPY_INTEL', value='0x4205'),
var42=VarDef(name='CL_COMMAND_MIGRATEMEM_INTEL', value='0x4206'),
var43=VarDef(name='CL_COMMAND_MEMADVISE_INTEL', value='0x4207')
)

Enum(name='CLExecutionStatus', type='cl_int',
var1=VarDef(name='CL_COMPLETE', value='0x0'),
var2=VarDef(name='CL_RUNNING', value='0x1'),
var3=VarDef(name='CL_SUBMITTED', value='0x2'),
var4=VarDef(name='CL_QUEUED', value='0x3')
)

Enum(name='cl_buffer_create_type',
var1=VarDef(name='CL_BUFFER_CREATE_TYPE_REGION', value='0x1220')
)

Enum(name='cl_profiling_info',
var1=VarDef(name='CL_PROFILING_COMMAND_QUEUED', value='0x1280'),
var2=VarDef(name='CL_PROFILING_COMMAND_SUBMIT', value='0x1281'),
var3=VarDef(name='CL_PROFILING_COMMAND_START', value='0x1282'),
var4=VarDef(name='CL_PROFILING_COMMAND_END', value='0x1283'),
var5=VarDef(name='CL_PROFILING_COMMAND_COMPLETE', value='0x1284')
)

Enum(name='cl_gl_object_type',
var1=VarDef(name='CL_GL_OBJECT_BUFFER', value='0x2000'),
var2=VarDef(name='CL_GL_OBJECT_TEXTURE2D', value='0x2001'),
var3=VarDef(name='CL_GL_OBJECT_TEXTURE3D', value='0x2002'),
var4=VarDef(name='CL_GL_OBJECT_RENDERBUFFER', value='0x2003'),
var5=VarDef(name='CL_GL_OBJECT_TEXTURE2D_ARRAY', value='0x200E'),
var6=VarDef(name='CL_GL_OBJECT_TEXTURE1D', value='0x200F'),
var7=VarDef(name='CL_GL_OBJECT_TEXTURE1D_ARRAY', value='0x2010'),
var8=VarDef(name='CL_GL_OBJECT_TEXTURE_BUFFER', value='0x2011')
)

Enum(name='cl_gl_texture_info',
var1=VarDef(name='CL_GL_TEXTURE_TARGET', value='0x2004'),
var2=VarDef(name='CL_GL_MIPMAP_LEVEL', value='0x2005'),
var3=VarDef(name='CL_GL_NUM_SAMPLES', value='0x2012')
)

Enum(name='cl_dx9_media_adapter_type_khr',
var1=VarDef(name='CL_ADAPTER_D3D9_KHR', value='0x2020'),
var2=VarDef(name='CL_ADAPTER_D3D9EX_KHR', value='0x2021'),
var3=VarDef(name='CL_ADAPTER_DXVA_KHR', value='0x2022')
)

Enum(name='cl_dx9_media_adapter_set_khr',
var1=VarDef(name='CL_CURRENT_DEVICE_FOR_DX9_MEDIA_ADAPTER_KHR', value='0x2023'),
var2=VarDef(name='CL_ALL_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR', value='0x2024')
)

Enum(name='cl_dx9_device_source_intel',
var1=VarDef(name='CL_D3D9_DEVICE_INTEL', value='0x4022'),
var2=VarDef(name='CL_D3D9EX_DEVICE_INTEL', value='0x4070'),
var3=VarDef(name='CL_DXVA_DEVICE_INTEL', value='0x4071')
)

Enum(name='cl_dx9_device_set_intel',
var1=VarDef(name='CL_PREFERRED_DEVICES_FOR_DX9_INTEL', value='0x4024'),
var2=VarDef(name='CL_ALL_DEVICES_FOR_DX9_INTEL', value='0x4025')
)

Enum(name='cl_d3d10_device_source_khr',
var1=VarDef(name='CL_D3D10_DEVICE_KHR', value='0x4010'),
var2=VarDef(name='CL_D3D10_DXGI_ADAPTER_KHR', value='0x4011')
)

Enum(name='cl_d3d10_device_set_khr',
var1=VarDef(name='CL_PREFERRED_DEVICES_FOR_D3D10_KHR', value='0x4012'),
var2=VarDef(name='CL_ALL_DEVICES_FOR_D3D10_KHR', value='0x4013')
)

Enum(name='cl_d3d11_device_source_khr',
var1=VarDef(name='CL_D3D11_DEVICE_KHR', value='0x4019'),
var2=VarDef(name='CL_D3D11_DXGI_ADAPTER_KHR', value='0x401A')
)

Enum(name='cl_d3d11_device_set_khr',
var1=VarDef(name='CL_PREFERRED_DEVICES_FOR_D3D11_KHR', value='0x401B'),
var2=VarDef(name='CL_ALL_DEVICES_FOR_D3D11_KHR', value='0x401C')
)

Enum(name='cl_d3d11_device_source_nv',
var1=VarDef(name='CL_D3D11_DEVICE_NV', value='0x4019'),
var2=VarDef(name='CL_D3D11_DXGI_ADAPTER_NV', value='0x401A')
)

Enum(name='cl_d3d11_device_set_nv',
var1=VarDef(name='CL_PREFERRED_DEVICES_FOR_D3D11_NV', value='0x401B'),
var2=VarDef(name='CL_ALL_DEVICES_FOR_D3D11_NV', value='0x401C')
)

Enum(name='cl_resource_memory_scope',
var1=VarDef(name='CL_MEMORY_SCOPE_DEVICE', value='0x0'),
var2=VarDef(name='CL_MEMORY_SCOPE_ALL_SVM_DEVICES', value='0x1')
)

Enum(name='cl_resource_barrier_type',
var1=VarDef(name='CL_RESOURCE_BARRIER_TYPE_ACQUIRE', value='0x0'),
var2=VarDef(name='CL_RESOURCE_BARRIER_TYPE_RELEASE', value='0x1'),
var3=VarDef(name='CL_RESOURCE_BARRIER_TYPE_DISCARD', value='0x2')
)

Argument(name='cl_command_queue', obj=True)
Argument(name='cl_context', obj=True)
Argument(name='cl_device_id', obj=True)
Argument(name='cl_event', obj=True)
Argument(name='cl_kernel', obj=True)
Argument(name='cl_mem', obj=True)
Argument(name='cl_platform_id', obj=True)
Argument(name='cl_program', obj=True)
Argument(name='cl_sampler', obj=True)

Argument(name='cl_buffer_region', custom_tostring=True)
Argument(name='cl_image_desc', custom_tostring=True)
Argument(name='cl_image_format', custom_tostring=True)
Argument(name='cl_pipe_properties')

# deprecate and use common instead?
Argument(name='cl_GLenum', custom_tostring=True)
Argument(name='cl_GLint')
Argument(name='cl_GLuint')
Argument(name='cl_GLsync')
Argument(name='cl_int')
Argument(name='cl_uint')
Argument(name='cl_ulong')
Argument(name='cl_float')
Argument(name='int')
Argument(name='size_t')
Argument(name='UINT')
#Argument(name='CLeglDisplayKHR')
#Argument(name='CLeglImageKHR')
#Argument(name='CLeglSyncKHR')
