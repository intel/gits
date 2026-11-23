#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import array
from dataclasses import dataclass, field
from enum import Enum

class Api(Enum):
    COMMON = 1  
    DXGI = 2
    DXGI_DEBUG = 3
    D3D12 = 4
    D3D12_DEBUG = 5
    DML = 6
    XESS = 7
    DSTORAGE = 8
    XELL = 9
    XEFG = 10

@dataclass
class Parameter:
    name: str = ''
    type: str = ''
    is_const: bool = False
    is_pointer: bool = False
    is_pointer_to_pointer: bool = False
    is_array: bool = False
    is_array_of_arrays: bool = False
    is_interface: bool = False
    is_void: bool = False
    size: int = None
    size_secondary: int = None
    sal: str = ''
    is_interface_creation: bool = False # _COM_Outptr_, _Out_writes_, _Out_
    sal_size: str = '' # _In_reads_, _Out_writes_, _Inout_updates_bytes_, _Field_size_, _In_opt_count_, _In_count_
    is_parameter: bool = True
    structure_with_interfaces: bool = False
    is_context: bool = False
    is_context_output: bool = False

@dataclass
class Function:
    name: str = ''
    ret: Parameter = None
    params: list[Parameter] = field(default_factory=list)
    api: Api = None

@dataclass
class Interface:
    name: str = ''
    base_name : str = ''
    functions: list[Function] = field(default_factory=list)
    latest_interface: str = ''
    api: Api = None

@dataclass
class Union:
    fields: list[None] = field(default_factory=list) # [Parameter | Struct]
    is_parameter: bool = False
    api: Api = None
    
@dataclass
class Struct:
    name: str = ''
    type: str = ''
    fields: list[None] = field(default_factory=list) # [Parameter | Union]
    has_interfaces: bool = False
    has_unions: bool = False
    api: Api = None

@dataclass
class Enum:
    name: str = ''
    values: list[str] = field(default_factory=list)
    api: Api = None
