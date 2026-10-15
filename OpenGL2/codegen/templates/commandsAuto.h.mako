// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Typed Command for every enabled command; see modern-opengl-backend-architecture.md, gate 5
// ("per-command support"). Mirrors Vulkan's commandsAuto.h.mako: every field is a
// TypeFacts-driven wrapper from arguments.h, never a raw member - a fixed-size scalar/opaque
// handle (TypeFacts.is_scalar / is_opaque_handle) is an Argument<T>, a handle GITS remaps
// (TypeFacts.is_mapped_handle - currently just HGLRC) is a HandleArgument<T>, a pointer
// with a pointer_metadata.py POINTER_PARAM_OVERRIDES entry (TypeFacts.is_array_element_eligible)
// is an ArrayArgument<T>, and a GL object name with an object_name_lifecycle.py
// OBJECT_NAME_PARAM_OVERRIDES entry is a HandleArgument<T>/HandleArrayArgument<T> instead (same
// scalar/array split as above, just remapped) - see wrapper_for()/pointer_override_for()/
// object_name_override_for() in generator_helpers.py. Actual serialization lives in
// commandCodersAuto.{h,cpp} - this file only declares storage. The default constructor exists
// solely so the decode side (commandCodersAuto.cpp's Decode()) can default-construct a command
// before filling in its fields field-by-field.

#pragma once

#include "argumentCoders.h"
#include "arguments.h"
#include "command.h"
#include "commandIdsAuto.h"
#include "glTypes.h"
#include "pixelTransferSize.h"

#include <cstdint>

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
<%
  is_void = cmd.return_value.type.strip().lower() == 'void'  # WGL uses Win32 'VOID', GL uses 'void'.
%>\
class ${cmd.name}Command : public Command {
public:
  ${cmd.name}Command() : Command(CommandId::ID_${cmd.name.upper()}) {}
  ${cmd.name}Command(uint32_t threadId\
% for p in cmd.params:
, ${p.type} ${p.name}\
% endfor
)
      : Command(CommandId::ID_${cmd.name.upper()}, threadId)\
% for p in cmd.params:
% if p.name not in consumed_pointer_params(cmd.name):
<%
  override = pointer_override_for(cmd, p.name)
  aoa_override = array_of_arrays_override_for(cmd.name, p.name)
  # 'const void *' blob params (glBufferData et al.) need an explicit cast to the
  # substituted element type (see array_element_cpp_type()) - void* has no implicit
  # conversion to uint8_t*, unlike every other element type this increment uses.
  arg_expr = (f'static_cast<const {array_element_cpp_type(p.type_facts.base_type)}*>({p.name})'
              if override is not None and p.type_facts.base_type == 'void'
              else p.name)
%>\
% if aoa_override is not None:
## Doubly-indirect (glShaderSource's `string`): the ArrayOfArrays<T> constructor itself reads
## both this parameter and its sibling element-length pointer (never given a member of its
## own - see array_of_arrays_metadata.py's consumed_pointer_params()). Passed as the signed
## sibling type verbatim (no cast here) - ArrayOfArrays<T>'s templated constructor clamps a
## negative count to zero via ClampCountToSizeT() before narrowing to size_t (see arguments.h's
## own comment on that helper) - a negative count is a GL client error the real driver call
## below must still receive unmodified, not something this constructor may crash turning into
## a huge allocation via a naive static_cast. element_length_param is None for
## glCreateShaderProgramv/glTransformFeedbackVaryings (no length-array sibling exists at all
## in gl.xml - see ArrayOfArraysOverride's own comment); pass a literal nullptr in that case so
## the constructor falls back to its own null-terminated-element scan.
, m_${p.name}(${p.name}, ${aoa_override.outer_count_param}, \
${aoa_override.element_length_param if aoa_override.element_length_param else 'nullptr'})\
% elif override is None:
, m_${p.name}(${p.name})\
% elif override.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN:
## Capture the pointer's own bit pattern verbatim, never its pointee - see that
## LengthSourceKind's own comment (pointer_metadata.py) and
## docs/development/opengl2-rendering-pipeline-pointer-support-plan.md.
, m_${p.name}(reinterpret_cast<uintptr_t>(${p.name}))\
% elif override.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY:
## Doubly-indirect pointer whose own elements are opaque bit patterns, never dereferenced
## (e.g. glMultiDrawElementsBaseVertex's `indices`) - see that LengthSourceKind's own comment
## (pointer_metadata.py). Passed straight through (no cast at this call site) to
## ArrayArgument<uintptr_t>'s dedicated `const void* const*` constructor overload
## (arguments.h), which converts each element with its own reinterpret_cast<uintptr_t>(...) -
## see that constructor's own comment for why this must be a per-element pointer-to-integer
## conversion rather than reinterpreting the whole array's pointer type and bulk-copying
## through it (a strict-aliasing violation this call site used to have).
, m_${p.name}(${p.name}, ${override.length_source_value})\
## OUT-direction: default-constructed, no eager read of not-yet-written app memory.
% elif override.direction == PointerDirection.OUT:
% elif override.length_source_kind == LengthSourceKind.NULL_TERMINATED:
, m_${p.name}(${arg_expr}, NullTerminated)\
% elif override.length_source_kind == LengthSourceKind.KEY_VALUE_LIST:
## See pointer_metadata.py's LengthSourceKind.KEY_VALUE_LIST/arguments.h's KeyValueListTag -
## a WGL {key, value, ..., 0} attribute list, terminated on a zero KEY, not just any zero
## element.
, m_${p.name}(${arg_expr}, KeyValueList)\
% elif override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE:
## COMPSIZE(format,type,width[,height[,depth]]) has no single sibling to read directly - see
## that LengthSourceKind's own comment (pointer_metadata.py) and pixelTransferSize.h. This
## branch is only ever reached for an IN-direction PIXEL_TRANSFER_SIZE param (the OUT-direction
## branch above already returns before this point) - i.e. glTexImage2D-shaped commands, which
## read GL_UNPACK_ALIGNMENT, never GL_PACK_ALIGNMENT. pixel_height_param is None for a 1D-only
## COMPSIZE(format,type,width) command (pass the literal 1 - ComputePixelTransferByteSize's own
## height parameter, not a sibling read); pixel_depth_param is None for the plain 2D shape
## (ComputePixelTransferByteSize's own `depth` parameter defaults to 1 - see that function's
## header comment - so it is simply omitted from the call rather than passed explicitly). Only
## when pixel_depth_param IS set (a genuine 3D transfer) do we also query and pass
## GL_UNPACK_IMAGE_HEIGHT/GL_UNPACK_SKIP_IMAGES - see ComputePixelTransferByteSize's own header
## comment for why a 3D transfer with non-default pixel-store state there
## gates capture off instead of under-counting it; a 2D-or-1D command never needs to query
## either (GL ignores both without a third dimension), so those two args are simply omitted
## (defaulting to 0, 0) rather than queried needlessly. GL_(UN)PACK_ROW_LENGTH/SKIP_ROWS/
## SKIP_PIXELS and this command's own name, by contrast (see
## ComputePixelTransferByteSize's own header comment for the full rationale), are ALWAYS
## queried and passed, for every dimensionality alike - those three affect every pixel
## transfer, not just a 3D one, so (unlike imageHeight/skipImages) there is no 2D-or-1D case
## that may skip them.
, m_${p.name}(${arg_expr}, ComputePixelTransferByteSize(\
${override.pixel_format_param}, ${override.pixel_type_param}, \
${override.pixel_width_param}, \
${override.pixel_height_param if override.pixel_height_param else '1'}, \
QueryUnpackAlignment(), \
${override.pixel_depth_param if override.pixel_depth_param else '1'}, \
% if override.pixel_depth_param:
QueryUnpackImageHeight(), QueryUnpackSkipImages(), \
% else:
0, 0, \
% endif
QueryUnpackRowLength(), QueryUnpackSkipRows(), QueryUnpackSkipPixels(), "${cmd.name}"\
))\
% elif override.length_source_kind == LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT:
## COMPSIZE(buffer) depends on the sibling enum's *value*, not just its presence - see that
## LengthSourceKind's own comment (pointer_metadata.py) and pixelTransferSize.h.
, m_${p.name}(${arg_expr}, ComputeClearBufferValueCount(${override.clear_buffer_param}))\
% elif override.length_source_kind == LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT:
## COMPSIZE(pname) depends on the sibling enum's *value* AND on which command family is
## calling (GL_TEXTURE_SWIZZLE_RGBA is legal only for the Tex- family - see that
## LengthSourceKind's own comment (pointer_metadata.py), TexOrSamplerParameterCommand's own
## comment, and pixelTransferSize.h's own comment). IN direction only - see
## _validate_pointer_override's own direction check for this kind.
, m_${p.name}(${arg_expr}, ComputeTexOrSamplerParameterValueCount(${override.pname_param}, \
TexOrSamplerParameterCommand::${'kTexParameter' if override.tex_sampler_family == TexOrSamplerParameterFamily.TEX_PARAMETER else 'kSamplerParameter'}))\
% elif override.length_source_kind == LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT:
## COMPSIZE(pname) depends on the sibling enum's *value*, not just its presence - see that
## LengthSourceKind's own comment (pointer_metadata.py) and pixelTransferSize.h. IN direction
## only - see _validate_pointer_override's own direction check for this kind.
, m_${p.name}(${arg_expr}, ComputePatchParameterValueCount(${override.pname_param}))\
% else:
## Passed as the sibling's own type verbatim, times length_multiplier as a plain int
## (no unsigned 'u' suffix) - no cast needed here at all: ArrayArgument<T>/
## HandleArrayArgument<T>'s constructor is templated on SizeT (mirrors Vulkan's
## BufferArgument - see arguments.h's own comment) and does the static_cast<size_t> itself,
## sidestepping both MSVC's C2398 brace-init narrowing check and the uint32_t-narrowing bug
## a hand-written static_cast<uint32_t> here would reintroduce for a 64-bit sibling like
## glBufferData/glBufferSubData's `size` (GLsizeiptr).
, m_${p.name}(${arg_expr}, ${override.length_source_value} * ${override.length_multiplier})\
% endif
% endif
% endfor
 {}

% for p in cmd.params:
% if p.name not in consumed_pointer_params(cmd.name):
<%
  member_ptr_override = pointer_override_for(cmd, p.name)
  member_aoa_override = array_of_arrays_override_for(cmd.name, p.name)
%>\
  ${wrapper_for(p.type_facts, object_name_override_for(cmd.name, p.name, p.class_), member_ptr_override, member_aoa_override)}<${member_cpp_type(p, member_ptr_override, member_aoa_override)}> m_${p.name};
% if pointer_needs_raw_offset_capture(member_ptr_override):
  // See pointer_needs_raw_offset_capture()'s own header comment
  // (generator_helpers.py). Populated unconditionally with reinterpret_cast<uintptr_t>(the
  // ORIGINAL, unguarded ${p.name}) by recorderWrappersAuto.cpp.mako, independently of whatever
  // m_${p.name} itself captured (or didn't, if a buffer object guard suppressed it) - so the
  // player can still reconstruct the exact buffer-relative offset the app passed, instead of
  // always falling back to nullptr/offset-0, whenever m_${p.name}.HasValue is false. Never
  // dereferenced on either side, exactly like OPAQUE_BIT_PATTERN's own m_<param> field.
  Argument<uintptr_t> m_${p.name}RawPointer;
% endif
% if pointer_needs_pixel_transfer_unknown_flag(member_ptr_override):
  // See pointer_needs_pixel_transfer_unknown_flag()'s own
  // header comment (generator_helpers.py) and PixelTransferSizeIsUnknown()'s own header
  // comment (pixelTransferSize.h). Populated unconditionally by
  // recorderWrappersAuto.cpp.mako, independently of whatever m_${p.name} itself captured, so
  // the player can tell "this call genuinely transfers zero real bytes" (false - the real
  // driver call is safe, m_${p.name}.HasValue's empty-but-true Data buffer notwithstanding)
  // apart from "this call's real byte range could not be determined at capture time" (true -
  // commandRunnersAuto.cpp.mako's Run() skips the real driver call entirely for this command
  // when true, rather than risk it against a buffer whose captured size does not match what
  // the driver would actually read/write).
  Argument<bool> m_${p.name}SizeUnknown;
  // See pointer_needs_pixel_transfer_unknown_
  // flag()'s own header comment (generator_helpers.py) and UnpackPixelsPointerIsBufferOffset()/
  // PackPixelsPointerIsBufferOffset()'s own header comment (clientMemoryCapture.h). Populated
  // unconditionally by recorderWrappersAuto.cpp.mako, independently of m_${p.name}SizeUnknown
  // above: true iff this call's ${p.name} was actually a buffer-relative offset (a pixel
  // (un)pack buffer object was bound), in which case commandRunnersAuto.cpp.mako's Run() must
  // proceed with the real driver call - using m_${p.name}RawPointer's preserved offset - even
  // when m_${p.name}SizeUnknown is also true, since a bound buffer needs no captured size at
  // all and SizeUnknown alone cannot tell "buffer-offset, size irrelevant" apart from "client
  // memory, size genuinely could not be determined".
  Argument<bool> m_${p.name}IsBufferOffset;
% endif
% endif
% endfor
% if not is_void:
  ${wrapper_for(cmd.return_value.type_facts, object_name_override_for(cmd.name, 'return_value', cmd.return_value.class_))}<${cmd.return_value.type}> m_Return;
% endif
};

% endif
% endfor
} // namespace gits::opengl2
