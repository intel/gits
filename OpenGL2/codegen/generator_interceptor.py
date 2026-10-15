#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Generates files related to the interceptor DLL.

from dataclasses import replace

from generator_helpers import mako_write
from intermediates import Api, CmdType, Command, Parameter, ReturnValue

# Real opengl32.dll exports these under their "wgl"-prefixed name, but wgl.xml
# lists them only under their unprefixed GDI32-style names. Each is handled by
# cloning the signature of the matching unprefixed command and renaming it,
# rather than writing a signature by hand.
_WGL_RENAMED_FROM_UNPREFIXED = [
    ('wglChoosePixelFormat', 'ChoosePixelFormat'),
    ('wglDescribePixelFormat', 'DescribePixelFormat'),
    ('wglGetPixelFormat', 'GetPixelFormat'),
    ('wglSetPixelFormat', 'SetPixelFormat'),
    ('wglSwapBuffers', 'SwapBuffers'),
]

# wgl.xml contains these, but they're not exported by opengl32.dll.
_WGL_NOT_REAL_EXPORTS = {
    # Win32 API convention is to export only the 'A'/'W'-suffixed variants
    # (wglUseFontBitmapsA/W, wglUseFontOutlinesA/W), and <windows.h> #defines
    # the unsuffixed name as a compile-time macro to one of those two.
    # Generating an unsuffixed trampoline named e.g. "wglUseFontBitmaps" would
    # get silently macro-expanded by the preprocessor into "wglUseFontBitmapsA",
    # colliding with that command's own (separately generated) trampoline.
    'wglUseFontBitmaps',
    'wglUseFontOutlines',
    # gdi32.dll functions, present in wgl.xml presumably for reference.
    # *PixelFormat/SwapBuffers need their signature cloned for the wgl-prefixed
    # names above (see _WGL_RENAMED_FROM_UNPREFIXED).
    'ChoosePixelFormat',
    'DescribePixelFormat',
    'GetPixelFormat',
    'SetPixelFormat',
    'SwapBuffers',
    'GetEnhMetaFilePixelFormat',
}


# TODO: These two functions for adding a synthetic command are a temporary
# hack. We will have a better solution for this in the future.

def _make_wgl_swap_multiple_buffers() -> Command:
    """Returns wglSwapMultipleBuffers as a Command.

    wglSwapMultipleBuffers is exported by opengl32.dll, but not present in
    wgl.xml, not even under an unprefixed name. We have to add it manually.
    The signature is:
    `WINGDIAPI DWORD WINAPI wglSwapMultipleBuffers(UINT n, const WGLSWAP* ps)`.
    """
    return Command(
        name='wglSwapMultipleBuffers',
        enabled=False,
        command_type=CmdType.NONE,
        comment='Absent from wgl.xml, but exported by OpenGL32.dll.',
        return_value=ReturnValue(type='DWORD'),
        params=[
            Parameter(name='n', type='UINT'),
            Parameter(name='pBuffers', type='const WGLSWAP*'),
        ],
    )


def _get_synthetic_wgl_commands(wgl_commands: list[Command]) -> list[Command]:
    """Return additional WGL Command entries for real opengl32.dll exports
    that can't be found under their real name in the parsed WGL command list
    (see comments above). All of these have a real driver symbol to bind
    (matching their own Command.name) - unlike a hypothetical future
    internal-only synthetic command (not needed yet), which would need a
    trampoline/logging entry but no dispatch-table member and no .def export.
    """
    synthetic = []

    # Renamed commands.
    by_name = {c.name: c for c in wgl_commands}
    for new_name, source_name in _WGL_RENAMED_FROM_UNPREFIXED:
        source = by_name.get(source_name)
        if source is None:
            raise KeyError(
                f'Expected to find "{source_name}" among parsed WGL commands to clone as '
                f'"{new_name}", but it was not found. Did wgl.xml change?')
        synthetic.append(replace(source, name=new_name))

    # Completely synthetic commands.
    synthetic.append(_make_wgl_swap_multiple_buffers())

    return synthetic


def generate_interceptor_files(commands: dict[Api, list[Command]]) -> None:
    """Generate all of the interceptor's generated sources from a single,
    shared command list (see module docstring).
    """
    wgl_commands = [c for c in commands[Api.WGL] if c.name not in _WGL_NOT_REAL_EXPORTS]
    # Pass unfiltered WGL commands, since this function needs the signatures of some
    # GDI commands that _WGL_NOT_REAL_EXPORTS filters out (e.g. wglChoosePixelFormat
    # is cloned from ChoosePixelFormat, which is getting filtered).
    wgl_commands += _get_synthetic_wgl_commands(commands[Api.WGL])
    all_commands = commands[Api.GL] + wgl_commands

    # wglGetProcAddress gets a hand-written override instead of a generated
    # trampoline (see interceptor/interceptorCustom.cpp) - it needs to hand
    # out our own trampolines instead of raw real driver pointers, wherever
    # we have one, which the uniform generated trampoline shape can't do.
    # It's still part of the dispatch table and .def export list below
    # (unchanged - our override still needs drv.wglGetProcAddress, and the
    # symbol is still exported, just backed by the override instead).
    trampoline_commands = [c for c in all_commands if c.name != 'wglGetProcAddress']

    mako_write('templates/interceptorAuto.cpp.mako', 'interceptor/interceptorAuto.cpp',
               commands=trampoline_commands)
    mako_write('templates/openglDriversAuto.h.mako', 'interceptor/openglDriversAuto.h',
               commands=all_commands)
    mako_write('templates/openglDriversAuto.cpp.mako', 'interceptor/openglDriversAuto.cpp',
               commands=all_commands)
    mako_write('templates/interceptorAuto.def.mako', 'interceptor/interceptorAuto.def',
               commands=all_commands)
