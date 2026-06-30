#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file

def generate_player_files(context, output_path):
    files_to_generate = [
      'dispatchTableAuto.h',
      'vulkanLibrary2Auto.cpp',
      'commandPlayersAuto.h',
      'commandPlayersAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context, file_name, output_path)
