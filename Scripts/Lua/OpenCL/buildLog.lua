-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

PTR_SIZE = gits.getPtrSize()

CL_PROGRAM_NUM_DEVICES = 0x1162
CL_PROGRAM_DEVICES = 0x1163

CL_PROGRAM_BUILD_STATUS = 0x1181
CL_PROGRAM_BUILD_OPTIONS = 0x1182
CL_PROGRAM_BUILD_LOG = 0x1183

function clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
    local return_value = drvCl.clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
    local size_ret_ptr = gits.allocBytes(PTR_SIZE)
    local devices_queried = false
    if num_devices == 0 then
        drvCl.clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, PTR_SIZE, size_ret_ptr, gits.nullUdt())
        num_devices = gits.getInt(size_ret_ptr, 0)
        device_list = gits.allocBytes(num_devices * PTR_SIZE) -- cl_device_id is _cl_device_id*
        drvCl.clGetProgramInfo(program, CL_PROGRAM_DEVICES, num_devices * PTR_SIZE, device_list, gits.nullUdt())
        devices_queried = true
    end
    for i = 0, num_devices - 1 do
        drvCl.clGetProgramBuildInfo(program, gits.getUdt(device_list, i), CL_PROGRAM_BUILD_LOG, gits.nullUdt(), gits.nullUdt(), size_ret_ptr)
        local log_len = gits.getInt(size_ret_ptr, 0)
        gits.log(2, 'Log length: ' .. log_len)
        local build_log = gits.allocBytes(log_len)
        drvCl.clGetProgramBuildInfo(program, gits.getUdt(device_list, i), CL_PROGRAM_BUILD_LOG, log_len, build_log, gits.nullUdt())
        gits.log(2, 'Build log:\n' .. gits.udtToStr(build_log))
        gits.freeBytes(build_log)
    end
    if devices_queried then
        gits.freeBytes(device_list)
    end
    gits.freeBytes(size_ret_ptr)
    return return_value
end
