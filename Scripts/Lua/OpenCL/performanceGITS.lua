-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- OpenCL performance dump in old GITS format.
-- Dumps CPU measurements for all calls and GPU profiling info for NDRangeKernel.

PTR_SIZE = gits.getPtrSize()

-- Global variables
CALL_RESULTS = {}
CALLS = {}

-- CSV table enums
HEADER = 'Function,cl_context,cl_command_queue,kernel,APP begin [ms],APP end [ms],APP callback [ms],DRV queue [ms],DRV submit [ms],DRV start [ms],DRV end [ms],APP end-begin [ms],APP callback-begin [ms],DRV submit-queue [ms],DRV start-submit [ms],DRV end-start [ms]'

FUNCTION = 1
CONTEXT = 2
COMMAND_QUEUE = 3
KERNEL = 4
APP_BEGIN = 5
APP_END = 6
APP_CALLBACK = 7
DRV_QUEUE = 8
DRV_SUBMIT = 9
DRV_START = 10
DRV_END = 11
APP_END_BEGIN = 12
APP_CALLBACK_BEGIN = 13
DRV_SUBMIT_QUEUE = 14
DRV_START_SUBMIT = 15
DRV_END_START = 16
LAST = DRV_END_START

-- CL enums
CL_QUEUE_PROPERTIES = 0x1093
CL_QUEUE_PROFILING_ENABLE = 0x0002

CL_KERNEL_FUNCTION_NAME = 0x1190

CL_PROFILING_COMMAND_QUEUED = 0x1280
CL_PROFILING_COMMAND_SUBMIT = 0x1281
CL_PROFILING_COMMAND_START = 0x1282
CL_PROFILING_COMMAND_END = 0x1283
CL_PROFILING_COMMAND_COMPLETE = 0x1284

CL_COMPLETE = 0

function gitsProgramExit()
    local csvFile = io.open('ocl_perf.csv', 'w')
    if csvFile == nil then
        error('Failed to open results file for writing.')
        return
    end

    io.output(csvFile)
    io.write(HEADER .. '\n')

    local count = #CALLS
    for i = 1, count do
        local c = CALLS[i]
        local line = ''
        for j = 1, LAST do
            if type(c[j]) == 'number'  then
                c[j] = string.format("%.3f", c[j])
            end
            if c[j] ~= nil then
                line = line .. c[j]
            end
            if j ~= LAST then
                line = line .. ','
            end
        end
        io.write(line .. '\n')
    end

    io.close(csvFile)
end

function performanceStart(name)
    CALL_RESULTS[FUNCTION] = name
    CALL_RESULTS[APP_BEGIN] = gits.currTime() / 1000
end

function performanceEnd(name)
    CALL_RESULTS[APP_END] = gits.currTime() / 1000
    CALL_RESULTS[APP_END_BEGIN] = CALL_RESULTS[APP_END] - CALL_RESULTS[APP_BEGIN]
    table.insert(CALLS, CALL_RESULTS)
    CALL_RESULTS = {}
end

function getProfilingInfo(event)
    local timestamp = gits.allocBytes(8) -- cl_ulong
    drvCl.clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, 8, timestamp, gits.nullUdt())
    CALL_RESULTS[DRV_QUEUE] = gits.getInt64(timestamp, 0) / 1000000
    drvCl.clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, 8, timestamp, gits.nullUdt())
    CALL_RESULTS[DRV_SUBMIT] = gits.getInt64(timestamp, 0) / 1000000
    drvCl.clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, 8, timestamp, gits.nullUdt())
    CALL_RESULTS[DRV_START] = gits.getInt64(timestamp, 0) / 1000000
    drvCl.clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, 8, timestamp, gits.nullUdt())
    CALL_RESULTS[DRV_END] = gits.getInt64(timestamp, 0) / 1000000
    CALL_RESULTS[DRV_SUBMIT_QUEUE] = CALL_RESULTS[DRV_SUBMIT] - CALL_RESULTS[DRV_QUEUE]
    CALL_RESULTS[DRV_START_SUBMIT] = CALL_RESULTS[DRV_START] - CALL_RESULTS[DRV_SUBMIT]
    CALL_RESULTS[DRV_END_START] = CALL_RESULTS[DRV_END] - CALL_RESULTS[DRV_START]
    gits.freeBytes(timestamp)
end

function getKernelFunctionName(kernel)
    local name_len_ptr = gits.allocBytes(PTR_SIZE)
    drvCl.clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, gits.nullUdt(), gits.nullUdt(), name_len_ptr)
    local name_ptr = gits.allocBytes(gits.getSizeT(name_len_ptr, 0))
    drvCl.clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, gits.getSizeT(name_len_ptr, 0), name_ptr, gits.nullUdt())
    local name = gits.udtToStr(name_ptr)
    gits.freeBytes(name_len)
    gits.freeBytes(name)
    CALL_RESULTS[KERNEL] = name
end

function clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
    performanceStart('clBuildProgram')
    local ret = drvCl.clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
    performanceEnd('clBuildProgram')
    return ret
end

function clCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data)
    performanceStart('clCompileProgram')
    local ret = drvCl.clCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data)
    performanceEnd('clCompileProgram')
    return ret
end

function clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    performanceStart('clCreateBuffer')
    local ret = drvCl.clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    performanceEnd('clCreateBuffer')
    return ret
end

function clCreateCommandQueue(context, device, properties, errcode_ret)
    performanceStart('clCreateCommandQueue')
    properties = properties | CL_QUEUE_PROFILING_ENABLE
    local ret = drvCl.clCreateCommandQueue(context, device, properties, errcode_ret)
    performanceEnd('clCreateCommandQueue')
    return ret
end

function clCreateCommandQueueWithProperties(context, device, properties, errcode_ret)
    performanceStart('clCreateCommandQueueWithProperties')
    local properties_index = -1
    local i = 0
    while true do
        if gits.getInt64(properties, i) == CL_QUEUE_PROPERTIES then
            properties_index = i + 1
            break
        end
        if gits.getInt64(properties, i) == 0 then
            break
        end
        i = i + 2
    end
    if properties_index ~= -1 then
        local properties_value = gits.getInt64(properties, properties_index)
        properties_value = properties_value | CL_QUEUE_PROFILING_ENABLE
        gits.setInt64(properties, properties_index, properties_value)
    end
    local ret = drvCl.clCreateCommandQueueWithProperties(context, device, properties, errcode_ret)
    performanceEnd('clCreateCommandQueueWithProperties')
    return ret
end

function clCreateContext(properties, num_devices, devices, pfn_notify, user_data, errcode_ret)
    performanceStart('clCreateContext')
    local ret = drvCl.clCreateContext(properties, num_devices, devices, pfn_notify, user_data, errcode_ret)
    performanceEnd('clCreateContext')
    return ret
end

function clCreateContextFromType(properties, device_type, pfn_notify, user_data, errcode_ret)
    performanceStart('clCreateContextFromType')
    local ret = drvCl.clCreateContextFromType(properties, device_type, pfn_notify, user_data, errcode_ret)
    performanceEnd('clCreateContextFromType')
    return ret
end

function clCreateEventFromGLsyncKHR(context, sync, errcode_ret)
    performanceStart('clCreateEventFromGLsyncKHR')
    local ret = drvCl.clCreateEventFromGLsyncKHR(context, sync, errcode_ret)
    performanceEnd('clCreateEventFromGLsyncKHR')
    return ret
end

function clCreateFromD3D10BufferKHR(context, flags, resource, errcode_ret)
    performanceStart('clCreateFromD3D10BufferKHR')
    local ret = drvCl.clCreateFromD3D10BufferKHR(context, flags, resource, errcode_ret)
    performanceEnd('clCreateFromD3D10BufferKHR')
    return ret
end

function clCreateFromD3D10Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    performanceStart('clCreateFromD3D10Texture2DKHR')
    local ret = drvCl.clCreateFromD3D10Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    performanceEnd('clCreateFromD3D10Texture2DKHR')
    return ret
end

function clCreateFromD3D10Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    performanceStart('clCreateFromD3D10Texture3DKHR')
    local ret = drvCl.clCreateFromD3D10Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    performanceEnd('clCreateFromD3D10Texture3DKHR')
    return ret
end

function clCreateFromD3D11BufferKHR(context, flags, resource, errcode_ret)
    performanceStart('clCreateFromD3D11BufferKHR')
    local ret = drvCl.clCreateFromD3D11BufferKHR(context, flags, resource, errcode_ret)
    performanceEnd('clCreateFromD3D11BufferKHR')
    return ret
end

function clCreateFromD3D11Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    performanceStart('clCreateFromD3D11Texture2DKHR')
    local ret = drvdrvCl.clCreateFromD3D11Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    performanceEnd('clCreateFromD3D11Texture2DKHR')
    return ret
end

function clCreateFromD3D11Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    performanceStart('clCreateFromD3D11Texture3DKHR')
    local ret = drvCl.clCreateFromD3D11Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    performanceEnd('clCreateFromD3D11Texture3DKHR')
    return ret
end

function clCreateFromDX9MediaSurfaceINTEL(context, flags, resource, shared_handle, plane, errcode_ret)
    performanceStart('clCreateFromDX9MediaSurfaceINTEL')
    local ret = drvCl.clCreateFromDX9MediaSurfaceINTEL(context, flags, resource, shared_handle, plane, errcode_ret)
    performanceEnd('clCreateFromDX9MediaSurfaceINTEL')
    return ret
end

function clCreateFromGLBuffer(context, flags, bufobj, errcode_ret)
    performanceStart('clCreateFromGLBuffer')
    local ret = drvCl.clCreateFromGLBuffer(context, flags, bufobj, errcode_ret)
    performanceEnd('clCreateFromGLBuffer')
    return ret
end

function clCreateFromGLRenderbuffer(context, flags, bufobj, errcode_ret)
    performanceStart('clCreateFromGLRenderbuffer')
    local ret = drvCl.clCreateFromGLRenderbuffer(context, flags, bufobj, errcode_ret)
    performanceStart('clCreateFromGLRenderbuffer')
    return ret
end

function clCreateFromGLTexture(context, flags, target, miplevel, texture, errcode_ret)
    performanceStart('clCreateFromGLTexture')
    local ret = drvCl.clCreateFromGLTexture(context, flags, target, miplevel, texture, errcode_ret)
    performanceEnd('clCreateFromGLTexture')
    return ret
end

function clCreateFromGLTexture2D(context, flags, target, miplevel, texture, errcode_ret)
    performanceStart('clCreateFromGLTexture2D')
    local ret = drvCl.clCreateFromGLTexture2D(context, flags, target, miplevel, texture, errcode_ret)
    performanceEnd('clCreateFromGLTexture2D')
    return ret
end

function clCreateFromGLTexture3D(context, flags, target, miplevel, texture, errcode_ret)
    performanceStart('clCreateFromGLTexture3D')
    local ret = drvCl.clCreateFromGLTexture3D(context, flags, target, miplevel, texture, errcode_ret)
    performanceEnd('clCreateFromGLTexture3D')
    return ret
end

function clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
    performanceStart('clCreateImage')
    local ret = drvCl.clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
    performanceEnd('clCreateImage')
    return ret
end

function clCreateImage2D(context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret)
    performanceStart('clCreateImage2D')
    local ret = drvCl.clCreateImage2D(context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret)
    performanceEnd('clCreateImage2D')
    return ret
end

function clCreateImage3D(context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret)
    performanceStart('clCreateImage3D')
    local ret = drvCl.clCreateImage3D(context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret)
    performanceEnd('clCreateImage3D')
    return ret
end

function clCreateKernel(program, kernel_name, errcode_ret)
    performanceStart('clCreateKernel')
    local ret = drvCl.clCreateKernel(program, kernel_name, errcode_ret)
    performanceEnd('clCreateKernel')
    return ret
end

function clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    performanceStart('clCreateKernelsInProgram')
    local ret = drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    performanceEnd('clCreateKernelsInProgram')
    return ret
end

function clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
    performanceStart('clCreateProgramWithBinary')
    local ret = drvCl.clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
    performanceEnd('clCreateProgramWithBinary')
    return ret
end

function clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
    performanceStart('clCreateProgramWithBuiltInKernels')
    local ret = drvCl.clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
    performanceEnd('clCreateProgramWithBuiltInKernels')
    return ret
end

function clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
    performanceStart('clCreateProgramWithSource')
    local ret = drvCl.clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
    performanceEnd('clCreateProgramWithSource')
    return ret
end

function clCreateSampler(context, normalized_coords, addressing_mode, filter_mode, errcode_ret)
    performanceStart('clCreateSampler')
    local ret = drvCl.clCreateSampler(context, normalized_coords, addressing_mode, filter_mode, errcode_ret)
    performanceEnd('clCreateSampler')
    return ret
end

function clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
    performanceStart('clCreateSubBuffer')
    local ret = drvCl.clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
    performanceEnd('clCreateSubBuffer')
    return ret
end

function clCreateSubDevices(in_device, properties, num_entries, out_devices, num_devices)
    performanceStart('clCreateSubDevices')
    local ret = drvCl.clCreateSubDevices(in_device, properties, num_entries, out_devices, num_devices)
    performanceEnd('clCreateSubDevices')
    return ret
end

function clCreateSubDevicesEXT(in_device, properties, num_entries, out_devices, num_devices)
    performanceStart('clCreateSubDevicesEXT')
    local ret = drvCl.clCreateSubDevicesEXT(in_device, properties, num_entries, out_devices, num_devices)
    performanceEnd('clCreateSubDevicesEXT')
    return ret
end

function clCreateUserEvent(context, errcode_ret)
    performanceStart('clCreateUserEvent')
    local ret = drvCl.clCreateUserEvent(context, errcode_ret)
    performanceEnd('clCreateUserEvent')
    return ret
end

function clEnqueueAcquireD3D10ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueAcquireD3D10ObjectsKHR')
    local ret = drvCl.clEnqueueAcquireD3D10ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueAcquireD3D10ObjectsKHR')
    return ret
end

function clEnqueueAcquireD3D11ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueAcquireD3D11ObjectsKHR')
    local ret = drvCl.clEnqueueAcquireD3D11ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueAcquireD3D11ObjectsKHR')
    return ret
end

function clEnqueueAcquireDX9ObjectsINTEL(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueAcquireDX9ObjectsINTEL')
    local ret = drvCl.clEnqueueAcquireDX9ObjectsINTEL(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueAcquireDX9ObjectsINTEL')
    return ret
end

function clEnqueueAcquireGLObjects(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueAcquireGLObjects')
    local ret = drvCl.clEnqueueAcquireGLObjects(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueAcquireGLObjects')
    return ret
end

function clEnqueueBarrier(command_queue)
    performanceStart('clEnqueueBarrier')
    local ret = drvCl.clEnqueueBarrier(command_queue)
    performanceEnd('clEnqueueBarrier')
    return ret
end

function clEnqueueBarrierWithWaitList(command_queue, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueBarrierWithWaitList')
    local ret = drvCl.clEnqueueBarrierWithWaitList(command_queue, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueBarrierWithWaitList')
    return ret
end

function clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, src_offset, dst_offset, cb, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueCopyBuffer')
    local ret = drvCl.clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, src_offset, dst_offset, cb, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueCopyBuffer')
    return ret
end

function clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueCopyBufferRect')
    local ret = drvCl.clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueCopyBufferRect')
    return ret
end

function clEnqueueCopyBufferToImage(command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueCopyBufferToImage')
    local ret = drvCl.clEnqueueCopyBufferToImage(command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueCopyBufferToImage')
    return ret
end

function clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueCopyImage')
    local ret = drvCl.clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueCopyImage')
    return ret
end

function clEnqueueCopyImageToBuffer(command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueCopyImageToBuffer')
    local ret = drvCl.clEnqueueCopyImageToBuffer(command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueCopyImageToBuffer')
    return ret
end

function clEnqueueFillBuffer(command_queue, buffer, pattern, pattern_size, offset, cb, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueFillBuffer')
    local ret = drvCl.clEnqueueFillBuffer(command_queue, buffer, pattern, pattern_size, offset, cb, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueFillBuffer')
    return ret
end

function clEnqueueFillImage(command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueFillImage')
    local ret = drvCl.clEnqueueFillImage(command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueFillImage')
    return ret
end

function clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    performanceStart('clEnqueueMapBuffer')
    local ret = drvCl.clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    performanceEnd('clEnqueueMapBuffer')
    return ret
end

function clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    performanceStart('clEnqueueMapImage')
    local ret = drvCl.clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    performanceEnd('clEnqueueMapImage')
    return ret
end

function clEnqueueMarker(command_queue, event)
    performanceStart('clEnqueueMarker')
    local ret = drvCl.clEnqueueMarker(command_queue, event)
    performanceEnd('clEnqueueMarker')
    return ret
end

function clEnqueueMarkerWithWaitList(command_queue, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueMarkerWithWaitList')
    local ret = drvCl.clEnqueueMarkerWithWaitList(command_queue, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueMarkerWithWaitList')
    return ret
end

function clEnqueueMigrateMemObjects(command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueMigrateMemObjects')
    local ret = drvCl.clEnqueueMigrateMemObjects(command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueMigrateMemObjects')
    return ret
end

function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueNDRangeKernel')
    local injected = false
    if gits.udtToInt(event) == 0 then
        -- inject event
        event = gits.allocBytes(PTR_SIZE) -- cl_event is _cl_event*
        injected = true
    end
    local ret = drvCl.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
    local ev = gits.getUdt(event, 0) -- dereference ptr
    drvCl.clWaitForEvents(1, event)
    getProfilingInfo(ev)
    if injected then
        drvCl.clReleaseEvent(ev)
        gits.freeBytes(event)
    end
    getKernelFunctionName(kernel)
    performanceEnd('clEnqueueNDRangeKernel')
    return ret
end

function clEnqueueNativeKernel(command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueNativeKernel')
    local ret = drvCl.clEnqueueNativeKernel(command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueNativeKernel')
    return ret
end

function clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReadBuffer')
    local ret = drvCl.clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReadBuffer')
    return ret
end

function clEnqueueReadBufferRect(command_queue, buffer, blocking_read, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReadBufferRect')
    local ret = drvCl.clEnqueueReadBufferRect(command_queue, buffer, blocking_read, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReadBufferRect')
    return ret
end

function clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReadImage')
    local ret = drvCl.clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReadImage')
    return ret
end

function clEnqueueReleaseD3D10ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReleaseD3D10ObjectsKHR')
    local ret = drvCl.clEnqueueReleaseD3D10ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReleaseD3D10ObjectsKHR')
    return ret
end

function clEnqueueReleaseD3D11ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReleaseD3D11ObjectsKHR')
    local ret = drvCl.clEnqueueReleaseD3D11ObjectsKHR(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReleaseD3D11ObjectsKHR')
    return ret
end

function clEnqueueReleaseDX9ObjectsINTEL(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReleaseDX9ObjectsINTEL')
    local ret = drvCl.clEnqueueReleaseDX9ObjectsINTEL(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReleaseDX9ObjectsINTEL')
    return ret
end

function clEnqueueReleaseGLObjects(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueReleaseGLObjects')
    local ret = drvCl.clEnqueueReleaseGLObjects(command_queue, num_objects, mem_objects, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueReleaseGLObjects')
    return ret
end

function clEnqueueTask(command_queue, kernel, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueTask')
    local ret = drvCl.clEnqueueTask(command_queue, kernel, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueTask')
    return ret
end

function clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueUnmapMemObject')
    local ret = drvCl.clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueUnmapMemObject')
    return ret
end

function clEnqueueWaitForEvents(command_queue, num_events, event_list)
    performanceStart('clEnqueueWaitForEvents')
    local ret = drvCl.clEnqueueWaitForEvents(command_queue, num_events, event_list)
    performanceEnd('clEnqueueWaitForEvents')
    return ret
end

function clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueWriteBuffer')
    local ret = drvCl.clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueWriteBuffer')
    return ret
end

function clEnqueueWriteBufferRect(command_queue, buffer, blocking_write, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueWriteBufferRect')
    local ret = drvCl.clEnqueueWriteBufferRect(command_queue, buffer, blocking_write, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueWriteBufferRect')
    return ret
end

function clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceStart('clEnqueueWriteImage')
    local ret = drvCl.clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    performanceEnd('clEnqueueWriteImage')
    return ret
end

function clFinish(command_queue)
    performanceStart('clFinish')
    local ret = drvCl.clFinish(command_queue)
    performanceEnd('clFinish')
    return ret
end

function clFlush(command_queue)
    performanceStart('clFlush')
    local ret = drvCl.clFlush(command_queue)
    performanceEnd('clFlush')
    return ret
end

function clGetCommandQueueInfo(command_queue, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetCommandQueueInfo')
    local ret = drvCl.clGetCommandQueueInfo(command_queue, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetCommandQueueInfo')
    return ret
end

function clGetContextInfo(context, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetContextInfo')
    local ret = drvCl.clGetContextInfo(context, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetContextInfo')
    return ret
end

function clGetDeviceIDs(platform, device_type, num_entries, devices, num_devices)
    performanceStart('clGetDeviceIDs')
    local ret = drvCl.clGetDeviceIDs(platform, device_type, num_entries, devices, num_devices)
    performanceEnd('clGetDeviceIDs')
    return ret
end

function clGetDeviceIDsFromD3D10KHR(platform, d3d_device_source, d3d_object, d3d_device_set, num_entries, devices, num_devices)
    performanceStart('clGetDeviceIDsFromD3D10KHR')
    local ret = drvCl.clGetDeviceIDsFromD3D10KHR(platform, d3d_device_source, d3d_object, d3d_device_set, num_entries, devices, num_devices)
    performanceEnd('clGetDeviceIDsFromD3D10KHR')
    return ret
end

function clGetDeviceIDsFromD3D11KHR(platform, d3d_device_source, d3d_object, d3d_device_set, num_entries, devices, num_devices)
    performanceStart('clGetDeviceIDsFromD3D11KHR')
    local ret = drvCl.clGetDeviceIDsFromD3D11KHR(platform, d3d_device_source, d3d_object, d3d_device_set, num_entries, devices, num_devices)
    performanceEnd('clGetDeviceIDsFromD3D11KHR')
    return ret
end

function clGetDeviceIDsFromDX9INTEL(platform, dx9_device_source, dx9_object, dx9_device_set, num_entries, devices, num_devices)
    performanceStart('clGetDeviceIDsFromDX9INTEL')
    local ret = drvCl.clGetDeviceIDsFromDX9INTEL(platform, dx9_device_source, dx9_object, dx9_device_set, num_entries, devices, num_devices)
    performanceEnd('clGetDeviceIDsFromDX9INTEL')
    return ret
end

function clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetDeviceInfo')
    local ret = drvCl.clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetDeviceInfo')
    return ret
end

function clGetEventInfo(event, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetEventInfo')
    local ret = drvCl.clGetEventInfo(event, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetEventInfo')
    return ret
end

function clGetEventProfilingInfo(event, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetEventProfilingInfo')
    local ret = drvCl.clGetEventProfilingInfo(event, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetEventProfilingInfo')
    return ret
end

function clGetExtensionFunctionAddress(function_name)
    performanceStart('clGetExtensionFunctionAddress')
    local ret = drvCl.clGetExtensionFunctionAddress(function_name)
    performanceEnd('clGetExtensionFunctionAddress')
    return ret
end

function clGetExtensionFunctionAddressForPlatform(platform, function_name)
    performanceStart('clGetExtensionFunctionAddressForPlatform')
    local ret = drvCl.clGetExtensionFunctionAddressForPlatform(platform, function_name)
    performanceEnd('clGetExtensionFunctionAddressForPlatform')
    return ret
end

function clGetGLContextInfoKHR(properties, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetGLContextInfoKHR')
    local ret = drvCl.clGetGLContextInfoKHR(properties, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetGLContextInfoKHR')
    return ret
end

function clGetGLObjectInfo(memobj, gl_object_type, gl_object_name)
    performanceStart('clGetGLObjectInfo')
    local ret = drvCl.clGetGLObjectInfo(memobj, gl_object_type, gl_object_name)
    performanceEnd('clGetGLObjectInfo')
    return ret
end

function clGetGLTextureInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetGLTextureInfo')
    local ret = drvCl.clGetGLTextureInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetGLTextureInfo')
    return ret
end

function clGetImageInfo(image, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetImageInfo')
    local ret = drvCl.clGetImageInfo(image, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetImageInfo')
    return ret
end

function clGetKernelArgInfo(kernel, arg_indx, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetKernelArgInfo')
    local ret = drvCl.clGetKernelArgInfo(kernel, arg_indx, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetKernelArgInfo')
    return ret
end

function clGetKernelInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetKernelInfo')
    local ret = drvCl.clGetKernelInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetKernelInfo')
    return ret
end

function clGetKernelSubGroupInfo(in_kernel, in_device, param_name, input_value_size, input_value, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetKernelSubGroupInfo')
    local ret = drvCl.clGetKernelSubGroupInfo(in_kernel, in_device, param_name, input_value_size, input_value, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetKernelSubGroupInfo')
    return ret
end

function clGetKernelSubGroupInfoKHR(in_kernel, in_device, param_name, input_value_size, input_value, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetKernelSubGroupInfoKHR')
    local ret = drvCl.clGetKernelSubGroupInfoKHR(in_kernel, in_device, param_name, input_value_size, input_value, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetKernelSubGroupInfoKHR')
    return ret
end

function clGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetKernelWorkGroupInfo')
    local ret = drvCl.clGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetKernelWorkGroupInfo')
    return ret
end

function clGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetMemObjectInfo')
    local ret = drvCl.clGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetMemObjectInfo')
    return ret
end

function clGetPlatformIDs(num_entries, platforms, num_platforms)
    performanceStart('clGetPlatformIDs')
    local ret = drvCl.clGetPlatformIDs(num_entries, platforms, num_platforms)
    performanceEnd('clGetPlatformIDs')
    return ret
end

function clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetPlatformInfo')
    local ret = drvCl.clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetPlatformInfo')
    return ret
end

function clGetProgramBuildInfo(program, device, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetProgramBuildInfo')
    local ret = drvCl.clGetProgramBuildInfo(program, device, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetProgramBuildInfo')
    return ret
end

function clGetProgramInfo(program, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetProgramInfo')
    local ret = drvCl.clGetProgramInfo(program, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetProgramInfo')
    return ret
end

function clGetSamplerInfo(sampler, param_name, param_value_size, param_value, param_value_size_ret)
    performanceStart('clGetSamplerInfo')
    local ret = drvCl.clGetSamplerInfo(sampler, param_name, param_value_size, param_value, param_value_size_ret)
    performanceEnd('clGetSamplerInfo')
    return ret
end

function clGetSupportedImageFormats(context, flags, image_type, num_entries, image_formats, num_image_formats)
    performanceStart('clGetSupportedImageFormats')
    local ret = drvCl.clGetSupportedImageFormats(context, flags, image_type, num_entries, image_formats, num_image_formats)
    performanceEnd('clGetSupportedImageFormats')
    return ret
end

function clLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret)
    performanceStart('clLinkProgram')
    local ret = drvCl.clLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret)
    performanceEnd('clLinkProgram')
    return ret
end

function clReleaseCommandQueue(command_queue)
    performanceStart('clReleaseCommandQueue')
    local ret = drvCl.clReleaseCommandQueue(command_queue)
    performanceEnd('clReleaseCommandQueue')
    return ret
end

function clReleaseContext(context)
    performanceStart('clReleaseContext')
    local ret = drvCl.clReleaseContext(context)
    performanceEnd('clReleaseContext')
    return ret
end

function clReleaseDevice(device)
    performanceStart('clReleaseDevice')
    local ret = drvCl.clReleaseDevice(device)
    performanceEnd('clReleaseDevice')
    return ret
end

function clReleaseDeviceEXT(device)
    performanceStart('clReleaseDeviceEXT')
    local ret = drvCl.clReleaseDeviceEXT(device)
    performanceEnd('clReleaseDeviceEXT')
    return ret
end

function clReleaseEvent(event)
    performanceStart('clReleaseEvent')
    local ret = drvCl.clReleaseEvent(event)
    performanceEnd('clReleaseEvent')
    return ret
end

function clReleaseKernel(kernel)
    performanceStart('clReleaseKernel')
    local ret = drvCl.clReleaseKernel(kernel)
    performanceEnd('clReleaseKernel')
    return ret
end

function clReleaseMemObject(memobj)
    performanceStart('clReleaseMemObject')
    local ret = drvCl.clReleaseMemObject(memobj)
    performanceEnd('clReleaseMemObject')
    return ret
end

function clReleaseProgram(program)
    performanceStart('clReleaseProgram')
    local ret = drvCl.clReleaseProgram(program)
    performanceEnd('clReleaseProgram')
    return ret
end

function clReleaseSampler(sampler)
    performanceStart('clReleaseSampler')
    local ret = drvCl.clReleaseSampler(sampler)
    performanceEnd('clReleaseSampler')
    return ret
end

function clRetainCommandQueue(command_queue)
    performanceStart('clRetainCommandQueue')
    local ret = drvCl.clRetainCommandQueue(command_queue)
    performanceEnd('clRetainCommandQueue')
    return ret
end

function clRetainContext(context)
    performanceStart('clRetainContext')
    local ret = drvCl.clRetainContext(context)
    performanceEnd('clRetainContext')
    return ret
end

function clRetainDevice(device)
    performanceStart('clRetainDevice')
    local ret = drvCl.clRetainDevice(device)
    performanceEnd('clRetainDevice')
    return ret
end

function clRetainDeviceEXT(device)
    performanceStart('clRetainDeviceEXT')
    local ret = drvCl.clRetainDeviceEXT(device)
    performanceEnd('clRetainDeviceEXT')
    return ret
end

function clRetainEvent(event)
    performanceStart('clRetainEvent')
    local ret = drvCl.clRetainEvent(event)
    performanceEnd('clRetainEvent')
    return ret
end

function clRetainKernel(kernel)
    performanceStart('clRetainKernel')
    local ret = drvCl.clRetainKernel(kernel)
    performanceEnd('clRetainKernel')
    return ret
end

function clRetainMemObject(memobj)
    performanceStart('clRetainMemObject')
    local ret = drvCl.clRetainMemObject(memobj)
    performanceEnd('clRetainMemObject')
    return ret
end

function clRetainProgram(program)
    performanceStart('clRetainProgram')
    local ret = drvCl.clRetainProgram(program)
    performanceEnd('clRetainProgram')
    return ret
end

function clRetainSampler(sampler)
    performanceStart('clRetainSampler')
    local ret = drvCl.clRetainSampler(sampler)
    performanceEnd('clRetainSampler')
    return ret
end

function clSetCommandQueueProperty(command_queue, properties, enable, old_properties)
    performanceStart('clSetCommandQueueProperty')
    local ret = drvCl.clSetCommandQueueProperty(command_queue, properties, enable, old_properties)
    performanceEnd('clSetCommandQueueProperty')
    return ret
end

function clSetEventCallback(event, command_exec_callback_type, pfn_notify, user_data)
    performanceStart('clSetEventCallback')
    local ret = drvCl.clSetEventCallback(event, command_exec_callback_type, pfn_notify, user_data)
    performanceEnd('clSetEventCallback')
    return ret
end

function clSetKernelArg(kernel, arg_index, arg_size, arg_value)
    performanceStart('clSetKernelArg')
    local ret = drvCl.clSetKernelArg(kernel, arg_index, arg_size, arg_value)
    performanceEnd('clSetKernelArg')
    return ret
end

function clSetMemObjectDestructorCallback(memobj, pfn_notify, user_data)
    performanceStart('clSetMemObjectDestructorCallback')
    local ret = drvCl.clSetMemObjectDestructorCallback(memobj, pfn_notify, user_data)
    performanceEnd('clSetMemObjectDestructorCallback')
    return ret
end

function clSetUserEventStatus(event, execution_status)
    performanceStart('clSetUserEventStatus')
    local ret = drvCl.clSetUserEventStatus(event, execution_status)
    performanceEnd('clSetUserEventStatus')
    return ret
end

function clUnloadCompiler()
    performanceStart('clUnloadCompiler')
    local ret = drvCl.clUnloadCompiler()
    performanceEnd('clUnloadCompiler')
    return ret
end

function clUnloadPlatformCompiler(platform)
    performanceStart('clUnloadPlatformCompiler')
    local ret = drvCl.clUnloadPlatformCompiler(platform)
    performanceEnd('clUnloadPlatformCompiler')
    return ret
end

function clWaitForEvents(num_events, event_list)
    performanceStart('clWaitForEvents')
    local ret = drvCl.clWaitForEvents(num_events, event_list)
    performanceEnd('clWaitForEvents')
    return ret
end
