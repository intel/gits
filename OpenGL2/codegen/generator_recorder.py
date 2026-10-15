#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import mako_write
from intermediates import Command


def generate_recorder_files(commands: list[Command]) -> None:
    """Generate forwarding wrappers and real-driver dispatch for interceptor commands."""
    mako_write('templates/openglDriversAuto.h.mako',
               'recorder/include/openglDriversAuto.h', commands=commands)
    mako_write('templates/openglDriversAuto.cpp.mako',
               'recorder/openglDriversAuto.cpp', commands=commands)
    mako_write('templates/recorderWrappersAuto.cpp.mako',
               'recorder/recorderWrappersAuto.cpp', commands=commands)