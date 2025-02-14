#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from intermediates import Api

def dml_struct_is_custom(structure):
    is_custom = [
        'DML_OPERATOR_DESC',
        'DML_TENSOR_DESC',
        'DML_GRAPH_EDGE_DESC',
        'DML_GRAPH_NODE_DESC',
        'DML_BINDING_DESC'
    ]
    return structure.name in is_custom

def dml_enum_get_type(enums, desc_type):
    enum_type = desc_type.replace('DESC', 'TYPE')
    for enum in enums:
        if enum.name == enum_type:
            return enum
    return None

def dml_enum_is_valid(enum_name):
    return (not enum_name.endswith('INVALID') and not enum_name.endswith('NONE'))

def dml_enum_get_desc_type(enum_name):
    if not dml_enum_is_valid(enum_name):
        return 'INVALID'
    
    if enum_name.startswith('DML_OPERATOR_'):
        desc_type = enum_name[len('DML_OPERATOR_'):]
        desc_type = f"DML_{desc_type}_OPERATOR_DESC"
        return desc_type
    
    type_name_map = {
        # TENSOR
        'DML_TENSOR_TYPE_BUFFER': 'DML_BUFFER_TENSOR_DESC',
        # GRAPH NODE
        'DML_GRAPH_NODE_TYPE_OPERATOR': 'DML_OPERATOR_GRAPH_NODE_DESC',
        'DML_GRAPH_NODE_TYPE_CONSTANT': 'DML_CONSTANT_DATA_GRAPH_NODE_DESC',
        # GRAPH EDGE
        'DML_GRAPH_EDGE_TYPE_INPUT': 'DML_INPUT_GRAPH_EDGE_DESC',
        'DML_GRAPH_EDGE_TYPE_OUTPUT': 'DML_OUTPUT_GRAPH_EDGE_DESC',
        'DML_GRAPH_EDGE_TYPE_INTERMEDIATE': 'DML_INTERMEDIATE_GRAPH_EDGE_DESC',
        # BINDING
        'DML_BINDING_TYPE_BUFFER': 'DML_BUFFER_BINDING',
        'DML_BINDING_TYPE_BUFFER_ARRAY': 'DML_BUFFER_ARRAY_BINDING',
    }
    return type_name_map[enum_name]