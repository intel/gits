#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file


def generate_api_debug_files(context, out_path):
    files_to_generate = [
        'logVkErrorLayerAuto.h',
        'logVkErrorLayerAuto.cpp',
    ]

    for file_name in files_to_generate:
        generate_file(context, file_name, out_path)
