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
    length: (str|list[str]) = ''
    is_null_terminated: bool = False
    fixed_array_size: list[str] = field(default_factory=list)
    is_handle: bool = False
    is_handle_output: bool = False
    is_struct: bool = False
    is_struct_with_handles: bool = False
    is_union: bool = False

@dataclass
class Command:
    name: str = ''
    return_type: str = ''
    params: list[Parameter] = field(default_factory=list)
    success_codes: list[str] = field(default_factory=list)
    error_codes: list[str] = field(default_factory=list)
    dispatch_level: str = ''
    platform: str = ''

@dataclass
class Member:
    name: str = ''
    base_type: str = ''
    full_type: str = ''
    is_const: bool = False
    is_void: bool = False
    is_pointer: bool = False
    is_pointer_to_pointer: bool = False
    length: (str|list[str]) = ''
    is_null_terminated: bool = False
    fixed_array_size: list[str] = field(default_factory=list)
    bitfield: (int|None) = None
    values: str = ''
    is_handle: bool = False
    is_struct: bool = False
    is_struct_with_handles: bool = False
    is_union: bool = False

@dataclass
class Structure:
    name: str = ''
    members: list[Member] = field(default_factory=list)
    has_handles: bool = False
    platform: str = ''
    aliases: list[str] = field(default_factory=list)

@dataclass
class Union:
    name: str = ''
    members: list[Member] = field(default_factory=list)
    platform: str = ''
    
@dataclass
class Handle:
    name: str = ''
    type: str = ''
    parent: str = ''
    dispatchable: bool = False

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
