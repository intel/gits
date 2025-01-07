-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
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
-- ${values['name']} --
  %for vars in values['vars']:
${vars['name']} = ${vars['value']}
  %endfor

%endfor
