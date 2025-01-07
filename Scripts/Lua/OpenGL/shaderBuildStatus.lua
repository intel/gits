-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

local max_size = 1024*64
local GL_PROGRAM_ERROR_STRING_ARB = 0x8874
local GL_PROGRAM_BINDING_ARB = 0x8677

function glCompileShader(shd)
	drv.glCompileShader(shd)

	local data = gits.allocBytes(max_size)
	drv.glGetShaderInfoLog(shd, max_size, gits.nullUdt(), data)

	print('shader ' .. shd .. ' status log: ')
	print(gits.udtToStr(data))

	gits.freeBytes(data)
end

function glCompileShaderARB(shd)
	drv.glCompileShaderARB(shd)

	local data = gits.allocBytes(max_size)
	drv.glGetShaderInfoLogARB(shd, max_size, gits.nullUdt(), data)

	print('shader ' .. shd .. ' status log: ')
	print(gits.udtToStr(data))

	gits.freeBytes(data)
end

function glLinkProgram(prg)
	drv.glLinkProgram(prg)

	local data = gits.allocBytes(max_size)
	drv.glGetProgramInfoLog(prg, max_size, gits.nullUdt(), data)

	print('program ' .. prg .. ' status log: ')
	print(gits.udtToStr(data))

	gits.freeBytes(data)
end

function glLinkProgramARB(prg)
	drv.glLinkProgramARB(prg)

	local data = gits.allocBytes(max_size)
	drv.glGetProgramInfoLogARB(prg, max_size, gits.nullUdt(), data)

	print('program ' .. prg .. ' status log: ')
	print(gits.udtToStr(data))

	gits.freeBytes(data)
end


function glProgramStringARB(target, format, len, string)
  prg = gits.allocBytes(4)
  drv.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, prg)
  drv.glProgramStringARB(target, format, len, string)
  local cstr = drv.glGetString(GL_PROGRAM_ERROR_STRING_ARB)
  print('ARB program ' ..gits.getInt(prg, 0).. ' status log: ')
  print(gits.udtToStr(cstr))
end
