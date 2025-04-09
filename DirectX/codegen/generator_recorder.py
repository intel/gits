#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *
from generator_xess import is_get_xess_context_needed, is_set_xess_context_needed, is_xess_d3d12_init
from intermediates import Parameter
from intermediates import Api

def wrappers_update_created(function, indent):
    str = ''
    if not is_interface_creation(function):
        return str
    previous_param = Parameter()
    first = True
    for param in function.params:
        if param.is_interface_creation:
            if not first:
                str += '\n' + indent
            first = False
            if not param.sal_size:
                if previous_param.type == 'REFIID' and param.is_pointer_to_pointer and param.type == 'void':
                    str += f'UpdateOutputInterface<InterfaceOutputArgument<{param.type}>, {param.type}> update_{param.name}(command.{param.name}_, result, {previous_param.name}, {param.name});'
                else:
                    str += f'UpdateOutputInterface<InterfaceOutputArgument<{param.type}>, {param.type}> update_{param.name}(command.{param.name}_, result, IID_{param.type}, {param.name});'
            else:
                str += f'UpdateOutputInterface<InterfaceOutputArgument<{param.type}>, {param.type}> update_{param.name}(command.{param.name}_, result, IID_{param.type}, {param.name});'
        previous_param = param
    return str

def get_dispatch_table(function):
    dispatch_tables = {
        Api.DXGI: "getDXGIDispatchTable()",
        Api.D3D12: "getD3D12DispatchTable()",
        Api.DML: "getDMLDispatchTable()",
        Api.XESS: "getXessDispatchTable()",
        Api.DSTORAGE: "getDStorageDispatchTable()"
    }
    return dispatch_tables[function.api]

def generate_recorder_files(context, out_path):
    additional_context = {
        'wrappers_update_created': wrappers_update_created,
        'get_dispatch_table': get_dispatch_table,
        'is_get_xess_context_needed': is_get_xess_context_needed,
        'is_set_xess_context_needed': is_set_xess_context_needed,
        'is_xess_d3d12_init': is_xess_d3d12_init
    }
    files_to_generate = [
        'wrappersAuto.h',
        'wrappersAuto.cpp',
        'wrapperCreatorsAuto.h',
        'wrapperCreatorsAuto.cpp',
        'encoderLayerAuto.h',
        'encoderLayerAuto.cpp',
        'globalSynchronizationLayerAuto.h',
        'globalSynchronizationLayerAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
