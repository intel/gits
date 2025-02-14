#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file
from intermediates import Api, Function, Parameter


def is_xess_function(function: Function) -> bool:
    return function.api == Api.XESS


def is_context_in_params(params: list[Parameter]) -> bool:
    return any(param.type == 'xess_context_handle_t' and not param.is_pointer for param in params)


def is_context_output_in_params(params: list[Parameter]) -> bool:
    return any(param.type == 'xess_context_handle_t' and param.is_pointer for param in params)


def is_get_xess_context_needed(function: Function) -> bool:
    return is_xess_function(function) and is_context_in_params(function.params)


def is_set_xess_context_needed(function: Function) -> bool:
    return is_xess_function(function) and is_context_output_in_params(function.params)


def is_xess_d3d12_init(function: Function) -> bool:
    return function.name == 'xessD3D12Init'


def generate_xess_dispatch_table(context: dict[str, list], out_paths: list[str]) -> None:
    additional_context = { 'is_xess_function': is_xess_function }
    file_name = 'xessDispatchTableAuto.h'

    for out_path in out_paths:
        generate_file(context | additional_context, file_name, out_path)
