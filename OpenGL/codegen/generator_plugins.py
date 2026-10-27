#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from pathlib import Path

from generator_helpers import mako_write
from handle_lifecycle import FRAME_END_COMMANDS
from intermediates import Command

_STATISTICS_DIR = Path(__file__).resolve().parents[2] / 'plugins' / 'OpenGL' / 'statistics'


def generate_plugin_files(commands: list[Command]) -> None:
    """Generate the per-command layers of the OpenGL plugins in plugins/OpenGL."""
    for name in ('statisticsLayerAuto.h', 'statisticsLayerAuto.cpp'):
        mako_write(str(_STATISTICS_DIR / 'codegen' / 'templates' / f'{name}.mako'),
                   str(_STATISTICS_DIR / name), commands=commands,
                   frame_end_commands=FRAME_END_COMMANDS)
