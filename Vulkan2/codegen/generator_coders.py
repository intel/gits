#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re

from generator_helpers import generate_file

def is_complex_struct(base_type, structures_by_name):
    s = structures_by_name.get(base_type)
    if s is None:
        return False

    for m in s.members:
        if m.is_handle:
            return True
        elif m.name == 'pNext':
            return True
        elif m.is_pointer:
            return True
        elif m.is_struct:
            if is_complex_struct(m.base_type, structures_by_name):
                return True

    return False

def is_basic_struct(base_type, structures_by_name):
    s = structures_by_name.get(base_type)
    if s is None:
        return False
    return not is_complex_struct(base_type, structures_by_name)

def struct_needs_coder(structure, structures_list):
    structures_by_name = {s.name: s for s in structures_list}
    for m in structure.members:
        if m.is_handle:
            return True
        elif m.name == 'pNext':
            return True
        elif m.is_pointer:
            return True
        elif is_complex_struct(m.base_type, structures_by_name):
            return True
    return False

def get_inner_count(inner_length, var_name):
    return inner_length if inner_length.isdigit() else f'{var_name}->{inner_length}'

def is_constant(value):
    return bool(re.fullmatch(r'[A-Z0-9_]+', value))

def is_expression(value):
    return bool(re.search(r'[+\-*/()]', value))

def get_length_expression(length, var_name):
    if not is_expression(length):
        if is_constant(length) or length.isdigit():
            return length
        return f'{var_name}->{length}'
    expression = '(' + re.sub(r'[a-zA-Z_]\w*', lambda m: m.group() if is_constant(m.group()) else f'{var_name}->{m.group()}', length) + ')'
    return expression

def get_size_lines(structure, structures_list, var_name):
    structures_by_name = {s.name: s for s in structures_list}
    lines = []

    for member in structure.members:
        complex_struct = is_complex_struct(member.base_type, structures_by_name)
        basic_struct = is_basic_struct(member.base_type, structures_by_name)
        if member.name == 'pNext':
            lines.append(f'blobSize += GetPNextChainSize({var_name}->pNext);')
        elif member.is_handle:
            if member.is_pointer and not member.length:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  blobSize += sizeof(GITSKey);')
                lines.append('}')
            elif member.is_pointer and member.length:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  blobSize += sizeof(GITSKey) * {var_name}->{member.length};')
                lines.append('}')
            elif not member.is_pointer and member.length:
                lines.append(f'blobSize += sizeof(GITSKey) * {var_name}->{member.length};')
            else:
                lines.append(f'if ({var_name}->{member.name} != VK_NULL_HANDLE) {{')
                lines.append('  blobSize += sizeof(GITSKey);')
                lines.append('}')
        elif member.is_handle and member.length:
            lines.append(f'blobSize += sizeof(GITSKey) * {var_name}->{member.length};')
        elif member.is_pointer and member.is_null_terminated:
            lines.append(f'blobSize += GetStringSize({var_name}->{member.name});')
        elif member.is_pointer_to_pointer and member.is_null_terminated:
            lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
            lines.append(f'  blobSize += GetStringArraySize({var_name}->{member.name}, {var_name}->{member.length});')
            lines.append('}')
        elif member.is_pointer and member.base_type == 'void':
            lines.append('blobSize += sizeof(void*);')
        elif complex_struct:
            if member.length and member.is_pointer_to_pointer:
                outer = member.length[0]
                inner = get_inner_count(member.length[1], var_name)
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{outer} > 0) {{')
                lines.append(f'  for (uint32_t j = 0; j < {var_name}->{outer}; ++j) {{')
                lines.append(f'    blobSize += GetSize({var_name}->{member.name}[j], {inner});')
                lines.append('  }')
                lines.append('}')
            elif member.length and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  blobSize += GetSize({var_name}->{member.name}, {var_name}->{member.length});')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  blobSize += GetSize({var_name}->{member.name}, 1);')
                lines.append('}')
        elif basic_struct:
            if member.length and member.is_pointer_to_pointer:
                outer = member.length[0]
                inner = get_inner_count(member.length[1], var_name)
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{outer} > 0) {{')
                lines.append(f'  blobSize += sizeof({member.base_type}) * {var_name}->{outer} * {inner};')
                lines.append('}')
            elif member.length and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  blobSize += sizeof({member.base_type}) * {var_name}->{member.length};')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  blobSize += sizeof({member.base_type});')
                lines.append('}')
        elif member.is_union:
            if member.length and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  blobSize += sizeof({member.base_type}) * {var_name}->{member.length};')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  blobSize += sizeof({member.base_type});')
                lines.append('}')
        elif member.is_pointer and member.length:
            length_expr = get_length_expression(member.length, var_name)
            lines.append(f'if ({var_name}->{member.name} && {length_expr} > 0) {{')
            lines.append(f'  blobSize += sizeof({member.base_type}) * {length_expr};')
            lines.append('}')
        elif member.is_pointer:
            lines.append(f'if ({var_name}->{member.name}) {{')
            lines.append(f'  blobSize += sizeof({member.base_type});')
            lines.append('}')

    return lines

def get_encode_lines(structure, structures_list, var_name_src, var_name_dst):
    structures_by_name = {s.name: s for s in structures_list}
    lines = []

    for member in structure.members:
        complex_struct = is_complex_struct(member.base_type, structures_by_name)
        basic_struct = is_basic_struct(member.base_type, structures_by_name)
        if member.name == 'pNext':
            lines.append(f'EncodePNextChain(dst, offset, {var_name_src}->pNext);')
            lines.append(f'{var_name_dst}->pNext = nullptr;')
        elif member.is_handle:
            if member.is_pointer and not member.length:
                lines.append(f'if ({var_name_src}->{member.name}) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}>(static_cast<uintptr_t>(offset));')
                lines.append(f'  GITSKey key = HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>(*{var_name_src}->{member.name}));')
                lines.append(f'  std::memcpy(dst + offset, &key, sizeof(GITSKey));')
                lines.append(f'  offset += sizeof(GITSKey);')
                lines.append('}')
            elif member.is_pointer and member.length:
                lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{member.length} > 0) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  for (uint32_t j = 0; j < {var_name_src}->{member.length}; ++j) {{')
                lines.append(f'    GITSKey key = HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>({var_name_src}->{member.name}[j]));')
                lines.append(f'    std::memcpy(dst + offset, &key, sizeof(GITSKey));')
                lines.append(f'    offset += sizeof(GITSKey);')
                lines.append('  }')
                lines.append('}')
            elif not member.is_pointer and member.length:
                lines.append(f'for (uint32_t j = 0; j < {var_name_src}->{member.length}; ++j) {{')
                lines.append(f'  GITSKey key = HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>({var_name_src}->{member.name}[j]));')
                lines.append(f'  std::memcpy(dst + offset, &key, sizeof(GITSKey));')
                lines.append(f'  offset += sizeof(GITSKey);')
                lines.append('}')
            else:
                lines.append(f'if ({var_name_src}->{member.name} != VK_NULL_HANDLE) {{')
                lines.append(f'  GITSKey key = HandleMapService::Get().GetKey(reinterpret_cast<uint64_t>({var_name_src}->{member.name}));')
                lines.append(f'  std::memcpy(dst + offset, &key, sizeof(GITSKey));')
                lines.append(f'  offset += sizeof(GITSKey);')
                lines.append('}')
        elif member.is_pointer and member.is_null_terminated:
            lines.append(f'if ({var_name_src}->{member.name}) {{')
            lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<const char*>(static_cast<uintptr_t>(offset));')
            lines.append('}')
            lines.append(f'EncodeString({var_name_src}->{member.name}, dst, offset);')
        elif member.is_pointer_to_pointer and member.is_null_terminated:
            lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{member.length}) {{')
            lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<const char* const*>(static_cast<uintptr_t>(offset));')
            lines.append(f'  EncodeStringArray({var_name_src}->{member.name}, {var_name_src}->{member.length}, dst, offset);')
            lines.append('}')
        elif member.is_pointer and member.base_type == 'void':
            lines.append('{')
            lines.append(f'  void* marker = const_cast<void*>({var_name_src}->{member.name});')
            lines.append(f'  std::memcpy(dst + offset, &marker, sizeof(void*));')
            lines.append(f'  offset += sizeof(void*);')
            lines.append(f'  {var_name_dst}->{member.name} = nullptr;')
            lines.append('}')
        elif complex_struct:
            if member.length and member.is_pointer_to_pointer:
                outer = member.length[0]
                inner = get_inner_count(member.length[1], var_name_src)
                lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{outer} > 0) {{')
                lines.append(f'  for (uint32_t j = 0; j < {var_name_src}->{outer}; ++j) {{')
                lines.append(f'    const_cast<{member.base_type}*&>({var_name_dst}->{member.name}[j]) = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'    Encode({var_name_src}->{member.name}[j], {inner}, dst, offset);')
                lines.append('  }')
                lines.append('}')
            elif member.length and member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{member.length} > 0) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  Encode({var_name_src}->{member.name}, {var_name_src}->{member.length}, dst, offset);')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name}) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  Encode({var_name_src}->{member.name}, 1, dst, offset);')
                lines.append('}')
        elif basic_struct:
            if member.length and member.is_pointer_to_pointer:
                outer = member.length[0]
                inner = get_inner_count(member.length[1], var_name_src)
                lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{outer} > 0) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}**>(static_cast<uintptr_t>(offset));')
                lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}) * {var_name_src}->{outer} * {inner});')
                lines.append(f'  offset += sizeof({member.base_type}) * {var_name_src}->{outer} * {inner};')
                lines.append('}')
            elif member.length and member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{member.length} > 0) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}) * {var_name_src}->{member.length});')
                lines.append(f'  offset += sizeof({member.base_type}) * {var_name_src}->{member.length};')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name}) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}));')
                lines.append(f'  offset += sizeof({member.base_type});')
                lines.append('}')
        elif member.is_union:
            if member.length and member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name} && {var_name_src}->{member.length} > 0) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}) * {var_name_src}->{member.length});')
                lines.append(f'  offset += sizeof({member.base_type}) * {var_name_src}->{member.length};')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name}) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}));')
                lines.append(f'  offset += sizeof({member.base_type});')
                lines.append('}')
        elif member.is_pointer and member.length:
            length_expr = get_length_expression(member.length, var_name_src)
            lines.append(f'if ({var_name_src}->{member.name} && {length_expr} > 0) {{')
            lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
            lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}) * {length_expr});')
            lines.append(f'  offset += sizeof({member.base_type}) * {length_expr};')
            lines.append('}')
        elif member.is_pointer:
            lines.append(f'if ({var_name_src}->{member.name}) {{')
            lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
            lines.append(f'  std::memcpy(dst + offset, {var_name_src}->{member.name}, sizeof({member.base_type}));')
            lines.append(f'  offset += sizeof({member.base_type});')
            lines.append('}')

    return lines

def get_decode_lines(structure, structures_list, var_name):
    structures_by_name = {s.name: s for s in structures_list}
    lines = []

    for member in structure.members:
        complex_struct = is_complex_struct(member.base_type, structures_by_name)
        basic_struct = is_basic_struct(member.base_type, structures_by_name)
        if member.name == 'pNext':
            if member.base_type != 'void':
                lines.append(f'DecodePNextChain(src, offset, reinterpret_cast<void**>(const_cast<{member.base_type}**>(&{var_name}->pNext)));')
            else:
                lines.append(f'DecodePNextChain(src, offset, const_cast<void**>(&{var_name}->pNext));')
        elif member.is_handle:
            if member.is_pointer and not member.length:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof(GITSKey);')
                lines.append('}')
            elif member.is_pointer and member.length:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof(GITSKey) * {var_name}->{member.length};')
                lines.append('}')
            elif not member.is_pointer and member.length:
                lines.append(f'for (uint32_t j = 0; j < {var_name}->{member.length}; ++j) {{')
                lines.append(f'  GITSKey key;')
                lines.append(f'  std::memcpy(&key, src + offset, sizeof(GITSKey));')
                lines.append(f'  offset += sizeof(GITSKey);')
                lines.append(f'  {var_name}->{member.name}[j] = reinterpret_cast<{member.base_type}>(key);')
                lines.append('}')
            else:
                lines.append(f'if ({var_name}->{member.name} != VK_NULL_HANDLE) {{')
                lines.append(f'  GITSKey key;')
                lines.append(f'  std::memcpy(&key, src + offset, sizeof(GITSKey));')
                lines.append(f'  offset += sizeof(GITSKey);')
                lines.append(f'  {var_name}->{member.name} = reinterpret_cast<{member.base_type}>(key);')
                lines.append('}')
        elif member.is_pointer and member.is_null_terminated:
            lines.append(f'DecodeString(src, offset, &{var_name}->{member.name});')
        elif member.is_pointer_to_pointer and member.is_null_terminated:
            lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length}) {{')
            lines.append(f'  DecodeStringArray(src, offset, const_cast<const char***>(reinterpret_cast<const char* const**>(&{var_name}->{member.name})), {var_name}->{member.length});')
            lines.append('}')
        elif member.is_pointer and member.base_type == 'void':
            lines.append('{')
            lines.append(f'  void* marker;')
            lines.append(f'  std::memcpy(&marker, src + offset,sizeof(void*));')
            lines.append(f'  offset += sizeof(void*);')
            lines.append(f'  {var_name}->{member.name} = nullptr;')
            lines.append('}')
        elif complex_struct:
            if member.length and member.is_pointer_to_pointer:
                outer = member.length[0]
                inner = get_inner_count(member.length[1], var_name)
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{outer} > 0) {{')
                lines.append(f'  for (uint32_t j = 0; j < {var_name}->{outer}; ++j) {{')
                lines.append(f'    const_cast<{member.base_type}*&>({var_name}->{member.name}[j]) = AddPtrs({var_name}->{member.name}[j], src);')
                lines.append(f'    Decode({var_name}->{member.name}[j], {inner}, src, offset);')
                lines.append('  }')
                lines.append('}')
            elif member.length and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  Decode({var_name}->{member.name}, {var_name}->{member.length}, src, offset);')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  Decode({var_name}->{member.name}, 1, src, offset);')
                lines.append('}')
        elif basic_struct:
            if member.length and member.is_pointer_to_pointer:
                outer = member.length[0]
                inner = get_inner_count(member.length[1], var_name)
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{outer} > 0) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof({member.base_type}) * {var_name}->{outer} * {inner};')
                lines.append('}')
            elif member.length and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof({member.base_type}) * {var_name}->{member.length};')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof({member.base_type});')
                lines.append('}')
        elif member.is_union:
            if member.length and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof({member.base_type}) * {var_name}->{member.length};')
                lines.append('}')
            elif member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  offset += sizeof({member.base_type});')
                lines.append('}')
        elif member.is_pointer and member.length:
            length_expr = get_length_expression(member.length, var_name)
            lines.append(f'if ({var_name}->{member.name} && {length_expr} > 0) {{')
            lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
            lines.append(f'  offset += sizeof({member.base_type}) * {length_expr};')
            lines.append('}')
        elif member.is_pointer:
            lines.append(f'if ({var_name}->{member.name}) {{')
            lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
            lines.append(f'  offset += sizeof({member.base_type});')
            lines.append('}')

    return lines

def generate_coders_files(context, out_path):
    additional_context = {
      'struct_needs_coder': struct_needs_coder,
      'get_size_lines': get_size_lines,
      'get_encode_lines': get_encode_lines,
      'get_decode_lines': get_decode_lines
    }
    files_to_generate = [
      'commandCodersAuto.h',
      'commandCodersAuto.cpp',
      'commandWritersAuto.h',
      'commandWritersFactoryAuto.cpp',
      'argumentCodersAuto.h',
      'argumentCodersAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
