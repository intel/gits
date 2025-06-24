#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *

def command_encoders_sum_sizes(function, isObject, indent):
    str = 'getSize(command.key) + getSize(command.threadId)'
    if isObject:
        str += ' + getSize(command.object_.key)'
    for param in function.params:
        str += ' +\n' + indent + 'getSize(command.' + param.name + '_)'
    if not function.ret.is_void:
        str += ' +\n' + indent + 'getSize(command.result_)'
    str += ';'
    return str

def generate_coders_files(context, out_path):
    additional_context = {
        'command_encoders_sum_sizes': command_encoders_sum_sizes
    }
    files_to_generate = [
        'commandEncodersAuto.h',
        'commandEncodersAuto.cpp',
        'commandDecodersAuto.h',
        'commandDecodersAuto.cpp',
        'commandWritersAuto.h',
        'commandWritersFactoryAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
