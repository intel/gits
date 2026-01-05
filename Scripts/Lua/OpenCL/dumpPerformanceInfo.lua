-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- OpenCL performance dump in old GITS format.
-- Dumps GPU profiling info for NDRangeKernel.

local info = debug.getinfo(1, 'S')                   -- get source file name
local dir = string.match(info.source, '[^@]*/')            -- get directory part
if dir == nil then
    dir = '.'
end
ossep = package.config:sub(1,1)
dir = string.gsub(dir, '/', ossep)                    -- replace / with OS Separator
package.path = package.path .. ';' .. dir .. '?.lua' -- append to package.path
require 'clconstants'

PTR_SIZE = gits.getPtrSize()
INT_SIZE = 4
ULONG_SIZE = 8


-- Global variables
OCL_MAJ_VERSION_FOR_QUEUE = {}
OCL_MAJ_VERSION_FOR_DEVICE = {}

CALL_RESULTS = {}
CALLS = {}
INJECTED_EVENTS = {}
PENDING_EVENTS = {}
KERNEL_NAMES = {}
ENQUEUE_FOR_PENDING_EVENT = {}
ENQUEUE_COUNTER = 0
ENQUEUE_INFO = {}

status_ret_ptr = gits.allocBytes(PTR_SIZE)
status_size_ptr = gits.allocBytes(PTR_SIZE)
time_ret_ptr = gits.allocBytes(ULONG_SIZE)
device_address_bits = 0


EVINFO_HEADER = {'enqueue_no', 'kernel', 'workdim', 'gws', 'lws', 'oclver', 'queued_ns', 'submit_ns', 'start_ns', 'end_ns', 'complete_ns', 'duration_ns'}
IND_EVINFO_ENQUEUECOUNTER = 1
IND_EVINFO_KERNEL = 2
IND_EVINFO_WORKDIM = 3
IND_EVINFO_GWS = 4
IND_EVINFO_LWS = 5
IND_EVINFO_OCLVER = 6
IND_EVINFO_TS_QUEUE = 7
IND_EVINFO_TS_SUBMIT = 8
IND_EVINFO_TS_START = 9
IND_EVINFO_TS_END = 10
IND_EVINFO_TS_COMPLETE = 11
IND_EVINFO_TS_DURATION = 12
IND_EVINFO_EVENTID = 13
IND_EVINFO_LAST = IND_EVINFO_TS_DURATION   -- Eventid need not go into output, so ending it earlier



function get_ws(wsize, wdim)
    res = ""
    local ws = gits.udtToInt(wsize)
    if ws == 0 then
        return "0"
    end
    for i = 1, wdim do
        if device_address_bits == 64 then
            ws = gits.getInt64(wsize, i-1)
        else
            ws = gits.getInt(wsize, i-1)
        end
        if i > 1 then
            res = res .. "x" .. ws
        else
            res = res .. ws
        end
    end
    return res
end

function count_dict(dct)
    count = 0
    for k, v in pairs(dct) do
        count = count + 1
    end
    return count
end


function set_oclver(device, value)
    local verstring = gits.udtToStr(value)
    if verstring ~= nil then
        OCL_MAJ_VERSION_FOR_DEVICE[device] = string.sub(verstring, 8, 8)
    end
end


function find_oclver(device)
    if OCL_MAJ_VERSION_FOR_DEVICE[device] == nil
    then
        local param_value_size_ptr = gits.allocBytes(PTR_SIZE)
        local ret = drvCl.clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, 0, param_value_size_ptr)
        if ret ~= CL_SUCCESS then
            return nil
        end
        valuesize = gits.getSizeT(param_value_size_ptr, 0)

        local verUdt = gits.allocBytes(valuesize+1)
        ret = drvCl.clGetDeviceInfo(device, CL_DEVICE_VERSION, valuesize, verUdt, 0)
        set_oclver(device, verUdt)

        local ret = drvCl.clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, 4, param_value_size_ptr, 0)
        if ret ~= CL_SUCCESS then
            return nil
        end
        device_address_bits = gits.getSizeT(param_value_size_ptr, 0)
        gits.freeBytes(param_value_size_ptr)
        gits.freeBytes(verUdt)
    end
    return OCL_MAJ_VERSION_FOR_DEVICE[device]
end



function gitsProgramExit()
    -- Check if there are any waiting events now
    checkWaitingEvents()

    for k,v in pairs(ENQUEUE_INFO) do
        kernel = v[IND_EVINFO_KERNEL]
        v[IND_EVINFO_KERNEL] = KERNEL_NAMES[kernel]
    end
    local last_col = IND_EVINFO_LAST
    local path = 'benchmark.csv'
    out_dir = gits.getOutDir()
    if out_dir ~= gits.nullUdt() and out_dir ~= '' then
      path = out_dir .. ossep .. path
    end
    local csvFile = io.open(path, 'w')
    if csvFile == nil then
        error('Failed to open results file for writing.')
        return
    end

    io.output(csvFile)

    local header = ''
    for j = 1, last_col do
        if EVINFO_HEADER[j] ~= nil then
            header = header .. EVINFO_HEADER[j]
        end
        if j ~= last_col then
            header = header .. ','
        end
    end
    io.write(header .. '\n')

    for k,v in ipairs(ENQUEUE_INFO) do
        local c = v
        local line = ''
        for j = 1, last_col do
            if type(c[j]) == 'number'  then
                if j >= IND_EVINFO_TS_QUEUE and j <= IND_EVINFO_TS_COMPLETE then
                    c[j] = string.format("%.0f", c[j])
                else
                    c[j] = string.format("%d", c[j])
                end
            end
            if c[j] ~= nil then
                line = line .. c[j]
            end
            if j ~= last_col then
                line = line .. ','
            end
        end
        io.write(line .. '\n')
    end

    io.close(csvFile)
    gits.freeBytes(status_ret_ptr)
    gits.freeBytes(time_ret_ptr)
    gits.freeBytes(status_size_ptr)

end


function getProfCounter_us(event, counter)
    local errprof = drvCl.clGetEventProfilingInfo(event, counter, ULONG_SIZE, time_ret_ptr, gits.nullUdt())
    if errprof ~= CL_SUCCESS then print(errprof); abort() end
    return gits.getInt64(time_ret_ptr, 0)
end


function checkEvent(event)
    eventinfo = ENQUEUE_FOR_PENDING_EVENT[event]
    if eventinfo == nil then   -- In an earlier sync point, the event may have already got over
                               -- without the workload having waited for it.
        return false
    end
    err = drvCl.clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS, INT_SIZE, status_ret_ptr, status_size_ptr)
    if err ~= CL_SUCCESS then
        return false
    end
    local status = gits.getInt(status_ret_ptr, 0)
    if status ~= CL_COMPLETE then
        return false
    end
    eventinfo[IND_EVINFO_TS_QUEUE]    = getProfCounter_us(event, CL_PROFILING_COMMAND_QUEUED)
    eventinfo[IND_EVINFO_TS_SUBMIT]   = getProfCounter_us(event, CL_PROFILING_COMMAND_SUBMIT)
    eventinfo[IND_EVINFO_TS_START]    = getProfCounter_us(event, CL_PROFILING_COMMAND_START)
    eventinfo[IND_EVINFO_TS_END]      = getProfCounter_us(event, CL_PROFILING_COMMAND_END)
    if eventinfo[IND_EVINFO_OCLVER] == '2' then
        eventinfo[IND_EVINFO_TS_COMPLETE] = getProfCounter_us(event, CL_PROFILING_COMMAND_COMPLETE)
    else
        eventinfo[IND_EVINFO_TS_COMPLETE] = eventinfo[IND_EVINFO_TS_END]
    end
    eventinfo[IND_EVINFO_TS_DURATION] = eventinfo[IND_EVINFO_TS_COMPLETE] - eventinfo[IND_EVINFO_TS_START]
    ENQUEUE_INFO[eventinfo[IND_EVINFO_ENQUEUECOUNTER]] = eventinfo
    return true
end


function checkWaitingSingleEvent(event)
    if checkEvent(event) then
        ENQUEUE_FOR_PENDING_EVENT[event] = nil
    end
end



function checkWaitingEvents()
    local completed_enqueues = {}

    -- Is it ok to modify the table inside the loop iterating on it? Not sure.
    -- So, conservatively doing this in two passes.
    -- Modifying the container being iterated can be a hazardous operation, unless the language specifically allows it

    for event, eventinfo in pairs(ENQUEUE_FOR_PENDING_EVENT) do
        if checkEvent(event) then
            table.insert(completed_enqueues, event)
        end
    end
    for i = 1, #completed_enqueues do
          event = completed_enqueues[i]
          ENQUEUE_FOR_PENDING_EVENT[event] = nil
    end
end


function clCreateCommandQueue(context, device, properties, errcode_ret)
    properties = properties | CL_QUEUE_PROFILING_ENABLE
    local ret = drvCl.clCreateCommandQueue(context, device, properties, errcode_ret)
    OCL_MAJ_VERSION_FOR_QUEUE[ret] = find_oclver(device)
    return ret
end


function clCreateCommandQueueWithProperties(context, device, properties, errcode_ret)
    local propcount = 0
    local properties_index = -1
    -- Count properties, and get CL_QUEUE_PROPERTIES index, if one exists
    if properties ~= gits.nullUdt() then
        while true do
            propvalue = gits.getInt64(properties, propcount)
            if propvalue == 0 then
                break
            end
            if propvalue == CL_QUEUE_PROPERTIES then
                properties_index = propcount + 1
            end
            propcount = propcount + 2
        end
    end
    if properties_index == -1 then
        -- If properties does not exist, the list should be extended
        -- and a new properties value appended
        newpropcount = propcount + 3
        newproperties = gits.allocBytes(ULONG_SIZE*newpropcount)
        for i = 0, propcount-1 do
            gits.setInt64(newproperties, i, gits.getInt64(properties, i))
        end
        gits.setInt64(newproperties, propcount, CL_QUEUE_PROPERTIES)
        properties_index = propcount+1
        gits.setInt64(newproperties, propcount+1, 0)
        gits.setInt64(newproperties, propcount+2, 0)
        properties = newproperties
        propcount = propcount + 2
    end
    local properties_value = gits.getInt64(properties, properties_index)
    properties_value = properties_value | CL_QUEUE_PROFILING_ENABLE
    gits.setInt64(properties, properties_index, properties_value)
    local ret = drvCl.clCreateCommandQueueWithProperties(context, device, properties, errcode_ret)
    OCL_MAJ_VERSION_FOR_QUEUE[ret] = find_oclver(device)
    return ret
end

function clCreateKernel(program, kernel_name, errcode_ret)
    local ret = drvCl.clCreateKernel(program, kernel_name, errcode_ret)
    KERNEL_NAMES[ret] = gits.udtToStr(kernel_name)
    return ret
end


function getKernelFunctionName(kernel)
    local name_len_ptr = gits.allocBytes(PTR_SIZE)
    local r1 = drvCl.clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, gits.nullUdt(), gits.nullUdt(), name_len_ptr)
    if r1 ~= CL_SUCCESS then
        gits.log(5, 'kernel name could not be determined (clCreateKernelsInProgram)')
        abort()
    end
    local name_len = gits.getInt(name_len_ptr, 0)
    local name_ptr = gits.allocBytes(name_len+1)
    local r2 = drvCl.clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, name_len, name_ptr, gits.nullUdt())
    local name = gits.udtToStr(name_ptr)
    gits.freeBytes(name_len_ptr)
    gits.freeBytes(name_ptr)
    return name
end


function clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    local ret = drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    if gits.udtToInt(kernels) ~= 0 then
        local num = 0
        if gits.udtToInt(num_kernels_ret) == 0 then
            new_num_kernels_ret = gits.allocBytes(INT_SIZE)
            ret = drvCl.clCreateKernelsInProgram(program, num_kernels, gits.nullUdt(), new_num_kernels_ret)
            num = gits.getInt(new_num_kernels_ret, 0)
            gits.freeBytes(new_num_kernels_ret)
        else
            num = gits.getInt(num_kernels_ret, 0)
        end
        for i = 0, num-1 do
            local kernel = gits.getUdt(kernels, i)
            local kname = getKernelFunctionName(kernel)
            KERNEL_NAMES[kernel] = kname
        end
    end
    return ret
end


function clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    local ret = drvCl.clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    if blocking_map then
        checkWaitingEvents()
    end
    return ret
end

function clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    local ret = drvCl.clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    if blocking_map then
        checkWaitingEvents()
    end
    return ret
end

function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
    ENQUEUE_COUNTER = ENQUEUE_COUNTER + 1
    local injected = false
    if gits.udtToInt(event) == 0 then
        -- inject event
        event = gits.allocBytes(PTR_SIZE) -- cl_event is _cl_event*
        injected = true
        table.insert(INJECTED_EVENTS, event)
    end
    local ret = drvCl.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
    gwsstring = get_ws(global_work_size, work_dim)
    lwsstring = get_ws(local_work_size, work_dim)
    eventid = gits.getUdt(event, 0) -- Deref ptr
    enqueue_info = {ENQUEUE_COUNTER, kernel, work_dim, gwsstring, lwsstring, OCL_MAJ_VERSION_FOR_QUEUE[command_queue], '', 0, 0, 0, 0, 0, eventid}
    ENQUEUE_FOR_PENDING_EVENT[eventid] = enqueue_info
    return ret
end

function clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    local ret = drvCl.clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    if blocking_read then
        checkWaitingEvents()
    end
    return ret
end

function clEnqueueReadBufferRect(command_queue, buffer, blocking_read, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    local ret = drvCl.clEnqueueReadBufferRect(command_queue, buffer, blocking_read, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    if blocking_read then
         checkWaitingEvents()
     end
    return ret
end

function clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    local ret = drvCl.clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    if blocking_read then
         checkWaitingEvents()
    end
    return ret
end

function clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    local ret = drvCl.clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event)
    if blocking_write then
       checkWaitingEvents()
    end
    return ret
end

function clEnqueueWriteBufferRect(command_queue, buffer, blocking_write, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    local ret = drvCl.clEnqueueWriteBufferRect(command_queue, buffer, blocking_write, buffer_offset, host_offset, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    if blocking_write then
        checkWaitingEvents()
    end
    return ret
end

function clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    local ret = drvCl.clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
    if blocking_write then
        checkWaitingEvents()
    end
    return ret
end

function clFinish(command_queue)
    local ret = drvCl.clFinish(command_queue)
    checkWaitingEvents()
    return ret
end

function clFlush(command_queue)
    local ret = drvCl.clFlush(command_queue)
    checkWaitingEvents()
    return ret
end

function clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    local ret = drvCl.clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    if param_name == CL_DEVICE_VERSION then
        set_oclver(device, param_value)
    end
    return ret
end

function clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret)
    local ret = drvCl.clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret)
    return ret
end

function clReleaseEvent(event)
    local ret = CL_SUCCESS
    if checkWaitingSingleEvent(event, 'clReleaseEvent') then
        -- Do NOT chain to drivers ReleaseEvent if the event is not complete
        ret = drvCl.clReleaseEvent(event)
    end
    return ret
end

function clWaitForEvents(num_events, event_list)
    local ret = drvCl.clWaitForEvents(num_events, event_list)
    checkWaitingEvents()
    return ret
end
