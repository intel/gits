// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

<% added_headers = [] %>
%for name, enum in enums.items():
/* ${name} */
  %for var in enum['vars']:
    %if var['name'] not in added_headers:
#define ${var['name']} ${var['value']}
<% added_headers.append(var['name']) %>\
    %endif
  %endfor
%endfor
