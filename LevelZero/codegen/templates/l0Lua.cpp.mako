// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Lua.h"

namespace gits {
namespace l0 {
%for name, enum in enums.items():
  %if "_structure_type_t" in enum.get('name'):
template<>
void *lua_to_extension_struct<${get_namespace(enum.get('name'))}_structure_type_t>(lua_State* L, int pos) {
  const auto type = lua_type(L, pos);
  const char *typeName = lua_typename(L, type);
  if (std::strcmp(typeName, "table") == 0) {
    lua_getfield(L, pos, "stype");
    const auto val = lua_to_ext<${enum.get('name')}>(L, -1);
    lua_pop(L, 1);
    switch (val) {
    %for structure in enum['vars']:
    case ${structure['name']}:
      return lua_to_ext<${structure['struct']}*>(L, pos);
    %endfor
    }
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return nullptr;
}
template<>
void lua_push_extension_struct<${get_namespace(enum.get('name'))}_structure_type_t>(lua_State* L, const void *pNext) {
  if (pNext == nullptr) {
    gits::lua::lua_push(L, pNext);
    return;
  }
  auto *extendedProperties = reinterpret_cast<const ${get_namespace(enum.get('name'))}_base_properties_t *>(pNext);
  switch (extendedProperties->stype) {
    %for structure in enum['vars']:
  case ${structure['name']}:
    lua_push_ext(L, reinterpret_cast<const ${structure['struct']}*>(pNext));
    return;
    %endfor
  }
}
  %endif
%endfor
%for name, arg in arguments.items():
  %if not is_latest_version(arguments, arg) or not arg.get('enabled', True):
<% continue %>
  %endif
  %if 'vars' in arg:
int create_${arg.get('name')}(lua_State* L) {
  const auto top = lua_gettop(L);
  ${arg.get('name')}* val = nullptr;
  if (top == 0) {
    val = (${arg.get('name')}*)calloc(1, sizeof(${arg.get('name')}));
  } else if (top == 1) {
    val = reinterpret_cast<${arg.get('name')}*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
  } else {
    luaL_error(L, "invalid number of parameters");
  }
  lua_push_ext<${arg.get('name')}*>(L, val);
  return 1;
}
  %endif
%endfor
int export_FreeStruct(lua_State *L) {
  const auto top = lua_gettop(L);
  if (top != 1) {
    luaL_error(L, "invalid number of parameters");
  }
  const auto type = lua_type(L, -1);
  const char *typeName = lua_typename(L, type);
  if (std::strcmp(typeName, "table")) {
    luaL_error(L, "invalid type");
  }
  lua_getfield(L, -1, "__self");
  auto memory = lua_touserdata(L, -1);
  free(memory);
  lua_pushlightuserdata(L, nullptr);
  lua_setfield(L, -3, "__self");
  lua_pop(L, lua_gettop(L));
  return 0;
}
} // namespace l0
} // namespace gits