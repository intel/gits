#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from collections.abc import Iterable

from generator_helpers import mako_write
from intermediates import Api, Command, EnumValue
from pname_value_counts import multi_value_pname_cases, validate_pname_groups


def generate_pname_value_count_file(enums_by_api: dict[Api, list[EnumValue]]) -> None:
    """Generate ComputePnameValueCount() - see pname_value_counts.py."""
    validate_pname_groups(enums_by_api)
    mako_write('templates/pnameValueCountsAuto.cpp.mako',
               'common/layer_interface/pnameValueCountsAuto.cpp',
               cases=multi_value_pname_cases(enums_by_api))


def generate_layer_interface_files(
    commands: Iterable[Command], command_ids: dict[str, dict[str, int]]
) -> None:
    """Generate the merged command ID enum and typed Command/Serializer classes.

    Only commands with `enabled=True` get a typed class; everything else keeps going
    through UnsupportedCommand (see recorder/unsupportedCommand.h).
    """
    command_list = list(commands)
    mako_write('templates/commandIdsAuto.h.mako',
               'common/layer_interface/commandIdsAuto.h', command_ids=command_ids)
    for name in ('commandsAuto.h', 'layerAuto.h', 'commandCodersAuto.h', 'commandCodersAuto.cpp',
                 'commandSerializersAuto.h', 'commandSerializersAuto.cpp'):
        mako_write(f'templates/{name}.mako', f'common/layer_interface/{name}',
                   commands=command_list)
