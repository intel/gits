#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *

def generate_skip_calls_files(context, out_path):
    files_to_generate = [
        'skipCallsOnConfigLayerAuto.h',
        'skipCallsOnConfigLayerAuto.cpp',
        'skipCallsOnResultLayerAuto.h',
        'skipCallsOnResultLayerAuto.cpp',
    ]
    
    for file_name in files_to_generate:
        generate_file(context, file_name, out_path)
