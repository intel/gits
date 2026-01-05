// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helperL0.h"

#ifdef WITH_LEVELZERO
void InitL0() {
  gits::l0::drv.Initialize();
}
#endif

namespace api {
%for name, func in functions.items():
  %if is_latest_version(functions, func):
  ${func.get('type')} (STDCALL *&${func.get('name')}) (${make_params(func, with_types=True)}) = gits::l0::drv.${func.get('name')};
  %endif
%endfor
}
