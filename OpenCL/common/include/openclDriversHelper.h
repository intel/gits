// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclDrivers.h"
#include "openclStateDynamic.h"
#include "openclArgumentsAuto.h"
#include "openclTools.h"
#include "lua_bindings.h"
#include "opencl_apis_iface.h"

#include <tuple>
#include <type_traits>

#include "gits.h"

#include <filesystem>

using namespace gits::lua;

namespace gits {
namespace OpenCL {
bool CheckGPUPlatform(const cl_platform_id& platform) {
  cl_uint num_devices = 0;
  auto result = drvOcl.clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
  return result == CL_SUCCESS && num_devices > 0;
}

bool CheckIntelGPUPlatform(const cl_platform_id& platform) {
  constexpr char intelPlatformVendorName[] = "Intel(R) Corporation";
  constexpr size_t intelPlatformVendorNameSize =
      sizeof(intelPlatformVendorName) / sizeof(intelPlatformVendorName[0]);
  size_t platformNameSize = 0;
  cl_int result =
      drvOcl.clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 0, nullptr, &platformNameSize);
  if (result != CL_SUCCESS || platformNameSize != intelPlatformVendorNameSize) {
    return false;
  }
  char platformName[intelPlatformVendorNameSize];
  result = drvOcl.clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, intelPlatformVendorNameSize,
                                    platformName, nullptr);
  if (result == CL_SUCCESS && strcmp(platformName, intelPlatformVendorName) == 0) {
    return CheckGPUPlatform(platform);
  }
  return false;
}

cl_platform_id GetPlatform(const char* functionName) {
  const auto& platformStates = SD()._platformIDStates;
  cl_platform_id firstGPUPlatform = nullptr;
  cl_platform_id firstNonGPUPlatform = nullptr;
  for (const auto& state : platformStates) {
    const auto exists = state.second->ExtensionFunctionExists(functionName);
    if (exists) {
      const bool isGPU = state.second->GetDeviceType(CL_DEVICE_TYPE_GPU);
      if (isGPU && CheckIntelPlatform(state.first)) {
        return state.first;
      } else if (isGPU && firstGPUPlatform == nullptr) {
        firstGPUPlatform = state.first;
      } else if (firstNonGPUPlatform == nullptr) {
        firstNonGPUPlatform = state.first;
      }
    }
  }
  if (firstGPUPlatform != nullptr) {
    return firstGPUPlatform;
  } else if (firstNonGPUPlatform != nullptr) {
    return firstNonGPUPlatform;
  }

  for (const auto& state : platformStates) {
    if (CheckIntelPlatform(state.first) &&
        state.second->GetDeviceType(CL_DEVICE_TYPE_GPU) != nullptr) {
      return state.first;
    }
  }
  if (!platformStates.empty()) {
    return platformStates.begin()->first;
  }

  cl_uint numberOfPlatforms = 0;
  cl_int result = drvOcl.clGetPlatformIDs(0, nullptr, &numberOfPlatforms);
  if (result != CL_SUCCESS) {
    return nullptr;
  }
  std::vector<cl_platform_id> platforms(numberOfPlatforms);
  result = drvOcl.clGetPlatformIDs(numberOfPlatforms, platforms.data(), nullptr);
  if (result != CL_SUCCESS) {
    return nullptr;
  }
  for (const auto& platform : platforms) {
    if (CheckIntelGPUPlatform(platform)) {
      return platform;
    }
  }
  for (const auto& platform : platforms) {
    if (CheckGPUPlatform(platform)) {
      return platform;
    }
  }
  return platforms.empty() ? nullptr : platforms[0];
}

NOINLINE bool load_ocl_function_generic(void*& func, const char* name) {
  auto lib = drvOcl.Library();
  if (lib == nullptr) {
    return false;
  }
  func = dl::load_symbol(lib, name);
  if (func == nullptr) {
    if (drvOcl.clGetExtensionFunctionAddressForPlatform != nullptr) {
      const auto platform = GetPlatform(name);
      func = drvOcl.clGetExtensionFunctionAddressForPlatform(platform, name);
      if (func == nullptr) {
        LOG_ERROR << name << " not found";
        return false;
      }
    } else {
      LOG_ERROR << name << " not found";
      return false;
    }
  }

  return true;
}

static NOINLINE lua_State* GetLuaState() {
  return CGits::Instance().GetLua().get();
}

#define LUA_FUNCTION_EXISTS(function) lua::FunctionExists(function, GetLuaState())

int export_CLStatusToStr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }

  int status = lua_to<int>(L, 1);
  std::string str = gits::OpenCL::CLResultToString(status);
  lua_pushstring(L, str.c_str());
  return 1;
}

// callbacks support
struct CLCallbackProxy {
  lua_State* state;
  int funcID;
  int threadID;
  void* userData;
};

// event
std::function<void(cl_event, cl_int, void*)> CreateEventCallbackWrapper(lua_State* L,
                                                                        int function) {
  return [=](cl_event t0, cl_int t1, void* t2) -> void {
    lua_rawgeti(L, LUA_REGISTRYINDEX, function);
    lua_push(L, t0);
    lua_push(L, t1);
    lua_push(L, t2);
    if (lua_pcall(L, 3, 0, 0) != 0) {
      RaiseHookError("CLEventCallback", L);
    }
  };
}

void CL_CALLBACK EventCallback(cl_event e, cl_int s, void* ud) {
  if (ud != nullptr) {
    CLCallbackProxy* proxy = static_cast<CLCallbackProxy*>(ud);
    ud = proxy->userData;
    // This is called from a different thread, so GITS' exception handlers
    // won't catch it.
    try {
      CreateEventCallbackWrapper(proxy->state, proxy->funcID)(e, s, ud);
    } catch (std::runtime_error& e) {
      LOG_ERROR << e.what();
      delete proxy;
      return;
    }
    delete proxy;
  } else {
    LOG_WARNING << "CLEventCallback: something went wrong with sending data to callback function.";
  }
}

int export_EventCallbackPtr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 0) {
    luaL_error(L, "invalid number of parameters");
  }

  lua_pushlightuserdata(L, (void*)EventCallback);
  return 1;
}

// memobj
std::function<void(cl_mem, void*)> CreateMemObjCallbackWrapper(lua_State* L, int function) {
  return [=](cl_mem t0, void* t1) -> void {
    lua_rawgeti(L, LUA_REGISTRYINDEX, function);
    lua_push(L, t0);
    lua_push(L, t1);
    if (lua_pcall(L, 2, 0, 0) != 0) {
      RaiseHookError("CLMemObjCallback", L);
    }
  };
}

void CL_CALLBACK MemObjCallback(cl_mem m, void* ud) {
  if (ud != nullptr) {
    CLCallbackProxy* proxy = static_cast<CLCallbackProxy*>(ud);
    ud = proxy->userData;
    // This is called from a different thread, so GITS' exception handlers
    // won't catch it.
    try {
      CreateMemObjCallbackWrapper(proxy->state, proxy->funcID)(m, ud);
    } catch (std::runtime_error& e) {
      LOG_ERROR << e.what();
      delete proxy;
      return;
    }
    delete proxy;
  } else {
    LOG_WARNING << "CLMemObjCallback: something went wrong with sending data to callback function.";
  }
}

int export_MemObjCallbackPtr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 0) {
    luaL_error(L, "invalid number of parameters");
  }

  lua_pushlightuserdata(L, (void*)MemObjCallback);
  return 1;
}

// program
std::function<void(cl_program, void*)> CreateProgramCallbackWrapper(lua_State* L, int function) {
  return [=](cl_program t0, void* t1) -> void {
    lua_rawgeti(L, LUA_REGISTRYINDEX, function);
    lua_push(L, t0);
    lua_push(L, t1);
    if (lua_pcall(L, 2, 0, 0) != 0) {
      RaiseHookError("CLProgramCallback", L);
    }
  };
}

void CL_CALLBACK ProgramCallback(cl_program p, void* ud) {
  if (ud != nullptr) {
    CLCallbackProxy* proxy = static_cast<CLCallbackProxy*>(ud);
    ud = proxy->userData;
    // This is called from a different thread, so GITS' exception handlers
    // won't catch it.
    try {
      CreateProgramCallbackWrapper(proxy->state, proxy->funcID)(p, ud);
    } catch (std::runtime_error& e) {
      LOG_ERROR << e.what();
      delete proxy;
      return;
    }
    delete proxy;
  } else {
    LOG_WARNING
        << "CLProgramCallback: something went wrong with sending data to callback function.";
  }
}

int export_ProgramCallbackPtr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 0) {
    luaL_error(L, "invalid number of parameters");
  }

  lua_pushlightuserdata(L, (void*)ProgramCallback);
  return 1;
}

// context
std::function<void(const char*, const void*, size_t, void*)> CreateContextCallbackWrapper(
    lua_State* L, int function) {
  return [=](const char* t0, const void* t1, size_t t2, void* t3) -> void {
    lua_rawgeti(L, LUA_REGISTRYINDEX, function);
    lua_push(L, t0);
    lua_push(L, t1);
    lua_push(L, t2);
    lua_push(L, t3);
    if (lua_pcall(L, 4, 0, 0) != 0) {
      RaiseHookError("CLContextCallback", L);
    }
  };
}

void CL_CALLBACK ContextCallback(const char* ei, const void* pi, size_t cb, void* ud) {
  if (ud != nullptr) {
    CLCallbackProxy* proxy = static_cast<CLCallbackProxy*>(ud);
    ud = proxy->userData;
    // This is called from a different thread, so GITS' exception handlers
    // won't catch it.
    try {
      CreateContextCallbackWrapper(proxy->state, proxy->funcID)(ei, pi, cb, ud);
    } catch (std::runtime_error& e) {
      LOG_ERROR << e.what();
      delete proxy;
      return;
    }
    delete proxy;
  } else {
    LOG_WARNING
        << "CLContextCallback: something went wrong with sending data to callback function.";
  }
}

int export_ContextCallbackPtr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 0) {
    luaL_error(L, "invalid number of parameters");
  }

  lua_pushlightuserdata(L, (void*)ContextCallback);
  return 1;
}

// SVM free
std::function<void(cl_command_queue, cl_uint, void**, void*)> CreateSVMFreeCallbackWrapper(
    lua_State* L, int function) {
  return [=](cl_command_queue t0, cl_uint t1, void** t2, void* t3) -> void {
    lua_rawgeti(L, LUA_REGISTRYINDEX, function);
    lua_push(L, t0);
    lua_push(L, t1);
    lua_push(L, t2);
    lua_push(L, t3);
    if (lua_pcall(L, 4, 0, 0) != 0) {
      RaiseHookError("CLSVMFreeCallback", L);
    }
  };
}

void CL_CALLBACK SVMFreeCallback(cl_command_queue cq, cl_uint n, void** p, void* ud) {
  if (ud != nullptr) {
    CLCallbackProxy* proxy = static_cast<CLCallbackProxy*>(ud);
    ud = proxy->userData;
    // This is called from a different thread, so GITS' exception handlers
    // won't catch it.
    try {
      CreateSVMFreeCallbackWrapper(proxy->state, proxy->funcID)(cq, n, p, ud);
    } catch (std::runtime_error& e) {
      LOG_ERROR << e.what();
      delete proxy;
      return;
    }
    delete proxy;
  } else {
    LOG_WARNING
        << "CLSVMFreeCallback: something went wrong with sending data to callback function.";
  }
}

int export_SVMFreeCallbackPtr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 0) {
    luaL_error(L, "invalid number of parameters");
  }

  lua_pushlightuserdata(L, (void*)SVMFreeCallback);
  return 1;
}

// setting callback Lua function and user data
int export_CallbackData(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  auto user_data = lua_to<void**>(L, 2);
  lua_pop(L, 1); // required to get ref to function
  int function = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_State* state = lua_newthread(L);
  int thread = luaL_ref(L, LUA_REGISTRYINDEX);
  // XXX: (kkasper) potential leak, but I have no idea how to do it better
  CLCallbackProxy* proxy = new CLCallbackProxy;
  proxy->state = state;
  proxy->funcID = function;
  proxy->threadID = thread;
  proxy->userData = user_data;

  lua_pushlightuserdata(L, (void*)proxy);
  return 1;
}

class Tracer {
public:
  template <class T>
  NOINLINE static void TraceRet(T r) {
    LOG_TRACE << " = " << r << std::endl;
  }

  template <class T>
  NOINLINE static void TraceRet(T* r) {
    if (r == nullptr) {
      LOG_TRACE << " = nullptr" << std::endl;
    } else {
#ifdef GITS_PLATFORM_LINUX
      LOG_TRACE << " = " << r << std::endl;
#else
      LOG_TRACE << " = 0x" << r << std::endl;
#endif
    }
  }

  NOINLINE static void TraceRet(cl_int r) {
    LOG_TRACE << " = " << CLResultToString(r) << std::endl;
  }

  NOINLINE static void TraceRet(void_t r) {
    LOG_TRACE << std::endl;
  }
};
} // namespace OpenCL
} // namespace gits
