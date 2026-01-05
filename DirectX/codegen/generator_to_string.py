#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================
from generator_helpers import *

def generate_to_string_files(context, out_path):
    files_to_generate = [
        'guidToStrAuto.h',
        'guidToStrAuto.cpp',
        'enumToStrAuto.h',
        'enumToStrAuto.cpp',
    ]

    for file_name in files_to_generate:
        generate_file(context, file_name, out_path)