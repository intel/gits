-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

PTR_SIZE = gits.getPtrSize()

KERNEL = 0

CL_COMPLETE = 0

function allocEvent()
    return gits.allocBytes(PTR_SIZE)
end

function eventCallback(event, status, user_data)
    gits.log(1, 'clEnqueueNDRangeKernel number ' .. gits.getInt(user_data, 0) .. ' completed.')
    drvCl.clReleaseEvent(event)
    gits.freeBytes(user_data)
end

function memobjCallback(memobj, user_data)
    gits.log(1, 'Buffer destroyed.')
end

function clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    local ret = drvCl.clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    local callback = drvCl.getMemObjCallbackPtr()
    local data = drvCl.setCallbackData(memobjCallback, gits.nullUdt())
    drvCl.clSetMemObjectDestructorCallback(ret, callback, data)
    return ret
end

function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
    KERNEL = KERNEL + 1
    local newEvent = event
    if gits.udtToInt(event) == 0 then
        newEvent = allocEvent()
    end
    local ret = drvCl.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, newEvent)
    if gits.udtToInt(event) == 0 then
        local kernelCounterPtr = gits.allocBytes(4)
        gits.setInt(kernelCounterPtr, 0, KERNEL)
        local callback = drvCl.getEventCallbackPtr()
        local data = drvCl.setCallbackData(eventCallback, kernelCounterPtr)
        drvCl.clSetEventCallback(gits.getUdt(newEvent, 0), CL_COMPLETE, callback, data)
    end
    return ret
end
