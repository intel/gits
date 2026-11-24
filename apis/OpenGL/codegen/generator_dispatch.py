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


def generate_dispatch_files(commands: list[Command]) -> None:
    for extension in ('h', 'cpp'):
        mako_write(f'templates/glDispatchAuto.{extension}.mako',
                   f'common/dispatch/glDispatchAuto.{extension}',
                   commands=commands)
