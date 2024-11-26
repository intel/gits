// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

% for enum in vk_enums:
template<> inline ${enum.name} lua_to(lua_State* L, int pos) { return static_cast<${enum.name}>(lua_tointeger(L, pos)); }
% endfor
