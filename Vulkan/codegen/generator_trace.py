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

def member_name(member) -> str:
    """Bare member name with any array suffix from the registry name node removed."""
    return re.sub(r'\s*\[[^\]]*\]', '', member.name)

def is_pointer_to_array(member) -> bool:
    """True when full_type is a pointer to array syntax, not a built-in array member."""
    return bool(re.search(r'\(\s*\*\s*[A-Za-z_]\w*\s*\)', member.full_type))

# Scalar types (and vk.xml aliases) that trace can print element-wise via PrintArray.
# Opaque or externally defined aggregate types are excluded until added below via registry walk.
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

# Types with a generated Print<Type> printer, and types which only the generic FastOStream
# operator<< prints. Both are filled in by generate_trace_files.
named_print_types = set()
generic_print_types = set()

def print_value(expr, member):
    """Call printing expr of the member's type, picking the printer stream << would pick.

    A plain stream << is resolved against every operator<< generated for Vulkan types, which
    makes the printer sources very slow to compile, so it is only used for types which no named
    printer covers.
    """
    if is_pointer_to_array(member):
        return f'stream << {expr}'
    if member.base_type in named_print_types and not member.is_pointer_to_pointer \
            and not member.is_triple_pointer:
        # The generated printers take a const pointer, so for a pointer to a non-const value
        # stream << picks the generic operator<<, which prints only the address. These are output
        # members, which are not initialized yet when printed before the call.
        if member.is_pointer and not member.is_const:
            return f'PrintValue(stream, {expr})'
        return f'Print{member.base_type}(stream, {expr})'
    if member.is_pointer_to_pointer or member.is_triple_pointer \
            or member.base_type in generic_print_types or member.base_type.startswith('PFN_'):
        return f'PrintValue(stream, {expr})'
    return f'stream << {expr}'

class StatementWriter:
    """Collects printing statements, merging adjacent literals into a single PrintText call."""

    def __init__(self):
        self.code = ''
        self.text = ''

    def add_text(self, text):
        self.text += text

    def add_statement(self, statement):
        self.flush_text()
        self.code += f'  {statement};\n'

    def add_code(self, code):
        self.flush_text()
        self.code += code

    def flush_text(self):
        if self.text:
            self.code += f'  PrintText(stream, "{self.text}");\n'
            self.text = ''

    def get_code(self):
        self.flush_text()
        return self.code

def print_inline_struct(writer, access, nested_members):
    """Emit member-wise printing for an anonymous inline struct or union nested in full_type."""
    writer.add_text('{')
    for i, nested in enumerate(nested_members):
        nested_access = f'{access}.{nested.name}' if nested.name else access
        if nested.is_inline_struct:
            print_inline_struct(writer, nested_access, nested.nested_members)
        elif nested.is_pointer or nested.is_pointer_to_pointer or nested.is_triple_pointer:
            writer.add_statement(print_value(nested_access, nested))
        elif len(nested.fixed_array_size) == 1:
            writer.add_statement(f'PrintStaticArray(stream, {nested_access})')
        elif len(nested.fixed_array_size) == 2:
            writer.add_statement(f'PrintStatic2DArray(stream, {nested_access})')
        elif nested.bitfield is not None:
            writer.add_statement(
                print_value(f'static_cast<{nested.base_type}>({nested_access})', nested))
        else:
            writer.add_statement(print_value(nested_access, nested))
        if i != len(nested_members) - 1:
            writer.add_text(', ')
    writer.add_text('}')

def print_members(name, members, bitmasks):
    global bitmasks_dict
    if not bitmasks_dict:
        bitmasks_dict = {b.flag_name: b for b in bitmasks}

    writer = StatementWriter()
    writer.add_text(f'{name}{{')
    for i, member in enumerate(members):
        accessName = member_name(member)
        memberVal = f'value.{accessName}' if accessName else 'value'

        bitmask = bitmasks_dict.get(member.base_type)

        if accessName == 'pNext' and member.is_void and member.is_pointer:
            writer.add_statement('PrintPNext(stream, value.pNext)')
        elif member.is_inline_struct:
            access = memberVal if accessName else 'value'
            print_inline_struct(writer, access, member.nested_members)
        elif bitmask is not None:
            if member.is_pointer:
                length = '1'
                if member.length:
                    length = f'value.{member.length}'
                writer.add_statement(f'Print{bitmask.flag_name}(stream, {length}, {memberVal})')
            else:
                writer.add_statement(f'Print{bitmask.flag_name}(stream, {memberVal})')
        elif member.base_type == 'char' and member.is_pointer:
            writer.add_statement(f'PrintString(stream, {memberVal})')
        elif member.base_type == 'char' and member.is_pointer_to_pointer:
            writer.add_statement(f'PrintStringArray(stream, value.{member.length}, {memberVal})')
        elif len(member.fixed_array_size) == 1 and not is_pointer_to_array(member):
            writer.add_statement(f'PrintStaticArray(stream, {memberVal})')
        elif len(member.fixed_array_size) == 2 and not is_pointer_to_array(member):
            writer.add_statement(f'PrintStatic2DArray(stream, {memberVal})')
        elif member.length != '' and is_printable_pointer(member):
            writer.add_statement(
                f'PrintArray(stream, {get_array_len_expr(member)}, {memberVal})')
        elif member.length != '':
            len_expr = get_array_len_expr(member)
            writer.add_code(
                f'  if ({len_expr} > 0) {{\n'
                f'    {print_value(memberVal, member)};\n'
                f'  }}\n'
                f'  else {{\n'
                f'    PrintText(stream, "nullptr");\n'
                f'  }}\n'
            )
        elif member.bitfield is not None:
            writer.add_statement(
                print_value(f'static_cast<{member.base_type}>({memberVal})', member))
        else:
            writer.add_statement(print_value(memberVal, member))
        if i != len(members) - 1:
            writer.add_text(', ')
    writer.add_text('}')
    return writer.get_code()

def print_pnext_struct(structure):
    value = f'*reinterpret_cast<const {structure.name}*>(pNext)'
    if structure.name in named_print_types:
        return f'      Print{structure.name}(stream, {value});\n'
    return f'      stream << {value};\n'

def print_struct_members(struct, bitmasks):
    return print_members(struct.name, struct.members, bitmasks)

def print_union_members(union, bitmasks):
    return print_members(union.name, union.members, bitmasks)

def generate_trace_files(context, out_path):
    generic_print_types.update(
        printable_types
        | {'void', 'VkFlags', 'VkFlags64'}
        | {handle.name for handle in context['handles']}
        | {bitmask.name for bitmask in context['bitmasks']}
        | {bitmask.flag_name for bitmask in context['bitmasks']}
        | {flag.name for flag in context['flags']}
    )
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
    named_print_types.update(
        {structure.name for structure in context['structures']
         if structure.name not in structs_with_custom_print}
        | {union.name for union in context['unions']}
        | {enum.name for enum in context['enums']}
    )
    additional_context = {
        'print_struct_members': print_struct_members,
        'print_union_members': print_union_members,
        'print_pnext_struct': print_pnext_struct,
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

