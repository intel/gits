; ===================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
; ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER.replace('//',';')}

  LIBRARY ${library_name}
  EXPORTS
% for func in vk_functions:
  % if func.level != FuncLevel.PROTOTYPE:
    ${func.name}
  % endif
% endfor
