// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

% for enum in vk_enums:
template<> inline ${enum.name} lua_to(lua_State* L, int pos) { return static_cast<${enum.name}>(lua_tointeger(L, pos)); }
% endfor

% for union in vk_unions:
<%
    canon_member_type: str  # Type of the canonical union member.
    arrayless_canon_member_type: str
    array: str
    array_length: str
    for field in union.fields:
        if field.name == union.canonical_union_member:
            canon_member_type = field.type

            arrayless_canon_member_type, array = split_arrays_from_name(field.type)
            array_length = array.strip('[]')

            break
    else:
        raise ValueError(f"Union {union.name} has no correct canonical member set.")
%>\

% if array:
// ${union.name}.${union.canonical_union_member} is an array; push it as Lua table.
template<>
void lua_push(lua_State* L, ${union.name} value) {
  lua_newtable(L); // Create a new table and push it onto the stack.

  for (int i = 0; i < ${array_length}; ++i) {
    // Push key and value.
    lua_pushinteger(L, i + 1);
    lua_push(L, value.${union.canonical_union_member}[i]);
    // Stack indexes now look like this: table @ -3, key @ -2, value @ -1.
    lua_rawset(L, -3); // Pop key and value, put value in the table at this key.
  }
}
template<>
inline ${union.name} lua_to(lua_State* L, int pos) {
  ${union.name} retval;

  if (!lua_istable(L, pos)) {
    luaL_error(L, "Expected a table at position %d", pos);
  }

  for (int i = 0; i < ${array_length}; ++i) {
    lua_pushinteger(L, i + 1); // Push key onto the stack.
    lua_rawget(L, pos); // Pop key, get element from table, push it onto stack.
    retval.${union.canonical_union_member}[i] = lua_to<${arrayless_canon_member_type}>(L, -1);
    lua_pop(L, 1); // Remove the element from the stack.
  }

  return retval;
}
  % else:
template<>
void lua_push(lua_State* L, ${union.name} value) {
  lua_push(L, value.${union.canonical_union_member});
}
template<>
inline ${union.name} lua_to(lua_State* L, int pos) {
  ${union.name} retval;
  retval.${union.canonical_union_member} = lua_to<${canon_member_type}>(L, pos);
  return retval;
}
  % endif
% endfor
