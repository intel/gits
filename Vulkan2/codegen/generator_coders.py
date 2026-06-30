#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re

from generator_helpers import generate_file, get_define

# Structs that need custom CollectHandleKeys/ResolveHandleKeys implementations
# because the auto-generated ones are incorrect (e.g., VkWriteDescriptorSet has
# pImageInfo/pBufferInfo/pTexelBufferView but only one is valid depending on descriptorType).
CUSTOM_HANDLE_STRUCTS = {
    'VkWriteDescriptorSet',
    'VkPushDescriptorSetInfo',
}

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
            if structure.pnext_output:
                lines.append(f'blobSize += GetPNextChainSizeOutput({var_name}->pNext);')
            else:
                lines.append(f'if ({var_name}->pNext) {{')
                lines.append(f'  blobSize += GetPNextChainSizeInput({var_name}->pNext);')
                lines.append(f'}}')
        elif member.is_handle:
            # All handle members (single, pointer, array) are handled via HandleKeys vector.
            # Non-pointer handles are already in sizeof(struct). Pointer-to-handle arrays
            # will be allocated from HandleKeys by ResolveHandleKeys on the player side.
            # No extra blob space or encoding/decoding needed.
            pass
        elif member.is_pointer and member.is_null_terminated:
            lines.append(f'blobSize += GetStringSize({var_name}->{member.name});')
        elif member.is_pointer_to_pointer and member.is_null_terminated:
            lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length} > 0) {{')
            lines.append(f'  blobSize += GetStringArraySize({var_name}->{member.name}, {var_name}->{member.length});')
            lines.append('}')
        elif member.is_pointer and member.base_type == 'void':
            if member.length:
                length_expr = get_length_expression(member.length, var_name)
                lines.append('blobSize += sizeof(void*);')
                lines.append(f'if ({var_name}->{member.name} && {length_expr} > 0) {{')
                lines.append(f'  blobSize += {length_expr};')
                lines.append('}')
            else:
                lines.append('blobSize += sizeof(void*);')
        elif member.is_opaque_pointer:
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
            else:
                lines.append(f'blobSize += GetSize(&{var_name}->{member.name}, 1);')
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
            if structure.pnext_output:
                lines.append(f'EncodePNextChainOutput(dst, offset, {var_name_src}->pNext);')
            else:
                lines.append(f'if ({var_name_src}->pNext) {{')
                lines.append(f'  {var_name_dst}->pNext = reinterpret_cast<decltype({var_name_dst}->pNext)>(static_cast<uintptr_t>(offset));')
                lines.append(f'  EncodePNextChainInput(dst, offset, {var_name_src}->pNext);')
                lines.append(f'}} else {{')
                lines.append(f'  {var_name_dst}->pNext = nullptr;')
                lines.append(f'}}')
        elif member.is_handle:
            # All handle members are handled via HandleKeys vector.
            # No blob encoding needed - ResolveHandleKeys allocates and sets pointers.
            pass
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
            if member.length:
                length_expr = get_length_expression(member.length, var_name_src)
                lines.append('{')
                lines.append(f'  void* marker = const_cast<void*>({var_name_src}->{member.name});')
                lines.append(f'  std::memcpy(dst + offset, &marker, sizeof(void*));')
                lines.append(f'  offset += sizeof(void*);')
                lines.append(f'  if ({var_name_src}->{member.name} && {length_expr} > 0) {{')
                lines.append(f'    {var_name_dst}->{member.name} = reinterpret_cast<void*>(static_cast<uintptr_t>(offset));')
                lines.append(f'    std::memcpy(dst + offset, {var_name_src}->{member.name}, {length_expr});')
                lines.append(f'    offset += {length_expr};')
                lines.append(f'  }} else {{')
                lines.append(f'    {var_name_dst}->{member.name} = nullptr;')
                lines.append(f'  }}')
                lines.append('}')
            else:
                lines.append('{')
                lines.append(f'  void* marker = const_cast<void*>({var_name_src}->{member.name});')
                lines.append(f'  std::memcpy(dst + offset, &marker, sizeof(void*));')
                lines.append(f'  offset += sizeof(void*);')
                lines.append(f'  {var_name_dst}->{member.name} = nullptr;')
                lines.append('}')
        elif member.is_opaque_pointer:
            lines.append('{')
            lines.append(f'  void* opaquePointer = reinterpret_cast<void*>({var_name_src}->{member.name});')
            lines.append(f'  std::memcpy(dst + offset, &opaquePointer, sizeof(void*));')
            lines.append(f'  offset += sizeof(void*);')
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
            else:
                lines.append('{')
                lines.append(f'  uint32_t memberBase_{member.name} = offset;')
                lines.append(f'  Encode(&{var_name_src}->{member.name}, 1, dst, offset);')
                lines.append(f'  {var_name_dst}->{member.name} = *reinterpret_cast<{member.base_type}*>(dst + memberBase_{member.name});')
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
            if structure.pnext_output:
                lines.append(f'DecodePNextChainOutput(src, offset, &{var_name}->pNext);')
            elif member.base_type != 'void':
                lines.append(f'if ({var_name}->pNext) {{')
                lines.append(f'  DecodePNextChainInput(src, offset, reinterpret_cast<void**>(const_cast<{member.base_type}**>(&{var_name}->pNext)));')
                lines.append(f'}}')
            else:
                lines.append(f'if ({var_name}->pNext) {{')
                lines.append(f'  DecodePNextChainInput(src, offset, const_cast<void**>(&{var_name}->pNext));')
                lines.append(f'}}')
        elif member.is_handle:
            # All handle members are handled via HandleKeys vector.
            # No blob decoding needed - ResolveHandleKeys allocates and sets pointers.
            pass
        elif member.is_pointer and member.is_null_terminated:
            lines.append(f'DecodeString(src, offset, &{var_name}->{member.name});')
        elif member.is_pointer_to_pointer and member.is_null_terminated:
            lines.append(f'if ({var_name}->{member.name} && {var_name}->{member.length}) {{')
            lines.append(f'  DecodeStringArray(src, offset, const_cast<const char***>(reinterpret_cast<const char* const**>(&{var_name}->{member.name})), {var_name}->{member.length});')
            lines.append('}')
        elif member.is_pointer and member.base_type == 'void':
            if member.length:
                length_expr = get_length_expression(member.length, var_name)
                lines.append('{')
                lines.append(f'  void* marker;')
                lines.append(f'  std::memcpy(&marker, src + offset, sizeof(void*));')
                lines.append(f'  offset += sizeof(void*);')
                lines.append(f'  if (marker && {length_expr} > 0) {{')
                lines.append(f'    {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'    offset += {length_expr};')
                lines.append(f'  }} else {{')
                lines.append(f'    {var_name}->{member.name} = nullptr;')
                lines.append(f'  }}')
                lines.append('}')
            else:
                lines.append('{')
                lines.append(f'  void* marker;')
                lines.append(f'  std::memcpy(&marker, src + offset, sizeof(void*));')
                lines.append(f'  offset += sizeof(void*);')
                lines.append(f'  {var_name}->{member.name} = nullptr;')
                lines.append('}')
        elif member.is_opaque_pointer:
            lines.append('{')
            lines.append(f'  void* opaqueHandle;')
            lines.append(f'  std::memcpy(&opaqueHandle, src + offset, sizeof(void*));')
            lines.append(f'  offset += sizeof(void*);')
            lines.append(f'  {var_name}->{member.name} = reinterpret_cast<{member.base_type}*>(opaqueHandle);')
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
            else:
                lines.append(f'Decode(&{var_name}->{member.name}, 1, src, offset);')
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

def generate_child_handle_keys(child_handles, elem_expr='elem'):
    """Generate C++ lines to collect handle keys from child handle members of a struct element.

    Misses are tolerated via HandleMapService::GetKeyLenient (returns 0 and warns once
    per unique handle).  See handleMapService.h for rationale.
    """
    lines = []
    for child_kind, child_access, child_length, child_base_type, child_member_name in child_handles:
        if child_kind == 'handle_single':
            lines.append(f'    keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>({elem_expr}.{child_access})));')
        elif child_kind == 'handle_ptr':
            lines.append(f'    if ({elem_expr}.{child_access}) {{')
            lines.append(f'      keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>(*{elem_expr}.{child_access})));')
            lines.append(f'    }} else {{')
            lines.append(f'      keys.push_back(0);')
            lines.append(f'    }}')
        elif child_kind == 'handle_array_ptr':
            lines.append(f'    if ({elem_expr}.{child_access} && {elem_expr}.{child_length} > 0) {{')
            lines.append(f'      for (uint32_t handleIdx = 0; handleIdx < {elem_expr}.{child_length}; ++handleIdx) {{')
            lines.append(f'        keys.push_back(HandleMapService::Get().GetKeyLenient(reinterpret_cast<uint64_t>({elem_expr}.{child_access}[handleIdx])));')
            lines.append(f'      }}')
            lines.append(f'    }}')
        elif child_kind == 'handle_struct_ptr':
            nested_child_handles = child_member_name
            nested_lines = generate_child_handle_keys(nested_child_handles, f'(*{elem_expr}.{child_access})')
            if nested_lines:
                lines.append(f'    if ({elem_expr}.{child_access}) {{')
                lines.append(nested_lines)
                lines.append(f'    }}')
    return '\n'.join(lines)

def collect_handle_members(structure, structures_by_name, prefix=''):
    """Recursively collect handle members from a structure.
    Returns list of tuples: (kind, access_expr, length, base_type, member_name)
    """
    results = []
    for member in structure.members:
        if member.name in ('sType', 'pNext'):
            continue
        if member.is_typed_handle:
            access = f'{prefix}{member.name}'
            results.append(('handle_typed_uint64', access, None, member.base_type, member.name))
        elif member.is_handle:
            access = f'{prefix}{member.name}'
            if member.is_pointer and member.length:
                length = f'{prefix}{member.length}' if prefix else member.length
                results.append(('handle_array_ptr', access, length, member.base_type, member.name))
            elif member.is_pointer and not member.length:
                results.append(('handle_ptr', access, None, member.base_type, member.name))
            elif not member.is_pointer and member.length:
                length = f'{prefix}{member.length}' if prefix else member.length
                results.append(('handle_fixed_array', access, length, member.base_type, member.name))
            else:
                results.append(('handle_single', access, None, member.base_type, member.name))
        elif member.is_struct_with_handles and not member.is_pointer:
            child_struct = structures_by_name.get(member.base_type)
            if child_struct:
                results.extend(collect_handle_members(child_struct, structures_by_name, f'{prefix}{member.name}.'))
        elif member.is_struct_with_handles and member.is_pointer and member.length:
            child_struct = structures_by_name.get(member.base_type)
            if child_struct:
                child_handles = collect_handle_members(child_struct, structures_by_name)
                if child_handles:
                    length = f'{prefix}{member.length}' if prefix else member.length
                    results.append(('handle_struct_array_ptr', f'{prefix}{member.name}', length, member.base_type, child_handles))
        elif member.is_struct_with_handles and member.is_pointer and not member.length:
            child_struct = structures_by_name.get(member.base_type)
            if child_struct:
                child_handles = collect_handle_members(child_struct, structures_by_name)
                if child_handles:
                    results.append(('handle_struct_ptr', f'{prefix}{member.name}', None, member.base_type, child_handles))
    return results

def collect_pnext_handle_structs(structures):
    """Return pnext_input structs that contain at least one handle member.
    Used to generate pNext-chain handle collection (recorder) and remapping (player).
    Returns a list of (structure, handle_members) sorted by stype_value.
    """
    structures_by_name = {s.name: s for s in structures}
    result = []
    for s in structures:
        if not s.pnext_input or not s.stype_value:
            continue
        handle_members = collect_handle_members(s, structures_by_name)
        if handle_members:
            result.append((s, handle_members))
    result.sort(key=lambda x: x[0].stype_value)
    return result

def collect_structs_needing_handle_updater(commands, structures):
    """Return a list of structs that need UpdateHandle/ResolveHandleKeys generated for them.

    This is the pre-computed, pre-filtered equivalent of the structs_needing_updater
    logic that was previously duplicated in both handleArgumentUpdaters mako templates.

    Returns a sorted list of dicts, each containing:
      - 'name': struct name (str)
      - 'structure': the Structure object
      - 'has_pnext': bool, whether the struct has a pNext member
      - 'handle_members': list from collect_handle_members()
      - 'define': platform #ifdef guard string or None
    """
    structures_by_name = {s.name: s for s in structures}
    pnext_handle_structs = collect_pnext_handle_structs(structures)

    names = set()
    for command in commands:
        for param in command.params:
            if param.is_struct_with_handles:
                names.add(param.base_type)
            elif param.is_struct:
                struct_def = structures_by_name.get(param.base_type)
                if struct_def is not None and any(m.name == 'pNext' for m in struct_def.members):
                    names.add(param.base_type)

    result = []
    for struct_name in sorted(names):
        structure = structures_by_name.get(struct_name)
        if structure is None:
            continue
        if struct_name in CUSTOM_HANDLE_STRUCTS:
            continue
        has_pnext = any(m.name == 'pNext' for m in structure.members)
        handle_members = collect_handle_members(structure, structures_by_name)
        if not handle_members and not (has_pnext and pnext_handle_structs):
            continue
        result.append({
            'name': struct_name,
            'structure': structure,
            'has_pnext': has_pnext,
            'handle_members': handle_members,
            'define': get_define(structure.platform),
        })
    return result

def generate_coders_files(context, out_path):
    additional_context = {
      'struct_needs_coder': struct_needs_coder,
      'get_size_lines': get_size_lines,
      'get_encode_lines': get_encode_lines,
      'get_decode_lines': get_decode_lines,
      'collect_handle_members': collect_handle_members,
      'custom_handle_structs': CUSTOM_HANDLE_STRUCTS
    }
    files_to_generate = [
      'commandCodersAuto.h',
      'commandCodersAuto.cpp',
      'argumentCodersAuto.h',
      'argumentCodersAuto.cpp',
      'commandSerializersAuto.h',
      'commandSerializersFactoryAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)

