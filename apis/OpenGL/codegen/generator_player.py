#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re

from custom_player_runners import CUSTOM_PLAYER_RUNNERS, UNREPLAYED_COMMANDS
from generator_helpers import (PLATFORM_GUARDS, mako_write, mako_write_parts,
                               object_name_override_for)
from handle_lifecycle import FRAME_END_COMMANDS, HANDLE_CREATING_COMMANDS, SHARE_LISTS_COMMANDS
from intermediates import Command
from object_name_lifecycle import ObjectNameRole

# Disabled commands the player's hand-written code calls directly through g_DispatchTable, which
# WarmUpResolvableFunctions() warms up along with the enabled ones:
#   - wglGetProcAddress, glXGetProcAddressARB: the shared resolver's fallback path.
#   - glGetIntegerv/glMapBufferRange: player/mappedDataMetaRunner.cpp's buffer map-tracking
#     replay.
#   - glMapTexture2DINTEL: player/mappedTextureDataMetaRunner.cpp, which maps a texture level
#     itself to replay its captured contents.
# Not needed for correctness: these calls run with a context current, so each function's lazy
# resolve-thunk resolves it on first use anyway.
_PLAYER_DIRECT_DISPATCH_COMMANDS: frozenset[str] = frozenset({
    'wglGetProcAddress',
    'glXGetProcAddressARB',
    'glGetIntegerv',
    'glMapBufferRange',
    'glMapTexture2DINTEL',
})

# The command runners are split into this many files. Changing it also requires changing the
# commandRunnersNAuto.cpp lists in apis/OpenGL/codegen/CMakeLists.txt and
# apis/OpenGL/player/CMakeLists.txt, which the build does not derive.
COMMAND_RUNNER_PARTS = 4

# Commands subcapture state restore leaves out of the display lists it writes again. GL executes
# them instead of compiling them, and they create, delete or change objects the restore writes
# before the lists. The context state ones GL also executes are kept: commands compiled after
# them read that state, and the context state restore overrides it later.
_NOT_LISTED_RE = re.compile(
    r'^(?:wgl|glX'
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
ANALYZER_LAYER_CUSTOM_COMMANDS: frozenset[str] = (frozenset({'glNewList', 'glEndList'}) |
                                                  FRAME_END_COMMANDS)

_SUBCAPTURE_FRAME_END_COMMANDS: list[str] = sorted(FRAME_END_COMMANDS)


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
    """Generate replay warm-up, command runners, and command factories."""
    warmup_commands = [
        c for c in commands
        if c.enabled or c.name in _PLAYER_DIRECT_DISPATCH_COMMANDS
    ]
    mako_write('templates/dispatchWarmUpAuto.cpp.mako',
               'player/dispatchWarmUpAuto.cpp', warmup_commands=warmup_commands,
               platform_guards=PLATFORM_GUARDS)
    mako_write('templates/commandRunnersAuto.h.mako',
               'player/include/commandRunnersAuto.h', commands=commands,
               platform_guards=PLATFORM_GUARDS)
    # Only enabled commands get a runner, so the parts split them alone.
    mako_write_parts('templates/commandRunnersAuto.cpp.mako', 'player/commandRunners{}Auto.cpp',
                     COMMAND_RUNNER_PARTS, [c for c in commands if c.enabled],
                     handle_creating_commands=HANDLE_CREATING_COMMANDS,
                     share_lists_commands=SHARE_LISTS_COMMANDS,
                     unreplayed_commands=UNREPLAYED_COMMANDS,
                     platform_guards=PLATFORM_GUARDS)
    mako_write('templates/openGLCommandFactoryAuto.cpp.mako',
               'player/openGLCommandFactoryAuto.cpp', commands=commands,
               custom_player_runners=CUSTOM_PLAYER_RUNNERS,
               platform_guards=PLATFORM_GUARDS)
    mako_write('templates/traceLayerAuto.h.mako',
               'layers/trace/traceLayerAuto.h', commands=commands)
    mako_write('templates/traceLayerAuto.cpp.mako',
               'layers/trace/traceLayerAuto.cpp', commands=commands, enum_groups=enum_groups)
    mako_write('templates/recordingLayerAuto.h.mako',
               'subcapture/recordingLayerAuto.h', commands=commands,
               frame_end_commands=_SUBCAPTURE_FRAME_END_COMMANDS)
    mako_write('templates/recordingLayerAuto.cpp.mako',
               'subcapture/recordingLayerAuto.cpp', commands=commands,
               frame_end_commands=_SUBCAPTURE_FRAME_END_COMMANDS,
               list_commands=display_list_commands(commands))
    analyzed = analyzed_object_params(commands)
    mako_write('templates/analyzerLayerAuto.h.mako',
               'subcapture/analyzerLayerAuto.h', analyzed=analyzed,
               frame_end_commands=_SUBCAPTURE_FRAME_END_COMMANDS)
    mako_write('templates/analyzerLayerAuto.cpp.mako',
               'subcapture/analyzerLayerAuto.cpp', analyzed=analyzed)
