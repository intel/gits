-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

%for name, func in functions.items():
function ${func.get('name')}(${make_lua_params(func)})
  return drvl0.${func.get('name')}(${make_lua_params(func)})
end

%endfor