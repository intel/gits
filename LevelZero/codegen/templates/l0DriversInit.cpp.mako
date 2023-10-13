// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Drivers.h"

#include "dynamic_linker.h"
#include "log.h"
#include "config.h"
#include "gits.h"
#include "l0Header.h"
#include "l0Log.h"
#ifndef BUILD_FOR_CCODE
#include "l0Lua.h"
#include "l0Tools.h"
#endif
namespace gits {
namespace l0 {
namespace {
#ifndef BUILD_FOR_CCODE
using namespace lua;
static bool bypass_luascript;
#endif
bool load_l0_function_from_original_library_generic(void*& func, const char* name) {
  auto lib = drv.Library();
  if (lib == nullptr) {
    return false;
  }
  func = dl::load_symbol(lib, name);
  return func != nullptr;
}
bool load_l0_function_generic(void*& func, const char* name) {
  if (drv.Library() == nullptr) {
    return false;
  }
  load_l0_function_from_original_library_generic(func, name);
#ifndef BUILD_FOR_CCODE
  if (func == nullptr) {
    const auto hDrivers = GetDrivers(drv);
    for (const auto& hDriver : hDrivers) {
      if (drv.inject.zeDriverGetExtensionFunctionAddress(hDriver, name, &func) == ZE_RESULT_SUCCESS) {
        return true;
      }
    }
  }
#endif
  return func != nullptr;
}
template <typename Func>
struct NoopHelper;
template <typename ReturnType, typename... Args>
struct NoopHelper<ReturnType (*)(Args...)> {
  static ReturnType noop_function(Args...) {
    return ReturnType();
  }
};
template <typename Func>
auto noop(Func) {
  return &NoopHelper<Func>::noop_function;
}
template<class T>
bool load_l0_function(T& func, const char* name) {
  return load_l0_function_generic(reinterpret_cast<void*&>(func), name);
}
template<class T>
bool load_l0_function_from_original_library(T& func, const char* name) {
  return load_l0_function_from_original_library_generic(reinterpret_cast<void*&>(func), name);
}
%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
  %if func.get('component') != 'ze_gits_extension':
#ifndef BUILD_FOR_CCODE
int lua_${func.get('name')}(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(luaMutex);
  int top = lua_gettop(L);
  if (top != ${len(func['args'])}) {
    luaL_error(L, "invalid number of parameters");
  }
    %for arg in func['args']:
  auto ${arg['name']} = lua_to_ext<${arg['type']}>(L, ${loop.index+1});
    %endfor
  bypass_luascript = true;
  ze_result_t ret = drv.${func.get('name')}(${make_params(func)});
    %for arg in func['args']:
      %if 'out' in arg['tag'] and has_vars(arg['type'], arguments):
  lua_setTableFields(L, ${loop.index+1}, ${arg['name']});
      %endif
    %endfor
  lua_pop(L, lua_gettop(L));
  bypass_luascript = false;
  lua_push_ext<ze_result_t>(L, ret);
  return 1;
}
#endif
  %endif
${func.get('type')} __zecall special_${func.get('name')}(
  %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
) {
  %if func.get('type') != 'void':
  ze_result_t ret = ZE_RESULT_SUCCESS;
  %endif
  %if func.get('log', True):
  L0Log(TRACE, NO_NEWLINE) << "${func.get('name')}(";
    %for arg in func['args']:
  L0Log(TRACE, RAW) << ${arg['name']}${'' if loop.last else ' << ", "'};
    %endfor
  L0Log(TRACE, ${'RAW' if func['type'] != 'void' else 'NO_PREFIX'}) << ")";
  %endif
  %if func.get('component') != 'ze_gits_extension':
  bool call_orig = true;
#ifndef BUILD_FOR_CCODE
  if (gits::Config::Get().common.useEvents && !bypass_luascript) {
    auto L = CGits::Instance().GetLua().get();
    bool exists = FunctionExists("${func.get('name')}", L);
    if (exists) {
      std::unique_lock<std::recursive_mutex> lock(luaMutex);
      L0Log(TRACE, NO_PREFIX) << " Lua begin";
      lua_getglobal(L, "${func.get('name')}");
    %for arg in func['args']:
      lua_push_ext(L, ${arg['name']});
    %endfor
      if (lua_pcall(L, ${len(func['args'])}, 1, 0) != 0) {
        RaiseHookError("${func.get('name')}", L);
      }
      call_orig = false;
      const auto top = lua_gettop(L);
      ret = lua_to_ext<ze_result_t>(L, top);  
      lua_pop(L, top);
      L0Log(TRACE, NO_PREFIX) << "Lua End";
    }
  }
#endif
  if (call_orig) {
    ret = drv.original.${func.get('name')}(${make_params(func)});
    %if func.get('log', True):
    L0Log(TRACE, NO_PREFIX) << " = " << ret;
      %for arg in func['args']:
        %if 'out' in arg['tag']:
    L0Log(TRACEV, NO_PREFIX) << ">>>> ${arg['tag']} ${arg['name']}: " << \
          %if arg.get('range'):
ToStringHelperArrayRange(${arg['name']}, ${arg['range']});
          %else:
${arg['name']};
          %endif
        %endif
      %endfor
    %endif
  }
  %else:
  drv.original.${func.get('name')}(${make_params(func)});
    %if func.get('type') != 'void' and func.get('log', True):
  L0Log(TRACE, NO_PREFIX) << " = " << ret;
    %endif
  %endif
  %if func.get('type') != 'void':
  return ret;
  %endif
}

${func.get('type')} __zecall default_${func.get('name')}(
  %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
) {
  %if func.get('component') == 'ze_gits_extension':
  if (!load_l0_function_from_original_library(drv.original.${func.get('name')}, "${func.get('name')}")) {
    drv.original.${func.get('name')} = noop(static_cast<pfn_${func.get('name')}>(nullptr));
  }
  drv.${func.get('name')} = special_${func.get('name')};
  %else:
  if (drv.original.${func.get('name')} == nullptr) {
    if (!load_l0_function(drv.original.${func.get('name')}, "${func.get('name')}")) {
      L0Log(ERR) << "Could not load ${func.get('name')} function.";
      return ZE_RESULT_ERROR_UNINITIALIZED;
    }
  }
  if (ShouldLog(TRACE) || Config::Get().common.useEvents) {
    drv.${func.get('name')} = special_${func.get('name')};
    return drv.${func.get('name')}(${make_params(func)});
  }
  %endif
  ${'' if func.get('type') == 'void' else 'return '}drv.original.${func.get('name')}(${make_params(func)});
}

${func.get('type')} __zecall inject_${func.get('name')}(
  %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
) {
  drv.zeGitsStopRecording(${"ZE_GITS_SWITCH_NOMENCLATURE_COUNTING" if func.get('nomenclatureModifier', False) else "ZE_GITS_RECORDING_DEFAULT"});
  ${'' if func.get('type') == 'void' else 'const auto returnValue = '}drv.${func.get('name')}(${make_params(func)});
  Log(TRACE) << "^------------------ injected";
  drv.zeGitsStartRecording(${"ZE_GITS_SWITCH_NOMENCLATURE_COUNTING" if func.get('nomenclatureModifier', False) else "ZE_GITS_RECORDING_DEFAULT"});
  ${'' if func.get('type') == 'void' else 'return returnValue;'}
}

%endfor
#ifndef BUILD_FOR_CCODE
const luaL_Reg exports[] = {
  %for name, func in functions.items():
    %if not is_latest_version(functions, func):
<% continue %>
    %endif
    %if func.get('component') != 'ze_gits_extension':
  {"${func.get('name')}", lua_${func.get('name')}},
    %endif
  %endfor
  %for name, arg in arguments.items():
    %if 'vars' in arg:
  {"create_${arg.get('name')}", create_${arg.get('name')}},
    %endif
  %endfor
  {"free", export_FreeStruct},
  {nullptr, nullptr}
};
void RegisterLuaDriverFunctions() {
  auto L = CGits::Instance().GetLua();
  luaL_newlib(L.get(), exports);
  lua_setglobal(L.get(), "drvl0");
}
#endif
} // namespace

CDriver::CDriver() : initialized_(false), lib_(nullptr) {
%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
  drv.${func.get('name')} = default_${func.get('name')};
%endfor
%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
  drv.inject.${func.get('name')} = inject_${func.get('name')};
  drv.original.${func.get('name')} = nullptr;
%endfor
#ifndef BUILD_FOR_CCODE
  CGits::Instance().RegisterLuaFunctionsRegistrator(RegisterLuaDriverFunctions);
#endif
}
} // namespace l0
} // namespace gits
