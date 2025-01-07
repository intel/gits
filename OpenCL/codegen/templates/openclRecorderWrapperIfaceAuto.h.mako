// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

%for name, func in without_field(functions, 'version').items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
virtual void ${name}(${make_params(func, with_retval=True, with_types=True, one_line=True)}) const = 0;
  %if 'platform' in func:
#endif
  %endif
%endfor
