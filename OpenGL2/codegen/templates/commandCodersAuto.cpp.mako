// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Per-command GetSize/Encode/Decode: a fixed-order sum/walk over the command envelope
// (m_Key, m_ThreadId), then each parameter in declaration order, then m_Return if present -
// delegating the actual bytes-on-the-wire logic to argumentCoders.h's generic overloads, which
// dispatch on whether a field is a plain scalar, an Argument<T>, or a HandleArgument<T>.
// Mirrors Vulkan/codegen/templates/commandCodersAuto.cpp.mako.
//
// A parameter absorbed into a sibling's ArrayOfArrays<T> wrapper (see
// array_of_arrays_metadata.py's consumed_pointer_params() - e.g. glShaderSource's `length`)
// has no m_<name> member at all (commandsAuto.h.mako skips it too) and is walked over here,
// same as commandsAuto.h.mako/commandRunnersAuto.cpp.mako.
//
// A parameter meeting pointer_needs_raw_offset_capture() (generator_
// helpers.py) additionally walks its companion m_<param>RawPointer member right after its own
// m_<param> - see that predicate's own header comment and commandsAuto.h.mako's matching
// member-declaration branch.
//
// A parameter meeting pointer_needs_pixel_transfer_
// unknown_flag() (generator_helpers.py) additionally walks two more companions right after:
// m_<param>SizeUnknown, then m_<param>IsBufferOffset - see that predicate's own header comment
// and commandsAuto.h.mako's matching member-declaration branch.

#include "argumentCoders.h"
#include "commandCodersAuto.h"

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
<%
  is_void = cmd.return_value.type.strip().lower() == 'void'
%>\
uint64_t GetSize(const ${cmd.name}Command& command) {
  return GetSize(command.m_Key) +
         GetSize(command.m_ThreadId)\
% for p in cmd.params:
% if p.name not in consumed_pointer_params(cmd.name):
 +
         GetSize(command.m_${p.name})\
% if pointer_needs_raw_offset_capture(pointer_override_for(cmd, p.name)):
 +
         GetSize(command.m_${p.name}RawPointer)\
% endif
% if pointer_needs_pixel_transfer_unknown_flag(pointer_override_for(cmd, p.name)):
 +
         GetSize(command.m_${p.name}SizeUnknown) +
         GetSize(command.m_${p.name}IsBufferOffset)\
% endif
% endif
% endfor
% if not is_void:
 +
         GetSize(command.m_Return)\
% endif
;
}

void Encode(const ${cmd.name}Command& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
% for p in cmd.params:
% if p.name not in consumed_pointer_params(cmd.name):
  Encode(dest, offset, command.m_${p.name});
% if pointer_needs_raw_offset_capture(pointer_override_for(cmd, p.name)):
  Encode(dest, offset, command.m_${p.name}RawPointer);
% endif
% if pointer_needs_pixel_transfer_unknown_flag(pointer_override_for(cmd, p.name)):
  Encode(dest, offset, command.m_${p.name}SizeUnknown);
  Encode(dest, offset, command.m_${p.name}IsBufferOffset);
% endif
% endif
% endfor
% if not is_void:
  Encode(dest, offset, command.m_Return);
% endif
}

void Decode(const char* src, ${cmd.name}Command& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
% for p in cmd.params:
% if p.name not in consumed_pointer_params(cmd.name):
  Decode(src, offset, command.m_${p.name});
% if pointer_needs_raw_offset_capture(pointer_override_for(cmd, p.name)):
  Decode(src, offset, command.m_${p.name}RawPointer);
% endif
% if pointer_needs_pixel_transfer_unknown_flag(pointer_override_for(cmd, p.name)):
  Decode(src, offset, command.m_${p.name}SizeUnknown);
  Decode(src, offset, command.m_${p.name}IsBufferOffset);
% endif
% endif
% endfor
% if not is_void:
  Decode(src, offset, command.m_Return);
% endif
}

% endif
% endfor
} // namespace gits::opengl2
