// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

%for name, func in functions.items():
%if not is_latest_version(functions, func):
<% continue%>
%endif
     virtual void ${func.get('name')}(
  %if func.get('type') != 'void':
        ze_result_t return_value${',' if len(func['args']) > 0 else ''}
  %endif
  %for arg in func['args']:
        ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
    ) const override;
%endfor
