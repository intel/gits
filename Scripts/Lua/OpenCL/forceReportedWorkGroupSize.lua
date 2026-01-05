-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

CL_SUCCESS = 0
CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS = 0x1003
CL_DEVICE_MAX_WORK_GROUP_SIZE = 0x1004
CL_DEVICE_MAX_WORK_ITEM_SIZES = 0x1005
CL_DEVICE_ADDRESS_BITS = 0x100D
CL_KERNEL_WORK_GROUP_SIZE = 0x11B0

SIZE = 64
PTR_SIZE = gits.getPtrSize()

function clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    if param_name == CL_DEVICE_MAX_WORK_GROUP_SIZE then
        if gits.udtToInt(param_value) ~= 0 then
            gits.setSizeT(param_value, 0, SIZE)
        end
        if gits.udtToInt(param_value_size_ret) ~= 0 then
            gits.setSizeT(param_value_size_ret, 0, address_bits)
        end
        return CL_SUCCESS
    elseif param_name == CL_DEVICE_MAX_WORK_ITEM_SIZES then
        local dimensions_ptr = gits.allocBytes(4)
        local ret = drvCl.clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, 4, dimensions_ptr, gits.nullUdt())
        if ret == CL_SUCCESS and gits.udtToInt(param_value) ~= 0 then
            for i=0, gits.getInt(dimensions_ptr, 0) do
                gits.setSizeT(param_value, i, SIZE)
            end
        end
        if gits.udtToInt(param_value_size_ret) ~= 0 then
            gits.setSizeT(param_value_size_ret, 0, address_bits * gits.getInt(dimensions_ptr, 0))
        end
        gits.freeBytes(dimensions_ptr)
        return ret
    else
        return drvCl.clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret)
    end
end

function clGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret)
    if param_name == CL_KERNEL_WORK_GROUP_SIZE then
        if gits.udtToInt(param_value) ~= 0 then
            gits.setSizeT(param_value, 0, SIZE)
        end
        return CL_SUCCESS
    end
end
