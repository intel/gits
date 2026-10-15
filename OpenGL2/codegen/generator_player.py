#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re

from custom_player_runners import CUSTOM_PLAYER_RUNNERS
from generator_helpers import mako_write, object_name_override_for
from handle_lifecycle import FRAME_END_COMMANDS, HANDLE_CREATING_COMMANDS
from intermediates import Command
from object_name_lifecycle import ObjectNameRole

# HANDLE_CREATING_COMMANDS is already unioned across every mapped-handle type (HGLRC, GLsync,
# ...) by handle_lifecycle.py itself - kept in sync with the same table generator_recorder.py
# passes to recorderWrappersAuto.cpp.mako so both the recorder and player agree on which
# commands mint a fresh mapped-handle key from their return value.

# Disabled commands the player's own hand-written code still calls directly through
# g_DispatchTable, bypassing the generated per-command Run()/CommandFactory path entirely -
# found by grepping every hand-written player/*.cpp,*.h file (excluding the generated
# *Auto.{h,cpp} ones) for "g_DispatchTable.<name>" (WarmUpResolvableFunctions()'s
# enabled-only probing loop would otherwise never warm these up either - see below for why
# that still leaves them working correctly either way):
#   - wglGetProcAddress: openglDriversAuto.cpp.mako's own ResolveDriverFunction() fallback path
#     (this is the recorder/player-shared template - see that file's own comment for why
#     wglGetProcAddress itself has no generated Command/CommandRunner at all, on either side).
#   - glGetIntegerv/glMapBufferRange: player/mappedDataMetaRunner.cpp's buffer map-tracking
#     replay, which queries/re-establishes a mapped buffer's real GL state directly - this is
#     a low-level mechanism operating underneath the generated-command replay path, not a
#     replayed command in its own right (mirrors recorder/mapTrackingWrappers.cpp's identical
#     precedent for the same reason).
#   - glMapTexture2DINTEL: player/mappedTextureDataMetaRunner.cpp, which maps a texture level
#     itself to replay its captured contents.
#
# Included in `warmup_commands` below purely to preserve this function's pre-existing behavior
# for these specific names unchanged (they were warmed up before this filter existed too, as
# part of the full, unfiltered list) - not because it is load-bearing for correctness. Every
# other call site of the real, driver-resolved function for these (mappedDataMetaRunner.
# cpp's direct calls, ResolveDriverFunction's own recursive-looking call for
# wglGetProcAddress itself) runs while a context is already current by construction (mapping a
# buffer, or resolving some *other* function's real pointer, both require one) - the same
# condition WarmUpResolvableFunctions() exists to opportunistically catch just after a
# wglMakeCurrent, so each dispatch-table entry's own lazy resolve-thunk (ResolveAndCall_<cmd>,
# generated unconditionally for every name in `commands` regardless of this filter - see
# openglDriversAuto.cpp.mako's own page comment) would resolve them correctly on first use
# either way, warmed up or not.
_PLAYER_DIRECT_DISPATCH_COMMANDS: frozenset[str] = frozenset({
    'wglGetProcAddress',
    'glGetIntegerv',
    'glMapBufferRange',
    'glMapTexture2DINTEL',
})

# Commands subcapture state restore leaves out of the display lists it writes again. GL executes
# them instead of compiling them, and they create, delete or change objects the restore writes
# before the lists. The context state ones GL also executes are kept: commands compiled after
# them read that state, and the context state restore overrides it later.
_NOT_LISTED_RE = re.compile(
    r'^(?:wgl'
    r'|gl(?:Gen|Create|Delete)(?:$|[A-Z])'
    r'|glFenceSync|glGenerate\w*Mipmap'
    r'|gl(?:Named)?Buffer(?:Sub)?Data|gl(?:Named)?BufferStorage|glCopy(?:Named)?BufferSubData'
    r'|glClear(?:Named)?Buffer(?:Sub)?Data|glInvalidateBuffer|glUnmap|glFlushMapped'
    r'|glReadn?Pixels'
    r'|glShaderSource|glCompileShader|glLinkProgram|gl(?:Attach|Detach)(?:Shader|Object)'
    r'|glBind(?:Attrib|FragData)Location|glProgramParameteri|glTransformFeedbackVaryings'
    r'|glShaderBinary|glProgramBinary|glValidateProgram'
    r'|gl(?:Named)?RenderbufferStorage|gl(?:Tex|Texture)Storage|gl(?:Tex|Texture|MultiTex)Buffer'
    r'|gl(?:Named)?Framebuffer(?:Texture|Renderbuffer)|glBlit(?:Named)?Framebuffer)')


def display_list_commands(commands: list[Command]) -> frozenset[str]:
    """Enabled commands a display list body is recorded with before the subcapture range."""
    return frozenset(c.name for c in commands if c.enabled and not _NOT_LISTED_RE.match(c.name))


# Commands AnalyzerLayer handles by hand: display list compilation and frame ends.
ANALYZER_LAYER_CUSTOM_COMMANDS: frozenset[str] = frozenset({'glNewList', 'glEndList'}) | FRAME_END_COMMANDS


def analyzed_object_params(commands: list[Command]) -> dict[str, list[str]]:
    """Per enabled command, the parameters naming existing objects, whose keys the subcapture
    analysis pass collects. Objects a command creates need no restore."""
    analyzed = {}
    for cmd in commands:
        if not cmd.enabled or cmd.name in ANALYZER_LAYER_CUSTOM_COMMANDS:
            continue
        params = [p.name for p in cmd.params
                  if (override := object_name_override_for(cmd.name, p.name, p.class_)) is not None
                  and override.role != ObjectNameRole.CREATE]
        if params:
            analyzed[cmd.name] = params
    return analyzed


def generate_player_files(commands: list[Command], enum_groups: set[str]) -> None:
    """Generate the player's own real-driver dispatch and per-command CommandRunners.

    Mirrors generator_recorder.py's shape. The player gets its own, independent copy of
    openglDriversAuto.{h,cpp} - same templates as the recorder, instantiated a second time
    under the gits::opengl2::player namespace (see codegen/templates/openglDriversAuto.h.mako's
    header comment and docs/development/opengl2-player-skeleton-plan.md's Decision 3) - plus
    the new commandRunnersAuto.{h,cpp} and openGL2CommandFactoryAuto.cpp, which have no
    recorder-side equivalent.

    `commands` is passed to openglDriversAuto.{h,cpp}.mako unfiltered, same as the recorder's
    own instantiation of the same two templates (generator_recorder.py) - both need a full
    dispatch-table entry/resolve-thunk for every intercepted name, enabled or not: the
    interceptor must transparently forward every intercepted function to the real driver on the
    recorder side, and a small set of disabled commands are still called directly through the
    player's own g_DispatchTable by hand-written code, bypassing the generated
    Run()/CommandFactory path entirely (see _PLAYER_DIRECT_DISPATCH_COMMANDS above for exactly
    which ones and why). Only `warmup_commands` - consumed solely by
    WarmUpResolvableFunctions()'s own speculative probing loop, the one thing in this template
    that actually scales with how many *disabled* commands it iterates - is narrowed: a disabled
    command always replays through the generic UnsupportedCommandRunner instead (see
    commandRunnersAuto.cpp.mako's own `% if cmd.enabled:` guard), never touching g_DispatchTable,
    so probing one up front on every wglMakeCurrent (see
    player/commandRunnersCustom.cpp's WglMakeCurrentRunner::Run()) is pure waste.
    """
    warmup_commands = [
        c for c in commands
        if c.enabled or c.name in _PLAYER_DIRECT_DISPATCH_COMMANDS
    ]
    mako_write('templates/openglDriversAuto.h.mako',
               'player/include/openglDriversAuto.h', commands=commands,
               namespace='gits::opengl2::player')
    mako_write('templates/openglDriversAuto.cpp.mako',
               'player/openglDriversAuto.cpp', commands=commands,
               warmup_commands=warmup_commands, namespace='gits::opengl2::player')
    mako_write('templates/commandRunnersAuto.h.mako',
               'player/include/commandRunnersAuto.h', commands=commands)
    mako_write('templates/commandRunnersAuto.cpp.mako',
               'player/commandRunnersAuto.cpp', commands=commands,
               handle_creating_commands=HANDLE_CREATING_COMMANDS)
    mako_write('templates/openGL2CommandFactoryAuto.cpp.mako',
               'player/openGL2CommandFactoryAuto.cpp', commands=commands,
               custom_player_runners=CUSTOM_PLAYER_RUNNERS)
    mako_write('templates/traceLayerAuto.h.mako',
               'layers/trace/traceLayerAuto.h', commands=commands)
    mako_write('templates/traceLayerAuto.cpp.mako',
               'layers/trace/traceLayerAuto.cpp', commands=commands, enum_groups=enum_groups)
    mako_write('templates/recordingLayerAuto.h.mako',
               'subcapture/recordingLayerAuto.h', commands=commands,
               frame_end_commands=sorted(FRAME_END_COMMANDS))
    mako_write('templates/recordingLayerAuto.cpp.mako',
               'subcapture/recordingLayerAuto.cpp', commands=commands,
               frame_end_commands=sorted(FRAME_END_COMMANDS),
               list_commands=display_list_commands(commands))
    analyzed = analyzed_object_params(commands)
    mako_write('templates/analyzerLayerAuto.h.mako',
               'subcapture/analyzerLayerAuto.h', analyzed=analyzed,
               frame_end_commands=sorted(FRAME_END_COMMANDS))
    mako_write('templates/analyzerLayerAuto.cpp.mako',
               'subcapture/analyzerLayerAuto.cpp', analyzed=analyzed)
