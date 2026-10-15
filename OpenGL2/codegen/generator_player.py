#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from custom_player_runners import CUSTOM_PLAYER_RUNNERS
from generator_helpers import mako_write
from handle_lifecycle import HANDLE_CREATING_COMMANDS
from intermediates import Command

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
#
# Included in `warmup_commands` below purely to preserve this function's pre-existing behavior
# for these three specific names unchanged (they were warmed up before this filter existed too, as
# part of the full, unfiltered list) - not because it is load-bearing for correctness. Every
# other call site of the real, driver-resolved function for these three (mappedDataMetaRunner.
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
})


def generate_player_files(commands: list[Command]) -> None:
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
