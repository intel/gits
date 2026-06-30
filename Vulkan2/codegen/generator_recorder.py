#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file
from generator_coders import generate_child_handle_keys, collect_pnext_handle_structs, collect_structs_needing_handle_updater, CUSTOM_HANDLE_STRUCTS

def generate_recorder_files(context, output_path):
    additional_context = {
      'generate_child_handle_keys': generate_child_handle_keys,
      'collect_pnext_handle_structs': collect_pnext_handle_structs,
      'collect_structs_needing_handle_updater': collect_structs_needing_handle_updater,
      'custom_handle_structs': CUSTOM_HANDLE_STRUCTS
    }
    files_to_generate = [
      'wrappersAuto.h',
      'wrappersAuto.cpp',
      'encoderLayerAuto.h',
      'encoderLayerAuto.cpp',
      'handleArgumentUpdatersAuto.h',
      'handleArgumentUpdatersAuto.cpp',
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, output_path)
