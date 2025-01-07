// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclExecWrap.h"

#if defined(__cplusplus)
extern "C" {
#endif

%for name, func in without_field(functions, 'version').items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
CL_API_ENTRY VISIBLE ${func.get('type')} CL_API_CALL ${name}(${make_params(func, with_types=True)})\
 CL_API_SUFFIX__VERSION_${func.get('availableFrom')}
{
  %if func.get('recExecWrap'):
  ${'' if func.get('type') == 'void' else 'return '}${func.get('recExecWrapName')}(${make_params(func, one_line=True)});
  %else:
  GITS_ENTRY_OCL
  ${'' if func.get('type') == 'void' else 'auto return_value = '}drv.${name}(${make_params(func, one_line=True)});
  GITS_WRAPPER_PRE
    wrapper.${name}(${make_params(func, with_retval=True, one_line=True)});
  GITS_WRAPPER_POST
    %if func.get('type') != 'void':
  return return_value;
    %endif
  %endif
}
  %if 'platform' in func:
#endif
  %endif

%endfor
#if defined(__cplusplus)
}
#endif

namespace gits {
namespace OpenCL {
void* GetExtensionFunction(const char* function_name) {
  static const std::map<std::string, void*> funcMap {
%for name, func in without_field(functions, 'version').items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
  %if func.get('extension') == True:
    { "${name}", reinterpret_cast<void*>(${name}) },
  %endif
  %if 'platform' in func:
#endif
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
} // namespace OpenCL
} // namespace gits
