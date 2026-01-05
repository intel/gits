-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

---------------------------------------------------------------------------------------------------
-- Description
---------------------------------------------------------------------------------------------------
-- The script can be used to do the following:
--  1) Dump the shaders used by an OpenGL app to a user-specified folder.
--  2) Override the shader(s) used by an OpenGL app with the ones in a user-specified folder.
--  3) Replace any substring found with another substring for all shaders that are intercepted.
--
-- Instructions
--  1) First, save all the shaders used by the app to a folder of your choice by setting the 'MODE'
--     to 'DUMP' and specifying the folder to save the shaders to. The folder must already exist
--     (note: use double backslash and end with a double backslash).
--
--  2) Once the shaders are saved, set the 'MODE' to 'REPLACE'. Now, the script will override the
--     API calls so that the modified shaders in the dumped folder will be used. If you only want
--     to override a specified set of shaders, fill out 'REPLACE_SHADERS'.
---------------------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------------------
-- Enums
---------------------------------------------------------------------------------------------------
-- Modes
OFF = -1
DUMP = 0 -- saves shader to the specified folder
REPLACE = 1 -- replaces shaders with those stored in the specified folder

---------------------------------------------------------------------------------------------------
-- Options (these are for you to edit)
---------------------------------------------------------------------------------------------------
MODE = OFF
REPLACE_SHADERS = {} -- Leave empty to replace all shaders

SHADER_DUMP_FILE_PATH = "" -- Use double backslash and end with a double backslash!

-- String substitution
STRING_SUBSTITUTION = false
REPLACE_ALL_OCCURRENCES = false
FROM_STRING = {""}
TO_STRING = {""}

---------------------------------------------------------------------------------------------------
-- Helper Functions
---------------------------------------------------------------------------------------------------
function save_to_file(file_path, str)
    local file = io.open(file_path, "w")

    if (file == nil) then
        print("ERROR SAVING SHADER TO FILE")
        MODE = OFF
        return
    end

    str = removeBOM(str)
    str = normalizeLineEndings(str)
    -- TODO: do we need to null terminate?
    --str = nullTerminate(str)

    file:write(str)

    file:close()
end

function get_shader_override(file_path)
  local file = io.open(file_path, "rb")
  if (file == nil) then
    print(string.format("ERROR: cannot open shader at %s", file_path))
    MODE = OFF
    return
  end

  shader = file:read("*a")
  file:close()

  shader = removeBOM(shader)
  shader = normalizeLineEndings(shader)
  -- TODO: do we need to null terminate?
  --shader = nullTerminate(shader)

  return shader
end

function removeBOM(str)
  if str:sub(1, 3) == "\239\187\191" then  -- UTF-8 BOM bytes
      return str:sub(4)
  end
  return str
end

function normalizeLineEndings(str)
  str = str:gsub("\r\n", "\n")  
  str = str:gsub("\r", "\n")
  return str
end

function nullTerminate(str)
  return str .. "\0"
end

function noLength(length)
  if (length == gits.nullUdt() or length == 0) then
    return true
  else
    local val = gits.udtToInt(gits.getUdt(length, 0))
    return val > 0xFFFFFFFF
  end
end

function tableContains(tbl, element)
    for _, value in pairs(tbl) do
        if value == element then
            return true
        end
    end
    return false
end

function tableEmpty(tbl)
  return next(tbl) == nil
end

---------------------------------------------------------------------------------------------------
-- OpenGL API Call Interception
---------------------------------------------------------------------------------------------------
function glShaderSource(shader, count, _string, length)
  local substituted_shader_udt = {}
  local new_shader_udt = {}

  if STRING_SUBSTITUTION then
    for i=0,count-1,1 do
      local shader_str = gits.udtToStr(gits.getUdt(_string, i))
      for j=1,#FROM_STRING,1 do
        if REPLACE_ALL_OCCURRENCES then
          shader_str = shader_str:gsub(FROM_STRING[j],TO_STRING[j])
        else
          shader_str = shader_str:gsub(FROM_STRING[j],TO_STRING[j], 1)
        end
      end

      substituted_shader_udt[i] = gits.allocUdtFromStr(shader_str)
      gits.setUdt(_string, i, substituted_shader_udt[i])
    end
  end

  if (MODE == DUMP) then
    local file_path = ""
    local shader_str = ""
      if count == 1 then
        if (not noLength(length)) then
          local str_length = gits.udtToInt(gits.getUdt(length, 0))
          shader_str = string.sub(gits.udtToStr(gits.getUdt(_string, 0)), 1, str_length)
        else
          shader_str = gits.udtToStr(gits.getUdt(_string, 0))
        end
        
        file_path = SHADER_DUMP_FILE_PATH .. string.format("shader_%d.txt", shader)
      else
        for i=0,count-1,1 do
          local str = ""
          if (not noLength(length)) then
            local str_length = gits.udtToInt(gits.getUdt(length, i))
            str = string.sub(gits.udtToStr(gits.getUdt(_string, i)), 1, str_length)
          else
            str = gits.udtToStr(gits.getUdt(_string, i))
          end
          
          shader_str = shader_str .. str
  
          local sub_file_path = SHADER_DUMP_FILE_PATH .. string.format("shader_%d_%d.txt", shader, i)
          save_to_file(sub_file_path, str)
        end
        file_path = SHADER_DUMP_FILE_PATH .. string.format("shader_%d_combined.txt", shader)
      end
      save_to_file(file_path, shader_str)
  end

  if ((MODE == REPLACE) and (tableEmpty(REPLACE_SHADERS) or tableContains(REPLACE_SHADERS, shader))) then
    if count == 1 then
      local file_path = SHADER_DUMP_FILE_PATH .. string.format("shader_%d.txt", shader)
      local new_shader = get_shader_override(file_path)
      new_shader_udt[0] = gits.allocUdtFromStr(new_shader)
      gits.setUdt(_string, 0, new_shader_udt[0])
    else
      for i=0,count-1,1 do
        local sub_file_path = SHADER_DUMP_FILE_PATH .. string.format("shader_%d_%d.txt", shader, i)
        local new_shader = get_shader_override(sub_file_path)
        new_shader_udt[i] = gits.allocUdtFromStr(new_shader)
        gits.setUdt(_string, i, new_shader_udt[i])
      end
    end
  end

  drv.glShaderSource(shader, count, _string, 0)

  -- Clean up
  if (STRING_SUBSTITUTION) then
    for i=0,count-1,1 do
      gits.freeBytes(substituted_shader_udt[i])
    end
  end
  if (MODE == REPLACE) then
    for i=0,count-1,1 do
      gits.freeBytes(new_shader_udt[i])
    end
  end
end
