// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0ExecWrap.h"

#include <unordered_map>

#if defined(__cplusplus)
extern "C" {
#endif

%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
VISIBLE ${func.get('type')} __zecall ${func.get('name')}(${make_params(func, with_types=True)})
{
  %if func.get('recExecWrap'):
  ${'' if func.get('type') == 'void' else 'return '}${func.get('recExecWrapName')}(${make_params(func)});
  %else:
  GITS_ENTRY_L0
  ${'' if func.get('type') == 'void' else 'auto return_value = '}driver.${func.get('name')}(${make_params(func)});
  GITS_WRAPPER_PRE
    wrapper.${func.get('name')}(${make_params(func, with_retval=True)});
  GITS_WRAPPER_POST
    %if func.get('type') != 'void':
  return return_value;
    %endif
  %endif
}

%endfor
#if defined(__cplusplus)
}
#endif

namespace gits {
namespace l0 {
void* GetExtensionFunction(const char* function_name) {
  static const std::unordered_map<std::string, void*> funcMap {
%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
  %if func.get('extension') == True:
    { "${func.get('name')}", reinterpret_cast<void*>(${func.get('name')}) },
  %endif
%endfor
  };
  auto iter = funcMap.find(function_name);
  if (iter == funcMap.end()) {
    Log(WARN) << "Function " << function_name << " is not implemented in GITS";
    return nullptr;
  }
  return iter->second;
}
} // namespace l0
} // namespace gits