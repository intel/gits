#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *

def generate_resource_dumping_files(context, out_path):
    files_to_generate = [
        'resourceDumpLayerAuto.h',
        'resourceDumpLayerAuto.cpp'
    ]
    
    for file_name in files_to_generate:
        generate_file(context, file_name, out_path)
