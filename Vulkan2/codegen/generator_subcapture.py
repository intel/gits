#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os
from generator_helpers import generate_file
from generator_coders import collect_pnext_handle_structs, collect_structs_needing_handle_updater, CUSTOM_HANDLE_STRUCTS

# Commands that have a custom implementation in recordingLayerCustom.cpp and
# therefore must not be auto-generated in recordingLayerAuto.cpp.
RECORDING_LAYER_CUSTOM_COMMANDS = [
    'vkQueuePresentKHR',
]

def generate_subcapture_files(context, output_path):
    additional_context = {
      'collect_pnext_handle_structs': collect_pnext_handle_structs,
      'collect_structs_needing_handle_updater': collect_structs_needing_handle_updater,
      'custom_handle_structs': CUSTOM_HANDLE_STRUCTS,
      'recording_layer_custom_commands': RECORDING_LAYER_CUSTOM_COMMANDS,
    }
    os.makedirs(output_path, exist_ok=True)
    files_to_generate = [
      'recordingLayerAuto.h',
      'recordingLayerAuto.cpp',
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, output_path)
