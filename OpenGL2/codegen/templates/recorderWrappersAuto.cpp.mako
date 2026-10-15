// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Recorder-owned wrapper for every command the interceptor forwards to (see
// codegen/generator_recorder.py). For an enabled command, the typed Command object is
// constructed *before* the real driver call (this copies any IN-direction array/string bytes
// out of application memory immediately before the call - see arguments.h's ArrayArgument<T>
// and modern-opengl-backend-architecture.md's "Classify payloads from evidence" decision);
// the real driver is then always called, unconditionally, matching the app's expectations
// regardless of recording state; any OUT-direction array is captured immediately afterward,
// once the driver has actually written into the app's buffer; then the command is keyed,
// the return value and any handle keys are resolved, and the command is handed off to the
// Post layer chain for encoding. Disabled commands just report themselves once (per
// canonical name) as unsupported.
//
// The recursion guard already distinguishes the outermost call (depth one) from calls
// nested underneath it, so a nested call from within a Pre/Post layer will not re-record
// or re-warn.
//
// Pre/Post layers (see docs/development/opengl2-layer-architecture-alignment-plan.md):
// CaptureManager::GetPreLayers() runs immediately before the real driver call, once every
// input handle/object-name field has been resolved to its GITSKey (matching Vulkan's
// wrappersAuto.cpp.mako's own Pre-layer placement); CaptureManager::GetPostLayers() runs once
// output handle/object-name fields have been registered and command.m_Return is set -
// CaptureManager::LoadLayers() registers exactly one Post layer, EncoderLayer, as the last
// (and, today, only) entry in that chain, so this loop is where encoding onto the wire
// actually happens (see encoderLayerAuto.cpp.mako's EncoderLayer::Post), matching Vulkan's own
// Post-layer-does-the-encoding architecture instead of calling CaptureManager::RecordCommand()
// directly and inline here.
//
// handle_creating_commands / handle_param_to_erase_on_success come from
// codegen/handle_lifecycle.py - see that module for why this is a small hand-maintained
// table rather than something derived from parameter types alone. pointer_override_for()
// comes from codegen/pointer_metadata.py, same rationale for pointer direction/length.
// object_name_override_for() comes from codegen/object_name_lifecycle.py, which classifies GL
// object-name (GLuint) fields the same way handle_lifecycle.py does for HGLRC - see
// docs/development/opengl2-object-name-mapping-plan.md.

#include "captureManager.h"
#include "clientMemoryCapture.h"
#include "commandSerializersAuto.h"
#include "contextHandleMapService.h"
#include "handleArgumentUpdaters.h"
#include "layerAuto.h"
#include "mapTrackingWrappers.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "recorderWrappers.h"
#include "recursionGuard.h"
#include "shareGroupService.h"
#include "unsupportedCommand.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

// Hand-declared instead of including <windows.h>, which would redeclare PROC/tagRECT/etc.
// against glTypes.h's own self-contained declarations of those same names (see
// glTypes.h's header comment). DWORD is `unsigned long` there too.
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace gits::opengl2 {

namespace {

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID', GL commands use 'void'.
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
    erase_param = handle_param_to_erase_on_success.get(cmd.name)
%>\
${ret} APIENTRY Wrapper_${cmd.name}(${param_decls}) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.${cmd.name}.load(std::memory_order_acquire);
  // Reserved here - before the real driver dispatch below - rather than deferred into
  // CaptureManager::RecordCommand/RecordUnsupportedCommand (as it used to be), so concurrent
  // top-level calls on different threads get ordering keys assigned in DISPATCH order, matching
  // OrderingRecorder's own ordering guarantee, instead of driver-call-COMPLETION order (a later-
  // dispatched call that happens to return first would otherwise get a smaller/earlier key than
  // an earlier-dispatched call still in flight, letting replay reorder them). Mirrors Vulkan's
  // wrappersAuto.cpp.mako, which reserves its own key immediately before dispatch for the same
  // reason. Applies uniformly to the enabled-command path below (where it is copied into
  // `command->m_Key` before the CommandSerializer that encodes it is ever constructed - see
  // CaptureManager::RecordCommand's own header comment on why that ordering matters) and the
  // disabled/unsupported-command path (passed straight to ReportUnsupportedCommand). Named
  // `orderingKey`, not `key`, to avoid colliding with the handful of real GL/WGL parameters
  // literally named `key`/`key0`/`key1` (e.g. glAcquireKeyedMutexWin32EXT).
  const uint64_t orderingKey = record ? CaptureManager::Get().CreateCommandKey() : 0;
% if cmd.enabled:
  // Constructed before the real call so IN-direction array/string members are eager-copied
  // out of application memory immediately before the call (see arguments.h's ArrayArgument<T>
  // and modern-opengl-backend-architecture.md's "Classify payloads from evidence"); OUT-
  // direction array members stay default-constructed (empty) until Capture() runs below, once
  // the driver has actually written into the app's buffer. std::optional defers construction
  // to the `record` branch, matching prior behavior of only building a Command when recording.
  std::optional<${cmd.name}Command> command;
  if (record) {
    command.emplace(static_cast<uint32_t>(GetCurrentThreadId())\
% for p in cmd.params:
<%
    in_override = pointer_override_for(cmd, p.name)
    is_pixel_transfer_in = (in_override is not None and
                             in_override.direction == PointerDirection.IN and
                             in_override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE)
    # unpack_pbo_guard is a SIBLING_PARAM-shaped (or any other non-
    # PIXEL_TRANSFER_SIZE) IN-direction parameter with the identical client-pointer-or-PBO-
    # offset duality as the PIXEL_TRANSFER_SIZE case above (glCompressedTexImage*/
    # glCompressedTexSubImage*'s `data` - see pointer_metadata.py's own field comment) - route
    # it through the exact same GuardUnpackPixelsPointer() call rather than reinventing a
    # second guard mechanism. _validate_pointer_override already forbids setting both
    # unpack_pbo_guard and PIXEL_TRANSFER_SIZE on the same override, so these two conditions
    # are mutually exclusive in practice; written as two independent checks anyway (rather than
    # one combined boolean) so each stays independently readable at this call site.
    is_unpack_pbo_guard_in = in_override is not None and in_override.unpack_pbo_guard
%>\
% if is_pixel_transfer_in or is_unpack_pbo_guard_in:
, static_cast<${p.type}>(GuardUnpackPixelsPointer("${cmd.name}", ${p.name}))\
% else:
, ${p.name}\
% endif
% endfor
);
    // See this function's own header comment: reserved before the real driver dispatch above,
    // not here - `orderingKey` was already minted before `command` was even constructed.
    // Assigning it into the Command object now, rather than after the real call, still happens
    // well before the CommandSerializer that encodes `command->m_Key` onto the wire is
    // constructed further below.
    command->m_Key = orderingKey;
% for p in cmd.params:
<%
    raw_offset_in_override = pointer_override_for(cmd, p.name)
%>\
% if pointer_needs_raw_offset_capture(raw_offset_in_override) and raw_offset_in_override.direction == PointerDirection.IN:
    // See pointer_needs_raw_offset_capture()'s own header comment
    // (generator_helpers.py). ${p.name} here is still this wrapper's own, ORIGINAL,
    // never-guarded parameter - the emplace() call above only ever guards the copy it passes
    // into the constructor, never this local - so this always captures the real bit pattern
    // regardless of whether the guard above suppressed the byte-array capture for it.
    command->m_${p.name}RawPointer.Value = reinterpret_cast<uintptr_t>(${p.name});
% endif
% if pointer_needs_pixel_transfer_unknown_flag(raw_offset_in_override) and raw_offset_in_override.direction == PointerDirection.IN:
<%
    pixel_transfer_args_in = (
        f'{raw_offset_in_override.pixel_format_param}, {raw_offset_in_override.pixel_type_param}, '
        f'{raw_offset_in_override.pixel_width_param}, '
        f'{raw_offset_in_override.pixel_height_param if raw_offset_in_override.pixel_height_param else "1"}, '
        f'QueryUnpackAlignment(), '
        f'{raw_offset_in_override.pixel_depth_param if raw_offset_in_override.pixel_depth_param else "1"}, '
        + ('QueryUnpackImageHeight(), QueryUnpackSkipImages(), '
           if raw_offset_in_override.pixel_depth_param else '0, 0, ') +
        f'QueryUnpackRowLength(), QueryUnpackSkipRows(), QueryUnpackSkipPixels(), "{cmd.name}"'
    )
%>\
    // See pointer_needs_pixel_transfer_unknown_flag()'s
    // own header comment (generator_helpers.py) and PixelTransferSizeIsUnknown()'s own header
    // comment (pixelTransferSize.h). Uses the exact same argument list as the m_${p.name}
    // member's own ComputePixelTransferByteSize() call in commandsAuto.h.mako's constructor
    // (same GL_UNPACK_* pixel-store state, queried again here rather than threaded through -
    // none of it can have changed since the constructor ran a few lines above, and every
    // warning either call might log is warn-once per commandName regardless), so the two can
    // never disagree about whether this call's captured size is genuine-zero or unknown.
    command->m_${p.name}SizeUnknown.Value = PixelTransferSizeIsUnknown(${pixel_transfer_args_in});
    // See
    // UnpackPixelsPointerIsBufferOffset()'s own header comment (clientMemoryCapture.h). Uses
    // ${p.name} here too - this wrapper's own, ORIGINAL, never-guarded parameter, the same one
    // m_${p.name}RawPointer.Value above captures - never whatever the emplace() call's own
    // GuardUnpackPixelsPointer() invocation returned, so this always reflects the real guard
    // decision for this exact call regardless of what the constructor's captured Data ended
    // up holding.
    command->m_${p.name}IsBufferOffset.Value = UnpackPixelsPointerIsBufferOffset(${p.name});
% endif
% endfor
% for p in cmd.params:
% if p.type_facts.is_mapped_handle:
    UpdateHandle(command->m_${p.name});
% endif
<%
    obj_override = object_name_override_for(cmd.name, p.name, p.class_)
%>\
## CREATE never applies to a parameter (only object_name_lifecycle.py return-value entries use
## it) - a LOOKUP or DELETE-role field must resolve its key before the real call, both so the
## delete-on-success step below has a key to delete and so the field has something to serialize
## (HandleArgument<T>/HandleArrayArgument<T> only ever put .Key on the wire, never .Value).
% if obj_override is not None and obj_override.role != ObjectNameRole.CREATE:
% if p.type_facts.is_array_element_eligible:
    UpdateHandleArray(command->m_${p.name}, GLObjectKind::${obj_override.kind.value});
% else:
    UpdateHandle(command->m_${p.name}, GLObjectKind::${obj_override.kind.value});
% endif
% endif
% endfor
    for (Layer* layer : CaptureManager::Get().GetPreLayers()) {
      layer->Pre(*command);
    }
  }
% endif
% if is_void:
  realFunction(${param_names});
% else:
  // Not const: `ret` may already be a pointer/qualified type (e.g. "const GLubyte*"), and
  // blindly prepending const either double-qualifies it or - for a plain pointer return
  // type - turns "T*" into "const T*", changing the pointee's constness instead of the
  // pointer's, which then fails to convert back to `ret` at the return statement below.
  ${ret} result = realFunction(${param_names});
% endif
  if (record) {
% if cmd.enabled:
% for p in cmd.params:
<%
    override = pointer_override_for(cmd, p.name)
%>\
% if override is not None and override.direction == PointerDirection.OUT:
    // Read only now that the driver has actually written into the app's buffer.
<%
    # 'void *' OUT params (glReadPixels et al.) need an explicit cast to the substituted
    # element type (see array_element_cpp_type()) - void* has no implicit conversion to
    # uint8_t*, mirroring commandsAuto.h.mako's IN-direction arg_expr cast above. A
    # PIXEL_TRANSFER_SIZE OUT param (glReadPixels' `pixels`) additionally routes through
    # GuardPackPixelsPointer first - see that function's own header comment: `pixels` is a
    # buffer-relative offset, not a client pointer, when a pixel-pack buffer is bound, and must
    # not be read from directly in that case. A query_buffer_guard OUT param
    # (glGetQueryObjecti64v/ui64v/uiv's `params`) has the identical buffer-relative-offset-vs-
    # real-pointer duality, but for GL_QUERY_BUFFER rather than GL_PIXEL_PACK_BUFFER, and for a
    # non-`void*`-typed pointer (GLint64*/GLuint64*/GLuint*, never blob-shaped) - so it casts
    # back to this param's own declared pointer type (`p.type` already includes the `*`)
    # rather than array_element_cpp_type()'s void-substitution helper, which only applies to a
    # genuine `void*`/`const void*` field.
    is_pixel_transfer_out = override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE
    is_query_buffer_guard_out = override.query_buffer_guard
    if p.type_facts.base_type == 'void' and is_pixel_transfer_out:
        capture_arg_expr = (f'static_cast<{array_element_cpp_type(p.type_facts.base_type)}*>('
                            f'const_cast<void*>(GuardPackPixelsPointer("{cmd.name}", {p.name})))')
    elif is_query_buffer_guard_out:
        capture_arg_expr = (f'static_cast<{p.type}>('
                            f'const_cast<void*>(GuardQueryBufferPointer("{cmd.name}", {p.name})))')
    elif p.type_facts.base_type == 'void':
        capture_arg_expr = f'static_cast<{array_element_cpp_type(p.type_facts.base_type)}*>({p.name})'
    else:
        capture_arg_expr = p.name
%>\
% if override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE:
## COMPSIZE(format,type,width[,height[,depth]]) has no single sibling to read directly -
## mirrors commandsAuto.h.mako's own IN-direction PIXEL_TRANSFER_SIZE branch (glTexImage2D),
## just computed after the call instead of before it (see pixelTransferSize.h). This whole
## block is only reached for an OUT-direction override (guarded above) - i.e. glReadPixels-
## shaped commands, which read GL_PACK_ALIGNMENT, never GL_UNPACK_ALIGNMENT. No enabled
## OUT-direction command has a depth dimension today (glReadPixels is 2D-only), but
## pixel_height_param/pixel_depth_param (and, for symmetry with the IN-direction branch above,
## GL_PACK_IMAGE_HEIGHT/GL_PACK_SKIP_IMAGES querying - see ComputePixelTransferByteSize's own
## header comment) are handled the same optional way as the IN-direction branch
## above, in case a future OUT-direction 3D transfer is ever enabled. GL_PACK_ROW_LENGTH/
## SKIP_ROWS/SKIP_PIXELS and this command's own name, by contrast (see
## ComputePixelTransferByteSize's own header comment for the full rationale), are ALWAYS
## queried and passed here too, mirroring the IN-direction branch's own unconditional query of
## the GL_UNPACK_* counterparts.
<%
    pixel_transfer_args_out = (
        f'{override.pixel_format_param}, {override.pixel_type_param}, '
        f'{override.pixel_width_param}, '
        f'{override.pixel_height_param if override.pixel_height_param else "1"}, '
        f'QueryPackAlignment(), '
        f'{override.pixel_depth_param if override.pixel_depth_param else "1"}, '
        + ('QueryPackImageHeight(), QueryPackSkipImages(), '
           if override.pixel_depth_param else '0, 0, ') +
        f'QueryPackRowLength(), QueryPackSkipRows(), QueryPackSkipPixels(), "{cmd.name}"'
    )
%>\
    command->m_${p.name}.Capture(${capture_arg_expr}, ComputePixelTransferByteSize(${pixel_transfer_args_out}));
    // See pointer_needs_pixel_transfer_unknown_flag()'s
    // own header comment (generator_helpers.py) and PixelTransferSizeIsUnknown()'s own header
    // comment (pixelTransferSize.h). Same argument list as the Capture() call immediately
    // above, so the two can never disagree about whether this call's captured size is
    // genuine-zero or unknown; GL_PACK_* pixel-store state cannot have changed between the two
    // calls (no intervening GL call), and every warning either might log is warn-once per
    // commandName regardless, so this second call never double-logs.
    command->m_${p.name}SizeUnknown.Value = PixelTransferSizeIsUnknown(${pixel_transfer_args_out});
    // See PackPixelsPointerIsBufferOffset()'s
    // own header comment (clientMemoryCapture.h). Uses ${p.name} here too - this wrapper's
    // own, ORIGINAL, never-guarded parameter, the same one m_${p.name}RawPointer.Value below
    // captures - never whatever capture_arg_expr's own GuardPackPixelsPointer() invocation
    // returned, so this always reflects the real guard decision for this exact call
    // regardless of what the Capture() call above ended up storing.
    command->m_${p.name}IsBufferOffset.Value = PackPixelsPointerIsBufferOffset(${p.name});
% elif override.length_source_kind == LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT:
## COMPSIZE(buffer) depends on the sibling enum's *value* - mirrors commandsAuto.h.mako's own
## IN-direction CLEAR_BUFFER_VALUE_COUNT branch; no enabled command needs this combination yet,
## kept for symmetry with the PIXEL_TRANSFER_SIZE branch above.
    command->m_${p.name}.Capture(${capture_arg_expr}, ComputeClearBufferValueCount(${override.clear_buffer_param}));
% elif override.length_source_kind == LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT:
## COMPSIZE(pname) depends on the sibling enum's *value* - see that LengthSourceKind's own
## comment (pointer_metadata.py) and pixelTransferSize.h. OUT direction only
## (glGetVertexAttribiv is a getter; _validate_pointer_override's own direction check already
## enforces this), computed after the call like every other OUT-direction branch here, not
## before it like TEX_SAMPLER_PARAMETER_VALUE_COUNT/PATCH_PARAMETER_VALUE_COUNT's own IN-only
## commandsAuto.h.mako branch.
    command->m_${p.name}.Capture(${capture_arg_expr}, ComputeVertexAttribParameterValueCount(${override.pname_param}));
% else:
## Passed as the sibling's own type verbatim (no cast here) - ArrayArgument<T>/
## HandleArrayArgument<T>::Capture() is templated on SizeT and does the static_cast<size_t>
## itself (mirrors Vulkan's BufferArgument - see arguments.h's own comment and
## commandsAuto.h.mako's matching IN-direction branch), so a 64-bit sibling like
## glBufferSubData's `size` (GLsizeiptr) is captured correctly instead of narrowed here.
    command->m_${p.name}.Capture(${capture_arg_expr}, ${override.length_source_value});
% endif
% if pointer_needs_raw_offset_capture(override):
    // See pointer_needs_raw_offset_capture()'s own header comment
    // (generator_helpers.py). ${p.name} here is still this wrapper's own, ORIGINAL,
    // never-guarded parameter (the guard, if any, only ever affects capture_arg_expr above) -
    // so this always captures the real bit pattern regardless of whether a bound buffer
    // object caused the capture above to be suppressed for it.
    command->m_${p.name}RawPointer.Value = reinterpret_cast<uintptr_t>(${p.name});
% endif
<%
    out_obj_override = object_name_override_for(cmd.name, p.name, p.class_)
%>\
% if out_obj_override is not None:
    // CREATE role: the driver just wrote brand-new names into this buffer (see this array
    // param's OBJECT_NAME_PARAM_OVERRIDES entry) - create one key per element and register it.
    UpdateOutputHandleArray(command->m_${p.name}, GLObjectKind::${out_obj_override.kind.value});
% endif
% endif
% endfor
% if not is_void:
    command->m_Return.Value = result;
<%
    return_obj_override = object_name_override_for(cmd.name, 'return_value', cmd.return_value.class_)
%>\
% if cmd.return_value.type_facts.is_mapped_handle:
% if cmd.name in handle_creating_commands:
    UpdateOutputHandle(command->m_Return);
<%
    share_param = context_share_param.get(cmd.name)
%>\
    // See common/services/shareGroupService.h's own header comment and
    // codegen/handle_lifecycle.py's CONTEXT_SHARE_PARAM for why a newly-created context must
    // join its share context's group (if any) right away, using the real, recorder-process
    // HGLRC values (ShareGroupService is process-local, unserialized bookkeeping - see that
    // class's header comment).
    ShareGroupService::Get().RegisterNewContext(
        command->m_Return.Value, ${f'command->m_{share_param}.Value' if share_param is not None else 'nullptr'});
% else:
    UpdateHandle(command->m_Return);
% endif
% elif return_obj_override is not None:
    // return_obj_override.role is always CREATE - see intermediates_creator.py's
    // _validate_object_name_override (a return value can never be a LOOKUP/DELETE target).
    UpdateOutputHandle(command->m_Return, GLObjectKind::${return_obj_override.kind.value});
% endif
% endif
% if erase_param and is_void:
    ContextHandleMapService::Get().Erase(command->m_${erase_param}.Value);
% elif erase_param:
    if (result) {
      ContextHandleMapService::Get().Erase(command->m_${erase_param}.Value);
    }
% endif
<%
    share_lists_params = share_lists_commands.get(cmd.name)
%>\
% if share_lists_params is not None:
    // See common/services/shareGroupService.h's own header comment and
    // codegen/handle_lifecycle.py's SHARE_LISTS_COMMANDS - only merge on success, matching
    // wglShareLists' own documented failure semantics (no sharing takes effect).
    if (result) {
      ShareGroupService::Get().MergeGroups(command->m_${share_lists_params[0]}.Value,
                                            command->m_${share_lists_params[1]}.Value);
    }
% endif
<%
    delete_object_name_params = [
        p for p in cmd.params
        if (object_name_override_for(cmd.name, p.name, p.class_) is not None and
            object_name_override_for(cmd.name, p.name, p.class_).role == ObjectNameRole.DELETE)
    ]
%>\
% for p in delete_object_name_params:
<%
    delete_kind = object_name_override_for(cmd.name, p.name, p.class_).kind.value
%>\
% if is_void:
% if p.type_facts.is_array_element_eligible:
    for (GLuint name : command->m_${p.name}.Value) {
      ObjectNameMapService::Get().Delete(GLObjectKind::${delete_kind},
                                          GroupIdentityFor(GLObjectKind::${delete_kind}), name);
    }
% else:
    ObjectNameMapService::Get().Delete(GLObjectKind::${delete_kind},
                                        GroupIdentityFor(GLObjectKind::${delete_kind}),
                                        command->m_${p.name}.Value);
% endif
% else:
    if (result) {
% if p.type_facts.is_array_element_eligible:
      for (GLuint name : command->m_${p.name}.Value) {
        ObjectNameMapService::Get().Delete(GLObjectKind::${delete_kind},
                                            GroupIdentityFor(GLObjectKind::${delete_kind}), name);
      }
% else:
      ObjectNameMapService::Get().Delete(GLObjectKind::${delete_kind},
                                          GroupIdentityFor(GLObjectKind::${delete_kind}),
                                          command->m_${p.name}.Value);
% endif
    }
% endif
% endfor
    // See docs/development/opengl2-layer-architecture-alignment-plan.md: EncoderLayer - the
    // sole, last-registered Post layer (see captureManager.cpp's LoadLayers()) - does what
    // this loop used to do directly right here: hand the command to
    // CaptureManager::RecordCommand() for encoding onto the wire (and, for a
    // FRAME_END_COMMANDS entry like this one might be, additionally emit the FrameEndCommand
    // marker afterward - see encoderLayerAuto.cpp.mako's EncoderLayer::Post).
    for (Layer* layer : CaptureManager::Get().GetPostLayers()) {
      layer->Post(*command);
    }
% else:
    ReportUnsupportedCommand("${cmd.name}", orderingKey);
% endif
  }
% if not is_void:
  return result;
% endif
}

% endfor
} // namespace

void* GetGeneratedRecorderWrapper(const char* name) {
  // Checked first so glMapBufferRange/glUnmapBuffer never reach their own generated entries
  // below - see docs/development/opengl2-buffer-map-tracking-plan.md's Decision 3.
  if (void* custom = GetCustomRecorderWrapper(name)) {
    return custom;
  }
  static const std::unordered_map<std::string, void*> nameToWrapper = {
% for cmd in commands:
    {"${cmd.name}", reinterpret_cast<void*>(&Wrapper_${cmd.name})},
% endfor
  };
  auto it = nameToWrapper.find(name);
  if (it == nameToWrapper.end()) {
    return nullptr;
  }
  return it->second;
}

} // namespace gits::opengl2
