// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#include "exception.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <syslog.h>
// Undefine the macros to avoid conflicts with the GITS logging macros
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARNING
#endif

#include "lua_bindings.h"
#include "log.h"
#include "gits.h"

namespace gits {

namespace lua {
typedef std::shared_ptr<lua_State> lua_ptr;
void RaiseHookError(const char* where, lua_State* L) {
  std::string where_s = where;
  std::string error = lua_tostring(L, -1);
  throw std::runtime_error("Failed when calling event hook - '" + where_s +
                           "' please fix the script before continuing\n" + error);
}

bool FunctionExists(const char* name, lua_State* L) {
  // If there is not such function in the script, return empty handler.
  lua_getglobal(L, name);
  bool status = (bool)lua_isfunction(L, -1);
  lua_pop(L, 1);
  return status;
}

std::function<void()> CreateWrapper(lua_ptr& L, const char* name) {
  // If there is not such function in the script, return empty handler.
  if (!FunctionExists(name, L.get())) {
    return [] {};
  }

  std::string name2(name);
  return [name2 = std::move(name2), L]() -> void {
    lua_getglobal(L.get(), name2.c_str());
    if (lua_pcall(L.get(), 0, 0, 0) != 0) {
      RaiseHookError(name2.c_str(), L.get());
    }
  };
}

template <class T0>
std::function<void(T0)> CreateWrapper(lua_ptr& L, const char* name) {
  // If there is not such function in the script, return empty handler.
  if (!FunctionExists(name, L.get())) {
    return [](T0) {};
  }

  std::string name2(name);
  return [name2 = std::move(name2), L](T0 t0) -> void {
    lua_getglobal(L.get(), name2.c_str());
    lua_push(L.get(), t0);
    if (lua_pcall(L.get(), 1, 0, 0) != 0) {
      RaiseHookError(name2.c_str(), L.get());
    }
  };
}

template <class T0, class T1>
std::function<void(T0, T1)> CreateWrapper(lua_ptr& L, const char* name) {
  // If there is not such function in the script, return empty handler.
  if (!FunctionExists(name, L.get())) {
    return [](T0, T1) {};
  }

  std::string name2(name);
  return [=](T0 t0, T1 t1) -> void {
    lua_getglobal(L.get(), name2.c_str());
    lua_push(L.get(), t0);
    lua_push(L.get(), t1);
    if (lua_pcall(L.get(), 2, 0, 0) != 0) {
      RaiseHookError(name2.c_str(), L.get());
    }
  };
}

template <class T0, class T1, class T2>
std::function<void(T0, T1, T2)> CreateWrapper(lua_ptr& L, const char* name) {
  // If there is not such function in the script, return empty handler.
  if (!FunctionExists(name, L.get())) {
    return [](T0, T1, T2) {};
  }

  std::string name2(name);
  return [=](T0 t0, T1 t1, T2 t2) -> void {
    lua_getglobal(L.get(), name2.c_str());
    lua_push(L.get(), t0);
    lua_push(L.get(), t1);
    lua_push(L.get(), t2);
    if (lua_pcall(L.get(), 3, 0, 0) != 0) {
      RaiseHookError(name2.c_str(), L.get());
    }
  };
}

int export_UserDataToString(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  void* ptr = lua_to<void*>(L, 1);
  lua_pushstring(L, (const char*)ptr);
  return 1;
}

int export_AllocUserDataFromStr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  std::string str(lua_tostring(L, 1));
  char* data = (char*)malloc(str.size() + 1);
  if (data == nullptr) {
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  char* ptr = data;
  for (int i = 0; i < (int)str.size(); i++, ptr++) {
    *ptr = str[i];
  }
  *ptr = 0;
  lua_pushlightuserdata(L, (void*)data);
  return 1;
}

int export_LuaStrToCStr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  const char* cstr = lua_tostring(L, 1);
  lua_pushlightuserdata(L, (void*)cstr);
  return 1;
}

int export_CStrToLuaStr(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  const char* cstr = (const char*)lua_touserdata(L, 1);
  lua_pushstring(L, cstr);
  return 1;
}

int export_UserDataToInt(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  void* ptr = lua_to<void*>(L, 1);
  lua_pushnumber(L, static_cast<lua_Number>(reinterpret_cast<uintptr_t>(ptr)));
  return 1;
}

int export_UserdataFromArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  void** ptr = lua_to<void**>(L, 1);
  auto idx = lua_tointeger(L, 2);

  lua_pushlightuserdata(L, ptr[idx]);
  return 1;
}

int export_ByteFromArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  uint8_t* ptr = lua_to<uint8_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);

  lua_pushnumber(L, ptr[idx]);
  return 1;
}

int export_Int32FromArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  int32_t* ptr = lua_to<int32_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);

  lua_pushnumber(L, ptr[idx]);
  return 1;
}

int export_Int64FromArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  int64_t* ptr = lua_to<int64_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);

  lua_pushnumber(L, (lua_Number)ptr[idx]);
  return 1;
}

int export_SizeTFromArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  size_t* ptr = lua_to<size_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);

  lua_pushnumber(L, (lua_Number)ptr[idx]);
  return 1;
}

int export_FloatFromArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }
  float* ptr = (float*)lua_touserdata(L, 1);
  auto idx = lua_tointeger(L, 2);

  lua_pushnumber(L, ptr[idx]);
  return 1;
}

int export_SetUserdataInArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto arr = lua_to<void**>(L, 1);
  auto idx = lua_tointeger(L, 2);
  auto val = lua_to<void*>(L, 3);
  arr[idx] = val;

  return 0;
}

int export_SetByteInArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto arr = lua_to<uint8_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);
  auto val = lua_to<uint8_t>(L, 3);
  arr[idx] = val;

  return 0;
}

int export_SetInt32InArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto arr = lua_to<int32_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);
  auto val = lua_to<int32_t>(L, 3);
  arr[idx] = val;

  return 0;
}

int export_SetInt64InArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto arr = lua_to<int64_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);
  auto val = lua_to<int64_t>(L, 3);
  arr[idx] = val;

  return 0;
}

int export_SetSizeTInArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto arr = lua_to<size_t*>(L, 1);
  auto idx = lua_tointeger(L, 2);
  auto val = lua_to<size_t>(L, 3);
  arr[idx] = val;

  return 0;
}

int export_SetFloatInArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto arr = lua_to<float*>(L, 1);
  auto idx = lua_tointeger(L, 2);
  auto val = lua_to<float>(L, 3);
  arr[idx] = val;

  return 0;
}

int export_CurrTime(lua_State* L) {
  uint64_t usecs = gits::CGits::Instance().Timers().program.Get() / 1000ull;
  lua_pushnumber(L, static_cast<double>(usecs));
  return 1;
}

int export_NullUdt(lua_State* L) {
  lua_pushlightuserdata(L, nullptr);
  return 1;
}

#ifdef GITS_PLATFORM_WINDOWS
int export_MessageBox(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  auto title = lua_tostring(L, 1);
  auto message = lua_tostring(L, 2);
  auto type = lua_tointeger(L, 3);

  MessageBoxA(0, message, title, (UINT)type);
  return 0;
}

int export_GetWindowRectByHDC(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }

  auto hdc = lua_touserdata(L, 1);
  auto hwnd = WindowFromDC((HDC)hdc);
  RECT r;
  GetWindowRect(hwnd, &r);

  lua_pushinteger(L, r.bottom);
  lua_pushinteger(L, r.left);
  lua_pushinteger(L, r.right);
  lua_pushinteger(L, r.top);

  return 4;
}
#else
int export_MessageBox(lua_State* L) {
  return 0;
}
int export_GetWindowRectByHDC(lua_State* L) {
  return 0;
}
#endif

int export_GetPtrSize(lua_State* L) {
  const int ptrSize = sizeof(void*);
  lua_pushinteger(L, ptrSize);
  return 1;
}

int export_AllocByteArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }

  int amount = lua_to<int>(L, 1);
  lua_pushlightuserdata(L, malloc(amount));
  return 1;
}

int export_FreeByteArray(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }

  auto memory = lua_to<void*>(L, 1);
  free(memory);
  return 0;
}

int export_ArgsString(lua_State* L) {
  lua_pushstring(L, (const char*)Configurator::Get().common.shared.scriptArgsStr.c_str());
  return 1;
}

int export_OutDir(lua_State* L) {
  lua_pushstring(L, (const char*)Configurator::Get().common.player.outputDir.string().c_str());
  return 1;
}

int export_SystemLog(lua_State* L) {
#ifndef GITS_PLATFORM_WINDOWS
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  void* ptr = lua_to<void*>(L, 1);
  syslog(LOG_NOTICE, "%s", (const char*)ptr);
#else
  throw ENotImplemented("systemLog lua function not implemented for windows.");
#endif
  return 1;
}

int export_GetStreamDir(lua_State* L) {
  const auto& cfg = Configurator::Get();
  const auto& streamPath = Configurator::IsPlayer() ? cfg.common.player.streamDir.string()
                                                    : cfg.common.recorder.dumpPath.string();
  lua_pushstring(L, streamPath.c_str());
  return 1;
}

int export_Log(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }

  auto log_level = (LogLevel)lua_tointeger(L, 1);
  std::string text(lua_tostring(L, 2));

  auto severity = log::GetSeverity(log_level);
  PLOG(severity) << text;

  return 0;
}

int export_GetCurrentFrame(lua_State* L) {
  auto currFrame = gits::CGits::Instance().CurrentFrame();
  lua_pushinteger(L, currFrame);
  return 1;
}

int export_GetCurrentDraw(lua_State* L) {
  auto currDraw = gits::CGits::Instance().CurrentDrawCount();
  lua_pushinteger(L, currDraw);
  return 1;
}

int export_GetCurrentDrawInFrame(lua_State* L) {
  auto currDraw = gits::CGits::Instance().CurrentDrawInFrameCount();
  lua_pushinteger(L, currDraw);
  return 1;
}

int export_NewBitRange(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }

  std::string str(lua_tostring(L, 1));
  void* P = lua_newuserdata(L, sizeof(BitRange));
  new (P) BitRange(str);

  return 1;
}

int export_InBitRange(lua_State* L) {
  int top = lua_gettop(L);
  if (top != 2) {
    luaL_error(L, "invalid number of parameters");
  }

  BitRange* ptr = (BitRange*)lua_touserdata(L, 1);
  auto i = lua_tointeger(L, 2);
  auto ret = (*ptr)[i];
  lua_pushboolean(L, ret);
  return 1;
}

int export_IsInt64InRange(lua_State* L) {
  const auto top = lua_gettop(L);
  if (top != 3) {
    luaL_error(L, "invalid number of parameters");
  }

  const auto rangeBegin = lua_to<uint64_t>(L, 1);
  const auto rangeSize = lua_to<uint32_t>(L, 2);
  const auto int64Number = lua_to<uint64_t>(L, 3);
  const auto rangeEnd = rangeBegin + rangeSize;
  const auto ret = int64Number >= rangeBegin && int64Number < rangeEnd;
  lua_pushboolean(L, ret);
  return 1;
}

void CreateAndRegisterEvents(const char* script) {
  lua_ptr L(luaL_newstate(), lua_close);
  if (L.get() == nullptr) {
    const auto msg = "Couldn't initialize Lua state.";
    throw gits::EOperationFailed(std::string(EXCEPTION_MESSAGE) + msg);
  }
  const luaL_Reg exports[] = {{"udtToStr", export_UserDataToString},
                              {"allocUdtFromStr", export_AllocUserDataFromStr},
                              {"luaStrToCStr", export_LuaStrToCStr},
                              {"cStrToLuaStr", export_CStrToLuaStr},
                              {"udtToInt", export_UserDataToInt},

                              {"getUdt", export_UserdataFromArray},
                              {"getByte", export_ByteFromArray},
                              {"getInt", export_Int32FromArray},
                              {"getInt64", export_Int64FromArray},
                              {"getSizeT", export_SizeTFromArray},
                              {"getFloat", export_FloatFromArray},

                              {"setUdt", export_SetUserdataInArray},
                              {"setByte", export_SetByteInArray},
                              {"setInt", export_SetInt32InArray},
                              {"setInt64", export_SetInt64InArray},
                              {"setSizeT", export_SetSizeTInArray},
                              {"setFloat", export_SetFloatInArray},

                              {"isInt64InRange", export_IsInt64InRange},

                              {"nullUdt", export_NullUdt},
                              {"currTime", export_CurrTime},
                              {"msgBox", export_MessageBox},
                              {"getPtrSize", export_GetPtrSize},
                              {"allocBytes", export_AllocByteArray},
                              {"freeBytes", export_FreeByteArray},
                              {"getWndRectByHDC", export_GetWindowRectByHDC},
                              {"getArgsStr", export_ArgsString},
                              {"getOutDir", export_OutDir},
                              {"systemLog", export_SystemLog},
                              {"log", export_Log},
                              {"getStreamDir", export_GetStreamDir},
                              {"getCurrentFrame", export_GetCurrentFrame},
                              {"getCurrentDraw", export_GetCurrentDraw},
                              {"getCurrentDrawInFrame", export_GetCurrentDrawInFrame},
                              {"newBitRange", export_NewBitRange},
                              {"inBitRange", export_InBitRange},
                              {nullptr, nullptr}};

  luaL_newlib(L.get(), exports);
  lua_setglobal(L.get(), "gits");

  luaL_openlibs(L.get());
  if (luaL_dofile(L.get(), script)) {
    throw std::runtime_error("Failed to parse script " + std::string(script) +
                             ". Errors are: " + lua_tostring(L.get(), -1));
  }

  auto gitsStateRestoreBeginWrapper = CreateWrapper(L, "gitsStateRestoreBegin");
  auto gitsStateRestoreEndWrapper = CreateWrapper(L, "gitsStateRestoreEnd");
  auto gitsFrameBeginWrapper = CreateWrapper<int>(L, "gitsFrameBegin");
  auto gitsFrameEndWrapper = CreateWrapper<int>(L, "gitsFrameEnd");
  auto eventHandler = [gitsStateRestoreBeginWrapper, gitsStateRestoreEndWrapper,
                       gitsFrameBeginWrapper, gitsFrameEndWrapper](Topic t, const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<GitsEventMessage>(m);
    if (!msg) {
      return;
    }

    auto& data = msg->getData();

    if (data.Id == CToken::TId::ID_INIT_START) {
      gitsStateRestoreBeginWrapper();
    } else if (data.Id == CToken::TId::ID_INIT_END) {
      gitsStateRestoreEndWrapper();
    } else if (data.Id == CToken::TId::ID_FRAME_START) {
      gitsFrameBeginWrapper(data.FrameStartData.FrameNumber);
    } else if (data.Id == CToken::TId::ID_FRAME_END) {
      gitsFrameEndWrapper(data.FrameEndData.FrameNumber);
    }
  };
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_GITS_EVENT}, eventHandler);

  auto gitsProgramStartWrapper = CreateWrapper(L, "gitsProgramStart");
  auto gitsProgramExitWrapper = CreateWrapper(L, "gitsProgramExit");
  auto programHandler = [gitsProgramStartWrapper, gitsProgramExitWrapper](Topic t,
                                                                          const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<ProgramMessage>(m);
    if (!msg) {
      return;
    }

    if (t.topicId == TOPIC_PROGRAM_START) {
      gitsProgramStartWrapper();
    } else if (t.topicId == TOPIC_PROGRAM_EXIT) {
      gitsProgramExitWrapper();
    }
  };
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_START},
                                              programHandler);
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                              programHandler);
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_START},
                                              programHandler);
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT},
                                              programHandler);

  auto gitsLoopBeginWrapper = CreateWrapper(L, "gitsLoopBegin");
  auto gitsLoopEndWrapper = CreateWrapper(L, "gitsLoopEnd");
  auto loopHandler = [gitsLoopBeginWrapper, gitsLoopEndWrapper](Topic t, const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<LoopMessage>(m);
    if (!msg) {
      return;
    }

    if (t.topicId == TOPIC_LOOP_BEGIN) {
      gitsLoopBeginWrapper();
    } else if (t.topicId == TOPIC_LOOP_END) {
      gitsLoopEndWrapper();
    }
  };
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_LOOP_BEGIN}, loopHandler);
  CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_LOOP_END}, loopHandler);

  CGits::Instance().SetLua(L);
}

} // namespace lua
} // namespace gits
