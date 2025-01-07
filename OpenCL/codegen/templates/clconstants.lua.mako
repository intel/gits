-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

<% added_constants = [] %>
%for name, enum in enums.items():
-- ${name}
  %for var in enum['vars']:
    %if var['name'] not in added_constants:
      %if enum['name'].startswith('cl_') and enum['name'] != 'cl_build_status':
${var['name']} = ${'{:#06x}'.format(eval(var['value']))}
      %else:
${var['name']} = ${var['value']}
      %endif
<% added_constants.append(var['name']) %>\
    %endif
  %endfor
%endfor
