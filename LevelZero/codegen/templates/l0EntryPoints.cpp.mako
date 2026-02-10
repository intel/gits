// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0ExecWrap.h"

#include <unordered_map>

#if defined(__cplusplus)
extern "C" {
#endif

<%
npu_extensions = get_npu_extensions(functions, enums)
npu_components = [ext[0] for ext in npu_extensions]
%>\
%for (_, _, _, extension_functions, _) in npu_extensions:
  %for func in extension_functions:
${func.get('type')} __zecall ${func.get('name')}(${make_params(func, with_types=True, with_array=True)});
  %endfor
%endfor

%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
${"" if func.get('driver_extension') else "VISIBLE "}${func.get('type')} __zecall ${func.get('name')}(${make_params(func, with_types=True, with_array=True)})
{
  %if func.get('recExecWrap'):
    %if func.get('name') == 'zeDriverGetExtensionFunctionAddress':
      %for (_, extension_string, dditable, extension_functions, versions) in npu_extensions:
  if (strcmp(name, ${extension_string}) == 0) {
    GITS_ENTRY_L0
    auto return_value = driver.${func.get('name')}(${make_params(func)});
    GITS_WRAPPER_PRE
    wrapper.${func.get('name')}(${make_params(func, with_retval=True)});
    GITS_WRAPPER_POST
    if (return_value == ZE_RESULT_SUCCESS) {
      uint32_t count = 0;
      return_value = driver.original.zeDriverGetExtensionProperties(hDriver, &count, nullptr);
      if (return_value != ZE_RESULT_SUCCESS) {
        LOG_ERROR << "Could not count of extension properties";
        return return_value;
      }
      std::vector<ze_driver_extension_properties_t> extension_props(count);
      return_value = driver.original.zeDriverGetExtensionProperties(hDriver, &count, extension_props.data());
      if (return_value != ZE_RESULT_SUCCESS) {
        LOG_ERROR << "Could not get extension properties";
        return return_value;
      }
      uint32_t ext_version = 0;
      bool found = false;
      for (uint32_t i = 0; i < count; i++) {
        if (strncmp(extension_props[i].name, ${extension_string}, strlen(${extension_string})) == 0) {
          ext_version = extension_props[i].version;
          found = true;
          break;
        }
      }
      if (!found) {
        LOG_ERROR << "Could not find " << name << " extension version";
        return ZE_RESULT_ERROR_UNINITIALIZED;
      }
      ${dditable}* ddi = *reinterpret_cast<${dditable}**>(ppFunctionAddress);
      switch (ext_version) {
        %for var in versions:
      case ${var['name']}: {
          %for ext_func in extension_functions:
            %if get_api_version_from_string(get_api_version_from_enum(var)) >= get_api_version_from_string(ext_func.get('api_version')):
        driver.original.${ext_func.get('name')} = ddi->${ext_func.get('name_in_dditable')};
        ddi->${ext_func.get('name_in_dditable')} = ${ext_func.get('name')};
            %endif
          %endfor
        break;
      }
        %endfor
      }
    }
    return return_value;
  }
      %endfor
    %endif
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
            %for function in get_ddi_table_functions(func, functions, var):
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
  %if func.get('extension') == True and func.get('component') not in npu_components:
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
