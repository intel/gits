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
// Mirrors apis/Vulkan/codegen/templates/commandCodersAuto.cpp.mako.
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

namespace gits::opengl {

% for cmd in commands:
% if cmd.enabled:
<%
  members = ['m_Key', 'm_ThreadId']
  for p in cmd.params:
      if p.name in consumed_pointer_params(cmd.name):
          continue
      members.append(f'm_{p.name}')
      override = pointer_override_for(cmd, p.name)
      if pointer_needs_raw_offset_capture(override):
          members.append(f'm_{p.name}RawPointer')
      if pointer_needs_pixel_transfer_unknown_flag(override):
          members += [f'm_{p.name}SizeUnknown', f'm_{p.name}IsBufferOffset']
  if has_return_member(cmd):
      members.append('m_Return')
%>\
template <>
uint64_t GetSize(const ${cmd.name}Command& command) {
  return ${' +\n         '.join(f'GetSize(command.{member})' for member in members)};
}

template <>
void Encode(const ${cmd.name}Command& command, char* dest) {
  uint64_t offset = 0;
% for member in members:
  Encode(dest, offset, command.${member});
% endfor
}

template <>
void Decode(const char* src, ${cmd.name}Command& command) {
  uint64_t offset = 0;
% for member in members:
  Decode(src, offset, command.${member});
% endfor
}

% endif
% endfor
} // namespace gits::opengl
