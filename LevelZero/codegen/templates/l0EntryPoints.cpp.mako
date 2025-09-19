// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
VISIBLE ${func.get('type')} __zecall ${func.get('name')}(${make_params(func, with_types=True, with_array=True)})
{
  %if func.get('recExecWrap'):
  ${'' if func.get('type') == 'void' else 'return '}${func.get('recExecWrapName')}(${make_params(func)});
  %elif func.get('component') == 'ze_dditable':
  GITS_ENTRY_L0
  auto return_value = driver.${func.get('name')}(${make_params(func)});
  if (pDdiTable != nullptr) {
      %for name, value in enums.items():
        %if name == 'ze_api_version_t':
    switch(version) {
          %for var in value['vars']:
    case ${var['name']}: {
            %for function in get_ddi_table_functions(func, functions, var['name']):
      driver.original.${function} = pDdiTable->${function};
      pDdiTable->${function} = ${function};
            %endfor
      break;
    }
          %endfor
    default:
      return_value = ZE_RESULT_ERROR_UNSUPPORTED_VERSION;
      break;
    }
        %endif
      %endfor
  }
  GITS_WRAPPER_PRE
  GITS_WRAPPER_POST
  return return_value;
  %else:
  GITS_ENTRY_L0
    %if func.get('unprotectLogic', False) or func.get('protectLogic', False):
      %if func.get('type') != 'void':
  auto return_value = ZE_RESULT_SUCCESS;
      %endif
  GITS_WRAPPER_PRE
      %if not func.get('protectLogic', False):
  wrapper.UnProtectMemoryPointers();
      %endif
  ${'' if func.get('type') == 'void' else 'return_value = '}driver.${func.get('name')}(${make_params(func)});
  wrapper.${func.get('name')}(${make_params(func, with_retval=True)});
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  else {
    ${'' if func.get('type') == 'void' else 'return_value = '}driver.${func.get('name')}(${make_params(func)});
  }
    %else:
  ${'' if func.get('type') == 'void' else 'auto return_value = '}driver.${func.get('name')}(${make_params(func)});
  GITS_WRAPPER_PRE
  wrapper.${func.get('name')}(${make_params(func, with_retval=True)});
  GITS_WRAPPER_POST
    %endif
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
    LOG_WARNING << "Function " << function_name << " is not implemented in GITS";
    return nullptr;
  }
  return iter->second;
}
} // namespace l0
} // namespace gits