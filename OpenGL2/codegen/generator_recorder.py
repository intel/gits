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
    MAPPED_HANDLE_SERVICE_NAMES,
    SHARE_LISTS_COMMANDS,
)
from intermediates import Command

# HANDLE_CREATING_COMMANDS/HANDLE_PARAM_TO_ERASE_ON_SUCCESS are already unioned across every
# mapped-handle type (HGLRC, GLsync, ...) by handle_lifecycle.py itself - see that module's own
# header comment for why a single generic table, not a per-consumer union of per-type modules,
# is the right shape now that a second mapped-handle type has actually shown up.


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
               mapped_handle_service_names=MAPPED_HANDLE_SERVICE_NAMES,
               context_share_param=CONTEXT_SHARE_PARAM,
               share_lists_commands=SHARE_LISTS_COMMANDS)
    # EncoderLayer (docs/development/opengl2-layer-architecture-alignment-plan.md): the sole,
    # last-registered Post layer, dispatched through by the wrapper above via its
    # GetPostLayers() loop instead of calling CaptureManager::RecordCommand() inline.
    mako_write('templates/encoderLayerAuto.h.mako',
               'recorder/encoderLayerAuto.h', commands=commands)
    mako_write('templates/encoderLayerAuto.cpp.mako',
               'recorder/encoderLayerAuto.cpp', commands=commands,
               frame_end_commands=FRAME_END_COMMANDS)