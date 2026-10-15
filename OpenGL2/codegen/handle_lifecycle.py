#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Small, hand-maintained lookup tables describing HGLRC's lifecycle among WGL commands - kept
# separate from intermediates_creator.py's purely type-driven `enabled` gate, since "does this
# command mint/destroy an object" is a semantic fact about *which* command this is, not a
# derivable fact about its parameter *types* (many enabled commands take an HGLRC parameter;
# only two commands mint one, only one destroys one). See handleArgumentUpdaters.h for how the
# recorder wrapper template (recorderWrappersAuto.cpp.mako) uses these two tables.

# Commands whose *return value* mints a fresh HGLRC key (UpdateOutputHandle), rather than
# looking up an existing one (UpdateHandle) like every other HGLRC-typed field. Note
# wglCreateContextAttribsARB is listed for completeness but is not `enabled` yet (its
# `attribList` parameter is a raw pointer) - it is inert until pointer/array argument support
# lands (see modern-opengl-backend-architecture.md's scale_pointer roadmap item).
HANDLE_CREATING_COMMANDS: frozenset[str] = frozenset({
    'wglCreateContext', 'wglCreateContextAttribsARB',
})

# command name -> name of the HGLRC parameter to erase from ContextHandleMapService after a
# successful call. Only wglDeleteContext destroys a context today. Parameter name must match
# wgl.xml's declared name for this command (see recorder/include/openglDriversAuto.h).
HANDLE_PARAM_TO_ERASE_ON_SUCCESS: dict[str, str] = {
    'wglDeleteContext': 'oldContext',
}

# Every key here must also be a key of HANDLE_CREATING_COMMANDS above - name of the "share
# context" parameter a context-creating command takes (wglCreateContextAttribsARB's
# hShareContext), or None if the command never shares a new context at creation time
# (wglCreateContext has no such parameter). Consulted by recorderWrappersAuto.cpp.mako/
# commandRunnersCustom.cpp to tell ShareGroupService::RegisterNewContext() which existing
# context (if any) the newly-created one should join - see
# common/services/shareGroupService.h and modern-opengl-backend-architecture.md's share-group
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
# modern-opengl-backend-architecture.md's "Layer and command model").
FRAME_END_COMMANDS: frozenset[str] = frozenset({
    'wglSwapBuffers', 'wglSwapLayerBuffers',
})
