-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- Script disabling vsync by modifying vsync enabling APIs and additionally on Windows and for EGL forcing vsync disable on make context current.
-- Script for big OpenGL on Windows and for OpenGLES through EGL.

-- Big OpenGL

--   Windows
function wglSwapIntervalEXT(interval)
  return drv.wglSwapIntervalEXT(0)
end

function wglMakeCurrent(hdc, hglrc)
  local retVal = drv.wglMakeCurrent(hdc, hglrc)
  drv.wglSwapIntervalEXT(0)
  return retVal
end

--  Linux - no support

-- EGL OpenGL ES - Windows/Linux
function eglSwapInterval(dpy, interval)
  return drv.eglSwapInterval(dpy, 0)
end

function eglMakeCurrent(dpy, draw, read, ctx)
  local retVal = drv.eglMakeCurrent(dpy, draw, read, ctx)
  drv.eglSwapInterval(dpy, 0)
  return retVal
end
