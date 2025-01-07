-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- Removes source lengths. Makes driver assume strings are null-terminated
-- which in turn allows kernel sources to be modified freely.

function clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateProgramWithSource(context, count, strings, gits.nullUdt(), errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end
