#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================
from generator_helpers import *
from intermediates import Parameter, Function, Struct
from typing import Union

def generate_ccode_project_files(context, out_path):
    additional_context = {}
    files_to_generate = [
        'ccodeApiWrappersAuto.h',
        'ccodeApiWrappersAuto.cpp',
    ]

    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)