// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

%for name, func in without_field(functions, 'version').items():
    %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
void ${name}(${make_params(func, with_retval=True, with_types=True)}) const override;
  %if 'platform' in func:
#endif
  %endif
%endfor
