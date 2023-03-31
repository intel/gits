-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

CL_QUEUE_CONTEXT = 0x1090
CL_COMPLETE = 0
CL_SUCCESS = 0

PTR_SIZE = gits.getPtrSize()

function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset,
                                global_work_size, local_work_size, num_events_in_wait_list,
                                event_wait_list, event)
    if gits.udtToInt(event) ~= 0 then
        local context = gits.allocBytes(PTR_SIZE)
        drvCl.clGetCommandQueueInfo(command_queue, CL_QUEUE_CONTEXT, PTR_SIZE, context, gits.nullUdt())
        local new_event = drvCl.clCreateUserEvent(gits.getUdt(context, 0), gits.nullUdt())
        drvCl.clSetUserEventStatus(new_event, CL_COMPLETE)
        gits.setUdt(event, 0, new_event)
        gits.freeBytes(context)
    end
    return CL_SUCCESS
end

