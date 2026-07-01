#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import json
import os

ID_META_BEGIN = 14 * 0x10000
ID_VK_BEGIN = ID_META_BEGIN + 0x500

def cache_command(cache, cmd_name):
    if cmd_name in cache['command_ids']:
        return

    cmd_id = ID_VK_BEGIN + cache['command_count']
    cache['command_count'] += 1
    cache['command_ids'][cmd_name] = cmd_id

def build_command_ids(context, out_path):
    command_ids_file = os.path.join(out_path, 'command_ids.json')
    command_ids_cache = {}

    if not os.path.exists(command_ids_file):
        print(f"{command_ids_file} not found. Cannot generate Command IDs!")
        exit(-1)

    with open(command_ids_file, 'r') as f:
        command_ids_cache = json.load(f)

    for command in context['commands']:
        cmd_name = f"ID_{command.name.upper()}"
        cache_command(command_ids_cache, cmd_name)

    with open(command_ids_file, 'w') as f:
        json.dump(command_ids_cache, f, indent = 4)

    return command_ids_cache['command_ids']
