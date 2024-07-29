// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocDrivers.h"
#include "oclocTools.h"

#include "config.h"
#include "gits.h"

#include <string>

#ifndef BUILD_FOR_CCODE
#include "lua_bindings.h"
static bool bypass_luascript = false;
#endif

namespace gits {
namespace ocloc {
CDriver drv;
namespace {
bool load_ocloc_function_generic(void*& func, const char* name) {
  auto lib = drv.Library();
  if (lib == nullptr) {
    return false;
  }

  func = dl::load_symbol(lib, name);

  return true;
}

template <class T>
bool load_ocloc_function(T& func, const char* name) {
  return load_ocloc_function_generic(reinterpret_cast<void*&>(func), name);
}
#ifndef BUILD_FOR_CCODE
template <typename T>
inline void lua_push_ext(lua_State* L, T* arr, const uint32_t& size, bool forceString = false) {
  lua_newtable(L);
  int pos = lua_gettop(L);
  for (auto i = 0U; i < size; i++) {
    if (forceString) {
      lua_pushstring(L, (const char*)arr[i]);
    } else {
      gits::lua::lua_push(L, arr[i]);
    }
    lua_rawseti(L, pos, i + 1);
  }
}
template <typename T>
std::vector<T> lua_to_ext(lua_State* L, int pos, const uint32_t& count, bool forceString = false) {
  if (count == 0U) {
    return std::vector<T>();
  }
  int type = lua_type(L, pos);
  const char* typeName = lua_typename(L, type);
  if (!std::strcmp(typeName, "table")) {
    std::vector<T> args;
    for (auto i = 0U; i < count; i++) {
      lua_rawgeti(L, pos, i + 1);
      if (forceString) {
        auto val = (T)lua_tostring(L, -1);
        args.push_back(val);
      } else {
        auto val = gits::lua::lua_to<T>(L, -1);
        args.push_back(val);
      }
      lua_pop(L, 1);
    }
    return args;
  }
  throw EOperationFailed(EXCEPTION_MESSAGE +
                         std::string("\nLua argument type is incorrect. Your type: ") +
                         std::string(typeName));
}
#endif

int __ocloccall special_oclocInvoke(unsigned int argc,
                                    const char** argv,
                                    const uint32_t numSources,
                                    const uint8_t** sources,
                                    const uint64_t* sourceLens,
                                    const char** sourcesNames,
                                    const uint32_t numInputHeaders,
                                    const uint8_t** dataInputHeaders,
                                    const uint64_t* lenInputHeaders,
                                    const char** nameInputHeaders,
                                    uint32_t* numOutputs,
                                    uint8_t*** dataOutputs,
                                    uint64_t** lenOutputs,
                                    char*** nameOutputs) {
  LogOclocInvokeInput(argc, argv, numSources, sources, sourceLens, sourcesNames, numInputHeaders,
                      dataInputHeaders, lenInputHeaders, nameInputHeaders);
  bool call_orig = true;
  int ret = 0;
#ifndef BUILD_FOR_CCODE
  if (gits::Config::Get().common.shared.useEvents && !bypass_luascript) {
    auto L = CGits::Instance().GetLua().get();
    bool exists = gits::lua::FunctionExists("oclocInvoke", L);
    if (exists) {
      std::unique_lock<std::recursive_mutex> lock(gits::lua::luaMutex);
      OclocLog(TRACE, NO_PREFIX) << " Lua begin";
      lua_getglobal(L, "oclocInvoke");
      gits::lua::lua_push(L, argc);
      lua_push_ext(L, argv, argc, true);
      gits::lua::lua_push(L, numSources);
      lua_push_ext<const uint8_t*>(L, sources, numSources, true);
      lua_push_ext(L, sourceLens, numSources);
      lua_push_ext(L, sourcesNames, numSources, true);
      gits::lua::lua_push(L, numInputHeaders);
      lua_push_ext<const uint8_t*>(L, dataInputHeaders, numInputHeaders, true);
      lua_push_ext(L, lenInputHeaders, numInputHeaders);
      lua_push_ext(L, nameInputHeaders, numInputHeaders, true);
      lua_pushlightuserdata(L, (void*)numOutputs);
      lua_pushlightuserdata(L, (void*)dataOutputs);
      lua_pushlightuserdata(L, (void*)lenOutputs);
      lua_pushlightuserdata(L, (void*)nameOutputs);
      if (lua_pcall(L, 14, 1, 0) != 0) {
        gits::lua::RaiseHookError("oclocInvoke", L);
      }
      call_orig = false;
      lua_pop(L, lua_gettop(L));
      OclocLog(TRACE, NO_PREFIX) << "Lua End";
    }
  }
#endif
  if (call_orig) {
    ret = drv.orig_oclocInvoke(argc, argv, numSources, sources, sourceLens, sourcesNames,
                               numInputHeaders, dataInputHeaders, lenInputHeaders, nameInputHeaders,
                               numOutputs, dataOutputs, lenOutputs, nameOutputs);

    LogOclocInvokeOutput(ret, numOutputs, dataOutputs, lenOutputs, nameOutputs);
  }
  return ret;
}

int __ocloccall default_oclocInvoke(unsigned int argc,
                                    const char** argv,
                                    const uint32_t numSources,
                                    const uint8_t** sources,
                                    const uint64_t* sourceLens,
                                    const char** sourcesNames,
                                    const uint32_t numInputHeaders,
                                    const uint8_t** dataInputHeaders,
                                    const uint64_t* lenInputHeaders,
                                    const char** nameInputHeaders,
                                    uint32_t* numOutputs,
                                    uint8_t*** dataOutputs,
                                    uint64_t** lenOutputs,
                                    char*** nameOutputs) {
  if (!load_ocloc_function(drv.oclocInvoke, "oclocInvoke")) {
    return -1;
  }
  drv.orig_oclocInvoke = drv.oclocInvoke;
  if (ShouldLog(TRACE) || Config::Get().common.shared.useEvents) {
    drv.oclocInvoke = special_oclocInvoke;
  }
  return drv.oclocInvoke(argc, argv, numSources, sources, sourceLens, sourcesNames, numInputHeaders,
                         dataInputHeaders, lenInputHeaders, nameInputHeaders, numOutputs,
                         dataOutputs, lenOutputs, nameOutputs);
}

int __ocloccall special_oclocFreeOutput(uint32_t* numOutputs,
                                        uint8_t*** dataOutputs,
                                        uint64_t** lenOutputs,
                                        char*** nameOutputs) {
  int ret = 0;
  bool call_orig = true;
#ifndef BUILD_FOR_CCODE
  if (gits::Config::Get().common.shared.useEvents && !bypass_luascript) {
    auto L = CGits::Instance().GetLua().get();
    bool exists = gits::lua::FunctionExists("oclocFreeOutput", L);
    if (exists) {
      std::unique_lock<std::recursive_mutex> lock(gits::lua::luaMutex);
      OclocLog(TRACE, NO_PREFIX) << " Lua begin";
      lua_getglobal(L, "oclocFreeOutput");
      lua_pushlightuserdata(L, (void*)numOutputs);
      lua_pushlightuserdata(L, (void*)dataOutputs);
      lua_pushlightuserdata(L, (void*)lenOutputs);
      lua_pushlightuserdata(L, (void*)nameOutputs);
      if (lua_pcall(L, 4, 1, 0) != 0) {
        gits::lua::RaiseHookError("oclocFreeOutput", L);
      }
      call_orig = false;
      lua_pop(L, lua_gettop(L));
      OclocLog(TRACE, NO_PREFIX) << "Lua End";
    }
  }
#endif
  OclocLog(TRACE, NO_NEWLINE) << "oclocFreeOutput()";
  if (call_orig) {
    ret = drv.orig_oclocFreeOutput(numOutputs, dataOutputs, lenOutputs, nameOutputs);
  }
  OclocLog(TRACE, NO_PREFIX) << " = " << ret;
  return ret;
}

int __ocloccall default_oclocFreeOutput(uint32_t* numOutputs,
                                        uint8_t*** dataOutputs,
                                        uint64_t** lenOutputs,
                                        char*** nameOutputs) {
  if (!load_ocloc_function(drv.oclocFreeOutput, "oclocFreeOutput")) {
    return -1;
  }
  drv.orig_oclocFreeOutput = drv.oclocFreeOutput;
  if (ShouldLog(TRACE) || Config::Get().common.shared.useEvents) {
    drv.oclocFreeOutput = special_oclocFreeOutput;
  }
  return drv.oclocFreeOutput(numOutputs, dataOutputs, lenOutputs, nameOutputs);
}
} // namespace
#ifndef BUILD_FOR_CCODE
int lua_oclocInvoke(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(gits::lua::luaMutex);
  int top = lua_gettop(L);
  if (top != 14) {
    luaL_error(L, "invalid number of parameters");
  }
  unsigned int argc = lua::lua_to<int>(L, 1);

  auto argData = lua_to_ext<const char*>(L, 2, argc, true);
  const char** argv = argc ? argData.data() : nullptr;

  const uint32_t numSources = lua::lua_to<uint32_t>(L, 3);

  auto sourcesData = lua_to_ext<const uint8_t*>(L, 4, numSources, true);
  const uint8_t** sources = numSources ? sourcesData.data() : nullptr;

  auto sourceLensInfo = lua_to_ext<uint64_t>(L, 5, numSources);
  const uint64_t* sourceLens = numSources ? sourceLensInfo.data() : nullptr;

  auto sourcesNamesData = lua_to_ext<const char*>(L, 6, numSources, true);
  const char** sourcesNames = numSources ? sourcesNamesData.data() : nullptr;

  const uint32_t numInputHeaders = lua::lua_to<uint32_t>(L, 7);

  auto dataInputHeadersInfo = lua_to_ext<const uint8_t*>(L, 8, numInputHeaders, true);
  const uint8_t** dataInputHeaders = numInputHeaders ? dataInputHeadersInfo.data() : nullptr;

  auto lenInputHeadersInfo = lua_to_ext<uint64_t>(L, 9, numInputHeaders);
  const uint64_t* lenInputHeaders = numInputHeaders ? lenInputHeadersInfo.data() : nullptr;

  auto nameInputHeadersInfo = lua_to_ext<const char*>(L, 10, numInputHeaders, true);
  const char** nameInputHeaders = numInputHeaders ? nameInputHeadersInfo.data() : nullptr;

  uint32_t* numOutputs = (uint32_t*)lua_touserdata(L, 11);
  uint8_t*** dataOutputs = (uint8_t***)lua_touserdata(L, 12);
  uint64_t** lenOutputs = (uint64_t**)lua_touserdata(L, 13);
  char*** nameOutputs = (char***)lua_touserdata(L, 14);
  bypass_luascript = true;
  int ret = drv.oclocInvoke(argc, argv, numSources, sources, sourceLens, sourcesNames,
                            numInputHeaders, dataInputHeaders, lenInputHeaders, nameInputHeaders,
                            numOutputs, dataOutputs, lenOutputs, nameOutputs);
  lua_pop(L, lua_gettop(L));
  bypass_luascript = false;
  gits::lua::lua_push<int>(L, ret);
  return 1;
}

int lua_oclocFreeOutput(lua_State* L) {
  int ret = 0;
  std::unique_lock<std::recursive_mutex> lock(gits::lua::luaMutex);
  int top = lua_gettop(L);
  if (top != 4) {
    luaL_error(L, "invalid number of parameters");
  }
  uint32_t* numOutputs = (uint32_t*)lua_touserdata(L, 1);
  uint8_t*** dataOutputs = (uint8_t***)lua_touserdata(L, 2);
  uint64_t** lenOutputs = (uint64_t**)lua_touserdata(L, 3);
  char*** nameOutputs = (char***)lua_touserdata(L, 4);
  bypass_luascript = true;
  ret = drv.orig_oclocFreeOutput(numOutputs, dataOutputs, lenOutputs, nameOutputs);
  lua_pop(L, lua_gettop(L));
  bypass_luascript = false;
  gits::lua::lua_push<int>(L, ret);
  return 1;
}

const luaL_Reg exports[] = {
    {"oclocInvoke", lua_oclocInvoke}, {"oclocFreeOutput", lua_oclocFreeOutput}, {nullptr, nullptr}};
void RegisterLuaDriverFunctions() {
  auto L = CGits::Instance().GetLua();
  luaL_newlib(L.get(), exports);
  lua_setglobal(L.get(), "drvOcloc");
}
#endif

CDriver::CDriver() {
  oclocInvoke = default_oclocInvoke;
  oclocFreeOutput = default_oclocFreeOutput;
#ifndef BUILD_FOR_CCODE
  CGits::Instance().RegisterLuaFunctionsRegistrator(RegisterLuaDriverFunctions);
#endif
}

CDriver::~CDriver() {
  initialized_ = false;
}

void CDriver::Initialize() {
  if (initialized_) {
    return;
  }
  const std::string path = gits::Config::Get().common.shared.libOcloc.string();
  Log(INFO) << "Initializing Ocloc API";

  lib_ = nullptr;
  if (lib_ == nullptr) {
    Log(TRACE) << "Using LibOcloc: " << path;
    lib_ = std::make_unique<SharedLibrary>(path.c_str());

    initialized_ = lib_->getHandle() != nullptr;
  }
}
} // namespace ocloc
} // namespace gits
