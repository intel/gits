; ===================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
; ===================== end_copyright_notice ==============================

LIBRARY OpenCL.dll
EXPORTS
%for name, func in without_field(functions, 'version').items():
  ${name}
%endfor
