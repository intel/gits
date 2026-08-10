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
import re

def is_calculated_len(len_value: str | list) -> bool:
    """
    Returns True if the len expression is arithmetic (calculated),
    i.e. contains any of +, -, *, /
    Example: "(rasterizationSamples + 31) / 32"  -> True
    """
    if not isinstance(len_value, str):
        return False
    return bool(re.search(r"[+\-*/]", len_value))

def is_multi_dimensional_len(len_value: str) -> bool:
    """
    Returns True if the len expression is multi-dimensional.
    Example: "geometryCount,1" -> True
    """
    return bool(re.search(r",", len_value))

def parse_multidimensional_len(len_value: str | list) -> list[str]:
    """
    Parse a vk.xml 'len' attribute into its dimension components.
    Example: "geometryCount,1" -> ["geometryCount", "1"]
    """
    if isinstance(len_value, list):
        return len_value
    return len_value.split(",")

def add_prefix_to_calculated_len(len_value: str, prefix: str = "value.") -> str:
    """
    Prefixes every identifier in an arithmetic len expression with 'value.'.
    Example:"(rasterizationSamples + 31) / 32" ->
            "(value.rasterizationSamples + 31) / 32"
    """
    # Match C identifiers: start with letter or _, followed by letters/digits/_
    identifier_pattern = re.compile(r"\b([a-zA-Z_][a-zA-Z0-9_]*)\b")

    def _add_prefix(match: re.Match) -> str:
        token = match.group(1)
        if token.isdigit():
            return token
        if '_' in token:
            return token
        return prefix + token

    return identifier_pattern.sub(_add_prefix, len_value)

def get_array_len_expr(member) -> str:
    """
    Returns the C++ expression with the number of elements a member points to.
    """
    if is_calculated_len(member.length):
        return add_prefix_to_calculated_len(member.length)
    # Multi-dimensional len in vk.xml arrays of pointers point to a single element.
    # E.g. ("geometryCount,1"). 
    return f'value.{parse_multidimensional_len(member.length)[0]}'

# Contains all vk.xml types that are printable and scalars.
# Other types (e.g. the StdVideo* structures) are not printable.
printable_types = {
    'char', 'int', 'float', 'double', 'size_t', 'int8_t', 'int16_t', 'int32_t',
    'int64_t', 'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'VkBool32',
    'VkDeviceAddress', 'VkDeviceSize', 'VkSampleMask',
}

def is_printable_pointer(member) -> bool:
    if not member.is_pointer and not member.is_pointer_to_pointer:
        return False
    if member.is_void:
        # 'void*' is an opaque blob, but 'void* const*' is an array of addresses.
        return member.is_pointer_to_pointer
    return member.base_type in printable_types

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

        if member.name == 'pNext' and member.is_void and member.is_pointer:
            str += f'  PrintPNext(stream, value.pNext){separator};\n'
        elif bitmask is not None:
            if member.is_pointer:
                length = '1'
                if member.length:
                    length = f'value.{member.length}'
                str += f'  Print{bitmask.flag_name}(stream, {length}, value.{member.name}){separator};\n'
            else:
                str += f'  Print{bitmask.flag_name}(stream, value.{member.name}){separator};\n'
        elif member.base_type == 'char' and member.is_pointer:
            str += f'  PrintString(stream, value.{member.name}){separator};\n'
        elif member.base_type == 'char' and member.is_pointer_to_pointer:
            str += f'  PrintStringArray(stream, value.{member.length}, value.{member.name}){separator};\n'
        elif len(member.fixed_array_size) == 1:
            str += f'  PrintStaticArray(stream, value.{member.name}){separator};\n'
        elif len(member.fixed_array_size) == 2:
            str += f'  PrintStatic2DArray(stream, value.{member.name}){separator};\n'
        elif member.length != '' and is_printable_pointer(member):
            str += f'  PrintArray(stream, {get_array_len_expr(member)}, {memberVal}){separator};\n'
        elif member.length != '':
            len_expr = get_array_len_expr(member)
            str += f'  if ({len_expr} > 0) {{\n'
            str += f'    stream << {memberVal}{separator};\n'
            str += f'  }}\n'
            str += f'  else {{\n'
            str += f'    stream << "nullptr"{separator};\n'
            str += f'  }}\n'
        else:
            str += f'  stream << {memberVal}{separator};\n'
    str += '  stream << "}";\n'
    return str

def print_struct_members(struct, bitmasks):
    return print_members(struct.name, struct.members, bitmasks)

def print_union_members(union, bitmasks):
    return print_members(union.name, union.members, bitmasks)

def generate_trace_files(context, out_path):
    printable_types.update(
        {structure.name for structure in context['structures']}
        | {union.name for union in context['unions']}
        | {enum.name for enum in context['enums']}
        | {handle.name for handle in context['handles']}
        | {bitmask.name for bitmask in context['bitmasks']}
        | {bitmask.flag_name for bitmask in context['bitmasks']}
        | {flag.name for flag in context['flags']}
    )

    structs_with_custom_print = [
      'VkGraphicsPipelineCreateInfo',
      'VkWriteDescriptorSet',
      'VkSubmitInfo',
      'VkShaderModuleCreateInfo'
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
      'printStructuresAuto.cpp',
      'printPnextAuto.h',
      'printPnextAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
