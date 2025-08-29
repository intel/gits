#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import *

def generate_params_for_function(function):
    custom = [
        'D3D12_GRAPHICS_PIPELINE_STATE_DESC',
        'D3D12_COMPUTE_PIPELINE_STATE_DESC',
        'D3D12_TEXTURE_COPY_LOCATION',
        'D3D12_RESOURCE_BARRIER',
        'D3D12_GPU_VIRTUAL_ADDRESS',
        'D3D12_CONSTANT_BUFFER_VIEW_DESC',
        'D3D12_STREAM_OUTPUT_BUFFER_VIEW',
        'D3D12_VERTEX_BUFFER_VIEW',
        'D3D12_INDEX_BUFFER_VIEW',
        'D3D12_WRITEBUFFERIMMEDIATE_PARAMETER',
        'D3D12_PIPELINE_STATE_STREAM_DESC',
        'D3D12_STATE_OBJECT_DESC',
        'D3D12_RENDER_PASS_RENDER_TARGET_DESC',
        'D3D12_RENDER_PASS_DEPTH_STENCIL_DESC',
        'D3D12_BARRIER_GROUP',
        'DML_BINDING_TABLE_DESC',
        'DML_GRAPH_DESC',
        'DML_BINDING_DESC',
        'DML_OPERATOR_DESC',
        'LPCWSTR',
        'PCWSTR',
        'WCHAR',
        'PCSTR',
        'D3D12_SHADER_RESOURCE_VIEW_DESC',
        'DSTORAGE_REQUEST',
        'DSTORAGE_QUEUE_DESC',
        'xess_d3d12_init_params_t',
        'xess_d3d12_execute_params_t'
    ]

    list = []
    for param in function.params:
        pointerArg = False
        bufferArg = False
        customArg = False
        arrayArg = False
        str = ''
        if param.type in custom and not param.sal_size:
            str += param.type + '_'
            customArg = True
        elif param.type in custom and param.sal_size:
            str += param.type + 's_'
            customArg = True            
        elif param.sal.find('_Outptr_opt_result_bytebuffer_') >= 0:
            str += 'OutputBuffer'
            customArg = True
        elif param.is_interface_creation:
            str += 'InterfaceOutput'        
        elif param.is_interface and not param.sal_size:
            str += 'Interface'
        elif param.is_interface and param.sal_size:
            str += 'InterfaceArray'
        elif param.type == 'D3D12_CPU_DESCRIPTOR_HANDLE' or param.type == 'D3D12_GPU_DESCRIPTOR_HANDLE':
            if not param.sal_size:
                str += 'DescriptorHandle'  
            else:  
                str += 'DescriptorHandleArray'
                arrayArg = True
        elif param.type == 'xess_context_handle_t':
            str += 'XESSContext'
            if param.is_pointer:
                str += 'Output'
            customArg = True
        elif param.is_pointer and param.type == 'void' and param.sal_size:
            str += 'Buffer'
            bufferArg = True
        elif param.is_pointer and param.type != 'void' and param.sal_size:
            str += 'Array'
            arrayArg = True            
        elif param.is_pointer and param.type != 'void':
            str += 'Pointer'
            pointerArg = True
        elif param.is_array:
            str += 'StaticArray'           
        str += 'Argument'
        if not bufferArg and not customArg:
            str += '<'
            if param.is_const and not pointerArg and not arrayArg:
                str += 'const '
            if param.type == 'REFIID':
                str += 'IID'
            elif param.type == 'REFCLSID':
                str += 'CLSID'
            elif param.type == 'REFGUID':
                str += 'GUID'
            else:  
                str += param.type
            if not pointerArg and not arrayArg and not param.is_interface and not param.is_interface_creation:
                if param.is_pointer:
                    str += '*'
                elif param.is_pointer_to_pointer:
                    str += '**'
            elif param.is_array_of_arrays:
                print('ERROR param not handled: ' + param)
            if param.is_array:
                str += ', ' + param.size
            str += '>'
        str += ' ' + param.name + '_'
        list.append(str)
    return list

def generate_layer_files(context, out_path):
    additional_context = {
        'generate_params_for_function': generate_params_for_function
    }
    files_to_generate = [
        'include/commandIdsAuto.h',
        'include/commandsAuto.h',
        'include/layerAuto.h',
    ]
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
