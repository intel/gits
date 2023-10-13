// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#include "pragmas.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "log.h"

#include <cstdint>
#include <functional>
#include <mutex>

namespace gits {
namespace lua {

void CreateAndRegisterEvents(const char* script);
bool FunctionExists(const char* name, lua_State* L);
void RaiseHookError(const char* name, lua_State* L);

template <class T>
void lua_push(lua_State* L, T value) {
  lua_pushnumber(L, static_cast<lua_Number>(value));
}
template <>
inline void lua_push(lua_State* L, int64_t value) {
  lua_pushinteger(L, static_cast<lua_Integer>(value));
}
template <>
inline void lua_push(lua_State* L, uint64_t value) {
  lua_pushinteger(L, static_cast<lua_Integer>(value));
}
template <class T>
inline void lua_push(lua_State* L, T* value) {
  lua_pushlightuserdata(L, reinterpret_cast<void*>(value));
}
template <class T>
inline void lua_push(lua_State* L, const T* value) {
  lua_pushlightuserdata(L, const_cast<void*>(static_cast<const void*>(value)));
}

template <class T>
T lua_to(lua_State* L, int pos) {
  return (T)lua_touserdata(L, pos);
}
template <>
inline int8_t lua_to(lua_State* L, int pos) {
  return static_cast<int8_t>(lua_tointeger(L, pos));
}
template <>
inline uint8_t lua_to(lua_State* L, int pos) {
  return static_cast<uint8_t>(lua_tointeger(L, pos));
}
template <>
inline int16_t lua_to(lua_State* L, int pos) {
  return static_cast<int16_t>(lua_tointeger(L, pos));
}
template <>
inline uint16_t lua_to(lua_State* L, int pos) {
  return static_cast<uint16_t>(lua_tointeger(L, pos));
}
template <>
inline int32_t lua_to(lua_State* L, int pos) {
  return static_cast<int32_t>(lua_tointeger(L, pos));
}
template <>
inline uint32_t lua_to(lua_State* L, int pos) {
  return static_cast<uint32_t>(lua_tointeger(L, pos));
}
#if !defined GITS_PLATFORM_X11
template <>
inline long lua_to(lua_State* L, int pos) {
  return static_cast<long>(lua_tointeger(L, pos));
}
template <>
inline unsigned long lua_to(lua_State* L, int pos) {
  return static_cast<unsigned long>(lua_tointeger(L, pos));
}
#endif
template <>
inline int64_t lua_to(lua_State* L, int pos) {
  return lua_tointeger(L, pos);
}
template <>
inline uint64_t lua_to(lua_State* L, int pos) {
  return lua_tointeger(L, pos);
}
template <>
inline float lua_to(lua_State* L, int pos) {
  return static_cast<float>(lua_tonumber(L, pos));
}
template <>
inline double lua_to(lua_State* L, int pos) {
  return lua_tonumber(L, pos);
}

template <class T>
NOINLINE void SetGlobal(lua_State* L, const char* name, T value) {
  lua_push(L, value);
  lua_setglobal(L, name);
}

inline void lua_push_args(lua_State* /*L*/) {}
template <class Head, class... Tail>
void lua_push_args(lua_State* L, Head h, Tail... t) {
  lua_push(L, h);
  lua_push_args(L, t...);
}

struct ArgsPusher {
  ArgsPusher(lua_State* L) : _l(L) {}
  template <class... Args>
  void push(Args... args) {
    lua_push_args(_l, args...);
  }

private:
  lua_State* _l;
};

template <class T>
struct ArgNum;

template <class T, class... Args>
struct ArgNum<T(Args...)> {
  enum { value = sizeof...(Args) };
};

template <class>
struct Argnum;
template <class R, class... T>
struct Argnum<R(T...)> {
  enum { value = sizeof...(T) };
};

template <class>
struct FuncToTuple;
template <class R, class... T>
struct FuncToTuple<R(T...)> {
  typedef std::tuple<T...> type;
};

template <int...>
struct seq {};

template <int N, int... S>
struct gens : gens<N - 1, N - 1, S...> {};

template <int... S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

template <class... T>
void accept(T...) {}

template <class T, int... I>
void fill_tuple2(lua_State* L, T& tuple, seq<I...>) {
  accept(std::get<I>(tuple) =
             gits::lua::lua_to<typename std::remove_reference<decltype(std::get<I>(tuple))>::type>(
                 L, I + 1)...);
}

template <class R, class Callable, class T, int... I>
R call_tuple2(Callable callable, T& tuple, seq<I...>) {
  return callable(std::get<I>(tuple)...);
}

template <class... T>
void fill_tuple(lua_State* L, std::tuple<T...>& tuple) {
  fill_tuple2(L, tuple, typename gens<sizeof...(T)>::type());
}

template <class R, class Callable, class... T>
R call_tuple(Callable callable, std::tuple<T...>& tuple) {
  return call_tuple2<R>(callable, tuple, typename gens<sizeof...(T)>::type());
}

namespace {
std::recursive_mutex luaMutex;
}

#define LUA_CALL_FUNCTION(luas, name, callargs, declargs)                                          \
  {                                                                                                \
    using namespace gits::lua;                                                                     \
    lua_getglobal(luas, name);                                                                     \
    ArgsPusher ap(luas);                                                                           \
    ap.push callargs;                                                                              \
    if (lua_pcall(luas, ArgNum<void declargs>::value, 1, 0) != 0)                                  \
      RaiseHookError(name, luas);                                                                  \
  }

} // namespace lua
} // namespace gits
