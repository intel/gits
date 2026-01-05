-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

function clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
    local options_ptr = gits.allocUdtFromStr(gits.getArgsStr())
    local return_value = drvCl.clBuildProgram(program, num_devices, device_list, options_ptr, pfn_notify, user_data)
    gits.freeBytes(options_ptr)
    return return_value
end

function clCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data)
    local options_ptr = gits.allocUdtFromStr(gits.getArgsStr())
    local return_value = drvCl.clCompileProgram(program, num_devices, device_list, options_ptr, num_input_headers, input_headers, header_include_names, pfn_notify, user_data)
    gits.freeBytes(options_ptr)
    return return_value
end
