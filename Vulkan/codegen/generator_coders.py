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
    'VkImageCreateInfo',
    'VkRayTracingShaderGroupCreateInfoKHR',
    'VkRayTracingPipelineCreateInfoKHR',
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

# ---------------------------------------------------------------------------
# Union (de)serialization support.
#
# A struct member whose type is a union carries a `selector` naming the sibling
# field that picks the active union member; each union member carries the
# enum value(s) (`selection`) that select it. Non-pointer union members were
# previously copied as raw bytes only, so any pointer/pNext data reachable
# through the active member (e.g. VkAccelerationStructureGeometryKHR.geometry.
# triangles.pNext, VkDescriptorGetInfoEXT.data.pCombinedImageSampler) was lost.
# We now generate per-union GetSize/Encode/Decode overloads that take the
# selector value and (de)serialize the active member like a normal struct field.
# ---------------------------------------------------------------------------

def union_member_needs_coding(member, structures_by_name, structures_list):
    """True if a union member needs more than the raw union bytes to round-trip."""
    if member.is_handle:
        # Handle members (single or pointer) are carried by the HandleKeys sidecar,
        # never the blob, so no coder work is required here.
        return False
    if member.is_struct and member.base_type in structures_by_name:
        if member.is_pointer:
            return True
        return struct_needs_coder(structures_by_name[member.base_type], structures_list)
    return False

def union_needs_coder(union, structures_by_name, structures_list):
    return any(union_member_needs_coding(m, structures_by_name, structures_list) for m in union.members)

def unions_needing_coder(unions, structures):
    structures_by_name = {s.name: s for s in structures}
    return [u for u in unions if union_needs_coder(u, structures_by_name, structures)]

def get_union_selector_types(structures, unions):
    """Map union name -> enum type of the sibling field that selects its active member.
    Resolved from any struct member that references the union with a `selector`."""
    union_names = {u.name for u in unions}
    result = {}
    for s in structures:
        member_by_name = {m.name: m for m in s.members}
        for m in s.members:
            if m.base_type in union_names and m.selector:
                sel_member = member_by_name.get(m.selector)
                if sel_member is not None and m.base_type not in result:
                    result[m.base_type] = sel_member.base_type
    return result

def _selection_labels(member):
    return [v.strip() for v in member.selection.split(',') if v.strip()]

def get_union_size_switch(union, structures_by_name, structures_list, var_name):
    lines = []
    for m in union.members:
        if not union_member_needs_coding(m, structures_by_name, structures_list):
            continue
        labels = _selection_labels(m)
        if not labels:
            continue
        complex_struct = is_complex_struct(m.base_type, structures_by_name)
        for label in labels:
            lines.append(f'    case {label}:')
        if m.is_pointer:
            lines.append(f'      if ({var_name}->{m.name}) {{')
            if complex_struct:
                lines.append(f'        blobSize += GetSize({var_name}->{m.name}, 1);')
            else:
                lines.append(f'        blobSize += sizeof({m.base_type});')
            lines.append(f'      }}')
        else:
            lines.append(f'      blobSize += GetSize(&{var_name}->{m.name}, 1);')
        lines.append(f'      break;')
    return lines

def get_union_encode_switch(union, structures_by_name, structures_list, src, dst):
    lines = []
    for m in union.members:
        if not union_member_needs_coding(m, structures_by_name, structures_list):
            continue
        labels = _selection_labels(m)
        if not labels:
            continue
        complex_struct = is_complex_struct(m.base_type, structures_by_name)
        for label in labels:
            lines.append(f'    case {label}:')
        if m.is_pointer:
            lines.append(f'      if ({src}->{m.name}) {{')
            lines.append(f'        {dst}->{m.name} = reinterpret_cast<{m.base_type}*>(static_cast<uintptr_t>(offset));')
            if complex_struct:
                lines.append(f'        Encode({src}->{m.name}, 1, dst, offset);')
            else:
                lines.append(f'        std::memcpy(dst + offset, {src}->{m.name}, sizeof({m.base_type}));')
                lines.append(f'        offset += sizeof({m.base_type});')
            lines.append(f'      }} else {{')
            lines.append(f'        {dst}->{m.name} = nullptr;')
            lines.append(f'      }}')
        else:
            lines.append(f'      {{')
            lines.append(f'        uint32_t memberBase = offset;')
            lines.append(f'        Encode(&{src}->{m.name}, 1, dst, offset);')
            lines.append(f'        {dst}->{m.name} = *reinterpret_cast<{m.base_type}*>(dst + memberBase);')
            lines.append(f'      }}')
        lines.append(f'      break;')
    return lines

def get_union_decode_switch(union, structures_by_name, structures_list, var_name):
    lines = []
    for m in union.members:
        if not union_member_needs_coding(m, structures_by_name, structures_list):
            continue
        labels = _selection_labels(m)
        if not labels:
            continue
        complex_struct = is_complex_struct(m.base_type, structures_by_name)
        for label in labels:
            lines.append(f'    case {label}:')
        if m.is_pointer:
            lines.append(f'      if ({var_name}->{m.name}) {{')
            lines.append(f'        {var_name}->{m.name} = AddPtrs({var_name}->{m.name}, src);')
            if complex_struct:
                lines.append(f'        Decode({var_name}->{m.name}, 1, src, offset);')
            else:
                lines.append(f'        offset += sizeof({m.base_type});')
            lines.append(f'      }}')
        else:
            lines.append(f'      Decode(&{var_name}->{m.name}, 1, src, offset);')
        lines.append(f'      break;')
    return lines

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

def get_size_lines(structure, structures_list, unions_list, var_name):
    structures_by_name = {s.name: s for s in structures_list}
    unions_by_name = {u.name: u for u in unions_list}
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
            union = unions_by_name.get(member.base_type)
            union_coded = union is not None and member.selector and union_needs_coder(union, structures_by_name, structures_list)
            if union_coded and not member.is_pointer:
                lines.append(f'blobSize += GetSize(&{var_name}->{member.name}, 1, {var_name}->{member.selector});')
            elif union_coded and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  blobSize += GetSize({var_name}->{member.name}, 1, {var_name}->{member.selector});')
                lines.append('}')
            elif member.length and member.is_pointer:
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

def get_encode_lines(structure, structures_list, unions_list, var_name_src, var_name_dst):
    structures_by_name = {s.name: s for s in structures_list}
    unions_by_name = {u.name: u for u in unions_list}
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
            union = unions_by_name.get(member.base_type)
            union_coded = union is not None and member.selector and union_needs_coder(union, structures_by_name, structures_list)
            if union_coded and not member.is_pointer:
                lines.append('{')
                lines.append(f'  uint32_t memberBaseU_{member.name} = offset;')
                lines.append(f'  Encode(&{var_name_src}->{member.name}, 1, dst, offset, {var_name_src}->{member.selector});')
                lines.append(f'  {var_name_dst}->{member.name} = *reinterpret_cast<{member.base_type}*>(dst + memberBaseU_{member.name});')
                lines.append('}')
            elif union_coded and member.is_pointer:
                lines.append(f'if ({var_name_src}->{member.name}) {{')
                lines.append(f'  {var_name_dst}->{member.name} = reinterpret_cast<{member.base_type}*>(static_cast<uintptr_t>(offset));')
                lines.append(f'  Encode({var_name_src}->{member.name}, 1, dst, offset, {var_name_src}->{member.selector});')
                lines.append(f'}} else {{')
                lines.append(f'  {var_name_dst}->{member.name} = nullptr;')
                lines.append(f'}}')
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

def get_decode_lines(structure, structures_list, unions_list, var_name):
    structures_by_name = {s.name: s for s in structures_list}
    unions_by_name = {u.name: u for u in unions_list}
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
            union = unions_by_name.get(member.base_type)
            union_coded = union is not None and member.selector and union_needs_coder(union, structures_by_name, structures_list)
            if union_coded and not member.is_pointer:
                lines.append(f'Decode(&{var_name}->{member.name}, 1, src, offset, {var_name}->{member.selector});')
            elif union_coded and member.is_pointer:
                lines.append(f'if ({var_name}->{member.name}) {{')
                lines.append(f'  {var_name}->{member.name} = AddPtrs({var_name}->{member.name}, src);')
                lines.append(f'  Decode({var_name}->{member.name}, 1, src, offset, {var_name}->{member.selector});')
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

def _collect_union_cases(member, structures_by_name):
    """Build handle-collection cases for a union-typed struct member (selected by member.selector).
    Returns list of (labels, um_name, um_is_pointer, um_base_type, um_kind, child) where um_kind is
    one of 'handle' / 'typed' / 'struct' and child is the nested entry list for 'struct'."""
    union = member.union_ref
    cases = []
    for um in union.members:
        labels = _selection_labels(um)
        if not labels:
            continue
        if um.is_handle:
            cases.append((labels, um.name, um.is_pointer, um.base_type, 'handle', None))
        elif um.is_typed_handle:
            cases.append((labels, um.name, um.is_pointer, um.base_type, 'typed', None))
        elif um.is_struct and um.contributes_keys:
            um_struct = structures_by_name.get(um.base_type)
            child = collect_handle_members(um_struct, structures_by_name, include_pnext=True) if um_struct else []
            if child:
                cases.append((labels, um.name, um.is_pointer, um.base_type, 'struct', child))
    return cases

def collect_handle_members(structure, structures_by_name, include_pnext=False):
    """Recursively collect handle-bearing members from a structure as nested entries.

    Returns a list of tuples (kind, access, length, base_type, child). Aggregates are represented
    recursively (child is the nested entry list) so the recorder collect and player resolve can
    walk to arbitrary depth through embedded structs, pointer/array struct members, union members
    (selected via a sibling field) and nested pNext chains. include_pnext appends a 'handle_pnext'
    entry when the struct is extended by a handle-bearing pNext struct; it is only set while
    recursing into nested structs (the top-level pNext chain is walked separately in UpdateHandle).
    """
    results = []
    for member in structure.members:
        if member.name in ('sType', 'pNext'):
            continue
        if member.is_typed_handle:
            results.append(('handle_typed_uint64', member.name, None, member.base_type, None))
        elif member.is_handle:
            if member.is_pointer and member.length:
                results.append(('handle_array_ptr', member.name, member.length, member.base_type, None))
            elif member.is_pointer:
                results.append(('handle_ptr', member.name, None, member.base_type, None))
            elif member.length:
                results.append(('handle_fixed_array', member.name, member.length, member.base_type, None))
            else:
                results.append(('handle_single', member.name, None, member.base_type, None))
        elif member.is_union and member.union_ref is not None and member.contributes_keys:
            cases = _collect_union_cases(member, structures_by_name)
            if cases:
                results.append(('handle_union', member.name, member.selector, member.base_type, cases))
        elif member.is_struct and member.contributes_keys:
            child_struct = structures_by_name.get(member.base_type)
            if child_struct is None:
                continue
            child = collect_handle_members(child_struct, structures_by_name, include_pnext=True)
            if not child:
                continue
            if member.is_pointer_to_pointer and member.length:
                outer = member.length[0] if isinstance(member.length, list) else member.length
                results.append(('handle_struct_pp_array', member.name, outer, member.base_type, child))
            elif member.is_pointer and member.length:
                results.append(('handle_struct_array_ptr', member.name, member.length, member.base_type, child))
            elif member.is_pointer:
                results.append(('handle_struct_ptr', member.name, None, member.base_type, child))
            else:
                results.append(('handle_struct_embedded', member.name, None, member.base_type, child))
    if include_pnext and structure.pnext_extendable:
        results.append(('handle_pnext', None, None, None, None))
    return results

def entries_need_handle_data(entries):
    """True if resolving `entries` (player side) ever writes into the handleData scratch vector,
    i.e. it contains a pointer/array handle (directly, through a nested struct, through a union
    pointer-handle member, or through a nested pNext chain). Used to decide whether UpdateHandle
    must pre-reserve handleData so &handleData[...] pointers stay valid."""
    for kind, access, length, base_type, child in entries:
        if kind in ('handle_ptr', 'handle_array_ptr', 'handle_pnext'):
            return True
        if kind in ('handle_struct_embedded', 'handle_struct_ptr',
                    'handle_struct_array_ptr', 'handle_struct_pp_array'):
            if entries_need_handle_data(child):
                return True
        elif kind == 'handle_union':
            for labels, um_name, um_ptr, um_base, um_kind, um_child in child:
                if um_kind == 'handle' and um_ptr:
                    return True
                if um_kind == 'struct' and entries_need_handle_data(um_child):
                    return True
    return False

def generate_child_handle_keys(entries, elem_expr='s', depth=0, indent=2):
    """Recorder side: emit C++ that pushes GITSKeys for every handle reachable from `entries`.

    Misses are tolerated via HandleMapService::GetKeyLenient (returns 0 and warns once per unique
    handle). The push order here MUST stay identical to the player's generate_child_handle_resolve
    consume order; both walk the same entry list with the same guards, so per-index keys stay
    aligned. Type-erased (objecttype) handles are only remapped at the top level (depth 0), matching
    the original behaviour where nested objectHandle members are left untouched on both sides.

    `indent` is the leading space count for statements at this level; nested blocks add two spaces
    per level so the emitted code is already correctly formatted independent of clang-format.
    """
    p = ' ' * indent
    p2 = ' ' * (indent + 2)
    p3 = ' ' * (indent + 4)
    lines = []
    for kind, access, length, base_type, child in entries:
        if kind == 'handle_single':
            lines.append(f'{p}keys.push_back(HandleMapService::Get().GetKeyLenient({elem_expr}.{access}));')
        elif kind == 'handle_typed_uint64':
            if depth == 0:
                lines.append(f'{p}keys.push_back(HandleMapService::Get().GetKeyLenient({elem_expr}.{access}));')
        elif kind == 'handle_ptr':
            lines.append(f'{p}if ({elem_expr}.{access}) {{')
            lines.append(f'{p2}keys.push_back(HandleMapService::Get().GetKeyLenient(*{elem_expr}.{access}));')
            lines.append(f'{p}}} else {{')
            lines.append(f'{p2}keys.push_back(0);')
            lines.append(f'{p}}}')
        elif kind == 'handle_array_ptr':
            lines.append(f'{p}if ({elem_expr}.{access} && {elem_expr}.{length} > 0) {{')
            lines.append(f'{p2}for (uint32_t handleIdx = 0; handleIdx < {elem_expr}.{length}; ++handleIdx) {{')
            lines.append(f'{p3}keys.push_back(HandleMapService::Get().GetKeyLenient({elem_expr}.{access}[handleIdx]));')
            lines.append(f'{p2}}}')
            lines.append(f'{p}}}')
        elif kind == 'handle_fixed_array':
            lines.append(f'{p}for (uint32_t handleIdx = 0; handleIdx < {elem_expr}.{length}; ++handleIdx) {{')
            lines.append(f'{p2}keys.push_back(HandleMapService::Get().GetKeyLenient({elem_expr}.{access}[handleIdx]));')
            lines.append(f'{p}}}')
        elif kind == 'handle_pnext':
            lines.append(f'{p}CollectPNextHandleKeys(keys, {elem_expr}.pNext);')
        elif kind == 'handle_struct_embedded':
            nested = generate_child_handle_keys(child, f'{elem_expr}.{access}', depth + 1, indent)
            if nested:
                lines.append(nested)
        elif kind == 'handle_struct_ptr':
            nested = generate_child_handle_keys(child, f'(*{elem_expr}.{access})', depth + 1, indent + 2)
            if nested:
                lines.append(f'{p}if ({elem_expr}.{access}) {{')
                lines.append(nested)
                lines.append(f'{p}}}')
        elif kind == 'handle_struct_array_ptr':
            i = f'elemIdx{depth}'
            nested = generate_child_handle_keys(child, f'{elem_expr}.{access}[{i}]', depth + 1, indent + 4)
            if nested:
                lines.append(f'{p}if ({elem_expr}.{access} && {elem_expr}.{length} > 0) {{')
                lines.append(f'{p2}for (uint32_t {i} = 0; {i} < {elem_expr}.{length}; ++{i}) {{')
                lines.append(nested)
                lines.append(f'{p2}}}')
                lines.append(f'{p}}}')
        elif kind == 'handle_struct_pp_array':
            i = f'elemIdx{depth}'
            nested = generate_child_handle_keys(child, f'(*{elem_expr}.{access}[{i}])', depth + 1, indent + 6)
            if nested:
                lines.append(f'{p}if ({elem_expr}.{access} && {elem_expr}.{length} > 0) {{')
                lines.append(f'{p2}for (uint32_t {i} = 0; {i} < {elem_expr}.{length}; ++{i}) {{')
                lines.append(f'{p3}if ({elem_expr}.{access}[{i}]) {{')
                lines.append(nested)
                lines.append(f'{p3}}}')
                lines.append(f'{p2}}}')
                lines.append(f'{p}}}')
        elif kind == 'handle_union':
            selector = length
            cases = child
            case_lines = []
            for labels, um_name, um_ptr, um_base, um_kind, um_child in cases:
                base_expr = f'{elem_expr}.{access}.{um_name}'
                body = []
                if um_kind == 'handle':
                    if um_ptr:
                        body.append(f'{p2}if ({base_expr}) {{')
                        body.append(f'{p3}keys.push_back(HandleMapService::Get().GetKeyLenient(*{base_expr}));')
                        body.append(f'{p2}}} else {{')
                        body.append(f'{p3}keys.push_back(0);')
                        body.append(f'{p2}}}')
                    else:
                        body.append(f'{p2}keys.push_back(HandleMapService::Get().GetKeyLenient({base_expr}));')
                elif um_kind == 'typed':
                    body.append(f'{p2}keys.push_back(HandleMapService::Get().GetKeyLenient({base_expr}));')
                elif um_kind == 'struct':
                    if um_ptr:
                        nested = generate_child_handle_keys(um_child, f'(*{base_expr})', depth + 1, indent + 4)
                        if nested:
                            body.append(f'{p2}if ({base_expr}) {{')
                            body.append(nested)
                            body.append(f'{p2}}}')
                    else:
                        nested = generate_child_handle_keys(um_child, base_expr, depth + 1, indent + 2)
                        if nested:
                            body.append(nested)
                if not body:
                    continue
                for lab in labels:
                    case_lines.append(f'{p}case {lab}:')
                case_lines.extend(body)
                case_lines.append(f'{p2}break;')
            if case_lines:
                lines.append(f'{p}switch ({elem_expr}.{selector}) {{')
                lines.extend(case_lines)
                lines.append(f'{p}default:')
                lines.append(f'{p2}break;')
                lines.append(f'{p}}}')
    return '\n'.join(lines)

def generate_child_handle_resolve(entries, elem_expr='s', depth=0, indent=2):
    """Player side mirror of generate_child_handle_keys: consume keys[idx++] in the same order the
    recorder pushed them and write the remapped handles back into the deserialized struct. `indent`
    behaves as in generate_child_handle_keys."""
    p = ' ' * indent
    p2 = ' ' * (indent + 2)
    p3 = ' ' * (indent + 4)
    lines = []
    for kind, access, length, base_type, child in entries:
        if kind == 'handle_single':
            lines.append(f'{p}if (idx < keys.size()) {{')
            lines.append(f'{p2}GITSKey key = keys[idx++];')
            lines.append(f'{p2}{elem_expr}.{access} = key ? reinterpret_cast<{base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;')
            lines.append(f'{p}}}')
        elif kind == 'handle_typed_uint64':
            if depth == 0:
                lines.append(f'{p}if (idx < keys.size()) {{')
                lines.append(f'{p2}GITSKey key = keys[idx++];')
                lines.append(f'{p2}{elem_expr}.{access} = key ? HandleMapService::Get().GetHandle(key) : 0;')
                lines.append(f'{p}}}')
        elif kind == 'handle_ptr':
            lines.append(f'{p}if (idx < keys.size()) {{')
            lines.append(f'{p2}GITSKey key = keys[idx++];')
            lines.append(f'{p2}size_t dataOffset = handleData.size();')
            lines.append(f'{p2}handleData.push_back(key ? HandleMapService::Get().GetHandle(key) : 0);')
            lines.append(f'{p2}{elem_expr}.{access} = reinterpret_cast<{base_type}*>(&handleData[dataOffset]);')
            lines.append(f'{p}}}')
        elif kind == 'handle_array_ptr':
            lines.append(f'{p}if ({elem_expr}.{access} && {elem_expr}.{length} > 0) {{')
            lines.append(f'{p2}size_t dataOffset = handleData.size();')
            lines.append(f'{p2}handleData.resize(handleData.size() + {elem_expr}.{length});')
            lines.append(f'{p2}for (uint32_t handleIdx = 0; handleIdx < {elem_expr}.{length} && idx < keys.size(); ++handleIdx) {{')
            lines.append(f'{p3}GITSKey key = keys[idx++];')
            lines.append(f'{p3}handleData[dataOffset + handleIdx] = key ? HandleMapService::Get().GetHandle(key) : 0;')
            lines.append(f'{p2}}}')
            lines.append(f'{p2}{elem_expr}.{access} = reinterpret_cast<{base_type}*>(&handleData[dataOffset]);')
            lines.append(f'{p}}}')
        elif kind == 'handle_fixed_array':
            lines.append(f'{p}for (uint32_t handleIdx = 0; handleIdx < {elem_expr}.{length} && idx < keys.size(); ++handleIdx) {{')
            lines.append(f'{p2}GITSKey key = keys[idx++];')
            lines.append(f'{p2}{elem_expr}.{access}[handleIdx] = key ? reinterpret_cast<{base_type}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;')
            lines.append(f'{p}}}')
        elif kind == 'handle_pnext':
            lines.append(f'{p}ResolvePNextHandleKeys(keys, idx, handleData, {elem_expr}.pNext);')
        elif kind == 'handle_struct_embedded':
            nested = generate_child_handle_resolve(child, f'{elem_expr}.{access}', depth + 1, indent)
            if nested:
                lines.append(nested)
        elif kind == 'handle_struct_ptr':
            ref = f'elem{depth}'
            nested = generate_child_handle_resolve(child, ref, depth + 1, indent + 2)
            if nested:
                lines.append(f'{p}if ({elem_expr}.{access}) {{')
                lines.append(f'{p2}auto& {ref} = const_cast<{base_type}&>(*{elem_expr}.{access});')
                lines.append(nested)
                lines.append(f'{p}}}')
        elif kind == 'handle_struct_array_ptr':
            i = f'elemIdx{depth}'
            ref = f'elem{depth}'
            nested = generate_child_handle_resolve(child, ref, depth + 1, indent + 4)
            if nested:
                lines.append(f'{p}if ({elem_expr}.{access} && {elem_expr}.{length} > 0) {{')
                lines.append(f'{p2}for (uint32_t {i} = 0; {i} < {elem_expr}.{length}; ++{i}) {{')
                lines.append(f'{p3}auto& {ref} = const_cast<{base_type}&>({elem_expr}.{access}[{i}]);')
                lines.append(nested)
                lines.append(f'{p2}}}')
                lines.append(f'{p}}}')
        elif kind == 'handle_struct_pp_array':
            i = f'elemIdx{depth}'
            ref = f'elem{depth}'
            p4 = ' ' * (indent + 6)
            nested = generate_child_handle_resolve(child, ref, depth + 1, indent + 6)
            if nested:
                lines.append(f'{p}if ({elem_expr}.{access} && {elem_expr}.{length} > 0) {{')
                lines.append(f'{p2}for (uint32_t {i} = 0; {i} < {elem_expr}.{length}; ++{i}) {{')
                lines.append(f'{p3}if ({elem_expr}.{access}[{i}]) {{')
                lines.append(f'{p4}auto& {ref} = const_cast<{base_type}&>(*{elem_expr}.{access}[{i}]);')
                lines.append(nested)
                lines.append(f'{p3}}}')
                lines.append(f'{p2}}}')
                lines.append(f'{p}}}')
        elif kind == 'handle_union':
            selector = length
            cases = child
            case_lines = []
            for labels, um_name, um_ptr, um_base, um_kind, um_child in cases:
                base_expr = f'{elem_expr}.{access}.{um_name}'
                body = []
                if um_kind == 'handle':
                    if um_ptr:
                        body.append(f'{p2}if (idx < keys.size()) {{')
                        body.append(f'{p3}GITSKey key = keys[idx++];')
                        body.append(f'{p3}size_t dataOffset = handleData.size();')
                        body.append(f'{p3}handleData.push_back(key ? HandleMapService::Get().GetHandle(key) : 0);')
                        body.append(f'{p3}const_cast<{um_base}*&>({base_expr}) = reinterpret_cast<{um_base}*>(&handleData[dataOffset]);')
                        body.append(f'{p2}}}')
                    else:
                        body.append(f'{p2}if (idx < keys.size()) {{')
                        body.append(f'{p3}GITSKey key = keys[idx++];')
                        body.append(f'{p3}{base_expr} = key ? reinterpret_cast<{um_base}>(HandleMapService::Get().GetHandle(key)) : VK_NULL_HANDLE;')
                        body.append(f'{p2}}}')
                elif um_kind == 'typed':
                    body.append(f'{p2}if (idx < keys.size()) {{')
                    body.append(f'{p3}GITSKey key = keys[idx++];')
                    body.append(f'{p3}{base_expr} = key ? HandleMapService::Get().GetHandle(key) : 0;')
                    body.append(f'{p2}}}')
                elif um_kind == 'struct':
                    if um_ptr:
                        ref = f'uelem{depth}'
                        nested = generate_child_handle_resolve(um_child, ref, depth + 1, indent + 4)
                        if nested:
                            body.append(f'{p2}if ({base_expr}) {{')
                            body.append(f'{p3}auto& {ref} = const_cast<{um_base}&>(*{base_expr});')
                            body.append(nested)
                            body.append(f'{p2}}}')
                    else:
                        nested = generate_child_handle_resolve(um_child, base_expr, depth + 1, indent + 2)
                        if nested:
                            body.append(nested)
                if not body:
                    continue
                for lab in labels:
                    case_lines.append(f'{p}case {lab}:')
                case_lines.extend(body)
                case_lines.append(f'{p2}break;')
            if case_lines:
                lines.append(f'{p}switch ({elem_expr}.{selector}) {{')
                lines.extend(case_lines)
                lines.append(f'{p}default:')
                lines.append(f'{p2}break;')
                lines.append(f'{p}}}')
    return '\n'.join(lines)

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
                if struct_def is not None and (getattr(struct_def, 'contributes_keys', False)
                                               or any(m.name == 'pNext' for m in struct_def.members)):
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
      'custom_handle_structs': CUSTOM_HANDLE_STRUCTS,
      'unions_needing_coder': unions_needing_coder,
      'get_union_selector_types': get_union_selector_types,
      'get_union_size_switch': get_union_size_switch,
      'get_union_encode_switch': get_union_encode_switch,
      'get_union_decode_switch': get_union_decode_switch,
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

