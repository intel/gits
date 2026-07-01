#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file

def generate_params_for_function(command):
    list = []
    for param in command.params:
        s = ''
        if param.is_handle:
            s += 'Handle'
            if param.length:
                s += 'Array'
            s += f"Argument<{param.base_type}>"
        elif param.is_handle_output:
            s += 'Handle'
            if param.length:
                s += 'Array'
            s += f"OutputArgument<{param.base_type}>"
        elif param.is_opaque_pointer:
            s += f"OpaquePointerArgument<{param.base_type}>"
        elif param.is_pointer and not param.is_opaque_pointer and not param.length:
            if param.is_void:
                if param.is_descriptor_template_data:
                    s += f"DescriptorTemplateDataArgument"
                else:
                    s += f"OpaqueBufferArgument"
            else:
                s += f"PointerArgument<{param.base_type}>"
        elif param.is_pointer and param.length:
            if param.is_void:
                s += f"BufferArgument"
            else:
                s += f"ArrayArgument<{param.base_type}>"
        elif param.fixed_array_size:
            s += f"StaticArrayArgument<{param.base_type}, {', '.join(param.fixed_array_size)}>"
        elif param.is_pointer_to_pointer:
            if param.is_void:
                s += f"BufferOutputArgument"
            else:
                s += f"ArrayOfArrays<{param.base_type}>"
        else:
            s += f"Argument<{param.base_type}>"
        s += ' m_' + param.name
        list.append(s)
    return list

def generate_initializer_list(command):
    initializer_list = []
    for param in command.params:
        s = 'm_' + param.name + '{' + param.name
        if param.length:
            s += ', ' + param.length
        if param.is_pointer_to_pointer and not param.is_void:
            s += ', pInfos'
        s += '}'
        initializer_list.append(s)
    return initializer_list

def generate_layer_files(context, out_path):
    additional_context = {
      'generate_params_for_function': generate_params_for_function,
      'generate_initializer_list': generate_initializer_list
    }
    files_to_generate = [
      'commandIdsAuto.h',
      'commandsAuto.h',
      'layerAuto.h',
      'dispatchTableAuto.h'
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
