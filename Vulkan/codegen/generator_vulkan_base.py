#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import enum
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any

# TODO: use `@verify(NAMED_FLAGS)` when Python 3.11 becomes available.
class FuncType(IntFlag):
    """Flag for API call classification."""

    NONE = 0
    PARAM = 1
    QUEUE_SUBMIT = 2
    CREATE_IMAGE = 4
    CREATE_BUFFER = 8
    COMMAND_BUFFER_SET = 16
    COMMAND_BUFFER_BIND = 32
    COMMAND_BUFFER_PUSH = 64
    BEGIN_RENDER_PASS = 128
    END_RENDER_PASS = 256
    DRAW = 512
    BLIT = 1024
    DISPATCH = 2048
    NEXT_SUBPASS = 4096


class FuncLevel(enum.Enum):  # Direct Enum import would clash with Enum function.
    """Vulkan API function level."""

    PROTOTYPE = 0
    GLOBAL = 1
    INSTANCE = 2
    DEVICE = 4


@dataclass(frozen=True, kw_only=True)
class Argument:
    """Argument of a Token."""

    name: str
    type: str
    count: str | None = None
    wrap_type: str | None = None
    wrap_params: str | None = None
    remove_mapping: bool = False


@dataclass(frozen=True, kw_only=True)
class ReturnValue(Argument):
    """Return value of a Token."""

    # Only `type` is being used, hardcode the rest.
    name: str = field(init=False, default='return_value')
    type: str
    count: str | None = field(init=False, default=None)
    wrap_type: str | None = field(init=False, default=None)
    wrap_params: str | None = field(init=False, default=None)
    remove_mapping: bool = field(init=False, default=False)


@dataclass(frozen=True, kw_only=True)
class Token:
    """API call (or an internal GITS action)."""

    name: str
    enabled: bool
    function_type: FuncType
    level: FuncLevel = FuncLevel.DEVICE
    version: int = 0
    state_track: bool | str = False
    recorder_wrap: bool | str = False
    exec_post_recorder_wrap: bool = False
    recorder_exec_wrap: bool = False
    run_wrap: bool | str = False
    ccode_wrap: bool = False
    ccode_write_wrap: bool = False
    ccode_post_action_needed: bool | None = None
    plugin_wrap: bool = False
    custom_driver: bool = False
    end_frame_tag: bool = False
    pre_token: str | None = None
    post_token: str | None = None
    token_cache: str | None = None  # TODO: Make it an enum.
    return_value: ReturnValue
    args: list[Argument]


@dataclass(frozen=True, kw_only=True)
class Field:
    """Member variable (field) of a VkStruct."""

    name: str
    type: str
    wrap_type: str | None = None
    wrap_params: str | None = None
    count: str | None = None
    log_condition: str | None = None


@dataclass(frozen=True, kw_only=True)
class VkStruct:
    """Vulkan's C/C++ struct or union."""

    name: str
    enabled: bool
    type: str = 'struct'  # Other possible values include 'union'.
    version: int = 0
    custom: bool = False
    declare_array: bool = False
    declare_array_of_arrays: bool = False
    constructor_arguments: str | None = None
    constructor_wrap: bool = False
    declaration_needed_wrap: bool = False
    pass_struct_storage: bool = False
    fields: list[Field]


@dataclass(frozen=True, kw_only=True)
class Enumerator:
    """Member of a VkEnum."""

    name: str
    value: int


@dataclass(frozen=True, kw_only=True)
class VkEnum:  # TODO: Remove the "Vk" prefix here and in VkStruct when Enum is renamed to enum?
    """Vulkan's C/C++ enum."""

    name: str
    size: int = 32
    enumerators: list[Enumerator]


_enums_dict: dict[str, VkEnum] = {}
_functions_table: list[Token] = []
_structs_table: list[VkStruct] = []


def _merge_enums(a: VkEnum, b: VkEnum) -> VkEnum:
    """Take two VkEnums different only in enumerators and create one with merged enumerators."""
    if a.name != b.name:
        raise ValueError(f"Enum names don't match ('{a.name}', '{b.name}')")
    if a.size != b.size:
        raise ValueError(f"Enum sizes don't match ({a.size}, {b.size})")

    # The last value is VK_*_MAX_ENUM, we keep it at the end.
    merged_enumerators = a.enumerators[:-1] + b.enumerators + [a.enumerators[-1]]

    return VkEnum(name=a.name, size=a.size, enumerators=merged_enumerators)


def _replace_key(dictionary: dict, old_key, new_key) -> None:
    """Replaces the key if present in the dict."""
    if old_key in dictionary:
        dictionary[new_key] = dictionary.pop(old_key)

def _rename_keys(dictionary: dict) -> dict:
    """Rename keys in kwargs to match classes, e.g., 'wrapType' -> 'wrap_type'."""
    replacements: list[tuple[str, str]] = [
        ('wrapType', 'wrap_type'),
        ('wrapParams', 'wrap_params'),
        ('logCondition', 'log_condition'),
        ('declareArray', 'declare_array'),
        ('declareArrayOfArrays', 'declare_array_of_arrays'),
        ('constructorArgs', 'constructor_arguments'),
        ('constructorWrap', 'constructor_wrap'),
        ('declarationNeededWrap', 'declaration_needed_wrap'),
        ('passStructStorage', 'pass_struct_storage'),
        ('removeMapping', 'remove_mapping'),
        ('stateTrack', 'state_track'),
        ('recWrap', 'recorder_wrap'),
        ('execPostRecWrap', 'exec_post_recorder_wrap'),
        ('recExecWrap', 'recorder_exec_wrap'),
        ('runWrap', 'run_wrap'),
        ('ccodeWrap', 'ccode_wrap'),
        ('ccodeWriteWrap', 'ccode_write_wrap'),
        ('ccodePostActionNeeded', 'ccode_post_action_needed'),
        ('pluginWrap', 'plugin_wrap'),
        ('customDriver', 'custom_driver'),
        ('endFrameTag', 'end_frame_tag'),
        ('preToken', 'pre_token'),
        ('postToken', 'post_token'),
        ('tokenCache', 'token_cache'),
        ('retV', 'return_value'),
    ]

    for old, new in replacements:
        _replace_key(dictionary, old, new)

    return dictionary

def _gather_list(dictionary: dict[str, Any], *, prefix: str, list_name: str) -> dict:
    """Collect loose, numbered keys into a list."""
    if dictionary.get(list_name):
        raise RuntimeError(
            f"`{list_name}` list already present, did you call this function twice?")

    elems: list[Argument | Field] = []
    for i in range(1, 10000):
        elem = f'{prefix}{i}'
        if elem in dictionary:
            elems.append(dictionary.pop(elem))
        else:
            break
    dictionary[list_name] = elems

    return dictionary

def _preprocess_kwargs(dictionary: dict, *, prefix: str, list_name: str) -> dict:
    """Change kwargs to be suitable as dataclass constructor arguments."""
    renamed: dict = _rename_keys(dictionary)
    return _gather_list(renamed, prefix=prefix, list_name=list_name)


def Enum(**kwargs):
    """Add a VkEnum to the list (or merge enumerators if already present)."""
    enum = VkEnum(**kwargs)
    if enum.name not in _enums_dict:
        _enums_dict[enum.name] = enum
    else:
        _enums_dict[enum.name] = _merge_enums(_enums_dict[enum.name], enum)

def Function(**kwargs):
    # We want to convert `type` only in Tokens, not in other classes.
    _replace_key(kwargs, 'type', 'function_type')
    _functions_table.append(Token(**_preprocess_kwargs(kwargs, prefix='arg', list_name='args')))

def Struct(**kwargs):
    _structs_table.append(VkStruct(**_preprocess_kwargs(kwargs, prefix='var', list_name='fields')))

def ArgDef(**kwargs):
    return Argument(**_rename_keys(kwargs))

def RetDef(**kwargs):
    return ReturnValue(**_rename_keys(kwargs))

def VarDef(**kwargs):
    if 'value' in kwargs and 'type' in kwargs:
        raise ValueError(f"VarDef has both value and type arguments: {kwargs}")
    elif 'value' in kwargs:
        return Enumerator(**kwargs)
    elif 'type' in kwargs:
        return Field(**_rename_keys(kwargs))
    else:
        raise ValueError(f"VarDef is missing both value and type arguments: {kwargs}")

def get_enums():
    return _enums_dict.values()

def get_functions():
    return _functions_table

def get_structs():
    return _structs_table
