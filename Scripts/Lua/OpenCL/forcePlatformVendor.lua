-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

CL_SUCCESS = 0
CL_PLATFORM_VENDOR = 0x0903

VENDOR = '### put vendor here ###'

function clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret)
    if param_name == CL_PLATFORM_VENDOR then
        if gits.udtToInt(param_value) == 0 then
            if gits.udtToInt(param_value_size_ret) ~= 0 then
                gits.setInt(param_value_size_ret, 0, string.len(VENDOR) + 1)
            end
        else
            max_len = math.max(param_value_size - 1, string.len(VENDOR))
            for i=0, max_len-1 do
                gits.setByte(param_value, i, string.byte(VENDOR, i + 1))
            end
            gits.setByte(param_value, max_len, 0)
        end
        return CL_SUCCESS
    else
        return drvCl.clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret)
    end
end
