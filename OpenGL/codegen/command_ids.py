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

from intermediates import Api, Command

ID_META_BEGIN = 15 * 0x10000
ID_META_END = ID_META_BEGIN + 0x500

# Per-API reserved ranges within the OpenGL ApiId's single 0x10000-wide command ID block
# (see gits::stream::ApiId::ID_OPENGL in commandId.h) - one contiguous sub-range per
# registry (GL/WGL/EGL/GLX), each with its own append-only counter, mirroring how
# DirectX/codegen/command_ids.py partitions COMMON/DXGI/D3D12/etc. within its own ApiId
# block instead of using one flat namespace like Vulkan (which only has a single API).
#
# Unlike DirectX's uniform 0x500-per-API gap, each range here is sized to its own
# registry's *total* command count (not just the currently-enabled subset), with generous
# headroom for future extensions being added upstream - GL alone has ~3300 registry
# commands (dwarfing DirectX's largest section, D3D12) while WGL/EGL/GLX each have under
# 200, so a uniform gap would either overflow GL's range or waste enormous space on the
# other three. Re-run `python -c "from intermediates_creator import
# load_registries_and_extract_xml_data, FILE_TO_API; ..."` (count len(commands[api]) per
# api) before growing any of these if a registry update ever gets close to its ceiling.
ID_GL_BEGIN = ID_META_END              # 0xF0500; GL registry has ~3300 commands.
ID_WGL_BEGIN = ID_GL_BEGIN + 0x2000    # 0xF2500; room for 8192 GL commands.
ID_EGL_BEGIN = ID_WGL_BEGIN + 0x400    # 0xF2900; room for 1024 WGL commands (registry ~150).
ID_GLX_BEGIN = ID_EGL_BEGIN + 0x400    # 0xF2D00; room for 1024 EGL commands (registry ~160).
ID_OPENGL_END = ID_GLX_BEGIN + 0x400  # 0xF3100; room for 1024 GLX commands (registry ~140).

_API_RANGE_BEGIN: dict[Api, int] = {
    Api.GL: ID_GL_BEGIN,
    Api.WGL: ID_WGL_BEGIN,
    Api.EGL: ID_EGL_BEGIN,
    Api.GLX: ID_GLX_BEGIN,
}


class CommandIdAllocations(TypedDict):
    """Persistent, append-only command ID allocation table, partitioned per API."""

    command_count: dict[str, int]
    command_ids: dict[str, dict[str, int]]


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
    api_names = {api.value for api in Api}
    if not isinstance(command_count, dict) or set(command_count) != api_names:
        raise ValueError(f'{allocations_path} has an invalid command_count')
    if not isinstance(command_ids, dict) or set(command_ids) != api_names:
        raise ValueError(f'{allocations_path} has invalid command_ids')

    for api in Api:
        count = command_count[api.value]
        if type(count) is not int or count < 0:
            raise ValueError(f'{allocations_path} has an invalid command_count for {api.value}')

        api_ids = command_ids[api.value]
        if not isinstance(api_ids, dict):
            raise TypeError(f'{allocations_path} has invalid command_ids for {api.value}')
        unvalidated_api_ids = cast(dict[object, object], api_ids)
        if not all(
            isinstance(name, str) and type(command_id) is int
            for name, command_id in unvalidated_api_ids.items()
        ):
            raise ValueError(f'{allocations_path} has invalid command ID entries for {api.value}')
        validated_api_ids = cast(dict[str, int], unvalidated_api_ids)

        begin = _API_RANGE_BEGIN[api]
        expected_ids = set(range(begin, begin + count))
        assigned_ids = set(validated_api_ids.values())
        if len(validated_api_ids) != count or assigned_ids != expected_ids:
            raise ValueError(
                f'{allocations_path} command IDs for {api.value} are not a contiguous '
                f'append-only sequence')

    return cast(CommandIdAllocations, {'command_count': command_count, 'command_ids': command_ids})


def build_command_ids(commands: Iterable[Command], allocations_path: Path) -> dict[str, dict[str, int]]:
    """Append IDs for enabled commands, partitioned per API, and return all persisted IDs.

    The returned mapping is keyed by Api.value (e.g. "GL", "WGL") to a name->id mapping for
    that API alone - see commandIdsAuto.h.mako, which renders one contiguous enum section per
    API using this same partitioning.
    """

    allocations = _load_command_ids(allocations_path)
    command_count = allocations['command_count']
    command_ids = allocations['command_ids']

    names_by_api: dict[Api, list[str]] = {api: [] for api in Api}
    for command in commands:
        if command.enabled:
            names_by_api[command.api].append(_command_id_name(command))

    for api, names in names_by_api.items():
        if len(names) != len(set(names)):
            raise ValueError(f'Enabled {api.value} commands produce duplicate command ID names')

    changed = False
    for api, names in names_by_api.items():
        api_ids = command_ids[api.value]
        count = command_count[api.value]
        for command_name in names:
            if command_name in api_ids:
                continue
            api_ids[command_name] = _API_RANGE_BEGIN[api] + count
            count += 1
            changed = True
        command_count[api.value] = count

    if changed:
        with allocations_path.open('w', encoding='utf-8', newline='\n') as allocations_file:
            json.dump(allocations, allocations_file, indent=4)
            allocations_file.write('\n')

    return command_ids
