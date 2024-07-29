// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
<%
def get_name(name):
  return name.split('[')[0] if '[' in name else name
def get_array_size(name):
  return name.split('[')[1].split(']')[0]
%>
#include "l0Header.h"
#include "lua_bindings.h"
#include "exception.h"
#include <cstring>

namespace gits {
  namespace l0 {
    template<class T> void lua_push_ext(lua_State* L, T value) { gits::lua::lua_push(L, value); }
    template<class T> T lua_to_ext(lua_State* L, int pos) { return gits::lua::lua_to<T>(L, pos); }
    template<typename T> void *lua_to_extension_struct([[maybe_unused]] lua_State * L, [[maybe_unused]] int pos) {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    template<typename T> void lua_push_extension_struct([[maybe_unused]] lua_State * L,[[maybe_unused]] const void* pNext) {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    template<> inline zel_handle_type_t lua_to_ext(lua_State* L, int pos) { return static_cast<zel_handle_type_t>(lua_tointeger(L, pos)); }
%for name, enum in enums.items():
    template<> inline ${enum.get('name')} lua_to_ext(lua_State* L, int pos) { return static_cast<${enum.get('name')}>(lua_tointeger(L, pos)); }
    template<> inline ${enum.get('name')}* lua_to_ext(lua_State* L, int pos) { return static_cast<${enum.get('name')}*>(lua_touserdata(L, pos)); }
    template<> inline void lua_push_ext(lua_State* L, ${enum.get('name')} val) { lua_pushinteger(L, val); }
    template<> inline void lua_push_ext(lua_State* L, ${enum.get('name')}* val) { lua_pushlightuserdata(L, val); }
  %if "_structure_type_t" in enum.get('name'):
    template<> void *lua_to_extension_struct<${enum.get('name')}>(lua_State* L, int pos);
    template<> void lua_push_extension_struct<${enum.get('name')}>(lua_State* L, const void* pNext);
  %endif
%endfor
%for name, arg in arguments.items():
  %if not is_latest_version(arguments, arg):
<% continue %>
  %endif
  %if 'vars' in arg:

    /* ${arg.get('name')} */

    inline void lua_setTableFields(lua_State* L, int pos, ${arg.get('name')}& val) {
    %for var in arg['vars']:
      %if 'pNext' == get_name(var['name']):
      lua_push_extension_struct<${get_namespace(arg.get('name'))}_structure_type_t>(L, val.${get_name(var['name'])});
      %else:
      lua_push_ext(L, val.${get_name(var['name'])});
      %endif
      lua_setfield(L, pos, "${get_name(var['name'])}");
    %endfor
    }
    inline void lua_setTableFields(lua_State* L, int pos, ${arg.get('name')}* val) {
      if (val == nullptr) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    %for var in arg['vars']:
      %if 'pNext' == get_name(var['name']):
      lua_push_extension_struct<${get_namespace(arg.get('name'))}_structure_type_t>(L, val->${get_name(var['name'])});
      %else:
      lua_push_ext(L, val->${get_name(var['name'])});
      %endif
      lua_setfield(L, pos, "${get_name(var['name'])}");
    %endfor
    }
    inline void lua_setTableFields(lua_State* L, int pos, ${arg.get('name')}** val) {
      if (val == nullptr) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      lua_setTableFields(L, pos, *val);
    }
    template<>
    inline ${arg.get('name')} lua_to_ext(lua_State* L, int pos) {
      const auto type = lua_type(L, pos);
      const char* typeName = lua_typename(L, type);
      if (!std::strcmp(typeName, "table")) {
        lua_getfield(L, pos, "__self");
        auto val = *reinterpret_cast<${arg.get('name')}*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
    %for var in arg['vars']:
        lua_getfield(L, pos, "${get_name(var['name'])}");
      %if '[' in var['name']:
        std::memcpy(&val.${get_name(var['name'])}[0], (${var['type']}*)lua_to_ext<void*>(L, -1), ${get_array_size(var['name'])});
      %else:
        val.${var['name']} = lua_to_ext<${var['type']}>(L, -1);
      %endif
        lua_pop(L, 1);
    %endfor
        return val;
      }
      throw EOperationFailed(EXCEPTION_MESSAGE + std::string("\nLua argument type is incorrect. Your type: ") + std::string(typeName));
    }
    template<>
    inline ${arg.get('name')}* lua_to_ext(lua_State* L, int pos) {
      const auto type = lua_type(L, pos);
      const char* typeName = lua_typename(L, type);
      if (!std::strcmp(typeName, "table")) {
        lua_getfield(L, pos, "__self");
        auto val = static_cast<${arg.get('name')}*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
    %for var in arg['vars']:
        lua_getfield(L, pos, "${get_name(var['name'])}");
      %if '[' in var['name']:
        std::memcpy(&val->${get_name(var['name'])}[0], (${var['type']}*)lua_to_ext<void*>(L, -1), ${get_array_size(var['name'])});
      %else:
        val->${var['name']} = lua_to_ext<${var['type']}>(L, -1);
      %endif
        lua_pop(L, 1);
    %endfor
        return val;
      } else {
        return nullptr;
      }
    }
    template<>
    inline const ${arg.get('name')}* lua_to_ext(lua_State* L, int pos) {
      const auto type = lua_type(L, pos);
      const char* typeName = lua_typename(L, type);
      if (!std::strcmp(typeName, "table")) {
        lua_getfield(L, pos, "__self");
        auto val = static_cast<${arg.get('name')}*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
    %for var in arg['vars']:
        lua_getfield(L, pos, "${get_name(var['name'])}");
      %if '[' in var['name']:
        std::memcpy(&val->${get_name(var['name'])}[0], (${var['type']}*)lua_to_ext<void*>(L, -1), ${get_array_size(var['name'])});
      %else:
        %if var['name'] == 'pNext':
        val->${var['name']} = lua_to_extension_struct<${get_namespace(arg.get('name'))}_structure_type_t>(L, -1);
        %else:
        val->${var['name']} = lua_to_ext<${var['type']}>(L, -1);
        %endif
      %endif
        lua_pop(L, 1);
    %endfor
        return val;
      } else {
        return nullptr;
      }
    }
    inline void lua_push_ext(lua_State* L, ${arg.get('name')}& val) {
      lua_newtable(L);
      int pos = lua_gettop(L);
      gits::lua::lua_push(L, &val);
      lua_setfield(L, pos, "__self");
      lua_setTableFields(L, pos, val);
    }
    template<>
    inline void lua_push_ext(lua_State* L, ${arg.get('name')}* val) {
      if (val == nullptr) {
        gits::lua::lua_push(L, val);
        return;
      }
      lua_newtable(L);
      int pos = lua_gettop(L);
      gits::lua::lua_push(L, val);
      lua_setfield(L, pos, "__self");
      lua_setTableFields(L, pos, val);
    }
    template<>
    inline void lua_push_ext(lua_State* L, const ${arg.get('name')}* val) {
      lua_push_ext(L, const_cast<${arg.get('name')}*>(val));
    }
    int create_${arg.get('name')}(lua_State* L);
  %endif
%endfor
    int export_FreeStruct(lua_State* L);
  }
}
