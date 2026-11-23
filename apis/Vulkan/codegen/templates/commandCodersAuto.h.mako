// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandsAuto.h"

#include <concepts>

namespace gits {
namespace vulkan {

// The per-command coders are explicit specializations of these templates rather than one
// overload per command. Explicit specializations do not take part in overload resolution, so
// calls to them are not resolved against every command. Hand-written coders for custom commands
// stay ordinary overloads, which overload resolution prefers over these templates.
template <typename CommandT>
  requires std::derived_from<CommandT, Command>
uint32_t GetSize(const CommandT& command);

template <typename CommandT>
  requires std::derived_from<CommandT, Command>
void Encode(const CommandT& command, char* dest);

template <typename CommandT>
  requires std::derived_from<CommandT, Command>
void Decode(char* src, CommandT& command);

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
template <>
uint32_t GetSize(const ${command.name}Command& command);
template <>
void Encode(const ${command.name}Command& command, char* dest);
template <>
void Decode(char* src, ${command.name}Command& command);
% if define:
#endif
% endif
% endfor

} // namespace vulkan
} // namespace gits
