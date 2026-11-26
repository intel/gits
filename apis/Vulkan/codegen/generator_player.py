#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file, generate_file_parts
from generator_coders import collect_pnext_handle_structs, collect_structs_needing_handle_updater, generate_child_handle_resolve, entries_need_handle_data, CUSTOM_HANDLE_STRUCTS

# The command runners are split into this many files. Changing it also requires changing the
# commandRunnersNAuto.cpp list in apis/Vulkan/player/CMakeLists.txt, which the build does not
# derive.
COMMAND_RUNNER_PARTS = 4

def generate_player_files(context, output_path):
    additional_context = {
      'collect_pnext_handle_structs': collect_pnext_handle_structs,
      'collect_structs_needing_handle_updater': collect_structs_needing_handle_updater,
      'generate_child_handle_resolve': generate_child_handle_resolve,
      'entries_need_handle_data': entries_need_handle_data,
      'custom_handle_structs': CUSTOM_HANDLE_STRUCTS
    }
    files_to_generate = [
      'commandRunnersAuto.h',
      'vulkanCommandFactoryAuto.cpp',
      'handleArgumentUpdatersPlayerAuto.h',
      'handleArgumentUpdatersPlayerAuto.cpp',
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, output_path)
    # The runners are split into several files, each with a part of the commands, so that they
    # compile in parallel.
    generate_file_parts(context | additional_context, 'commandRunnersAuto.cpp', output_path,
                        'commands', COMMAND_RUNNER_PARTS)
