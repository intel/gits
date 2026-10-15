#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# GENERIC lifecycle metadata for every "mapped handle" type - an opaque, GITS-owned-lifecycle
# pointer typedef (see intermediates.py's _MAPPED_HANDLE_BASE_TYPES/TypeFacts.is_mapped_handle)
# that is NOT a GLuint-named object (so object_name_lifecycle.py's class=-based derivation does
# not apply to it - see intermediates_creator.py's _requires_object_name_override, which exempts
# every is_mapped_handle field up front for exactly this reason). `HGLRC` and `GLsync` are the
# two entries today; see MAPPED_HANDLE_LIFECYCLES below for how a third such type joins them.
#
# HISTORY: HGLRC (this module, then just "handle_lifecycle.py") and GLsync (originally a
# separate "sync_lifecycle.py", written as a structural copy of this module's own HGLRC tables
# under the theory that "two same-shaped small modules are simpler to review independently than
# one shared abstraction with no third user yet" - see git history for that module's own header
# comment) were merged here once GLsync actually arrived and turned out to need the exact same
# shape a second time: two hand-maintained tables (which commands mint a fresh key from their
# return value, which commands erase a key on successful destroy) feeding the exact same
# consumers (generator_recorder.py/generator_player.py, recorderWrappersAuto.cpp.mako). Once a
# second real user existed, "keep the two modules separate so neither reader wades through the
# other's specifics" stopped paying for itself: every future third opaque-pointer-handle type
# would otherwise mean a third near-identical parallel module (and, on the C++ side, a third
# near-identical parallel map-service class - see common/services/handleMapService.h, merged
# for the same reason). The one thing that is genuinely still HGLRC-only - context sharing
# (CONTEXT_SHARE_PARAM/SHARE_LISTS_COMMANDS below) - has no GLsync (or hypothetical third-type)
# equivalent at all, so it stays as its own, explicitly HGLRC-scoped table rather than being
# forced into MappedHandleLifecycle's shared shape just for the sake of uniformity.
#
# See handleArgumentUpdaters.h (both recorder and player copies) and
# common/services/handleMapService.h for how the C++ side mirrors this same generalization:
# `HandleMapService<HandleT>` is the one real implementation of Register/GetKey/Erase/
# SetHandle/GetHandle; `ContextHandleMapService`/`SyncHandleMapService` are just its two
# existing type aliases.

from dataclasses import dataclass


@dataclass(frozen=True, kw_only=True)
class MappedHandleLifecycle:
    """Data-driven lifecycle description of one mapped-handle type - the generic mechanism
    every such type plugs into. Adding a THIRD mapped-handle type (hypothetically) means:
      1. Adding its base type to intermediates.py's _MAPPED_HANDLE_BASE_TYPES (so
         TypeFacts.is_mapped_handle recognizes it).
      2. Adding one more entry to MAPPED_HANDLE_LIFECYCLES below, built from a
         MappedHandleLifecycle instance like HGLRC_LIFECYCLE/GLSYNC_LIFECYCLE.
      3. Adding a `using NewHandleMapService = HandleMapService<NewHandleT>;` alias (plus a
         HandleTypeInfo<NewHandleT> specialization for its log-message text) to
         common/services/handleMapService.h.
    No new Python module and no new C++ service class - see this module's own header comment
    and handleMapService.h's own header comment for why that used to be the (wrong) default.
    """

    cpp_type: str
    """The C++ handle typedef this entry describes - must match one of
    intermediates.py's _MAPPED_HANDLE_BASE_TYPES entries, and TypeFacts.base_type for every
    field this entry's tables mention."""

    service_name: str
    """The C++ class (a HandleMapService<cpp_type> alias, see common/services/
    handleMapService.h) recorderWrappersAuto.cpp.mako should call Get()/Erase() on for this
    type's erase-on-success path. Not needed for the player side or the recorder's own
    UpdateHandle/UpdateOutputHandle calls - both go through HandleMapService<HandleT> directly
    via a template, which C++ overload/template resolution already picks correctly from the
    field's own static type with no name lookup at all."""

    creating_commands: frozenset[str]
    """Commands whose *return value* mints a fresh key for this handle type (UpdateOutputHandle
    - see handleArgumentUpdaters.h), rather than looking up an existing one (UpdateHandle) like
    every other field of this type."""

    param_to_erase_on_success: dict[str, str]
    """Command name -> name of this handle type's parameter to erase from its
    HandleMapService<cpp_type> after a successful call. Parameter name must match the
    registry's declared name for this command (see recorder/include/openglDriversAuto.h)."""


# Commands whose *return value* mints a fresh HGLRC key (UpdateOutputHandle), rather than
# looking up an existing one (UpdateHandle) like every other HGLRC-typed field (many enabled
# commands take an HGLRC parameter; only two commands mint one, only one destroys one). Note
# wglCreateContextAttribsARB is listed for completeness but is not `enabled` yet (its
# `attribList` parameter is a raw pointer) - it is inert until pointer/array argument support
# lands (see modern-opengl-backend-architecture.md's scale_pointer roadmap item).
HGLRC_LIFECYCLE = MappedHandleLifecycle(
    cpp_type='HGLRC',
    service_name='ContextHandleMapService',
    creating_commands=frozenset({
        'wglCreateContext', 'wglCreateContextAttribsARB',
    }),
    # Only wglDeleteContext destroys a context today.
    param_to_erase_on_success={
        'wglDeleteContext': 'oldContext',
    },
)

# Commands whose *return value* mints a fresh GLsync key (UpdateOutputHandle), rather than
# looking up an existing one (UpdateHandle) like every other GLsync-typed field (glIsSync/
# glClientWaitSync/glWaitSync/glDeleteSync's `sync` parameter).
#
# glFenceSyncAPPLE really is a second, independent entry here, not just a comment-only mention
# like HGLRC_LIFECYCLE's wglCreateContextAttribsARB precedent: it is a real alias for
# glFenceSync (GL_APPLE_sync, same `(condition, flags) -> GLsync` shape, same GPU-internal
# fence-object semantics) that DOES end up `enabled` here - is_mapped_handle's type-driven gate
# has no notion of "the command family I originally asked for" vs. "an alias/extension variant
# gl.xml also defines with the identical shape" (see e.g. command_ids.json's many other already-
# enabled *APPLE-suffixed aliases, such as glBindVertexArrayAPPLE/glIsVertexArrayAPPLE). Every
# entry in this frozenset therefore needs verifying, not just glFenceSync's own two params:
# glFenceSyncAPPLE's params are the identical (GLenum, GLbitfield) pair, so this is safe. See
# intermediates_creator.py's _FORCE_DISABLED_COMMANDS for the one same-shaped sync command
# (glImportSyncEXT) that reaches the same gate but is NOT listed here or enabled, because its
# own semantics - not its type shape - make it unsafe to replay generically.
GLSYNC_LIFECYCLE = MappedHandleLifecycle(
    cpp_type='GLsync',
    service_name='SyncHandleMapService',
    creating_commands=frozenset({
        'glFenceSync', 'glFenceSyncAPPLE',
    }),
    # glDeleteSync and its real GL_APPLE_sync alias glDeleteSyncAPPLE both destroy a sync
    # object (see this entry's own comment on why an alias needs its own entry rather than
    # piggybacking on the other's). Per the GL 4.6 spec section 4.1.4 ("Deleting Sync Objects"),
    # glDeleteSync(sync) silently ignores a `sync` value of zero, and flags GL_INVALID_VALUE
    # (with no other side effects) if `sync` is non-zero but not the name of an existing sync
    # object - there is no code path where deleting a real, existing sync object fails partway.
    # glDeleteSync's return type is void (no result to gate an "on success" check on, unlike
    # wglDeleteContext's BOOL), so recorderWrappersAuto.cpp.mako's `is_void` branch already
    # erases this unconditionally. A spurious erase for an already-invalid `sync` argument is
    # harmless either way: Erase() on a handle with no registered key is a silent no-op (see
    # common/services/handleMapService.h).
    param_to_erase_on_success={
        'glDeleteSync': 'sync',
        'glDeleteSyncAPPLE': 'sync',
    },
)

# Every mapped-handle type's lifecycle entry - the ONE table a new mapped-handle type joins
# (see MappedHandleLifecycle's own docstring for the full checklist). Order does not matter to
# any consumer below; listed in the order each type was added.
MAPPED_HANDLE_LIFECYCLES: tuple[MappedHandleLifecycle, ...] = (
    HGLRC_LIFECYCLE,
    GLSYNC_LIFECYCLE,
)

# Derived, pre-unioned views for generator_recorder.py/generator_player.py, so neither consumer
# has to union every entry's tables itself. Safe to union blindly: every MappedHandleLifecycle's
# creating_commands/param_to_erase_on_success is keyed by command name, and no command mints or
# erases two different handle types at once.
HANDLE_CREATING_COMMANDS: frozenset[str] = frozenset().union(
    *(entry.creating_commands for entry in MAPPED_HANDLE_LIFECYCLES)
)
HANDLE_PARAM_TO_ERASE_ON_SUCCESS: dict[str, str] = {
    command_name: param_name
    for entry in MAPPED_HANDLE_LIFECYCLES
    for command_name, param_name in entry.param_to_erase_on_success.items()
}

# base_type (TypeFacts.base_type, e.g. 'HGLRC') -> the C++ HandleMapService<base_type> alias
# recorderWrappersAuto.cpp.mako's erase-on-success path should call Get()/Erase() on - see
# MappedHandleLifecycle.service_name's own docstring for why only the erase path (not
# UpdateHandle/UpdateOutputHandle, which resolve the right HandleMapService<T> specialization
# via a C++ template with no name lookup needed) consults this.
MAPPED_HANDLE_SERVICE_NAMES: dict[str, str] = {
    entry.cpp_type: entry.service_name for entry in MAPPED_HANDLE_LIFECYCLES
}

# HGLRC-ONLY tables below: context sharing has no counterpart for GLsync (or any hypothetical
# future mapped-handle type) - a GLsync is never part of any "share group", so these stay their
# own, explicitly HGLRC-scoped tables rather than being forced into MappedHandleLifecycle's
# shared shape (which every entry would then have to answer "None"/"{}" for) just for the sake
# of uniformity. See ShareGroupService's own header comment (recorder/include/
# shareGroupService.h) for the GL spec rule this exists to satisfy.

# Every key here must also be a key of HGLRC_LIFECYCLE.creating_commands above - name of the
# "share context" parameter a context-creating command takes (wglCreateContextAttribsARB's
# hShareContext), or None if the command never shares a new context at creation time
# (wglCreateContext has no such parameter). Consulted by recorderWrappersAuto.cpp.mako/
# commandRunnersCustom.cpp to tell ShareGroupService::RegisterNewContext() which existing
# context (if any) the newly-created one should join - see
# recorder/include/shareGroupService.h and modern-opengl-backend-architecture.md's share-group
# decision record for why GL object names need this at all (they are unique only within a
# share group, not process-globally).
CONTEXT_SHARE_PARAM: dict[str, str | None] = {
    'wglCreateContext': None,
    'wglCreateContextAttribsARB': 'hShareContext',
}

# Commands that merge two already-existing contexts' share groups after a successful call (see
# ShareGroupService::MergeGroups()) - value is (param1, param2), the names of the two HGLRC
# parameters to merge. Only wglShareLists does this today. Consulted by
# recorderWrappersAuto.cpp.mako the same way CONTEXT_SHARE_PARAM is.
SHARE_LISTS_COMMANDS: dict[str, tuple[str, str]] = {
    'wglShareLists': ('hrcSrvShare', 'hrcSrvSource'),
}

# Commands whose successful call ends a frame - see stream::CommonCommandId::ID_FRAME_END and
# unsupportedCommandTypes.h's FrameEndCommand. Consulted by recorderWrappersAuto.cpp.mako to
# record a FrameEndCommand immediately after the command's own CommandSerializer, mirroring
# DirectX's EncoderLayer::Post()-driven FrameEndSerializer calls for its own present commands
# (this backend has no Post layer yet to hook this into instead - see
# modern-opengl-backend-architecture.md's "Layer and command model"). Unrelated to mapped-handle
# lifecycle - kept in this module only for historical reasons (HGLRC's own recorder-generation
# consumer, generator_recorder.py, already needed a small hand-maintained WGL command table, and
# this is another one).
FRAME_END_COMMANDS: frozenset[str] = frozenset({
    'wglSwapBuffers', 'wglSwapLayerBuffers',
})
