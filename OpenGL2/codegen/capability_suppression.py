#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Commands through which the recorder hides capabilities from the application
# (recorder/include/capabilitySuppression.h): the extensions in OpenGL.Recorder.SuppressExtensions,
# and, with OpenGL.Recorder.SuppressProgramBinary, binaries only the recording driver can load, so
# the application compiles its shaders from source. See
# docs/development/modern-opengl-backend-architecture.md's capability suppression decision.
#
# Hidden results are never recorded: query results are discarded, and string results are not
# stored. A skipped binary load is not recorded at all.

from intermediates import Command

# State queries the recorder may answer itself: extension counts and binary formats.
STATE_QUERIES: frozenset[str] = frozenset({
    'glGetBooleanv', 'glGetIntegerv', 'glGetInteger64v', 'glGetFloatv', 'glGetDoublev',
})

# Binary loads, by the C++ condition under which the wrapper returns before executing or recording
# them: a format the queries above hide. A program whose binary is not loaded stays unlinked, as
# one the driver rejects, so the application builds it from source. SPIR-V shader binaries are
# portable and still loaded.
BINARY_LOAD_SKIPS: dict[str, str] = {
    'glProgramBinary': 'SkipsProgramBinary(binaryFormat)',
    'glProgramBinaryOES': 'SkipsProgramBinary(binaryFormat)',
    'glShaderBinary': 'SkipsShaderBinary(binaryFormat)',
}

# Arguments the driver gets in place of the application's: (parameter, C++ replacement). An index
# into the filtered extension list selects the same extension in the driver's.
ARGUMENT_REMAPS: dict[str, tuple[str, str]] = {
    'glGetStringi': ('index', 'DriverExtensionIndex(name, index)'),
}

# Results the application gets in place of the driver's, by C++ replacement of `result`.
RESULT_FILTERS: dict[str, str] = {
    'glGetString': 'HideExtensions(name, result)',
    'wglGetExtensionsStringARB': 'HideExtensions(result)',
    'wglGetExtensionsStringEXT': 'HideExtensions(result)',
}


def _by_name(commands: list[Command], names, kind: str) -> dict[str, Command]:
    found = {cmd.name: cmd for cmd in commands if cmd.name in names}
    missing = set(names) - found.keys()
    if missing:
        raise ValueError(f"{kind} not found: {sorted(missing)}")
    return found


def _params(cmd: Command) -> list[str]:
    return [p.name for p in cmd.params]


def state_queries(commands: list[Command]) -> frozenset[str]:
    """STATE_QUERIES, checked to take the `pname` and `data` the hook writes through."""
    for cmd in _by_name(commands, STATE_QUERIES, 'state queries').values():
        if _params(cmd) != ['pname', 'data']:
            raise ValueError(f"{cmd.name} does not take (pname, data)")
    return STATE_QUERIES


def binary_load_skips(commands: list[Command]) -> dict[str, str]:
    """BINARY_LOAD_SKIPS, checked to name commands that take a `binaryFormat`."""
    for cmd in _by_name(commands, BINARY_LOAD_SKIPS, 'binary loads').values():
        if 'binaryFormat' not in _params(cmd):
            raise ValueError(f"{cmd.name} takes no binaryFormat")
    return BINARY_LOAD_SKIPS


def argument_remaps(commands: list[Command]) -> dict[str, tuple[str, str]]:
    """ARGUMENT_REMAPS, checked to name parameters the commands take."""
    for cmd in _by_name(commands, ARGUMENT_REMAPS, 'argument remaps').values():
        if ARGUMENT_REMAPS[cmd.name][0] not in _params(cmd):
            raise ValueError(f"{cmd.name} takes no {ARGUMENT_REMAPS[cmd.name][0]}")
    return ARGUMENT_REMAPS


def result_filters(commands: list[Command]) -> dict[str, str]:
    """RESULT_FILTERS, checked to name commands returning a string."""
    for cmd in _by_name(commands, RESULT_FILTERS, 'result filters').values():
        if cmd.return_value.type.replace(' ', '') not in ('constGLubyte*', 'constchar*'):
            raise ValueError(f"{cmd.name} does not return a string")
    return RESULT_FILTERS
