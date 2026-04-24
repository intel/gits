#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *

def command_encoders_sum_sizes(function, isObject):
    str = 'GetSize(command.Key) + GetSize(command.ThreadId)'
    if isObject:
        str += ' + GetSize(command.m_Object.Key)'
    for param in function.params:
        str += ' + GetSize(command.m_' + param.name + ')'
    if not function.ret.is_void:
        str += ' + GetSize(command.m_Result)'
    str += ';'
    return str

def generate_coders_files(context, out_path):
    additional_context = {
        'command_encoders_sum_sizes': command_encoders_sum_sizes
    }
    files_to_generate = [
        'include/commandEncodersAuto.h',
        'commandEncodersAuto.cpp',
        'include/commandDecodersAuto.h',
        'commandDecodersAuto.cpp',
        'include/commandSerializersAuto.h',
        'commandSerializersFactoryAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
