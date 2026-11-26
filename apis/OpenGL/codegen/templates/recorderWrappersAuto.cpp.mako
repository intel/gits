// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
## Without these arguments the template renders every command as one file, as the tests do.
<%page args="part_index=0, part_count=1, command_count=None"/>\
${AUTO_GENERATED_HEADER}

// Recorder-owned wrapper for every command the interceptor forwards to (see
// codegen/generator_recorder.py). For an enabled command, the typed Command object is
// constructed *before* the real driver call (this copies any IN-direction array/string bytes
// out of application memory immediately before the call - see arguments.h's ArrayArgument<T>
// and modern-opengl-backend-architecture.md's "Classify payloads from evidence" decision).
// Once its input handles are resolved and its OUT-direction arrays hold the application's
// pointers, the Pre layers run (CaptureManager::GetPreLayers(), as in Vulkan's
// wrappersAuto.cpp.mako). The driver is then called with the application's own parameters,
// unless a Pre layer set command.m_Skip and answered the command itself. OUT-direction arrays are
// captured once written, the return value and output handles are resolved, and the Post layers
// run, with EncoderLayer last (see CaptureLayerManager::LoadLayers()). The application gets
// command.m_Return, which a Post layer may have replaced. Disabled commands just report
// themselves as unsupported.
//
// The recursion guard already distinguishes the outermost call (depth one) from calls
// nested underneath it, so a nested call from within a Pre/Post layer will not re-record
// or re-warn, and runs no layers.
//
// handle_creating_commands / handle_param_to_erase_on_success come from
// codegen/handle_lifecycle.py - a single table already unioned across every mapped-handle type
// (HGLRC, GLsync, ...; see that module's own header comment for why this is a small hand-
// maintained table rather than something derived from parameter types alone). Which
// HandleMapService<T> alias applies to a given mapped-handle field's erase-on-success path
// (ContextHandleMapService vs SyncHandleMapService) is looked up here, generically, from the
// field's own type_facts.base_type via handle_lifecycle.py's MAPPED_HANDLE_SERVICE_NAMES -
// never hardcoded per command or per type name. pointer_override_for()
// comes from codegen/pointer_metadata.py, same rationale for pointer direction/length.
// object_name_override_for() comes from codegen/object_name_lifecycle.py, which classifies GL
// object-name (GLuint) fields the same way handle_lifecycle.py does for HGLRC - see
// docs/development/opengl-object-name-mapping-plan.md.
//
// The wrappers are split into several files, each with a part of the commands, so that they
// compile in parallel. Each part adds its wrappers to the lookup of GetGeneratedRecorderWrapper(),
// which part 0 defines.

#include "bufferShadowService.h"
#include "captureManager.h"
#include "clientArraysMetaTypes.h"
#include "clientArraysService.h"
#include "clientMemoryCapture.h"
#include "glBufferQueries.h"
#include "glEnumsAuto.h"
#include "commandSerializersAuto.h"
#include "handleArgumentUpdaters.h"
#include "handleMapService.h"
#include "layerAuto.h"
#include "log.h"
#include "mapTrackingService.h"
#include "mapTrackingWrappers.h"
#include "objectNameMapService.h"
#include "glDispatchAuto.h"
#include "recorderWrappers.h"
#include "recursionGuard.h"
#include "shareGroupService.h"
#include "systemApi.h"
#include "unsupportedCommand.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

<%def name="command_arguments(cmd)">\
% for p in cmd.params:
<%
    in_override = pointer_override_for(cmd, p.name)
    in_kind = in_override.length_source_kind if in_override is not None else None
    # An unpack_pbo_guard parameter (glCompressedTexImage*'s `data`) is a client pointer or an
    # unpack buffer offset, like an IN-direction pixel transfer.
    is_unpack_pixels_in = in_override is not None and (
        in_override.unpack_pbo_guard or
        (in_override.direction == PointerDirection.IN and
         in_kind == LengthSourceKind.PIXEL_TRANSFER_SIZE))
%>\
% if is_unpack_pixels_in:
, static_cast<${p.type}>(GuardUnpackPixelsPointer("${cmd.name}", ${p.name}))\
% elif in_kind == LengthSourceKind.INDIRECT_COMMANDS_BYTE_SIZE:
, GuardDrawIndirectPointer(${p.name})\
% elif in_kind == LengthSourceKind.ELEMENT_INDICES_BYTE_SIZE:
, GuardElementArrayPointer(${p.name})\
% else:
, ${p.name}\
% endif
% endfor
</%def>\
## Gives a Pre layer that sets m_Skip the application's OUT buffers to answer through.
<%def name="out_pointers(cmd, indent)">\
% for p in cmd.params:
<%
    target_override = pointer_override_for(cmd, p.name)
    target_wrapper = (wrapper_for(p.type_facts, object_name_override_for(cmd.name, p.name, p.class_),
                                  target_override, array_of_arrays_override_for(cmd.name, p.name))
                      if target_override is not None and p.name not in consumed_pointer_params(cmd.name)
                      else None)
%>\
% if target_wrapper == 'ArrayArgument' and target_override.direction == PointerDirection.OUT:
${indent}command.m_${p.name}.Pointer = static_cast<${member_cpp_type(p, target_override)}*>(${p.name});
% endif
% endfor
</%def>\
## What the guards in command_arguments() hide of an IN pointer: its own bit pattern, which is a
## buffer offset while a buffer is bound, and for a pixel transfer whether its size is known and
## whether it is a buffer offset.
<%def name="input_pointer_facts(cmd, indent)">\
% for p in cmd.params:
<%
    raw_offset_in_override = pointer_override_for(cmd, p.name)
%>\
% if pointer_needs_raw_offset_capture(raw_offset_in_override) and raw_offset_in_override.direction == PointerDirection.IN:
${indent}// See pointer_needs_raw_offset_capture()'s own header comment
${indent}// (generator_helpers.py). ${p.name} here is still this wrapper's own, ORIGINAL,
${indent}// never-guarded parameter - the constructor call above only ever guards the copy it passes
${indent}// into the constructor, never this local - so this always captures the real bit pattern
${indent}// regardless of whether the guard above suppressed the byte-array capture for it.
${indent}command.m_${p.name}RawPointer.Value = reinterpret_cast<uintptr_t>(${p.name});
% endif
% if pointer_needs_pixel_transfer_unknown_flag(raw_offset_in_override) and raw_offset_in_override.direction == PointerDirection.IN:
<%
    pixel_transfer_args_in = pixel_transfer_size_args(raw_offset_in_override, cmd.name, 'Unpack')
%>\
${indent}// See pointer_needs_pixel_transfer_unknown_flag()'s
${indent}// own header comment (generator_helpers.py) and PixelTransferSizeIsUnknown()'s own header
${indent}// comment (pixelTransferSize.h). Uses the exact same argument list as the m_${p.name}
${indent}// member's own ComputePixelTransferByteSize() call in commandsAuto.h.mako's constructor
${indent}// (same GL_UNPACK_* pixel-store state, queried again here rather than threaded through -
${indent}// none of it can have changed since the constructor ran a few lines above, and every
${indent}// warning either call might log is warn-once per commandName regardless), so the two can
${indent}// never disagree about whether this call's captured size is genuine-zero or unknown.
${indent}command.m_${p.name}SizeUnknown.Value = PixelTransferSizeIsUnknown(${pixel_transfer_args_in});
${indent}// See
${indent}// UnpackPixelsPointerIsBufferOffset()'s own header comment (clientMemoryCapture.h). Uses
${indent}// ${p.name} here too - this wrapper's own, ORIGINAL, never-guarded parameter, the same one
${indent}// m_${p.name}RawPointer.Value above captures - never whatever the constructor call's own
${indent}// GuardUnpackPixelsPointer() invocation returned, so this always reflects the real guard
${indent}// decision for this exact call regardless of what the constructor's captured Data ended
${indent}// up holding.
${indent}command.m_${p.name}IsBufferOffset.Value = UnpackPixelsPointerIsBufferOffset(${p.name});
% endif
% endfor
</%def>\
## The defs below keep the keys of GL objects and handles, the share groups and the handle
## lifecycles. They are process-local and never touch the stream, so they run whether or not
## recording is enabled.
##
## Before the call: CREATE never applies to a parameter (only object_name_lifecycle.py
## return-value entries use it) - a LOOKUP or DELETE-role field must resolve its key before the
## real call, both so the delete-on-success step has a key to delete and so the field has
## something to serialize (HandleArgument<T>/HandleArrayArgument<T> only ever put .Key on the
## wire, never .Value).
<%def name="input_keys(cmd, indent)">\
% for p in cmd.params:
% if p.type_facts.is_mapped_handle:
${indent}UpdateHandle(command.m_${p.name});
% endif
<%
    obj_override = object_name_override_for(cmd.name, p.name, p.class_)
%>\
% if obj_override is not None and obj_override.role != ObjectNameRole.CREATE:
% if p.type_facts.is_array_element_eligible and obj_override.role == ObjectNameRole.DELETE:
${indent}UpdateDeletedHandleArray(command.m_${p.name}, GLObjectKind::${obj_override.kind.value});
% elif p.type_facts.is_array_element_eligible:
${indent}UpdateHandleArray(command.m_${p.name}, GLObjectKind::${obj_override.kind.value});
% elif obj_override.role == ObjectNameRole.LOOKUP_OR_CREATE:
${indent}// See ObjectNameRole.LOOKUP_OR_CREATE's own comment - a miss here may be the real driver
${indent}// about to implicitly create a new object; ConfirmImplicitCreate decides after the call.
${indent}UpdateHandleIfKnown(command.m_${p.name}, GLObjectKind::${obj_override.kind.value});
% elif is_existence_query(cmd.name, cmd.return_value.type, obj_override):
${indent}UpdateHandleIfRegistered(command.m_${p.name}, GLObjectKind::${obj_override.kind.value});
% elif obj_override.target_param is not None:
${indent}UpdateHandle(command.m_${p.name}, ImageObjectKind(${obj_override.target_param}));
% elif obj_override.identifier_param is not None:
${indent}if (ObjectIdentifierMapsNames(${obj_override.identifier_param})) {
${indent}  UpdateHandle(command.m_${p.name}, ObjectIdentifierKind(${obj_override.identifier_param}));
${indent}} else {
${indent}  command.m_${p.name}.Key = ${p.name};
${indent}}
% else:
${indent}UpdateHandle(command.m_${p.name}, GLObjectKind::${obj_override.kind.value});
% endif
% endif
% endfor
</%def>\
## After the Pre layers, right before the real call.
<%def name="implicit_create_checks(cmd, indent)">\
% for p in cmd.params:
<%
    bind_override = object_name_override_for(cmd.name, p.name, p.class_)
%>\
% if bind_override is not None and bind_override.role == ObjectNameRole.LOOKUP_OR_CREATE:
${indent}const bool ${p.name}ExistedBeforeBind = ExistsBeforeImplicitCreate(
${indent}    command.m_${p.name},
${indent}    g_DispatchTable.${bind_override.is_command}.load(std::memory_order_acquire));
% endif
% endfor
</%def>\
## Right after the real call.
<%def name="implicit_create_confirmations(cmd, indent)">\
% for p in cmd.params:
<%
    create_override = object_name_override_for(cmd.name, p.name, p.class_)
%>\
% if create_override is not None and create_override.role == ObjectNameRole.LOOKUP_OR_CREATE:
${indent}ConfirmImplicitCreate(
${indent}    command.m_${p.name}, GLObjectKind::${create_override.kind.value},
${indent}    g_DispatchTable.${create_override.is_command}.load(std::memory_order_acquire),
${indent}    ${p.name}ExistedBeforeBind);
% elif create_override is not None and not p.type_facts.is_array_element_eligible and is_existence_query(cmd.name, cmd.return_value.type, create_override):
${indent}ConfirmExistenceQuery(command.m_${p.name}, GLObjectKind::${create_override.kind.value}, result);
% endif
% endfor
</%def>\
## The names the driver wrote into an OUT array (glGen*). With `capture`, first reads them from
## the application's array, which recording does with the other OUT arrays.
<%def name="output_name_keys(cmd, indent, capture)">\
% for p in cmd.params:
<%
    names_override = pointer_override_for(cmd, p.name)
    out_obj_override = object_name_override_for(cmd.name, p.name, p.class_)
    is_output_names = (names_override is not None and out_obj_override is not None and
                       names_override.direction == PointerDirection.OUT and
                       names_override.length_source_kind != LengthSourceKind.DISCARDED_OUTPUT)
    if (is_output_names and capture and
            names_override.length_source_kind not in (LengthSourceKind.SIBLING_PARAM,
                                                       LengthSourceKind.FIXED_COUNT)):
        raise ValueError(f'{cmd.name}.{p.name}: output names need a sibling or fixed count')
%>\
% if is_output_names:
% if capture:
${indent}command.m_${p.name}.Capture(${p.name}, ${names_override.length_source_value});
% endif
${indent}// CREATE role: the driver just wrote brand-new names into this buffer (see this array
${indent}// param's OBJECT_NAME_PARAM_OVERRIDES entry) - create one key per element and register it.
${indent}UpdateOutputHandleArray(command.m_${p.name}, GLObjectKind::${out_obj_override.kind.value});
% endif
% endfor
</%def>\
## Stores the result and registers the handle or object it names.
<%def name="return_keys(cmd, indent)">\
% if has_return_member(cmd):
${indent}command.m_Return.Value = result;
<%
    return_obj_override = object_name_override_for(cmd.name, 'return_value', cmd.return_value.class_)
%>\
% if cmd.return_value.type_facts.is_mapped_handle:
% if cmd.name in handle_creating_commands:
${indent}UpdateOutputHandle(command.m_Return);
% if cmd.name in context_share_param:
<%
    share_param = context_share_param.get(cmd.name)
%>\
${indent}// See common/services/shareGroupService.h's own header comment and
${indent}// codegen/handle_lifecycle.py's CONTEXT_SHARE_PARAM for why a newly-created context must
${indent}// join its share context's group (if any) right away, using the real, recorder-process
${indent}// context values (ShareGroupService is process-local, unserialized bookkeeping - see that
${indent}// class's header comment). Guarded to context-creating commands: handle_creating_commands
${indent}// is a combined set also covering glFenceSync's GLsync return (see handle_lifecycle.py's
${indent}// MAPPED_HANDLE_LIFECYCLES), and a GLsync is never part of any share group.
${indent}ShareGroupService::Get().RegisterNewContext(
${indent}    command.m_Return.Value, ${f'command.m_{share_param}.Value' if share_param is not None else 'nullptr'});
% endif
% else:
${indent}UpdateHandle(command.m_Return);
% endif
% elif return_obj_override is not None:
${indent}// return_obj_override.role is always CREATE - see intermediates_creator.py's
${indent}// _validate_object_name_override (a return value can never be a LOOKUP/DELETE target).
${indent}UpdateOutputHandle(command.m_Return, GLObjectKind::${return_obj_override.kind.value});
% endif
% elif has_unserialized_return(cmd):
${indent}command.m_Return.Value = result;
% endif
</%def>\
## Erases deleted handles and object names, and merges share groups.
<%def name="lifecycle_updates(cmd, indent)">\
<%
    is_void = returns_void(cmd)
    erase_param = handle_param_to_erase_on_success.get(cmd.name)
    # The map service whose Erase() to call follows from erase_param's own type.
    erase_service = None
    if erase_param:
        erase_param_obj = next(p for p in cmd.params if p.name == erase_param)
        erase_service = mapped_handle_service_names[erase_param_obj.type_facts.base_type]
    share_lists_params = share_lists_commands.get(cmd.name)
    delete_kinds = {}
    for p in cmd.params:
        delete_override = object_name_override_for(cmd.name, p.name, p.class_)
        if delete_override is not None and delete_override.role == ObjectNameRole.DELETE:
            delete_kinds[p.name] = delete_override.kind.value
    delete_object_name_params = [p for p in cmd.params if p.name in delete_kinds]
%>\
## HandleShareGroupFor() (recorder/include/handleArgumentUpdaters.h) is overloaded per HandleT -
## 0 (process-global) for HGLRC, the real CurrentShareGroup() for GLsync - so this one call site
## is correct for every erase_service alike; see that function's own header comment.
% if erase_param and is_void:
${indent}${erase_service}::Get().Erase(command.m_${erase_param}.Value,
${indent}                               HandleShareGroupFor(command.m_${erase_param}.Value));
% elif erase_param:
${indent}if (result) {
${indent}  ${erase_service}::Get().Erase(command.m_${erase_param}.Value,
${indent}                                 HandleShareGroupFor(command.m_${erase_param}.Value));
${indent}}
% endif
% if share_lists_params is not None:
${indent}// See common/services/shareGroupService.h's own header comment and
${indent}// codegen/handle_lifecycle.py's SHARE_LISTS_COMMANDS - only merge on success, matching
${indent}// wglShareLists' own documented failure semantics (no sharing takes effect).
${indent}if (result) {
${indent}  ShareGroupService::Get().MergeGroups(command.m_${share_lists_params[0]}.Value,
${indent}                                        command.m_${share_lists_params[1]}.Value);
${indent}}
% endif
% for p in delete_object_name_params:
<%
    delete_kind = delete_kinds[p.name]
    delete_condition = '!command.m_Skip' if is_void else '!command.m_Skip && result'
%>\
${indent}if (${delete_condition}) {
% if p.type_facts.is_array_element_eligible:
${indent}  for (GLuint name : command.m_${p.name}.Value) {
${indent}    ObjectNameMapService::Get().Delete(GLObjectKind::${delete_kind},
${indent}                                        GroupIdentityFor(GLObjectKind::${delete_kind}), name);
${indent}  }
% else:
${indent}  ObjectNameMapService::Get().Delete(GLObjectKind::${delete_kind},
${indent}                                      GroupIdentityFor(GLObjectKind::${delete_kind}),
${indent}                                      command.m_${p.name}.Value);
% endif
${indent}}
% endfor
</%def>\
<%def name="buffer_shadow_call(guard, call)">\
% if guard:
  if (${guard}) {
    BufferShadowService::Get().${call};
  }
% else:
  BufferShadowService::Get().${call};
% endif
</%def>\
namespace gits::opengl {

namespace {

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = returns_void(cmd)
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
%>\
${ret} APIENTRY Wrapper_${cmd.name}(${param_decls}) {
  const RecursionGuard guard;
  auto realFunction = g_DispatchTable.${cmd.name}.load(std::memory_order_acquire);
  if (guard.IsNested() || !RecordingEnabled()) {
% if cmd.enabled:
    if (!guard.IsNested() && PluginLayersLoaded()) {
      // Only the plugin layers run. They see the arguments and the keys of the GL objects and
      // handles, without the output arrays other than new object names that recording collects.
      ${cmd.name}Command command(CurrentThreadId()${command_arguments(cmd)});
      command.m_Key = CaptureManager::Get().CreatePluginCommandKey();
${input_pointer_facts(cmd, '      ')}\
${input_keys(cmd, '      ')}\
${out_pointers(cmd, '      ')}\
      for (Layer* preLayer : CaptureManager::Get().GetPreLayers()) {
        preLayer->Pre(command);
      }
${implicit_create_checks(cmd, '      ')}\
% if is_void:
      if (!command.m_Skip) {
        realFunction(${param_names});
      }
% else:
      ${ret} result = command.m_Skip ? command.m_Return.Value : realFunction(${param_names});
% endif
${implicit_create_confirmations(cmd, '      ')}\
${output_name_keys(cmd, '      ', capture=True)}\
${return_keys(cmd, '      ')}\
${lifecycle_updates(cmd, '      ')}\
      for (Layer* postLayer : CaptureManager::Get().GetPostLayers()) {
        postLayer->Post(command);
      }
% if is_void:
      return;
% else:
      return command.m_Return.Value;
% endif
    }
% endif
% if is_void:
    realFunction(${param_names});
    return;
% else:
    return realFunction(${param_names});
% endif
  }
<% hook = buffer_content_hooks.get(cmd.name) %>\
% if hook is not None and hook.before_call:
  // See buffer_content_hooks.py's BufferContentHook.before_call.
${buffer_shadow_call(hook.guard, hook.call)}\
% endif
% if hook is not None and hook.prepare:
  // See buffer_content_hooks.py's BufferContentHook.prepare.
${buffer_shadow_call(hook.guard, hook.prepare)}\
% endif
% if cmd.name in mapped_write_publishers:
  // See generator_recorder.py's mapped_write_publishers(): the published writes must replay before
  // this command, so they are recorded before its key is reserved.
  MapTrackingService::Get().PublishMappedWrites();
% elif cmd.name in query_buffer_publishers:
  // See generator_recorder.py's query_buffer_publishers().
  if (QueryBufferIsBound()) {
    MapTrackingService::Get().PublishMappedWrites();
  }
% elif cmd.name in ('glDeleteBuffers', 'glDeleteBuffersARB'):
  // The names may be in a persistent mapping of a deleted buffer, whose shadow is freed once
  // MapTrackingService forgets the mapping, so the call and every hook use a copy.
  std::vector<GLuint> deletedBuffers;
  if (n > 0 && buffers != nullptr) {
    deletedBuffers.assign(buffers, buffers + n);
    buffers = deletedBuffers.data();
  }
  // The deletion unmaps its buffers, so their published writes must replay before it.
  MapTrackingService::Get().PublishBeforeDelete(n, buffers);
% endif
  // Reserved here - before the real driver dispatch below - rather than deferred into
  // CaptureManager::RecordCommand/RecordUnsupportedCommand (as it used to be), so concurrent
  // top-level calls on different threads get ordering keys assigned in DISPATCH order, matching
  // OrderingRecorder's own ordering guarantee, instead of driver-call-COMPLETION order (a later-
  // dispatched call that happens to return first would otherwise get a smaller/earlier key than
  // an earlier-dispatched call still in flight, letting replay reorder them). Mirrors Vulkan's
  // wrappersAuto.cpp.mako, which reserves its own key immediately before dispatch for the same
  // reason. Applies uniformly to the enabled-command path below (where it is copied into
  // `command.m_Key` before the CommandSerializer that encodes it is ever constructed - see
  // CaptureManager::RecordCommand's own header comment on why that ordering matters) and the
  // disabled/unsupported-command path (passed straight to ReportUnsupportedCommand). Named
  // `orderingKey`, not `key`, to avoid colliding with the handful of real GL/WGL parameters
  // literally named `key`/`key0`/`key1` (e.g. glAcquireKeyedMutexWin32EXT).
% if cmd.name in pre_hook_key_pair_commands:
  // This command is one of generator_recorder.py's pre_hook_key_pair_commands() - see that
  // function's and PRE_HOOK_KEY_PAIR_COMMANDS' own comments and
  // CaptureManager::CreateCommandKeyPair()'s own header comment (captureManager.h) for the
  // full rationale. Reserves BOTH this command's own orderingKey and a second, spare key for
  // the one Pre layer hooking this command (WindowGeometryTrackingLayer or ClientArraysLayer,
  // via TakePendingPreHookKey()) as ONE atomic pair, right here -
  // the same point a lone CreateCommandKey() call would otherwise occupy - rather than
  // reserving the spare key separately, later, inside that Pre() override itself: reserving
  // them together, before either is ever exposed to another thread, is what actually
  // guarantees no OTHER thread's own key can land between them, and that this command's own
  // orderingKey ends up in the exact same position relative to every OTHER, independently-
  // recording thread that a lone CreateCommandKey() call would have given it.
% if cmd.name in pre_hook_extra_key_commands:
  // See generator_recorder.py's PRE_HOOK_EXTRA_KEY_COMMANDS: one more key, ahead of the pair,
  // for another Pre layer's metacommand, reserved in the same atomic step.
% if cmd.name in frame_end_commands and cmd.enabled:
  const uint64_t firstKey = CaptureManager::Get().CreateFrameEndKeyRange(3);
% else:
  const uint64_t firstKey = CaptureManager::Get().CreateCommandKeyRange(3);
% endif
  const std::pair<uint64_t, uint64_t> preHookKeyPair{firstKey + 1, firstKey + 2};
  CaptureManager::Get().${pre_hook_extra_key_commands[cmd.name]}(firstKey);
% else:
  const auto preHookKeyPair = CaptureManager::Get().CreateCommandKeyPair();
% endif
  const uint64_t orderingKey = preHookKeyPair.second;
  // Handed off to the hooking Pre layer (the sole consumer for this command) via
  // CaptureManager::TakePendingPreHookKey() - see SetPendingPreHookKey()'s own header
  // comment for why this thread-local handoff is safe: the Pre layer loop a few lines below
  // runs on this very thread, before this thread does anything else.
  CaptureManager::Get().SetPendingPreHookKey(preHookKeyPair.first);
% elif cmd.name in post_hook_key_pair_commands:
  // See generator_recorder.py's POST_HOOK_KEY_PAIR_COMMANDS: the second key of the pair is for
  // a Post layer's metacommand, which must replay right after this command.
  const auto postHookKeyPair = CaptureManager::Get().CreateCommandKeyPair();
  const uint64_t orderingKey = postHookKeyPair.first;
  CaptureManager::Get().SetPendingPostHookKey(postHookKeyPair.second);
% else:
  const uint64_t orderingKey = CaptureManager::Get().CreateCommandKey();
% endif
% if cmd.enabled:
  // Constructed before the real call so IN-direction array/string members are eager-copied
  // out of application memory immediately before the call (see arguments.h's ArrayArgument<T>
  // and modern-opengl-backend-architecture.md's "Classify payloads from evidence"); OUT-
  // direction array members stay default-constructed (empty) until Capture() runs below, once
  // the driver has actually written into the app's buffer.
  ${cmd.name}Command command(CurrentThreadId()${command_arguments(cmd)});
  // See this function's own header comment: reserved before the real driver dispatch above,
  // not here - `orderingKey` was already minted before `command` was even constructed.
  // Assigning it into the Command object now, rather than after the real call, still happens
  // well before the CommandSerializer that encodes `command.m_Key` onto the wire is
  // constructed further below.
  command.m_Key = orderingKey;
${input_pointer_facts(cmd, '  ')}\
${input_keys(cmd, '  ')}\
${out_pointers(cmd, '  ')}\
  for (Layer* preLayer : CaptureManager::Get().GetPreLayers()) {
    preLayer->Pre(command);
  }
${implicit_create_checks(cmd, '  ')}\
% if cmd.name in frame_end_commands:
  CaptureManager::Get().FrameEndPreLayersRan(orderingKey);
% endif
% endif
% if cmd.name in client_array_setters:
<%
    setter = client_array_setters[cmd.name]
%>\
  // Reported whether or not this command is enabled - see client_array_draws.py's setter table.
% if cmd.enabled:
  if (!command.m_Skip) {
% else:
  {
% endif
% if setter.kind == 'Interleaved':
    ClientArraysService::Get().NoteInterleavedArrays(pointer);
% else:
    ClientArraysService::Get().NoteArrayPointer(ClientArrayKind::${setter.kind}, ${setter.slot_expr},
                                                pointer, ${setter.flags});
% endif
  }
% endif
% if is_void and cmd.enabled:
  if (!command.m_Skip) {
    realFunction(${param_names});
  }
% elif is_void:
  realFunction(${param_names});
% else:
  // Not const: `ret` may already be a pointer/qualified type (e.g. "const GLubyte*"), and
  // blindly prepending const either double-qualifies it or - for a plain pointer return
  // type - turns "T*" into "const T*", changing the pointee's constness instead of the
  // pointer's, which then fails to convert back to `ret` at the return statement below.
% if cmd.enabled:
  ${ret} result = command.m_Skip ? command.m_Return.Value : realFunction(${param_names});
% else:
  ${ret} result = realFunction(${param_names});
% endif
% endif
% if cmd.name in primitive_pair_commands:
  // See generator_recorder.py's PRIMITIVE_PAIR_COMMANDS.
  MapTrackingService::Get().SetInPrimitivePair(${'true' if primitive_pair_commands[cmd.name] else 'false'});
% endif
% if hook is not None and not hook.before_call:
<%
    hook_conditions = (['!command.m_Skip'] if cmd.enabled else []) + ([hook.guard] if hook.guard else [])
%>\
  // Reported whether or not this command is enabled - see buffer_content_hooks.py.
% if hook_conditions:
  if (${' && '.join(hook_conditions)}) {
% else:
  {
% endif
% if cmd.name in ('glDeleteBuffers', 'glDeleteBuffersARB'):
    MapTrackingService::Get().OnDelete(n, buffers);
% elif cmd.name in ('glBufferData', 'glBufferDataARB'):
    MapTrackingService::Get().OnBufferData(
        MapTrackingService::Get().ResolveBoundBuffer(target),
        [target](GLint& mapped) {
          return GetBoundBufferParameter(target, GL_BUFFER_MAPPED, mapped);
        });
% elif cmd.name in ('glNamedBufferData', 'glNamedBufferDataEXT'):
<%
    parameter_getter = ('glGetNamedBufferParameterivEXT' if cmd.name.endswith('EXT')
                        else 'glGetNamedBufferParameteriv')
%>\
    MapTrackingService::Get().OnBufferData(buffer, [buffer](GLint& mapped) {
      if (!TryResolveRealFunction("${parameter_getter}")) {
        return false;
      }
      g_DispatchTable.${parameter_getter}.load(std::memory_order_acquire)(
          buffer, GL_BUFFER_MAPPED, &mapped);
      return true;
    });
% endif
    BufferShadowService::Get().${hook.call};
  }
% endif
% if cmd.enabled:
${implicit_create_confirmations(cmd, '  ')}\
% for p in cmd.params:
<%
    override = pointer_override_for(cmd, p.name)
%>\
% if override is not None and override.length_source_kind == LengthSourceKind.DISCARDED_OUTPUT:
  // m_${p.name} stays empty - see LengthSourceKind.DISCARDED_OUTPUT (pointer_metadata.py).
% elif override is not None and override.direction == PointerDirection.OUT:
  // Read only now that the driver has actually written into the app's buffer.
<%
    # 'void *' OUT params (glReadPixels et al.) need an explicit cast to the substituted element
    # type (see array_element_cpp_type()). A pixel transfer is a buffer offset while a pixel pack
    # buffer is bound, and a query_buffer_guard param (glGetQueryObject*v's `params`) while a
    # query buffer is bound, so neither is read through directly: GuardPackPixelsPointer/
    # GuardQueryBufferPointer decide.
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
## An OUT-direction (glReadPixels-shaped) pixel transfer, sized after the call from the
## GL_PACK_* pixel-store state - or, for a texture level readback, from the level itself.
<%
    if override.pixel_texture_level_param:
        compute_size_out = 'ComputeTextureLevelPackByteSize'
        size_unknown_out = 'TextureLevelPackSizeIsUnknown'
        pixel_transfer_args_out = (
            f'{override.pixel_texture_target_param}, {override.pixel_texture_level_param}, '
            f'{override.pixel_format_param}, {override.pixel_type_param}, "{cmd.name}"')
    else:
        compute_size_out = 'ComputePixelTransferByteSize'
        size_unknown_out = 'PixelTransferSizeIsUnknown'
        pixel_transfer_args_out = pixel_transfer_size_args(override, cmd.name, 'Pack')
%>\
  command.m_${p.name}.Capture(${capture_arg_expr}, ${compute_size_out}(${pixel_transfer_args_out}));
  // See pointer_needs_pixel_transfer_unknown_flag()'s
  // own header comment (generator_helpers.py) and PixelTransferSizeIsUnknown()'s own header
  // comment (pixelTransferSize.h). Same argument list as the Capture() call immediately
  // above, so the two can never disagree about whether this call's captured size is
  // genuine-zero or unknown; GL_PACK_* pixel-store state cannot have changed between the two
  // calls (no intervening GL call), and every warning either might log is warn-once per
  // commandName regardless, so this second call never double-logs.
  command.m_${p.name}SizeUnknown.Value = ${size_unknown_out}(${pixel_transfer_args_out});
  // See PackPixelsPointerIsBufferOffset()'s
  // own header comment (clientMemoryCapture.h). Uses ${p.name} here too - this wrapper's
  // own, ORIGINAL, never-guarded parameter, the same one m_${p.name}RawPointer.Value below
  // captures - never whatever capture_arg_expr's own GuardPackPixelsPointer() invocation
  // returned, so this always reflects the real guard decision for this exact call
  // regardless of what the Capture() call above ended up storing.
  command.m_${p.name}IsBufferOffset.Value = PackPixelsPointerIsBufferOffset(${p.name});
% elif (cmd.name, p.name) in TEXT_OUTPUT_BUFFERS:
<% length_param = TEXT_OUTPUT_BUFFERS[(cmd.name, p.name)] %>\
  command.m_${p.name}.Capture(${capture_arg_expr}, TextOutputCaptureSize(${p.name}, ${override.length_source_value}, ${length_param}));
% elif override.length_source_kind == LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT:
## COMPSIZE(buffer) depends on the sibling enum's value.
  command.m_${p.name}.Capture(${capture_arg_expr}, ComputeClearBufferValueCount(${override.clear_buffer_param}));
% elif override.length_source_kind == LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT:
## COMPSIZE(pname) of glGetVertexAttribiv depends on the sibling enum's value. OUT direction
## only.
  command.m_${p.name}.Capture(${capture_arg_expr}, ComputeVertexAttribParameterValueCount(${override.pname_param}));
% else:
## The sibling is passed in its own type; Capture() converts it to size_t itself.
  command.m_${p.name}.Capture(${capture_arg_expr}, ${override.length_source_value});
% endif
% if pointer_needs_raw_offset_capture(override):
  // See pointer_needs_raw_offset_capture()'s own header comment
  // (generator_helpers.py). ${p.name} here is still this wrapper's own, ORIGINAL,
  // never-guarded parameter (the guard, if any, only ever affects capture_arg_expr above) -
  // so this always captures the real bit pattern regardless of whether a bound buffer
  // object caused the capture above to be suppressed for it.
  command.m_${p.name}RawPointer.Value = reinterpret_cast<uintptr_t>(${p.name});
% endif
% endif
% endfor
${output_name_keys(cmd, '  ', capture=False)}\
${return_keys(cmd, '  ')}\
${lifecycle_updates(cmd, '  ')}\
  for (Layer* postLayer : CaptureManager::Get().GetPostLayers()) {
    postLayer->Post(command);
  }
% else:
  ReportUnsupportedCommand("${cmd.name}", orderingKey);
% endif
% if cmd.name in post_hook_key_pair_commands:
  CaptureManager::Get().ReleasePendingPostHookKey();
% endif
% if not is_void and cmd.enabled:
  // A Post layer may have replaced the result.
  return command.m_Return.Value;
% elif not is_void:
  return result;
% endif
}

% endfor
} // namespace

% if part_index == 0:
% for other_part in range(1, part_count):
void AddRecorderWrappers${other_part}(std::unordered_map<std::string_view, void*>& nameToWrapper);
% endfor

% endif
void AddRecorderWrappers${part_index}(std::unordered_map<std::string_view, void*>& nameToWrapper) {
  // A braced map initializer would construct every key inline, which makes this function very
  // slow to compile.
  struct WrapperEntry {
    const char* Name;
    void* Wrapper;
  };
  static const WrapperEntry entries[] = {
% for cmd in commands:
    {"${cmd.name}", reinterpret_cast<void*>(&Wrapper_${cmd.name})},
% endfor
  };
  for (const WrapperEntry& entry : entries) {
    nameToWrapper.emplace(entry.Name, entry.Wrapper);
  }
}
% if part_index == 0:

void* GetGeneratedRecorderWrapper(const char* name) {
  // Checked first so glMapBufferRange/glUnmapBuffer never reach their own generated entries
  // below - see docs/development/opengl-buffer-map-tracking-plan.md's Decision 3. Maps are tracked
  // only while recording.
  if (RecordingEnabled()) {
    if (void* custom = GetCustomRecorderWrapper(name)) {
      return custom;
    }
  }
  static const std::unordered_map<std::string_view, void*> nameToWrapper = [] {
    std::unordered_map<std::string_view, void*> map;
    map.reserve(${command_count if command_count is not None else len(commands)});
% for other_part in range(part_count):
    AddRecorderWrappers${other_part}(map);
% endfor
    return map;
  }();
  auto it = nameToWrapper.find(name);
  if (it == nameToWrapper.end()) {
    return nullptr;
  }
  return it->second;
}
% endif

} // namespace gits::opengl
