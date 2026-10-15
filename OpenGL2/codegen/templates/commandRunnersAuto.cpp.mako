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
//   - Argument<HDC>: HdcVirtualizationService::Resolve() substitutes a player-owned window's
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
// Mirrors Vulkan's commandRunnersAuto.cpp.mako shape. Pre/Post layers ARE wired via the exact same
// inline `for (Layer* layer : manager.GetPreLayers()) { layer->Pre(command); }` loop shape Vulkan/
// DX12 use (and, on the OpenGL2 side, the same shape every hand-written runner in
// commandRunnersCustom.cpp also repeats inline at the equivalent point in its own Run()):
// PlayerManager::GetPreLayers() runs immediately before the real
// driver call, once every LOOKUP/DELETE-role handle/object-name field has been resolved to its
// live, replay-time value (matching Vulkan's commandRunnersAuto.cpp.mako's own Pre-layer
// placement); PlayerManager::GetPostLayers() runs at the very end of Run(), once every CREATE-role
// field has been registered. The trace layer registers for Pre when enabled; Post remains empty.
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
//   - OPAQUE_BIT_PATTERN_ARRAY (glMultiDrawElementsBaseVertex's `indices`): the multi-draw
//     counterpart of OPAQUE_BIT_PATTERN above - a doubly-indirect `const void* const*` whose
//     own elements are opaque bit patterns, rebuilt into a temporary
//     std::vector<const void*> ("<param>Ptrs") just before the call, same as ArrayOfArrays<T>
//     above but one level shallower (each element is reinterpret_cast directly, never copied
//     byte-by-byte through a per-element length).
//
// PIXEL_TRANSFER_SIZE (glTexImage2D's/glReadPixels'
// `pixels`): checked BEFORE the real driver call runs, not just used as captured. Its
// companion m_<param>SizeUnknown member (see pointer_needs_pixel_transfer_unknown_flag()'s own
// header comment, generator_helpers.py) distinguishes "this call genuinely transfers zero real
// bytes" from "this call's real byte range could not be determined at capture time" - the real
// driver call is skipped entirely (with a degraded-fidelity warning) whenever the latter,
// since an empty-but-HasValue-true captured Data buffer would otherwise stand in for a buffer
// the driver actually reads/writes a real, nonzero range through at replay time.

#include "commandRunnersAuto.h"
#include "bindlessHandleMapService.h"
#include "discardedOutputElements.h"
#include "handleArgumentUpdaters.h"
#include "hdcVirtualizationService.h"
#include "layerAuto.h"
#include "log.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "shareGroupService.h"
#include "threadContextService.h"
#include "uniformLocationMapService.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace gits::opengl2 {

namespace {

// Destination for a DISCARDED_OUTPUT query result (pointer_metadata.py), which is never
// recorded. `listElements` is the live length of a list result, queried from the replay driver
// (discardedOutputElements.h), the recorded capacity of a capacity-sized result, or 0. Other
// results have a fixed count of at most 16 values (pointer_length_derivation.py's
// _DISCARDED_OUTPUT_LENS); the minimum capacity is far above it. `Slot` is the parameter index,
// so outputs of one call never share a buffer.
constexpr size_t DISCARDED_OUTPUT_MIN_ELEMENTS = 4096;

template <typename T, size_t Slot>
T* DiscardedOutputScratch(size_t listElements) {
  thread_local std::vector<T> scratch(DISCARDED_OUTPUT_MIN_ELEMENTS);
  if (scratch.size() < listElements) {
    scratch.resize(listElements);
  }
  return scratch.data();
}

} // namespace

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
  # Deleting a program pipeline unbinds it, so once the real call ran, UniformLocationMapService
  # no longer resolves classic glUniform*() calls against its active program.
  deleted_pipeline_params = [
      p for p in resolve_object_name_params
      if (object_name_override_for(cmd.name, p.name, p.class_).kind ==
              GLObjectKind.PROGRAM_PIPELINE and
          object_name_override_for(cmd.name, p.name, p.class_).role == ObjectNameRole.DELETE)
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

  # glMultiDrawElementsBaseVertex-shaped doubly-indirect params whose ELEMENTS are opaque bit
  # patterns, never dereferenced (see pointer_metadata.py's
  # LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY): the real driver call needs a live
  # `const void* const*`, but ArrayArgument<uintptr_t> only stores a std::vector<uintptr_t> of
  # raw values - so a temporary pointer array is rebuilt just before the call, named
  # "<param>Ptrs" below, exactly like array_of_arrays_params above but one level shallower
  # (each element is reinterpreted directly, never copied byte-by-byte).
  opaque_bit_pattern_array_params = [
      p for p in cmd.params
      if (pointer_override_for(cmd, p.name) is not None and
          pointer_override_for(cmd, p.name).length_source_kind ==
              LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY)
  ]

  # Every PIXEL_TRANSFER_SIZE-shaped parameter (glTexImage2D-
  # /glReadPixels-shaped) this command has, i.e. every parameter with its own companion
  # m_<param>SizeUnknown member (see pointer_needs_pixel_transfer_unknown_flag()'s own header
  # comment, generator_helpers.py, and commandsAuto.h.mako's matching member-declaration
  # branch). Checked immediately below, before the real driver call: an
  # empty-but-HasValue-true captured Data buffer for one of these params does NOT mean "this
  # call transfers zero real bytes" unless m_<param>SizeUnknown.Value is false - see
  # PixelTransferSizeIsUnknown()'s own header comment (pixelTransferSize.h) for the full
  # rationale. In practice at most one such parameter exists per enabled command today, but
  # this is written to cover every one of them uniformly rather than assume that.
  pixel_transfer_size_params = [
      p for p in cmd.params
      if (pointer_override_for(cmd, p.name) is not None and
          pointer_override_for(cmd, p.name).length_source_kind ==
              LengthSourceKind.PIXEL_TRANSFER_SIZE)
  ]
  # Only glTexImage*-shaped commands accept a null client pointer, which allocates storage
  # without data. Every other pixel transfer reads or writes through the pointer.
  null_pixels_allocate = cmd.name.startswith(('glTexImage', 'glTextureImage', 'glMultiTexImage'))

  # Any command with a scalar GLint parameter literally named 'location' is one of the
  # glUniform*(location, ...) family (glUniform1f/.../4fv, glUniformMatrix*fv, ...) - a
  # structural rule rather than a per-command-name table, so it automatically covers every
  # such command that is or becomes enabled, present and future, with no table to keep in
  # sync. See UniformLocationMapService's own header comment for why the captured location
  # value cannot simply be replayed verbatim. A GLint* 'location' is an output
  # (glGetTransformFeedbackVaryingNV, glGetShadingRateSampleLocationivNV), not a uniform.
  def is_uniform_location_param(p):
      return (p.name == 'location' and p.type_facts.base_type == 'GLint'
              and p.type_facts.is_scalar)

  # glProgramUniform*(program, location, ...) DSA commands also have a 'location' param, but
  # unlike classic glUniform*(), they resolve against their own explicit program parameter
  # instead of whichever program happens to be glUseProgram-bound at replay time - see
  # UniformLocationMapService::ResolveForProgram's own comment. Also a structural rule (any
  # command with both a Program-kind object-name parameter and a 'location' parameter), not a
  # per-command-name table, for the same reason as is_uniform_location_param above. The
  # parameter is `program` in core and `programObj` in GL_ARB_shader_objects.
  # Bindless texture and image handles are assigned by the driver - see
  # BindlessHandleMapService. A glGet*Handle* command returns one; a scalar GLuint64 `handle`,
  # or the `value` of a glUniformHandleui64* or glProgramUniformHandleui64* setter, passes one.
  returns_bindless_handle = (not is_void and 'Handle' in cmd.name and
                             cmd.return_value.type_facts.base_type == 'GLuint64')

  def is_bindless_handle_param(p):
      return (p.type_facts.base_type == 'GLuint64' and p.type_facts.is_scalar and
              (p.name == 'handle' or (p.name == 'value' and 'UniformHandleui64' in cmd.name)))

  def explicit_program_param():
      for p in cmd.params:
          override = object_name_override_for(cmd.name, p.name, p.class_)
          if override is not None and override.kind == GLObjectKind.PROGRAM:
              return p
      return None

  def call_arg(p):
      if is_uniform_location_param(p):
          program_param = explicit_program_param()
          if program_param is not None:
              # UniformLocationMapService keys by the program's stream-stable GITSKey
              # (command.m_<program>.Key), not its live replay-time handle - see that class's
              # own header comment for why keying by the live handle collides across
              # independent, non-sharing contexts (each share group's driver-assigned names
              # independently start from small integers, so two unrelated programs can both be
              # live handle 1 at the same time). Unrelated to the real call itself, which still
              # uses command.m_<program>.Value via this same function's own fallback branch below.
              return (f'UniformLocationMapService::Get().ResolveForProgram('
                      f'command.m_{program_param.name}.Key, command.m_{p.name}.Value)')
          return (f'UniformLocationMapService::Get().Resolve(CurrentContext(), '
                  f'command.m_{p.name}.Value)')
      if is_bindless_handle_param(p):
          return f'BindlessHandleMapService::Get().Resolve(command.m_{p.name}.Value)'
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
      ptr_override = pointer_override_for(cmd, p.name)
      if ptr_override is not None and ptr_override.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY:
          # Same HasValue guard as the ArrayArgument branch below, same rationale - see that
          # branch's own comment. ${p.name}Ptrs is rebuilt from the captured bit patterns just
          # above this function's own call site - see opaque_bit_pattern_array_params.
          return f'(command.m_{p.name}.HasValue ? {p.name}Ptrs.data() : nullptr)'
      if ptr_override is not None and ptr_override.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN:
          # Cast the captured bit pattern back to its original pointer type - see
          # docs/development/opengl2-rendering-pipeline-pointer-support-plan.md; never
          # dereferenced, so a reinterpret_cast of the raw uintptr_t is exactly right whether
          # the value is really a buffer-relative offset or (unsupported here) a client
          # -memory address.
          return f'reinterpret_cast<{p.type}>(command.m_{p.name}.Value)'
      if ptr_override is not None and ptr_override.length_source_kind == LengthSourceKind.DISCARDED_OUTPUT:
          slot = next(i for i, q in enumerate(cmd.params) if q.name == p.name)
          element = p.type[:p.type.rindex('*')].strip() if p.type_facts.pointer_depth == 2 else p.type_facts.base_type
          scratch = f'DiscardedOutputScratch<{element}, {slot}>'
          if ptr_override.length_source_value is not None:
              return (f'{scratch}(ClampCountToSizeT(command.m_{ptr_override.length_source_value}.Value) * '
                      f'{ptr_override.length_multiplier})')
          sizer = discarded_output_list_sizer(cmd.name)
          if sizer is None:
              return f'{scratch}(0)'
          sizer_args = ', '.join(f'command.m_{name}.Value' for name in sizer[1])
          return f'{scratch}({sizer[0]}({sizer_args}))'
      override = object_name_override_for(cmd.name, p.name, p.class_)
      w = wrapper_for(p.type_facts, override, ptr_override, array_of_arrays_override_for(cmd.name, p.name))
      if w == 'ArrayArgument':
          # HasValue (not just .Data.data()) is the correct null check here: a
          # default-constructed/empty-but-HasValue-false vector's .data() is not guaranteed
          # nullptr by the standard, so forwarding it unconditionally can hand the driver a
          # non-null-but-non-dereferenceable pointer for a parameter whose captured value was
          # actually a null pointer (a legal call for some commands, e.g. wglCreateContextAttribsARB's
          # attribList - "use default attributes") - see
          # WglSetPixelFormatRunner::Run()'s hand-written precedent (commandRunnersCustom.cpp).
          #
          # For a pointer_needs_raw_offset_capture() parameter (glTexImage2D-
          # shaped/glCompressedTexImage2D-shaped/glGetQueryObject*-shaped - see that predicate's
          # own header comment, generator_helpers.py), a HasValue-false Command was NOT
          # necessarily a genuine null pointer at capture time - it may equally be a real,
          # non-null buffer-relative offset that GuardUnpackPixelsPointer/GuardPackPixelsPointer/
          # GuardQueryBufferPointer deliberately declined to dereference. Falling back to a bare
          # nullptr here (offset 0) in that case would silently replay the call against the
          # WRONG offset within whichever buffer object ends up bound at replay time. Falling
          # back to the companion m_<param>RawPointer field instead reconstructs the app's own
          # original bit pattern verbatim - correct whether it turns out to be a genuine null
          # pointer (RawPointer.Value == 0, identical to the plain nullptr fallback below) or a
          # real offset (RawPointer.Value == that offset, exactly reproducing the original call).
          if pointer_needs_raw_offset_capture(ptr_override):
              return (f'(command.m_{p.name}.HasValue ? command.m_{p.name}.Data.data() : '
                      f'reinterpret_cast<{p.type}>(command.m_{p.name}RawPointer.Value))')
          return f'(command.m_{p.name}.HasValue ? command.m_{p.name}.Data.data() : nullptr)'
      if w == 'HandleArrayArgument':
          # Same HasValue guard as the ArrayArgument branch above, same rationale - see that
          # branch's own comment.
          return f'(command.m_{p.name}.HasValue ? command.m_{p.name}.Value.data() : nullptr)'
      return f'command.m_{p.name}.Value'

  call_args = ', '.join(call_arg(p) for p in cmd.params)
  call_indent = ' ' * 6 if pixel_transfer_size_params else ' ' * 4
%>\
void ${cmd.name}Runner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
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
    kind_expr = (f'ImageObjectKind(command.m_{resolve_override.target_param}.Value)'
                 if resolve_override.target_param else f'GLObjectKind::{kind}')
%>\
% if p.type_facts.is_array_element_eligible:
  if (!UpdateHandleArray(command.m_${p.name}, GLObjectKind::${kind})) {
    SkipUnmappedObjectNameCall(command, "${cmd.name}");
  }
% elif resolve_override.role == ObjectNameRole.LOOKUP_OR_CREATE:
  // See ObjectNameRole.LOOKUP_OR_CREATE's own comment - a never-seen key here is the recorder
  // having minted one right at this same Bind call (see the recorder-side ConfirmImplicitCreate),
  // not a stream gap, so it mints a fresh replay-time name, through this bind's own family's
  // Gen command, instead of skipping the call.
  if (!UpdateHandleOrCreate(command.m_${p.name}, GLObjectKind::${kind},
                            player::g_DispatchTable.${resolve_override.gen_command}.load(std::memory_order_acquire),
                            player::g_DispatchTable.${resolve_override.is_command}.load(std::memory_order_acquire))) {
    SkipUnmappedObjectNameCall(command, "${cmd.name}");
  }
% else:
  if (!UpdateHandle(command.m_${p.name}, ${kind_expr})) {
    SkipUnmappedObjectNameCall(command, "${cmd.name}");
  }
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
% if returns_bindless_handle:
  const GLuint64 capturedHandle = command.m_Return.Value;
% endif
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
% for p in cmd.params:
% if (cmd.name, p.name) in TEXT_OUTPUT_BUFFERS:
  if (command.m_${p.name}.HasValue) {
    command.m_${p.name}.Data.resize(std::max<size_t>(1, ClampCountToSizeT(command.m_${pointer_override_for(cmd, p.name).length_source_value}.Value)));
  }
% endif
% endfor
% for p in array_of_arrays_params:
  // Built AFTER the Pre-layer loop above, not before: a Pre layer that edits Value or resizes
  // the outer array must be reflected in ${p.name}Ptrs, and editing a std::vector element
  // in-place can reallocate its backing storage - building this array any earlier risks
  // handing the real driver call a stale or dangling pointer.
  //
  // Single, stable dummy element shared by every empty-AND-zero-length entry below (never read
  // through - see the Lengths[i] == 0 gate just below for why that combination, specifically,
  // can never be dereferenced by the real driver call). Value[i].empty() alone is NOT a safe
  // substitution trigger on its own - ArrayOfArrays<T>'s own constructor
  // (arguments.h) preserves Lengths[i] as whatever explicit length (or -1, "null-terminated")
  // the app actually passed for element i even when v[i] itself was nullptr (Value[i] stays
  // empty in that case purely because there was nothing to copy FROM, not because element i's
  // declared length was itself zero) - so an empty Value[i] can legitimately pair with a
  // positive Lengths[i], or with -1, not just with 0. Substituting the one-element dummy
  // unconditionally for every empty Value[i] regardless of Lengths[i] would, for either of
  // those two non-zero cases, hand the real driver call a genuinely non-null-but-only-
  // 1-element-long address paired with a length that tells it to read far more than that one
  // element - an out-of-bounds read.
  //
  // Lengths[i] == 0 is therefore the correct and only safe trigger for the dummy substitution:
  // it is the one case where Value[i].empty() is guaranteed to mean "zero bytes will ever be
  // read through this pointer" (whether element i was really a null pointer or a genuine,
  // explicitly-zero-length string - GL draws no behavioral line between those two, see
  // ArrayOfArrays<T>'s own constructor comment) - std::vector::data() on an empty vector is
  // not guaranteed to be nullptr, but is also not guaranteed to be a stable/dereferenceable
  // non-null pointer either (implementation-defined, can even change between two empty
  // vectors), and some drivers treat string[i] == nullptr as "null-terminated" or otherwise
  // special-case it regardless of what length[i] says - so falling back to the single stable
  // dummy element above (backed by a real, valid, non-null address, and itself zero-
  // initialized, so a driver that scans it for a null terminator anyway immediately finds one
  // right there) resolves that one specific hazard, for that one specific case, with no need
  // to distinguish why Value[i] is empty.
  //
  // Every other empty-Value[i] case (Lengths[i] > 0, or Lengths[i] == -1) preserves a real
  // nullptr instead - exactly reproducing the original call's own semantics for a source
  // pointer that really was null (the only way Value[i] can end up empty with a non-zero
  // Lengths[i] - see ArrayOfArrays<T>'s own constructor comment above), rather than
  // substituting a synthetic non-null address the paired non-zero/sentinel length would then
  // over-read from.
  static const ${p.type_facts.base_type} k${p.name}DummyElement{};
  std::vector<const ${p.type_facts.base_type}*> ${p.name}Ptrs(command.m_${p.name}.Value.size());
  for (size_t i = 0; i < command.m_${p.name}.Value.size(); ++i) {
    if (!command.m_${p.name}.Value[i].empty()) {
      ${p.name}Ptrs[i] = command.m_${p.name}.Value[i].data();
    } else if (command.m_${p.name}.Lengths[i] == 0) {
      ${p.name}Ptrs[i] = &k${p.name}DummyElement;
    } else {
      ${p.name}Ptrs[i] = nullptr;
    }
  }
% endfor
% for p in opaque_bit_pattern_array_params:
  // Built AFTER the Pre-layer loop above, same rationale as array_of_arrays_params above.
  //
  // Rebuild a live pointer array from the captured bit patterns - see
  // pointer_metadata.py's LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY. Each element is
  // reinterpreted back to its original pointer type, exactly like the single-pointer
  // OPAQUE_BIT_PATTERN case below, never dereferenced - it may be a genuine client-memory
  // address or (the only form any real 3.2+ core-profile caller uses) a buffer-relative byte
  // offset.
  std::vector<const void*> ${p.name}Ptrs(command.m_${p.name}.Data.size());
  for (size_t i = 0; i < command.m_${p.name}.Data.size(); ++i) {
    ${p.name}Ptrs[i] = reinterpret_cast<const void*>(command.m_${p.name}.Data[i]);
  }
% endfor
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.${cmd.name}.load(std::memory_order_acquire);
% if pixel_transfer_size_params:
    // An empty-but-HasValue-true captured Data buffer here does not mean this call safely
    // transfers zero real bytes - only m_<param>SizeUnknown.Value == false means that. Skip the
    // real driver call entirely (never partially - there is no way to safely call with only
    // SOME of a multi-parameter command's pointers replaced by a correctly-sized buffer) rather
    // than risk it against a buffer whose captured size does not match what the driver would
    // actually read/write, mirroring UnsupportedCommandRunner::Run()'s own warn-once, degraded-
    // fidelity wording/convention.
    //
    // m_<param>SizeUnknown.Value == true does NOT
    // always mean "must skip" - m_<param>IsBufferOffset.Value == true (this exact call's pointer
    // was actually a buffer-relative offset, per UnpackPixelsPointerIsBufferOffset()/
    // PackPixelsPointerIsBufferOffset()'s own header comment, clientMemoryCapture.h) means the
    // real driver call needs no captured size at all - call_arg() above already falls back to
    // this same parameter's m_<param>RawPointer.Value (the preserved offset) whenever
    // m_<param>.HasValue is false, so the call below is safe and correct to make. The two flags
    // are orthogonal (SizeUnknown reasons about format/type/dimensions/pixel-store state only;
    // IsBufferOffset reasons about buffer bindings only), so each parameter's own contribution to
    // this skip gate is `SizeUnknown && !IsBufferOffset` - buffer-offset mode always takes
    // priority over an otherwise-unknown size, never the other way around.
% if null_pixels_allocate:
    // A null client pointer only allocates storage, so its size never matters either.
% endif
    const bool ${cmd.name}PixelTransferSizeUnknown = \
% for p in pixel_transfer_size_params:
% if not loop.first:
  || \
% endif
% if null_pixels_allocate:
    (command.m_${p.name}SizeUnknown.Value && !command.m_${p.name}IsBufferOffset.Value && command.m_${p.name}RawPointer.Value != 0)\
% else:
    (command.m_${p.name}SizeUnknown.Value && !command.m_${p.name}IsBufferOffset.Value)\
% endif
% endfor
;
    if (${cmd.name}PixelTransferSizeUnknown) {
      static bool alreadyWarned = false;
      if (!alreadyWarned) {
        alreadyWarned = true;
        LOG_WARNING << "Replay skipped ${cmd.name} call (degraded fidelity): this call's "
                       "pixel-transfer byte size could not be determined at record time "
                       "(non-default pixel-store state, an unrecognized format/type "
                       "combination, or an overflowing size computation), so no captured buffer "
                       "can safely stand in for the real, unknown-sized transfer this call "
                       "would otherwise perform.";
      }
    } else {
% endif
% if has_return_member(cmd):
${call_indent}command.m_Return.Value = realFunction(${call_args});
% else:
${call_indent}realFunction(${call_args});
% endif
% if pixel_transfer_size_params:
    }
% endif
% if is_handle_creating:
    UpdateOutputHandle(command.m_Return);
% elif returns_bindless_handle:
    BindlessHandleMapService::Get().Register(capturedHandle, command.m_Return.Value);
% elif is_object_name_creating:
    UpdateOutputHandle(command.m_Return, GLObjectKind::${return_obj_override.kind.value});
% endif
% for p in register_object_name_array_params:
    UpdateOutputHandleArray(command.m_${p.name}, GLObjectKind::${object_name_override_for(cmd.name, p.name, p.class_).kind.value});
% endfor
% for p in deleted_pipeline_params:
    for (GITSKey key : command.m_${p.name}.Key) {
      UniformLocationMapService::Get().DeleteProgramPipeline(key);
    }
% endfor
% if cmd.name in share_lists_commands:
    if (command.m_Return.Value) {
      ShareGroupService::Get().MergeGroups(command.m_${share_lists_commands[cmd.name][0]}.Value,
                                            command.m_${share_lists_commands[cmd.name][1]}.Value);
    }
% endif
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

% endif
% endfor
} // namespace gits::opengl2
