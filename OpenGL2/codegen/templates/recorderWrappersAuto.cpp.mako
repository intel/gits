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
// the return value and any handle keys are resolved, and the command is handed to
// CaptureManager. Disabled commands just report themselves once (per canonical name) as
// unsupported.
//
// The recursion guard already distinguishes the outermost call (depth one) from calls
// nested underneath it, even though there is no layer work yet that could recurse into GL
// itself - recording/reporting only happens at depth one so a future nested call (e.g.
// from a layer) will not re-record or re-warn.
//
// handle_creating_commands / handle_param_to_erase_on_success come from
// codegen/handle_lifecycle.py - see that module for why this is a small hand-maintained
// table rather than something derived from parameter types alone. pointer_override_for()
// comes from codegen/pointer_metadata.py, same rationale for pointer direction/length.
// object_name_override_for() comes from codegen/object_name_lifecycle.py, which classifies GL
// object-name (GLuint) fields the same way handle_lifecycle.py does for HGLRC - see
// docs/development/opengl2-object-name-mapping-plan.md.

#include "captureManager.h"
#include "commandSerializersAuto.h"
#include "contextHandleMapService.h"
#include "handleArgumentUpdaters.h"
#include "mapTrackingWrappers.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "recorderWrappers.h"
#include "recursionGuard.h"
#include "shareGroupService.h"
#include "unsupportedCommand.h"
#include "unsupportedCommandTypes.h"

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
    in_override = pointer_override_for(cmd.name, p.name)
    is_pixel_transfer_in = (in_override is not None and
                             in_override.direction == PointerDirection.IN and
                             in_override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE)
%>\
% if is_pixel_transfer_in:
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
    override = pointer_override_for(cmd.name, p.name)
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
    # not be read from directly in that case.
    is_pixel_transfer_out = override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE
    if p.type_facts.base_type == 'void' and is_pixel_transfer_out:
        capture_arg_expr = (f'static_cast<{array_element_cpp_type(p.type_facts.base_type)}*>('
                            f'const_cast<void*>(GuardPackPixelsPointer("{cmd.name}", {p.name})))')
    elif p.type_facts.base_type == 'void':
        capture_arg_expr = f'static_cast<{array_element_cpp_type(p.type_facts.base_type)}*>({p.name})'
    else:
        capture_arg_expr = p.name
%>\
% if override.length_source_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE:
## COMPSIZE(format,type,width,height) has no single sibling to read directly - mirrors
## commandsAuto.h.mako's own IN-direction PIXEL_TRANSFER_SIZE branch (glTexImage2D), just
## computed after the call instead of before it (see pixelTransferSize.h). This whole block is
## only reached for an OUT-direction override (guarded above) - i.e. glReadPixels-shaped
## commands, which read GL_PACK_ALIGNMENT, never GL_UNPACK_ALIGNMENT.
    command->m_${p.name}.Capture(${capture_arg_expr}, ComputePixelTransferByteSize(\
${override.pixel_format_param}, ${override.pixel_type_param}, \
${override.pixel_width_param}, ${override.pixel_height_param}, QueryPackAlignment()));
% elif override.length_source_kind == LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT:
## COMPSIZE(buffer) depends on the sibling enum's *value* - mirrors commandsAuto.h.mako's own
## IN-direction CLEAR_BUFFER_VALUE_COUNT branch; no enabled command needs this combination yet,
## kept for symmetry with the PIXEL_TRANSFER_SIZE branch above.
    command->m_${p.name}.Capture(${capture_arg_expr}, ComputeClearBufferValueCount(${override.clear_buffer_param}));
% else:
## Passed as the sibling's own type verbatim (no cast here) - ArrayArgument<T>/
## HandleArrayArgument<T>::Capture() is templated on SizeT and does the static_cast<size_t>
## itself (mirrors Vulkan's BufferArgument - see arguments.h's own comment and
## commandsAuto.h.mako's matching IN-direction branch), so a 64-bit sibling like
## glBufferSubData's `size` (GLsizeiptr) is captured correctly instead of narrowed here.
    command->m_${p.name}.Capture(${capture_arg_expr}, ${override.length_source_value});
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
    CaptureManager::Get().RecordCommand(*command, new ${cmd.name}CommandSerializer(*command));
% if cmd.name in frame_end_commands:
    // See codegen/handle_lifecycle.py's FRAME_END_COMMANDS and
    // common/layer_interface/unsupportedCommandTypes.h's FrameEndCommand - only emit the
    // marker on a successful swap, mirroring the erase-on-success pattern used elsewhere in
    // this same function; an unsuccessful swap did not actually present anything and should
    // not advance the shared player infrastructure's frame count.
% if is_void:
    {
      FrameEndCommand frameEndCommand(static_cast<uint32_t>(GetCurrentThreadId()));
      frameEndCommand.m_Key = CaptureManager::Get().CreateCommandKey();
      CaptureManager::Get().RecordCommand(frameEndCommand, new FrameEndSerializer(frameEndCommand));
    }
% else:
    if (result) {
      FrameEndCommand frameEndCommand(static_cast<uint32_t>(GetCurrentThreadId()));
      frameEndCommand.m_Key = CaptureManager::Get().CreateCommandKey();
      CaptureManager::Get().RecordCommand(frameEndCommand, new FrameEndSerializer(frameEndCommand));
    }
% endif
% endif
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
