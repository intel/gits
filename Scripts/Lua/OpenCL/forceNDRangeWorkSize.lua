-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- Script overwrites GWS and LWS sizes

function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
    local work_size = tonumber(gits.getArgsStr())
    local work_size_ptr = gits.allocBytes(gits.getPtrSize() * work_dim)
    for i=0, work_dim-1 do
        gits.setSizeT(work_size_ptr, i, work_size)
    end
    local ret = drvCl.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, work_size_ptr, local_work_size, num_events_in_wait_list, event_wait_list, event)
    gits.freeBytes(work_size_ptr)
    return ret
end
