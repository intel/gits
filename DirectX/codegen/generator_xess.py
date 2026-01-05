#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from intermediates import Api, Function, Parameter
from generator_xess_helpers import *
from generator_helpers import generate_file


def generate_xess_dispatch_table(context: dict[str, list], out_paths: list[str]) -> None:
    additional_context = { 'is_xess_function': is_xess_function }
    file_name = 'xessDispatchTableAuto.h'

    for out_path in out_paths:
        generate_file(context | additional_context, file_name, out_path)
