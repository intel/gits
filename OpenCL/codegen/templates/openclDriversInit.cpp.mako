// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclDrivers.h"
#include "openclTracingAuto.h"

#ifndef BUILD_FOR_CCODE
#include "openclArgumentsAuto.h"

#include <tuple>
#include <type_traits>
#endif

#include "openclDriversHelper.h"
#include "config.h"
#include "gits.h"

#include <filesystem>

#ifndef BUILD_FOR_CCODE
using namespace gits::lua;
#endif

namespace gits {
namespace OpenCL {
#ifndef BUILD_FOR_CCODE
namespace {
static bool bypass_luascript;
} // namespace
#endif
%for name, func in without_field(functions, 'version').items():
#ifndef BUILD_FOR_CCODE
int lua_${name}(lua_State* L) {
  int top = lua_gettop(L);
  if (top != ${len(func['args'])}) {
    luaL_error(L, "invalid number of parameters");
  }
  FuncToTuple<${get_return_type(func)} (${make_params(func, one_line=True, with_types=True)})>::type args; 
  fill_tuple(L, args); 
  bypass_luascript = true;
  ${get_return_type(func)} ret = call_tuple<${get_return_type(func)}>(drvOcl.${name}, args);
  bypass_luascript = false;
  lua_push(L, ret);
  return 1;
}
#endif
${get_return_type(func)} STDCALL special_${name}(${make_params(func, with_types=True)}) {
  ${get_return_type(func)} gits_ret = static_cast<${get_return_type(func)}>(0);
#ifndef BUILD_FOR_CCODE
  bool doTrace = ShouldLog(LogLevel::TRACE);
  if (doTrace) {
    LOG_FORMAT_RAW
    LOG_TRACE << LOG_PREFIX << "${name}(";
  %for arg in func['args']:
    LOG_TRACE << ${format_trace_argument(arg, enums)}${' << ", "' if not loop.last else ""};
  %endfor
    LOG_TRACE << ")${'\\n' if func['type'] == 'void' else ''}";
  }
  bool call_orig = true;
  if (Configurator::Get().common.shared.useEvents) {
    std::unique_lock<std::recursive_mutex> lock(gits::lua::luaMutex);
    if (!bypass_luascript) {
      auto L = GetLuaState();
      bool exists = lua::FunctionExists("${name}", L);
      if (exists || !doTrace) {
        if(doTrace) {
          LOG_TRACE << " Lua Begin" << std::endl;
        }
        lua_getglobal(L, "${name}");
        ArgsPusher ap(L);
        ap.push(${make_params(func, one_line=True)});
        call_orig = false;
        if (lua_pcall(L, ${len(func['args'])}, 1, 0) != 0) {
          RaiseHookError("${name}", L);
        }
        int top = lua_gettop(L);
        gits_ret = lua::lua_to<${get_return_type(func)}>(L, top);
        lua_pop(L, top);
        if (doTrace) {
          LOG_FORMAT_RAW
          LOG_TRACE << "${name}" << " Lua End";
          Tracer::TraceRet(gits_ret);
        }
      }
    }
  }

  if (call_orig) {
    gits_ret = drvOcl.orig_${name}(${make_params(func, one_line=True)});
    if (doTrace) {
      LOG_FORMAT_RAW
      Tracer::TraceRet(gits_ret);
      %for arg in func['args']:
        %if 'out' in arg['tag']:
      LOG_TRACE << ">>>> ${arg['tag']} ${arg['name']}: " << ${format_trace_argument(arg, enums)} << std::endl;
        %endif
      %endfor
    }
  }
#endif
  return gits_ret;
}

${get_return_type(func)} STDCALL default_${name}(${make_params(func, with_types=True)}) {
  if (!load_ocl_function_generic(reinterpret_cast<void*&>(drvOcl.${name}), "${name}")) {
    LOG_WARNING << "Function ${name} not found in OpenCL library";
    return static_cast<${get_return_type(func)}>(0);
  }
  drvOcl.orig_${name} = drvOcl.${name};
  if ((gits::ShouldLog(LogLevel::TRACE)) ||
      (Configurator::Get().common.shared.useEvents && LUA_FUNCTION_EXISTS("${name}")) ||
      (!Configurator::Get().common.player.traceSelectedFrames.empty())) {
    drvOcl.${name} = special_${name};
  }
  return drvOcl.${name}(${make_params(func,one_line=True)});
}

%endfor

#ifndef BUILD_FOR_CCODE
const luaL_Reg exports[] = {{"statusToStr", export_CLStatusToStr},
                            {"getEventCallbackPtr", export_EventCallbackPtr},
                            {"getMemObjCallbackPtr", export_MemObjCallbackPtr},
                            {"getProgramCallbackPtr", export_ProgramCallbackPtr},
                            {"getContextCallbackPtr", export_ContextCallbackPtr},
                            {"getSVMFreeCallbackPtr", export_SVMFreeCallbackPtr},
                            {"setCallbackData", export_CallbackData},
%for name, func in without_field(functions, 'version').items():
                            {"${name}", lua_${name}},
%endfor
                            {nullptr, nullptr}};
#endif

void RegisterLuaDriverFunctions() {
#ifndef BUILD_FOR_CCODE
  auto L = CGits::Instance().GetLua();
  luaL_newlib(L.get(), exports);
  lua_setglobal(L.get(), "drvCl");
#endif
}

COclDriver::COclDriver() : _initialized(false), _lib(nullptr) {
%for name, func in without_field(functions, 'version').items():
  drvOcl.${name} = default_${name};
%endfor
#ifndef BUILD_FOR_CCODE
  CGits::Instance().RegisterLuaFunctionsRegistrator(RegisterLuaDriverFunctions);
#endif
}
} // namespace OpenCL
} // namespace gits
