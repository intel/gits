#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *

def generate_subcapture_files(context, out_path):
    files_to_generate = [
        'recordingLayerAuto.h',
        'recordingLayerAuto.cpp',
        'analyzerLayerAuto.h',
        'analyzerLayerAuto.cpp'
    ]
    
    for file_name in files_to_generate:
        generate_file(context, file_name, out_path)
