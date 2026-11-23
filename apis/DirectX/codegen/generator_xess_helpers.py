#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from intermediates import Api, Function, Parameter

# XESS

def is_xess_function(function: Function) -> bool:
    return function.api == Api.XESS


# XELL

def is_xell_function(function: Function) -> bool:
    return function.api == Api.XELL


# XEFG

def is_xefg_function(function: Function) -> bool:
    return function.api == Api.XEFG


# XESS SDK common

def is_xess_sdk_init_param(param: Parameter) -> bool:
    init_param_types = [
      'xess_d3d12_init_params_t',
      'xefg_swapchain_d3d12_init_params_t'
    ]
    return param.type in init_param_types and param.is_pointer and param.is_const
