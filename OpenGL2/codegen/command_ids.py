#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import json
from collections.abc import Iterable
from pathlib import Path
from typing import TypedDict, cast

from intermediates import Command

ID_META_BEGIN = 15 * 0x10000
ID_OPENGL_BEGIN = ID_META_BEGIN + 0x500


class CommandIdAllocations(TypedDict):
    """Persistent, append-only command ID allocation table."""

    command_count: int
    command_ids: dict[str, int]


def _command_id_name(command: Command) -> str:
    """Return command identifier used as key in the allocation table."""

    return f'ID_{command.name.upper()}'


def _load_command_ids(allocations_path: Path) -> CommandIdAllocations:
    """Load and validate persistent command ID allocations."""

    with allocations_path.open(encoding='utf-8') as allocations_file:
        allocations = json.load(allocations_file)

    if set(allocations) != {'command_count', 'command_ids'}:
        raise ValueError(f'{allocations_path} has an invalid top-level structure')

    command_count = allocations['command_count']
    command_ids = allocations['command_ids']
    if type(command_count) is not int or command_count < 0:
        raise ValueError(f'{allocations_path} has an invalid command_count')
    if not isinstance(command_ids, dict):
        raise TypeError(f'{allocations_path} has invalid command_ids')
    unvalidated_command_ids = cast(dict[object, object], command_ids)
    if not all(
        isinstance(name, str) and type(command_id) is int
        for name, command_id in unvalidated_command_ids.items()
    ):
        raise ValueError(f'{allocations_path} has invalid command ID entries')
    validated_command_ids = cast(dict[str, int], unvalidated_command_ids)

    expected_ids = set(range(ID_OPENGL_BEGIN, ID_OPENGL_BEGIN + command_count))
    assigned_ids = set(validated_command_ids.values())
    if len(validated_command_ids) != command_count or assigned_ids != expected_ids:
        raise ValueError(f'{allocations_path} command IDs are not a contiguous append-only sequence')

    return {'command_count': command_count, 'command_ids': validated_command_ids}


def build_command_ids(commands: Iterable[Command], allocations_path: Path) -> dict[str, int]:
    """Append IDs for enabled commands and return all persisted command IDs."""

    allocations = _load_command_ids(allocations_path)
    command_count = allocations['command_count']
    command_ids = allocations['command_ids']

    enabled_names = [_command_id_name(command) for command in commands if command.enabled]
    if len(enabled_names) != len(set(enabled_names)):
        raise ValueError('Enabled commands produce duplicate command ID names')

    changed = False
    for command_name in enabled_names:
        if command_name in command_ids:
            continue
        command_ids[command_name] = ID_OPENGL_BEGIN + command_count
        command_count += 1
        changed = True

    if changed:
        allocations['command_count'] = command_count
        with allocations_path.open('w', encoding='utf-8', newline='\n') as allocations_file:
            json.dump(allocations, allocations_file, indent=4)
            allocations_file.write('\n')

    return command_ids