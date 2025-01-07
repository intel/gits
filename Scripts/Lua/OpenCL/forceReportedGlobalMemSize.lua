-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

CL_SUCCESS = 0
CL_DEVICE_GLOBAL_MEM_SIZE = 0x101F
CL_DEVICE_MAX_MEM_ALLOC_SIZE = 0x1010

MEM_SIZE = 1024 * 1024 * 1024
MAX_MEM_ALLOC_SIZE = 128 * 1024 * 1024 -- minimum (spec)

function clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    if param_name == CL_DEVICE_GLOBAL_MEM_SIZE then
        if gits.udtToInt(param_value) ~= 0 then
            gits.setInt64(param_value, 0, MEM_SIZE)
        end
        if gits.udtToInt(param_value_size_ret) ~= 0 then
            gits.setSizeT(param_value_size_ret, 0, 8)
        end
        return CL_SUCCESS
    elseif param_name == CL_DEVICE_MAX_MEM_ALLOC_SIZE then
        if gits.udtToInt(param_value) ~= 0 then
            gits.setInt64(param_value, 0, MAX_MEM_ALLOC_SIZE)
        end
        if gits.udtToInt(param_value_size_ret) ~= 0 then
            gits.setSizeT(param_value_size_ret, 0, 8)
        end
        return CL_SUCCESS
    else
        return drvCl.clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    end
end
