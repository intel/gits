-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================
local info = debug.getinfo(1, 'S')                   -- get source file name
local dir = string.match(info.source, '[^@]*/')      -- get directory part
if dir == nil then
    dir = '.'
end
ossep = package.config:sub(1,1)
dir = string.gsub(dir, '/', ossep)                   -- replace / with OS Separator
package.path = package.path .. ';' .. dir .. '?.lua' -- append to package.path
require 'l0Constants'

WARN = 4

function zeMemAllocHost(hContext, host_desc, size, alignment, pptr)
  local ret = drvl0.zeMemAllocHost(hContext, host_desc, size, alignment, pptr)
  if ret == ZE_RESULT_SUCCESS then
    local ptr = gits.getUdt(pptr, 0)
    local int_ptr = gits.udtToInt(ptr)
    local output = string.format("%x", int_ptr)
    gits.log(WARN, output)
    local _, zero_numbers_count = string.gsub(output, "0", "")
    local _, non_zero_numbers_count = string.gsub(output, "[1-9a-fA-F]", "")
    gits.log(WARN, zero_numbers_count)
    gits.log(WARN, non_zero_numbers_count)
    if non_zero_numbers_count > zero_numbers_count then
      gits.setUdt(pptr, 0, ptr)
      return ret
    else
      ret = zeMemAllocHost(hContext, host_desc, size, alignment, pptr)
      drvl0.zeMemFree(hContext, ptr)
      return ret
    end
  end
  return ret
end

function zeMemAllocShared(hContext, device_desc, host_desc, size, alignment, hDevice, pptr)
  local ret = drvl0.zeMemAllocShared(hContext, device_desc, host_desc, size, alignment, hDevice, pptr)
  if ret == ZE_RESULT_SUCCESS then
    local ptr = gits.getUdt(pptr, 0)
    local int_ptr = gits.udtToInt(ptr)
    local output = string.format("%x", int_ptr)
    gits.log(WARN, output)
    local _, zero_numbers_count = string.gsub(output, "0", "")
    local _, non_zero_numbers_count = string.gsub(output, "[1-9a-fA-F]", "")
    gits.log(WARN, zero_numbers_count)
    gits.log(WARN, non_zero_numbers_count)
    if non_zero_numbers_count > zero_numbers_count then
      gits.setUdt(pptr, 0, ptr)
      return ret
    else
      ret = zeMemAllocShared(hContext, device_desc, host_desc, size, alignment, hDevice, pptr)
      drvl0.zeMemFree(hContext, ptr)
      return ret
    end
  end
  return ret
end
