-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2024 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

CL_SUCCESS = 0
CL_DEVICE_TYPE = 0x1000

CL_DEVICE_TYPE_DEFAULT = 0x00000001
CL_DEVICE_TYPE_CPU = 0x00000002
CL_DEVICE_TYPE_GPU = 0x00000004
CL_DEVICE_TYPE_ACCELERATOR = 0x00000008
CL_DEVICE_TYPE_CUSTOM = 0x00000016
CL_DEVICE_TYPE_ALL = 0xFFFFFFFF

TYPE = CL_DEVICE_TYPE_GPU

-- reported
function clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    if param_name == CL_DEVICE_TYPE then
        if gits.udtToInt(param_value) ~= 0 then
            gits.setInt(param_value, 0, TYPE)
        end
        if gits.udtToInt(param_value_size_ret) ~= 0 then
            gits.setSizeT(param_value_size_ret, 0, 8)
        end
        return CL_SUCCESS
    else
        return drvCl.clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    end
end

-- requested
function clGetDeviceIDs(platform, device_type, num_entries, devices, num_devices)
    if gits.udtToInt(devices) ~= 0 then
        for i=0, num_entries-1 do
            local ret = drvCl.clGetDeviceIDs(platform, TYPE, 1, devices, gits.nullUdt())
        end
    end
    if gits.udtToInt(num_devices) ~= 0 then
        gits.setInt(num_devices, 0, 1)
    end
    return ret
end

function clCreateContextFromType(properties, device_type, pfn_notify, user_data, errcode_ret)
    return drvCl.clCreateContextFromType(properties, TYPE, pfn_notify, user_data, errcode_ret)
end
