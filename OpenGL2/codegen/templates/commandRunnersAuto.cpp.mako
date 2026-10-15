// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Generated Run() per enabled command - see
// docs/development/opengl2-player-skeleton-plan.md's Decision 6. Every parameter gets exactly
// one of the following treatments, decided from the same TypeFacts/wrapper_for facts
// commandsAuto.h.mako already uses:
//   - HandleArgument<HGLRC>: UpdateHandle() substitutes the live handle for the decoded Key
//     immediately before the call (see handleArgumentUpdaters.h; named identically to the
//     recorder-side copy's own UpdateHandle/UpdateOutputHandle, matching Vulkan's convention -
//     see docs/development/opengl2-layer-architecture-alignment-plan.md's Phase A).
//   - Argument<HDC>: HdcVirtualizationService::Resolve() substitutes the one player-owned
//     real HDC for the recorded, process-foreign value.
//   - HandleArgument<GLuint>/HandleArrayArgument<GLuint> (see object_name_lifecycle.py): a
//     LOOKUP/DELETE-role field resolves its decoded Key(s) to live, player-local name(s) via
//     UpdateHandle()/UpdateHandleArray() immediately before the call; a CREATE-role array
//     field needs no pre-call action (its Value buffer, sized to match Key's decoded count by
//     argumentCoders.h's Decode(), is passed straight to the driver as the OUT buffer).
//   - Everything else (plain Argument<T>, ArrayArgument<T>): used as captured/decoded - see
//     call_arg()'s HasValue guard below for why a nullable one is not simply "used verbatim".
// The command's return value gets the same HandleArgument treatment plus, for
// wglCreateContext/wglCreateContextAttribsARB (handle_lifecycle.py's HANDLE_CREATING_COMMANDS)
// or glCreateProgram/glCreateShader (object_name_lifecycle.py's CREATE role on a return value),
// an UpdateOutputHandle() call once the real call has returned. A CREATE-role array
// parameter (glGenTextures/glGenBuffers) gets the array counterpart, UpdateOutputHandleArray().
//
// Mirrors Vulkan/codegen/templates/commandRunnersAuto.cpp.mako's shape, minus
// ExecuteCommands()/m_Skip support - not wired for this backend yet, see the plan's
// Non-goals. Pre/Post layers ARE wired (see
// docs/development/opengl2-layer-architecture-alignment-plan.md), via the exact same inline
// `for (Layer* layer : manager.GetPreLayers()) { layer->Pre(command); }` loop shape Vulkan's/
// DX12's own commandRunnersAuto.cpp.mako use (and, on the OpenGL2 side, the same shape every
// hand-written runner in commandRunnersCustom.cpp also repeats inline at the equivalent point
// in its own Run() - see that file's own header comment): PlayerManager::GetPreLayers() runs
// immediately before the real driver call, once every LOOKUP/DELETE-role handle/object-name
// field has been resolved to its live, replay-time value (matching Vulkan's
// commandRunnersAuto.cpp.mako's own Pre-layer placement); PlayerManager::GetPostLayers() runs
// at the very end of Run(), once every CREATE-role field has been registered. Both vectors are
// permanently empty today - no concrete Layer subclass registers into either one on the player
// side (see playerManager.h's own header comment on why the player has no
// EncoderLayer-equivalent) - so this is a zero-behavior-change wiring change, exactly like the
// recorder side's own Pre/Post loop.
//
// Two shapes added for the rendering-pipeline round 2 (see
// docs/development/opengl2-rendering-pipeline-pointer-support-plan.md), on top of the treatments
// above:
//   - ArrayOfArrays<T> (glShaderSource's `string`): rebuilt into a temporary
//     std::vector<const T*> ("<param>Ptrs") just before the call, since ArrayOfArrays<T> only
//     stores owned std::vector<T> elements, never a live `const T* const*`. Its element-length
//     sibling (`length`) is resolved straight from the owner's .Lengths - it has no Command
//     member of its own (see array_of_arrays_metadata.py's consumed_pointer_params()).
//   - OPAQUE_BIT_PATTERN (glVertexAttribPointer's `pointer`, glDrawElements' `indices`): the
//     captured uintptr_t is reinterpret_cast back to its original pointer type, never
//     dereferenced.

#include "commandRunnersAuto.h"
#include "handleArgumentUpdaters.h"
#include "hdcVirtualizationService.h"
#include "layerAuto.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "uniformLocationMapService.h"

#include <cstddef>
#include <vector>

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
<%
  is_void = cmd.return_value.type.strip().lower() == 'void'  # WGL uses Win32 'VOID', GL uses 'void'.
  handle_params = [p for p in cmd.params if p.type_facts.is_mapped_handle]
  hdc_params = [p for p in cmd.params
                if p.type_facts.is_opaque_handle and p.type_facts.base_type == 'HDC']
  hdc_names = {p.name for p in hdc_params}
  is_handle_creating = (not is_void and cmd.return_value.type_facts.is_mapped_handle and
                         cmd.name in handle_creating_commands)
  return_obj_override = object_name_override_for(cmd.name, 'return_value', cmd.return_value.class_)
  is_object_name_creating = not is_void and return_obj_override is not None
  # LOOKUP/DELETE-role fields need their name(s) resolved before the call; CREATE-role fields
  # never do (an OUT-direction array field is default-constructed empty/zero-Value until the
  # real call writes into it, and a CREATE-role return value has no Key yet at all).
  resolve_object_name_params = [
      p for p in cmd.params
      if (object_name_override_for(cmd.name, p.name, p.class_) is not None and
          object_name_override_for(cmd.name, p.name, p.class_).role != ObjectNameRole.CREATE)
  ]
  register_object_name_array_params = [
      p for p in cmd.params
      if (object_name_override_for(cmd.name, p.name, p.class_) is not None and
          object_name_override_for(cmd.name, p.name, p.class_).role == ObjectNameRole.CREATE)
  ]
  # glShaderSource-shaped doubly-indirect params (see array_of_arrays_metadata.py): the real
  # driver call needs a live `const T* const*`, but ArrayOfArrays<T> only stores owned
  # std::vector<T> elements - so a temporary pointer array is rebuilt just before the call,
  # named "<param>Ptrs" below. Its own element-length sibling (e.g. `length`) is never in this
  # list - it has no member of its own to resolve (see consumed_pointer_params()) - and is
  # instead resolved straight from the owner's .Lengths in call_arg() below.
  array_of_arrays_params = [
      p for p in cmd.params if array_of_arrays_override_for(cmd.name, p.name) is not None
  ]

  # Any command with a GLint parameter literally named 'location' is one of the
  # glUniform*(location, ...) family (glUniform1f/.../4fv, glUniformMatrix*fv, ...) - a
  # structural rule rather than a per-command-name table, so it automatically covers every
  # such command that is or becomes enabled, present and future, with no table to keep in
  # sync. See UniformLocationMapService's own header comment for why the captured location
  # value cannot simply be replayed verbatim.
  def is_uniform_location_param(p):
      return p.name == 'location' and p.type_facts.base_type == 'GLint'

  # glProgramUniform*(program, location, ...) DSA commands also have a 'location' param, but
  # unlike classic glUniform*(), they resolve against their own explicit 'program' parameter
  # instead of whichever program happens to be glUseProgram-bound at replay time - see
  # UniformLocationMapService::ResolveForProgram's own comment. Also a structural rule (any
  # command with both an explicit 'program' parameter and a 'location' parameter), not a
  # per-command-name table, for the same reason as is_uniform_location_param above.
  def is_explicit_program_uniform_command():
      return any(p.name == 'program' and p.type_facts.base_type == 'GLuint' for p in cmd.params)

  def call_arg(p):
      if is_uniform_location_param(p):
          if is_explicit_program_uniform_command():
              # UniformLocationMapService keys by the program's stream-stable GITSKey
              # (command.m_program.Key), not its live replay-time handle - see that class's
              # own header comment for why keying by the live handle collides across
              # independent, non-sharing contexts (each share group's driver-assigned names
              # independently start from small integers, so two unrelated programs can both be
              # live handle 1 at the same time). Unrelated to the real call itself, which still
              # uses command.m_program.Value via this same function's own fallback branch below.
              return (f'UniformLocationMapService::Get().ResolveForProgram('
                      f'command.m_program.Key, command.m_{p.name}.Value)')
          return (f'UniformLocationMapService::Get().Resolve(CurrentContext(), '
                  f'command.m_{p.name}.Value)')
      if p.name in hdc_names:
          return f'real_{p.name}'
      owner = consumed_param_owner(cmd.name, p.name)
      if owner is not None:
          return f'command.m_{owner}.Lengths.data()'
      if array_of_arrays_override_for(cmd.name, p.name) is not None:
          # HasValue guard mirrors the ArrayArgument/HandleArrayArgument branches below - see
          # their own comment for why an empty-but-HasValue-false container's .data() must not
          # be forwarded to the driver unconditionally.
          return f'(command.m_{p.name}.HasValue ? {p.name}Ptrs.data() : nullptr)'
      ptr_override = pointer_override_for(cmd.name, p.name)
      if ptr_override is not None and ptr_override.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN:
          # Cast the captured bit pattern back to its original pointer type - see
          # docs/development/opengl2-rendering-pipeline-pointer-support-plan.md; never
          # dereferenced, so a reinterpret_cast of the raw uintptr_t is exactly right whether
          # the value is really a buffer-relative offset or (unsupported here) a client
          # -memory address.
          return f'reinterpret_cast<{p.type}>(command.m_{p.name}.Value)'
      override = object_name_override_for(cmd.name, p.name, p.class_)
      w = wrapper_for(p.type_facts, override, ptr_override, array_of_arrays_override_for(cmd.name, p.name))
      if w == 'ArrayArgument':
          # HasValue (not just .Data.data()) is the correct null check here: a
          # default-constructed/empty-but-HasValue-false vector's .data() is not guaranteed
          # nullptr by the standard, so forwarding it unconditionally can hand the driver a
          # non-null-but-non-dereferenceable pointer for a parameter whose captured value was
          # actually a null pointer (a legal call for some commands, e.g. wglCreateContextAttribsARB's
          # attribList - "use default attributes") - see the alignment plan doc's A.2 finding
          # and WglSetPixelFormatRunner::Run()'s hand-written precedent (commandRunnersCustom.cpp).
          return f'(command.m_{p.name}.HasValue ? command.m_{p.name}.Data.data() : nullptr)'
      if w == 'HandleArrayArgument':
          # Same HasValue guard as the ArrayArgument branch above, same rationale - see that
          # branch's own comment.
          return f'(command.m_{p.name}.HasValue ? command.m_{p.name}.Value.data() : nullptr)'
      return f'command.m_{p.name}.Value'

  call_args = ', '.join(call_arg(p) for p in cmd.params)
%>\
void ${cmd.name}Runner::Run() {
  auto& manager = PlayerManager::Get();
% for p in handle_params:
  UpdateHandle(command.m_${p.name});
% endfor
% for p in resolve_object_name_params:
<%
    resolve_override = object_name_override_for(cmd.name, p.name, p.class_)
    kind = resolve_override.kind.value
    # Structural (kind/role-driven, not command-name-driven) rule: any command that DELETEs a
    # PROGRAM name must purge UniformLocationMapService's own stale per-program location
    # entries for that same program identity before it can be reused by a later, unrelated
    # program - see that class's own PurgeProgram() header comment for why. Deliberately keyed
    # off (kind, role) rather than `cmd.name == 'glDeleteProgram'` so a future second command
    # sharing this same (kind, role) pair is automatically covered too.
    purges_uniform_locations = (resolve_override.kind == GLObjectKind.PROGRAM and
                                resolve_override.role == ObjectNameRole.DELETE)
%>\
% if p.type_facts.is_array_element_eligible:
  UpdateHandleArray(command.m_${p.name}, GLObjectKind::${kind});
% else:
  UpdateHandle(command.m_${p.name}, GLObjectKind::${kind});
% if purges_uniform_locations:
  // Keyed by the program's stream-stable GITSKey (.Key, untouched by UpdateHandle above),
  // not its live handle - see UniformLocationMapService's own header comment.
  UniformLocationMapService::Get().PurgeProgram(command.m_${p.name}.Key);
% endif
% endif
% endfor
% for p in hdc_params:
  HDC real_${p.name} = HdcVirtualizationService::Get().Resolve(command.m_${p.name}.Value);
% endfor
% for p in array_of_arrays_params:
  // Single, stable dummy element shared by every present-but-empty entry below (never read
  // through - the paired ${p.name}Ptrs[i]'s Lengths[i]/count is always 0 for such an entry, so
  // the real driver call has no reason to dereference it) - see the loop's own comment for why
  // Value[i].data() alone is not a sufficient substitute here.
  static const ${p.type_facts.base_type} k${p.name}DummyElement{};
  std::vector<const ${p.type_facts.base_type}*> ${p.name}Ptrs(command.m_${p.name}.Value.size());
  for (size_t i = 0; i < command.m_${p.name}.Value.size(); ++i) {
    // command.m_${p.name}.HasElement[i] (not just an empty-vs-nonempty Value[i] check) is the
    // correct presence test here - an originally-null string pointer and a real, explicitly
    // zero-length string both decode to an empty Value[i] - see arguments.h's
    // ArrayOfArrays<T>::HasElement comment. For a present (HasElement[i] == true) entry,
    // Value[i].data() itself is ALSO not sufficient when Value[i] happens to be empty (a real,
    // explicitly-zero-length string): std::vector::data() on an empty vector is not guaranteed
    // to be nullptr, but is not guaranteed to be a stable/dereferenceable non-null pointer
    // either (implementation-defined, and can even change between two empty vectors) - some
    // drivers treat string[i] == nullptr as "null-terminated" or otherwise special-case it
    // regardless of what length[i] says, which would silently reinterpret a genuinely
    // present-but-empty entry as absent. Falling back to the single stable dummy element
    // above for exactly that case guarantees a real, valid, non-null address every time,
    // still backed by zero declared length.
    if (!command.m_${p.name}.HasElement[i]) {
      ${p.name}Ptrs[i] = nullptr;
    } else if (command.m_${p.name}.Value[i].empty()) {
      ${p.name}Ptrs[i] = &k${p.name}DummyElement;
    } else {
      ${p.name}Ptrs[i] = command.m_${p.name}.Value[i].data();
    }
  }
% endfor
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  auto realFunction = player::g_DispatchTable.${cmd.name}.load(std::memory_order_acquire);
% if is_void:
  realFunction(${call_args});
% else:
  command.m_Return.Value = realFunction(${call_args});
% endif
% if is_handle_creating:
  UpdateOutputHandle(command.m_Return);
% elif is_object_name_creating:
  UpdateOutputHandle(command.m_Return, GLObjectKind::${return_obj_override.kind.value});
% endif
% for p in register_object_name_array_params:
  UpdateOutputHandleArray(command.m_${p.name}, GLObjectKind::${object_name_override_for(cmd.name, p.name, p.class_).kind.value});
% endfor
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

% endif
% endfor
} // namespace gits::opengl2
