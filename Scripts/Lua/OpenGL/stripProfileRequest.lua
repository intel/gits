-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

local WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126

function wglCreateContextAttribsARB(dc, shared_context, attribs)
	local atr = gits.nullUdt
	if attribs ~= atr then
		local attribs_array = {}
		local idx = 0

		-- make lua array copy of requested attribytes w/o profile part
		local name = gits.getInt(attribs, idx)
		while name ~= 0 do
			if name ~= WGL_CONTEXT_PROFILE_MASK_ARB then
				table.insert(attribs_array, name)
				local valu = gits.getInt(attribs, idx + 1)
				table.insert(attribs_array, value)
			end
			idx = idx + 2
			name = gits.getInt(attribs, idx)
		end
		table.insert(attribs_array, 0)

		-- make a native array with the content of lua array created before
		atr = gits.allocBytes((idx + 1) * 4)
		for k,v in pairs(attribs_array) do
			gits.setInt(atr, k - 1, v)
		end
	end

	local ret = drv.wglCreateContextAttribsARB(dc, shared_context, atr)
	gits.freeBytes(atr)

	return ret
end

