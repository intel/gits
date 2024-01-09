-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2024 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================



WGL_CONTEXT_FLAGS_ARB = 0x2094
WGL_CONTEXT_DEBUG_BIT_ARB = 0x0001
EGL_CONTEXT_FLAGS_KHR = 0x30FC
EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR = 0x0001
EGL_NONE = 12344
usedCtxs = {}

-- WGL Windows

function wglCreateContext(hdc)
  local curr_ctx = drv.wglGetCurrentContext()
  
  -- Create context using basic OpenGL first if there is no current contex
  local temp_ctx = gits.getNullUdt;
  if gits.udtToInt(curr_ctx) == 0 then
    temp_ctx = drv.wglCreateContext(hdc)
    drv.wglMakeCurrent(hdc, temp_ctx)
  end
  -- Create context attribs table
  attribs = gits.allocBytes(3*4)
  gits.setInt(attribs, 0, WGL_CONTEXT_FLAGS_ARB)
  gits.setInt(attribs, 1, WGL_CONTEXT_DEBUG_BIT_ARB)
  gits.setInt(attribs, 2, 0)

  local ctx = drv.wglCreateContextAttribsARB(hdc, gits.NullUdt, attribs)
  gits.freeBytes(attribs)
  
  -- Remove temporary context
  if temp_ctx ~= gits.getNullUdt then
    drv.wglDeleteContext(temp_ctx)
    drv.wglMakeCurrent(0,0)
  end

  return ctx;
end

function wglCreateContextAttribsARB(dc, shared_context, attribs)
  local attribs_array = {}
  local idx = 0
  local origContextFlags = 0

  if attribs ~= gits.NullUdt then
		-- make lua array copy of non modified attributes part
		local name = gits.getInt(attribs, idx)
		while name ~= 0 do
     if name == WGL_CONTEXT_FLAGS_ARB then
        idx = idx + 1 --skip
		    origContextFlags = gits.getInt(attrib_list, idx) --copy original context flags value
        idx = idx + 1
      else
        table.insert(attribs_array, name)
			  idx = idx + 1
		    local value = gits.getInt(attribs, idx)
			  table.insert(attribs_array, value)
			  idx = idx + 1
      end
			  name = gits.getInt(attribs, idx)
		end
  end
  idx = idx + 3
  table.insert(attribs_array, WGL_CONTEXT_FLAGS_ARB)
  table.insert(attribs_array, WGL_CONTEXT_DEBUG_BIT_ARB | origContextFlags)
  table.insert(attribs_array, 0)
	
  -- make a native array with the content of lua array created before
  new_attribs = gits.allocBytes(idx * 4)
  for k,v in pairs(attribs_array) do
	  gits.setInt(new_attribs, k - 1, v)
  end

  local ret = drv.wglCreateContextAttribsARB(dc, shared_context, new_attribs)
  gits.freeBytes(new_attribs)

  return ret
end

function wglMakeCurrent(hdc, ctx)
  local ret = drv.wglMakeCurrent(hdc, ctx)
  -- set callback function only once per context
  if gits.udtToInt(ctx) ~= 0 and not usedCtxs[gits.udtToInt(ctx)] then
    usedCtxs[gits.udtToInt(ctx)] = true
    drv.glDebugMessageCallback(gits.gitsGlDebugProc(), 0)
	end
  return ret
end

-- EGL/OpenGLES Linux/Windows

function eglCreateContext(display, config, share_context, attrib_list)
	local new_attribs_array = {}
  local idx = 0
  local origContextFlags = 0

  if attrib_list ~= gits.NullUdt then
		-- make lua array copy of non modified attributes part
		local name = gits.getInt(attrib_list, idx)
		while name ~= EGL_NONE do
      if name == EGL_CONTEXT_FLAGS_KHR then
        idx = idx + 1 --skip
		    origContextFlags = gits.getInt(attrib_list, idx) --copy original context flags value
        idx = idx + 1
      else
		    table.insert(new_attribs_array, name)
			  idx = idx + 1
		    local value = gits.getInt(attrib_list, idx)
			  table.insert(new_attribs_array, value)
			  idx = idx + 1
      end
			name = gits.getInt(attrib_list, idx)
		end
  end
  idx = idx + 3
  -- add modified attribs to lua array
  table.insert(new_attribs_array, EGL_CONTEXT_FLAGS_KHR)
  table.insert(new_attribs_array, origContextFlags | EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR)
  table.insert(new_attribs_array, EGL_NONE)

  -- make a native array with the content of lua array created before
  new_Cattribs = gits.allocBytes(idx * 4)
  for k,v in pairs(new_attribs_array) do
		print(v)
	  gits.setInt(new_Cattribs, k - 1, v)
  end
  local ret = drv.eglCreateContext(display, config, share_context, new_Cattribs)
	gits.freeBytes(new_Cattribs)

  return ret
end

function eglMakeCurrent(dpy, draw, read, ctx)
  local ret = drv.eglMakeCurrent(dpy, draw, read, ctx)
  -- set callback function only once per context
  if gits.udtToInt(ctx) ~= 0 and not usedCtxs[gits.udtToInt(ctx)] then
    usedCtxs[gits.udtToInt(ctx)] = true
    drv.glDebugMessageCallback(gits.gitsGlDebugProc(), 0)
	end
  return ret
end


