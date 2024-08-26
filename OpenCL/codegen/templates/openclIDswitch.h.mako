// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

%for name, func in only_enabled(functions).items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
case ${func.get('id')}:
return new C${name};
  %if 'platform' in func:
#endif
  %endif
%endfor
