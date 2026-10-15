#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import mako_write
from handle_lifecycle import (
    CONTEXT_SHARE_PARAM,
    FRAME_END_COMMANDS,
    HANDLE_CREATING_COMMANDS,
    HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
    SHARE_LISTS_COMMANDS,
)
from intermediates import Command


def generate_recorder_files(commands: list[Command]) -> None:
    """Generate forwarding wrappers and real-driver dispatch for interceptor commands."""
    mako_write('templates/openglDriversAuto.h.mako',
               'recorder/include/openglDriversAuto.h', commands=commands)
    mako_write('templates/openglDriversAuto.cpp.mako',
               'recorder/openglDriversAuto.cpp', commands=commands)
    mako_write('templates/recorderWrappersAuto.cpp.mako',
               'recorder/recorderWrappersAuto.cpp', commands=commands,
               handle_creating_commands=HANDLE_CREATING_COMMANDS,
               handle_param_to_erase_on_success=HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
               context_share_param=CONTEXT_SHARE_PARAM,
               share_lists_commands=SHARE_LISTS_COMMANDS,
               frame_end_commands=FRAME_END_COMMANDS)