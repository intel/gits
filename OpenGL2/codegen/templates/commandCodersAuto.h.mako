// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Per-command GetSize/Encode/Decode declarations; see commandCodersAuto.cpp.mako for the
// definitions and argumentCoders.h for the generic per-field overloads these are built from.
// Mirrors Vulkan/codegen/templates/commandCodersAuto.h.mako, except GetSize returns uint64_t
// here (Vulkan's own equivalent stays uint32_t) - see argumentCoders.h's own top-of-file
// comment for why: a large array-typed field (e.g. glBufferData's `data`) can legitimately
// approach or exceed 4 GiB on this backend, and the whole point of widening ArrayArgument<T>'s
// own wire-level count to uint64_t is undermined if the command-level envelope wrapping it
// still narrows back down to uint32_t.

#pragma once

#include "commandsAuto.h"

#include <concepts>

namespace gits::opengl2 {

// The per-command coders are explicit specializations of these templates rather than one
// overload per command. Explicit specializations do not take part in overload resolution, so
// GetSize/Encode/Decode calls - including the per-field calls in commandCodersAuto.cpp - are
// not resolved against thousands of candidates. Hand-written coders for meta commands stay
// ordinary overloads, which overload resolution prefers over these templates.
template <typename CommandT>
  requires std::derived_from<CommandT, Command>
uint64_t GetSize(const CommandT& command);

template <typename CommandT>
  requires std::derived_from<CommandT, Command>
void Encode(const CommandT& command, char* dest);

template <typename CommandT>
  requires std::derived_from<CommandT, Command>
void Decode(const char* src, CommandT& command);

% for cmd in commands:
% if cmd.enabled:
template <>
uint64_t GetSize(const ${cmd.name}Command& command);
template <>
void Encode(const ${cmd.name}Command& command, char* dest);
template <>
void Decode(const char* src, ${cmd.name}Command& command);
% endif
% endfor

} // namespace gits::opengl2
