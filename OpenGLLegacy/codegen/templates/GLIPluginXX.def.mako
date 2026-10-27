; ===================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
; ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER.replace('//',';')}


LIBRARY OpenGL32.dll
EXPORTS
	GITSIdentificationToken
% if egl_function_names:
; EGL
${egl_function_names}
% endif
; WGL
${wgl_function_names}
; GL
% for name in gl_functions:
	${name}
% endfor
