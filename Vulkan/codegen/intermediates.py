#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from dataclasses import dataclass, field

@dataclass
class Parameter:
    name: str = ''
    base_type: str = ''
    full_type: str = ''
    is_const: bool = False
    is_void: bool = False
    is_pointer: bool = False
    is_pointer_to_pointer: bool = False
    is_triple_pointer: bool = False
    length: (str|list[str]) = ''
    is_null_terminated: bool = False
    fixed_array_size: list[str] = field(default_factory=list)
    is_handle: bool = False
    is_handle_output: bool = False
    # True for a handle-typed *input* parameter that is known to sometimes carry a
    # handle the recorder never registered a GITSKey for -- e.g. a view created by
    # the driver on a RecursionGuard-elided nested call. See the vendor-extension
    # codegen helpers for the full rationale and the curated list this applies to.
    # Codegen emits UpdateHandleLenient instead of the strict, asserting UpdateHandle
    # for these.
    is_lenient_handle: bool = False
    is_struct: bool = False
    is_struct_with_handles: bool = False
    is_struct_with_output_handles: bool = False
    is_union: bool = False
    is_opaque_pointer: bool = False
    is_descriptor_template_data: bool = False

@dataclass
class Command:
    name: str = ''
    return_type: str = ''
    params: list[Parameter] = field(default_factory=list)
    success_codes: list[str] = field(default_factory=list)
    error_codes: list[str] = field(default_factory=list)
    dispatch_level: str = ''
    platform: str = ''
    # vk.xml "tasks" attribute, e.g. ['action'], ['state'], ['action', 'indirection'].
    # An 'action' command performs work on the device; everything else only affects
    # command buffer state or synchronization.
    tasks: list[str] = field(default_factory=list)

@dataclass
class NestedStructMember:
    name: str = ''
    base_type: str = ''
    full_type: str = ''
    is_const: bool = False
    is_pointer: bool = False
    is_pointer_to_pointer: bool = False
    is_triple_pointer: bool = False
    fixed_array_size: list[str] = field(default_factory=list)
    bitfield: (int|None) = None
    is_inline_struct: bool = False
    nested_members: list['NestedStructMember'] = field(default_factory=list)

@dataclass
class Member:
    name: str = ''
    base_type: str = ''
    full_type: str = ''
    is_const: bool = False
    is_void: bool = False
    is_pointer: bool = False
    is_pointer_to_pointer: bool = False
    is_triple_pointer: bool = False
    length: (str|list[str]) = ''
    is_null_terminated: bool = False
    fixed_array_size: list[str] = field(default_factory=list)
    bitfield: (int|None) = None
    values: str = ''
    is_handle: bool = False
    is_struct: bool = False
    is_struct_with_handles: bool = False
    is_union: bool = False
    is_opaque_pointer: bool = False
    is_typed_handle: bool = False  # uint64_t member with objecttype="..." in vk.xml (type-erased handle)
    # Union discrimination (vk.xml). On a struct member whose type is a union, `selector` names the
    # sibling field that picks the active union member. On a union's own members, `selection` lists
    # the enum value(s) (comma-separated) that select that member.
    selector: str = ''
    selection: str = ''
    # True if this member's type (struct or union) transitively yields handle keys, i.e. the handle
    # updater must descend into it. Covers handles reachable via nested members, union members, and
    # pNext-extension structs. Set in postprocess().
    contributes_keys: bool = False
    union_ref: object = None  # the Union object, for members whose type is a union
    is_inline_struct: bool = False
    inline_struct_decl: str = ''
    nested_members: list[NestedStructMember] = field(default_factory=list)

@dataclass
class Structure:
    name: str = ''
    members: list[Member] = field(default_factory=list)
    has_handles: bool = False
    platform: str = ''
    aliases: list[str] = field(default_factory=list)
    pnext_input: bool = False
    pnext_output: bool = False
    stype_value: str = ''
    struct_extends: list[str] = field(default_factory=list)
    # True if this struct is extended (via pNext) by at least one handle-bearing struct, so its
    # pNext chain must be walked for handle keys even when nested inside another struct.
    pnext_extendable: bool = False
    # True if this struct transitively yields handle keys (members / unions / pNext extensions).
    contributes_keys: bool = False

@dataclass
class Union:
    name: str = ''
    members: list[Member] = field(default_factory=list)
    platform: str = ''
    contributes_keys: bool = False
    
@dataclass
class Handle:
    name: str = ''
    type: str = ''
    parent: str = ''
    dispatchable: bool = False

@dataclass
class FunctionPointer:
    name: str = ''
    full_decl: str = ''
    platform: str = ''

@dataclass
class Enum:
    name: str = ''
    values: dict[str, int] = field(default_factory=dict)
    platform: str = ''

@dataclass
class Bitmask:
    name: str = ''
    bitwidth: int = 32
    flag_name: str = ''
    bits: dict[str, int] = field(default_factory=dict)
    platform: str = ''

@dataclass
class Flag:
    name: str = ''
    bitmask_name: str = ''
    bitwidth: int = 32
    base_type: str = ''
    platform: str = ''
