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
  override = pointer_override_for(cmd.name, p.name)
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
## a huge allocation via a naive static_cast.
, m_${p.name}(${p.name}, ${aoa_override.outer_count_param}, ${aoa_override.element_length_param})\
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
## (pointer_metadata.py). Reinterpret the incoming `const void* const*` as `const uintptr_t*`
## (both are arrays of pointer-sized elements) and reuse ArrayArgument<uintptr_t>'s existing
## SizeT-templated constructor directly, exactly like every plain SIBLING_PARAM array above -
## no new capture mechanism needed, just a different element type than this pointer's own
## declared one.
, m_${p.name}(reinterpret_cast<const uintptr_t*>(${p.name}), ${override.length_source_value})\
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
## COMPSIZE(format,type,width,height) has no single sibling to read directly - see that
## LengthSourceKind's own comment (pointer_metadata.py) and pixelTransferSize.h. This branch is
## only ever reached for an IN-direction PIXEL_TRANSFER_SIZE param (the OUT-direction branch
## above already returns before this point) - i.e. glTexImage2D-shaped commands, which read
## GL_UNPACK_ALIGNMENT, never GL_PACK_ALIGNMENT.
, m_${p.name}(${arg_expr}, ComputePixelTransferByteSize(\
${override.pixel_format_param}, ${override.pixel_type_param}, \
${override.pixel_width_param}, ${override.pixel_height_param}, QueryUnpackAlignment()))\
% elif override.length_source_kind == LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT:
## COMPSIZE(buffer) depends on the sibling enum's *value*, not just its presence - see that
## LengthSourceKind's own comment (pointer_metadata.py) and pixelTransferSize.h.
, m_${p.name}(${arg_expr}, ComputeClearBufferValueCount(${override.clear_buffer_param}))\
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
  member_ptr_override = pointer_override_for(cmd.name, p.name)
  member_aoa_override = array_of_arrays_override_for(cmd.name, p.name)
%>\
  ${wrapper_for(p.type_facts, object_name_override_for(cmd.name, p.name, p.class_), member_ptr_override, member_aoa_override)}<${member_cpp_type(p, member_ptr_override, member_aoa_override)}> m_${p.name};
% endif
% endfor
% if not is_void:
  ${wrapper_for(cmd.return_value.type_facts, object_name_override_for(cmd.name, 'return_value', cmd.return_value.class_))}<${cmd.return_value.type}> m_Return;
% endif
};

% endif
% endfor
} // namespace gits::opengl2
