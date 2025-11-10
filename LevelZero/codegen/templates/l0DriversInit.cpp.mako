// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Drivers.h"

#include "dynamic_linker.h"
#include "log.h"
#include "gits.h"
#include "l0Header.h"
#include "l0Log.h"
#include "l0Lua.h"
#include "l0Tools.h"
namespace gits {
namespace l0 {
namespace {
using namespace lua;
static bool bypass_luascript;
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
  if (func == nullptr) {
    const auto hDrivers = GetDrivers(drv);
    for (const auto& hDriver : hDrivers) {
      if (drv.inject.zeDriverGetExtensionFunctionAddress(hDriver, name, &func) == ZE_RESULT_SUCCESS) {
        return true;
      }
    }
  }
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
  %if func.get('component') != 'ze_gits_extension' and func.get('enabled', True):
int lua_${func.get('name')}(lua_State* L) {
  int top = lua_gettop(L);
  if (top != ${len(func['args'])}) {
    luaL_error(L, "invalid number of parameters");
  }
    %for arg in func['args']:
  auto ${get_arg_name(arg['name'])} = lua_to_ext<${get_arg_type(arg['name'], arg['type'])}>(L, ${loop.index+1});
    %endfor
  bypass_luascript = true;
  ${func.get('type')} ret = drv.inject.${func.get('name')}(${make_params(func)});
    %for arg in func['args']:
      %if 'out' in arg['tag'] and has_vars(arg['type'], arguments):
  lua_setTableFields(L, ${loop.index+1}, ${get_arg_name(arg['name'])});
      %endif
    %endfor
  lua_pop(L, lua_gettop(L));
  bypass_luascript = false;
  lua_push_ext<${func.get('type')}>(L, ret);
  return 1;
}
  %endif
${func.get('type')} __zecall special_${func.get('name')}(
  %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
) {
  %if func.get('type') == 'ze_result_t':
  ze_result_t ret = ZE_RESULT_SUCCESS;
  %elif func.get('type') != 'void':
  ${func.get('type')} ret{};
  %endif
  %if func.get('log', True):
  LOG_TRACE_RAW << LOG_PREFIX << "${func.get('name')}(";
    %for arg in func['args']:
  LOG_TRACE_RAW << ${f"ToStringHelperArrayRange({get_arg_name(arg['name'])}, {arg['range']})" if arg.get('range') else f"ToStringHelper({get_arg_name(arg['name'])})"}${'' if loop.last else ' << ", "'};
    %endfor
  LOG_TRACE_RAW << ")${'\\n' if func['type'] == 'void' else ''}";
  %endif
  %if func.get('component') != 'ze_gits_extension':
  bool call_orig = true;
  if (Configurator::Get().common.shared.useEvents) {
    std::unique_lock<std::recursive_mutex> lock(luaMutex);
    if (!bypass_luascript) {
      auto L = CGits::Instance().GetLua().get();
      bool exists = FunctionExists("${func.get('name')}", L);
      if (exists) {
        LOG_TRACE_RAW << " Lua begin" << std::endl;
        lua_getglobal(L, "${func.get('name')}");
      %for arg in func['args']:
        lua_push_ext(L, ${get_arg_name(arg['name'])});
      %endfor
        if (lua_pcall(L, ${len(func['args'])}, 1, 0) != 0) {
          RaiseHookError("${func.get('name')}", L);
        }
        call_orig = false;
        const auto top = lua_gettop(L);
        ret = lua_to_ext<${func.get('type')}>(L, top);
        lua_pop(L, top);
        LOG_TRACE_RAW << "${name}" << " Lua End = " << ToStringHelper(ret) << std::endl;
      }
    }
  }
  if (call_orig) {
    ret = drv.original.${func.get('name')}(${make_params(func)});
    %if func.get('log', True):
    LOG_TRACE_RAW << " = " << ToStringHelper(ret) << std::endl;
      %for arg in func['args']:
        %if 'out' in arg['tag']:
    LOG_TRACE_RAW << ">>>> ${arg['tag']} ${get_arg_name(arg['name'])}: " << \
          %if arg.get('range'):
ToStringHelperArrayRange(${get_arg_name(arg['name'])}, ${arg['range']}) << std::endl;
          %else:
${get_arg_name(arg['name'])} << std::endl;
          %endif
        %endif
      %endfor
    %endif
  }
  %else:
  ${'' if func.get('type') == 'void' else 'ret = '}drv.original.${func.get('name')}(${make_params(func)});
    %if func.get('type') != 'void' and func.get('log', True):
  LOG_TRACE_RAW << " = " << ToStringHelper(ret) << std::endl;
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
  %else:
  if (drv.original.${func.get('name')} == nullptr) {
    if (!load_l0_function(drv.original.${func.get('name')}, "${func.get('name')}")) {
      LOG_ERROR << "Could not load ${func.get('name')} function.";
      return ${'ZE_RESULT_ERROR_UNINITIALIZED' if func.get('type') == 'ze_result_t' else (f'0' if func.get('type') == 'uint32_t' else 'nullptr')};
    }
  }
  %endif
  if (log::ShouldLog(LogLevel::TRACE) || Configurator::Get().common.shared.useEvents) {
    drv.${func.get('name')} = special_${func.get('name')};
    return drv.${func.get('name')}(${make_params(func)});
  }
  return drv.original.${func.get('name')}(${make_params(func)});
}

${func.get('type')} __zecall inject_${func.get('name')}(
  %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
) {
  drv.zeGitsStopRecording(${"ZE_GITS_SWITCH_NOMENCLATURE_COUNTING" if func.get('nomenclatureModifier', False) else "ZE_GITS_RECORDING_DEFAULT"});
  ${'' if func.get('type') == 'void' else 'const auto returnValue = '}drv.${func.get('name')}(${make_params(func, )});
  LOG_TRACE << "^------------------ injected";
  drv.zeGitsStartRecording(${"ZE_GITS_SWITCH_NOMENCLATURE_COUNTING" if func.get('nomenclatureModifier', False) else "ZE_GITS_RECORDING_DEFAULT"});
  ${'' if func.get('type') == 'void' else 'return returnValue;'}
}

%endfor
const luaL_Reg exports[] = {
  %for name, func in functions.items():
    %if not is_latest_version(functions, func):
<% continue %>
    %endif
    %if func.get('component') != 'ze_gits_extension' and func.get('enabled', True):
  {"${func.get('name')}", lua_${func.get('name')}},
    %endif
  %endfor
  %for name, arg in arguments.items():
    %if not is_latest_version(arguments, arg) or not arg.get('enabled', True):
<% continue %>
    %endif
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
} // namespace

CDriver::CDriver() {
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
  CGits::Instance().RegisterLuaFunctionsRegistrator(RegisterLuaDriverFunctions);
}
} // namespace l0
} // namespace gits
