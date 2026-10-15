#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Commands that can sample textures: draws, compute dispatches, and the fixed-function calls that
# rasterize textured fragments. The player runs MipmapCompletionService::BeforeSampling() right
# before each one and the hook from sampling_hook_after() right after it
# (commandRunnersAuto.cpp.mako), so the workaround sees the texture state the sampling actually
# uses, however that state was reached, and leaves none of its own behind. glBegin stands in for
# the vertices of its Begin/End block, where state queries and changes are not allowed, and glEnd
# closes it. A false positive only costs a few state queries; a missed command samples without
# the workaround.

import re

_SAMPLING_RE = re.compile(
    r'^gl(?:'
    r'(?:Multi)?Draw(?!Buffers?(?:[A-Z]|$))'  # Every draw, but not glDrawBuffer(s).
    r'|DispatchCompute'
    r'|Rect[dfis]v?$'
    r'|Begin$'
    r'|CallLists?$'
    r'|Bitmap$'
    r'|CopyPixels$'
    r'|EvalMesh[12]$'
    r')')


def samples_textures(cmd_name: str) -> bool:
    return _SAMPLING_RE.match(cmd_name) is not None


def sampling_hook_before(cmd_name: str) -> str | None:
    """The MipmapCompletionService call (over the runner's `command`) right before `cmd_name`'s
    real call, if any. Executing a display list also fixes the textures the list binds."""
    if cmd_name == 'glCallList':
        return 'BeforeCallList(command.m_list.Value)'
    if cmd_name == 'glCallLists':
        return ('BeforeCallLists(command.m_n.Value, command.m_type.Value, '
                'command.m_lists.HasValue ? command.m_lists.Data.data() : nullptr, '
                'command.m_lists.Data.size())')
    return 'BeforeSampling()' if samples_textures(cmd_name) else None


def sampling_hook_after(cmd_name: str) -> str | None:
    """The MipmapCompletionService call right after `cmd_name`'s real call, if any. glNewList
    and glEndList have custom runners (custom_player_runners.py) instead."""
    if cmd_name == 'glBegin':
        return 'EnterBeginEnd()'
    if cmd_name == 'glEnd':
        return 'LeaveBeginEnd()'
    return 'AfterSampling()' if samples_textures(cmd_name) else None
