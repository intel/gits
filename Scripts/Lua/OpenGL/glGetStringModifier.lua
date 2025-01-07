-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

--Script allows to modify strings returned by glGetString API

GL_VENDOR = 0x1F00
GL_RENDERER = 0x1F01
GL_VERSION = 0x1F02
GL_EXTENSIONS = 0x1F03
GL_SHADING_LANGUAGE_VERSION = 0x8B8C

vendorStr = gits.allocUdtFromStr("PASS_VENDOR_STR_THERE")
rendererStr = gits.allocUdtFromStr("PASS_RENDER_STRING_THERE")
versionStr = gits.allocUdtFromStr("PASS_GL_VERSION_THERE")

function glGetString(name)
  retv = drv.glGetString(name)
  if name == GL_VENDOR then
    return vendorStr
  elseif name == GL_RENDERER then
    return rendererStr
  elseif name == GL_VERSION then
    return versionStr
  else
    return retv
  end
end
