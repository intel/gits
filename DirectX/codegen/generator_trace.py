#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from textwrap import dedent
from generator_helpers import *
from generator_dml_helpers import *

PRINT_ARRAY_VALUES = [
  'DML_BUFFER_TENSOR_DESC',
  'DML_DEQUANTIZE_OPERATOR_DESC',
  'DML_QUANTIZE_OPERATOR_DESC',
  'DML_MEAN_VARIANCE_NORMALIZATION2_OPERATOR_DESC',
  'DML_QUANTIZED_LINEAR_AVERAGE_POOLING_OPERATOR_DESC',
  'DML_AVERAGE_POOLING1_OPERATOR_DESC',
  'DML_LP_POOLING1_OPERATOR_DESC',
  'DML_RESAMPLE_GRAD1_OPERATOR_DESC',
  'DML_RESAMPLE2_OPERATOR_DESC',
  'DML_ACTIVATION_HARDMAX1_OPERATOR_DESC',
  'DML_ACTIVATION_LOG_SOFTMAX1_OPERATOR_DESC',
  'DML_ACTIVATION_SOFTMAX1_OPERATOR_DESC',
  'DML_RESAMPLE_GRAD_OPERATOR_DESC',
  'DML_MAX_POOLING_GRAD_OPERATOR_DESC',
  'DML_AVERAGE_POOLING_GRAD_OPERATOR_DESC',
  'DML_QUANTIZED_LINEAR_CONVOLUTION_OPERATOR_DESC',
  'DML_CONVOLUTION_INTEGER_OPERATOR_DESC',
  'DML_RESAMPLE1_OPERATOR_DESC',
  'DML_MEAN_VARIANCE_NORMALIZATION1_OPERATOR_DESC',
  'DML_SLICE1_OPERATOR_DESC',
  'DML_MAX_POOLING2_OPERATOR_DESC',
  'DML_MAX_POOLING1_OPERATOR_DESC',
  'DML_GRU_OPERATOR_DESC',
  'DML_LSTM_OPERATOR_DESC',
  'DML_RNN_OPERATOR_DESC',
  'DML_TILE_OPERATOR_DESC',
  'DML_VALUE_SCALE_2D_OPERATOR_DESC',
  'DML_PADDING_OPERATOR_DESC',
  'DML_JOIN_OPERATOR_DESC',
  'DML_SPLIT_OPERATOR_DESC',
  'DML_SLICE_OPERATOR_DESC',
  'DML_MAX_POOLING_OPERATOR_DESC',
  'DML_LP_POOLING_OPERATOR_DESC',
  'DML_AVERAGE_POOLING_OPERATOR_DESC',
  'DML_REDUCE_OPERATOR_DESC',
  'DML_CONVOLUTION_OPERATOR_DESC'
]

def print_struct_values(struct):
    str = f'  stream << "{struct.name}{{";\n'
    for i, field in enumerate(struct.fields):
        is_last = (i == (len(struct.fields) - 1))
        is_array = False
        separator = '' if is_last else ' << ", "'
        representation = '"union"'
        if field.is_parameter:
            if field.sal_size:
                representation = '"array"'
                is_array = True
            else:
                representation = f'value.{field.name}'
        if is_array and (struct.name in PRINT_ARRAY_VALUES):
            str += f'  printArray(stream, value.{field.sal_size}, value.{field.name}){separator};\n'
        elif field.is_parameter and not is_array and field.type in ["wchar_t", "LPCWSTR", "WCHAR"]:
            str += f'  printString(stream, value.{field.name}){separator};\n'
        elif field.is_parameter and field.is_array:
            str += f'  printStaticArray(stream, value.{field.name}){separator};\n'
        elif field.is_parameter and field.is_array_of_arrays:
            str += f'  printStatic2DArray(stream, value.{field.name}){separator};\n'
        else:
            str += f'  stream << {representation}{separator};\n'
    str += '  stream << "}";\n'
    return str

def generate_trace_files(context, out_path):
    additional_context = {
        'print_struct_values': print_struct_values,
        'dml_enum_get_type': dml_enum_get_type,
        'dml_enum_get_desc_type': dml_enum_get_desc_type,
        'dml_enum_is_valid': dml_enum_is_valid,
        'dml_struct_is_custom': dml_struct_is_custom,
    }
    files_to_generate = [
        'traceLayerAuto.h',
        'traceLayerAuto.cpp',
        'printStructuresAuto.h',
        'printStructuresAuto.cpp',
        'printEnumsAuto.h',
        'printEnumsAuto.cpp'
    ]

    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
