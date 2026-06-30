#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file, get_define
import sys

bitmasks_dict = {}

def print_members(name, members, bitmasks):
    global bitmasks_dict
    if not bitmasks_dict:
        bitmasks_dict = {b.flag_name: b for b in bitmasks}
    
    str = f'  stream << "{name}{{";\n'
    for i, member in enumerate(members):
        is_last = (i == (len(members) - 1))
        separator = '' if is_last else ' << ", "'
        memberVal = f'value.{member.name}'
        
        bitmask = bitmasks_dict.get(member.base_type)
        
        if bitmask is not None:
            str += f'  Print{bitmask.flag_name}(stream, value.{member.name}){separator};\n'
        elif member.base_type == 'char' and member.is_pointer:
            str += f'  PrintString(stream, value.{member.name}){separator};\n'
        elif member.base_type == 'char' and member.is_pointer_to_pointer:
            str += f'  PrintStringArray(stream, value.{member.length}, value.{member.name}){separator};\n'
        elif len(member.fixed_array_size) == 1:
            str += f'  PrintStaticArray(stream, value.{member.name}){separator};\n'
        elif len(member.fixed_array_size) == 2:
            str += f'  PrintStatic2DArray(stream, value.{member.name}){separator};\n'
        else:
            str += f'  stream << {memberVal}{separator};\n'
    str += '  stream << "}";\n'
    return str

def print_struct_members(struct, bitmasks):
    return print_members(struct.name, struct.members, bitmasks)

def print_union_members(union, bitmasks):
    return print_members(union.name, union.members, bitmasks)

def generate_trace_files(context, out_path):
    structs_with_custom_print = [
        'VkDependencyInfo',
        'VkGraphicsPipelineCreateInfo',
        'VkPipelineColorBlendStateCreateInfo',
        'VkPipelineDynamicStateCreateInfo',
        'VkPipelineVertexInputStateCreateInfo',
        'VkRenderPassBeginInfo',
        'VkSubmitInfo',
        'VkWriteDescriptorSet',
    ]
    additional_context = {
        'print_struct_members': print_struct_members,
        'print_union_members': print_union_members,
        'structs_with_custom_print': structs_with_custom_print,
        'get_define': get_define,
    }
    files_to_generate = [
      'traceLayerAuto.h',
      'traceLayerAuto.cpp',
      'enumToStrAuto.h',
      'enumToStrAuto.cpp',
      'printBitmasksAuto.h',
      'printBitmasksAuto.cpp',
      'printEnumsAuto.h',
      'printEnumsAuto.cpp',
      'printUnionsAuto.h',
      'printUnionsAuto.cpp',
      'printStructuresAuto.h',
      'printStructuresAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
