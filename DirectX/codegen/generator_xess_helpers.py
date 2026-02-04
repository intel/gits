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


def is_xess_d3d12_init(function: Function) -> bool:
    return function.name == 'xessD3D12Init'


# XELL

def is_xell_function(function: Function) -> bool:
    return function.api == Api.XELL
