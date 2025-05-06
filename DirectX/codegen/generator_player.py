#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *
from generator_xess_helpers import *

def command_runner_call_parameters(function, indent):
    str = ''
    first = True
    for param in function.params:
        if not first:
            str += ',\n' + indent
        first = False
        str += 'command.' + param.name + '_.value'
    str += ");"
    return str

def generate_player_files(context, out_path):
    additional_context = { 
        'command_runner_call_parameters': command_runner_call_parameters,
        'is_xess_function': is_xess_function
    }
    files_to_generate = [
        'commandPlayersAuto.h',
        'commandPlayersAuto.cpp',
        'directXLibraryAuto.cpp',
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
