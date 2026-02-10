-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

function ZE_BIT(x)
  return 1 << x
end

function ZE_MAKE_VERSION(major, minor)
  return (( major << 16 )|( minor & 0x0000ffff))
end

%for values in enums.values():
<% prev_value = "-1" %>\
-- ${values['name']} --
  %for vars in values['vars']:
    %if 'value' in vars:
${vars['name']} = ${vars['value']}
<% prev_value = vars['value'] %>\
    %else:
<%
import re
if re.search("^0[xX][0-9a-fA-F]+$", prev_value):
  prev_value = hex(int(prev_value, 16) + 1)
elif re.search("^-?[0-9]+$", prev_value):
  prev_value = str(int(prev_value) + 1)
else:
  raise Exception(f"Invalid enum previous value: {prev_value}")
%>\
${vars['name']} = ${prev_value}
    %endif
  %endfor

%endfor
