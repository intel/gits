#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *
from generator_dml_helpers import *
from intermediates import Api

def get_pod_field_types(structures):
    pod_field_types = set()
    for structure in structures:
        if structure.api is not Api.DML:
            continue
        for field in structure.fields:
            if field.is_pointer \
                and not field.is_interface \
                and not field.type.endswith('_DESC') \
                and not field.type.endswith('_BINDING'):
                pod_field_types.add(field.type)
    return pod_field_types

def to_serialize(structure):
    to_serialize = [
        '_OPERATOR_DESC',
        '_GRAPH_DESC',
        '_TENSOR_DESC',
        '_GRAPH_NODE_DESC',
        '_GRAPH_EDGE_DESC',
        '_BINDING_DESC',
        '_BINDING'
    ]

    if structure.api is not Api.DML:
        return False
    
    for s in to_serialize:
        if structure.name.endswith(s):
            return True
    
    return False

def get_field_count(field, current_ptr):
    field_count = "1"
    if field.sal_size:
        field_count = current_ptr + field.sal_size
    return field_count

def generate_dml_files(context, out_path):
    additional_context = {
        'to_serialize': to_serialize,
        'get_pod_field_types': get_pod_field_types,
        'get_field_count': get_field_count,
        'dml_struct_is_custom': dml_struct_is_custom,
        'dml_enum_get_type': dml_enum_get_type,
        'dml_enum_get_desc_type': dml_enum_get_desc_type,
        'dml_enum_is_valid': dml_enum_is_valid,
    }
    files_to_generate = [
        'dmlCodersAuto.h',
        'dmlCodersAuto.cpp'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)