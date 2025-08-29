#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import cxxheaderparser.simple
import cxxheaderparser.types

from intermediates import *
from header_preprocessor import preprocess_header

def decode_sal(param):
    pos = param.type.rfind('__')
    if pos > 0:
        pos += 2  
        param.sal = param.type[0:pos]
        param.type = param.type[pos:]

        escapes = {
            '___1___' : '(',
            '___2___' : ')',
            '___3___' : '*',
            '___4___' : ' ',
            '___5___' : '"',
            '___6___' : '-',
            '___7___' : ','
        }

        for s in escapes:
            param.sal = param.sal.replace(s, escapes[s])

        if param.sal.find('_COM_Outptr_') >= 0:
            param.is_interface_creation = True

        sals = [
            '_In_reads_',
            '_Out_writes_',
            '_Out_writes_to_',
            '_Inout_updates_bytes_',
            '_Field_size_',
            '_In_opt_count_',
            '_In_count_',
            '_Field_size_bytes_'
            '_Field_size_opt_'
        ]
        for sal in sals:
            if param.sal.find(sal) >= 0:
                end = param.sal.rfind(')')
                start = param.sal.find('(', 0, end)
                start += 1
                param.sal_size = param.sal[start:end]
                param.sal_size = param.sal_size.strip()
                param.sal_size = param.sal_size
        
    return param

def find_typedef_name(header, type):
    for typedef in header.data.namespace.typedefs:
        if isinstance(typedef.type, cxxheaderparser.types.Type):
            if typedef.type.typename.segments[0].name == type:
                return typedef.name
    return ''

def parse_parameter(cxx_type):
    param = Parameter()
    if isinstance(cxx_type, cxxheaderparser.types.Type):
        if isinstance(cxx_type.typename, cxxheaderparser.types.PQName):
            if isinstance(cxx_type.typename.segments[0], cxxheaderparser.types.NameSpecifier):
                param.type = cxx_type.typename.segments[0].name
            elif isinstance(cxx_type.typename.segments[0], cxxheaderparser.types.FundamentalSpecifier):
                param.type = cxx_type.typename.segments[0].name
                if param.type == 'void':
                    param.is_void = True
            elif isinstance(cxx_type.typename.segments[0], cxxheaderparser.types.AnonymousName):
                param = Union()
            else:
                print(f'ERROR: Unknown cxx_type.typename.segments[0]: {cxx_type.typename.segments[0]}')
        else:
            print(f'ERROR: Unknown cxx_type.typename: {cxx_type.typename}')
    elif isinstance(cxx_type, cxxheaderparser.types.Pointer):
        if isinstance(cxx_type.ptr_to, cxxheaderparser.types.Type):
            param.type = cxx_type.ptr_to.typename.segments[0].name
            param.is_pointer = True
            if cxx_type.ptr_to.const:
                param.is_const = True
        elif isinstance(cxx_type.ptr_to, cxxheaderparser.types.Pointer):
            param.type = cxx_type.ptr_to.ptr_to.typename.segments[0].name
            param.is_pointer_to_pointer = True
            if cxx_type.ptr_to.ptr_to.const:
                param.is_const = True
        else:
           print(f'ERROR: Unknown cxx_type.ptr_to: {cxx_type.ptr_to}') 
    elif isinstance(cxx_type, cxxheaderparser.types.Array):
        if isinstance(cxx_type.array_of, cxxheaderparser.types.Type):
            param.type = cxx_type.array_of.typename.segments[0].name
            param.is_array = True
            param.size = cxx_type.size.tokens[0].value
        elif isinstance(cxx_type.array_of, cxxheaderparser.types.Array):
            param.type = cxx_type.array_of.array_of.typename.segments[0].name
            param.is_array_of_arrays = True
            param.size_secondary = cxx_type.size.tokens[0].value
            param.size = cxx_type.array_of.size.tokens[0].value
        else:
            print(f'ERROR: Unknown cxx_type.array_of: {cxx_type.array_of}')
    else:
        print(f'ERROR: Unknown cxx_type: {cxx_type}')

    param = decode_sal(param)
            
    return param
    
def parse_function(cxx_function):
    function = Function()
    function.ret = parse_parameter(cxx_function.return_type)
    function.name = cxx_function.name.segments[0].name
    for cxx_parameter in cxx_function.parameters:
        parameter = parse_parameter(cxx_parameter.type)
        parameter.name = cxx_parameter.name
        function.params.append(parameter)
    return function

def parse_interface(cxx_interface):
    interface = Interface()
    interface.name = cxx_interface.class_decl.typename.segments[0].name
    interface.base_name = cxx_interface.class_decl.bases[0].typename.segments[0].name
    for cxx_function in cxx_interface.methods:
        function = parse_function(cxx_function)
        interface.functions.append(function)
    return interface


def is_union(cxx_field):
    if isinstance(cxx_field.type, cxxheaderparser.types.Type):
        if isinstance(cxx_field.type.typename, cxxheaderparser.types.PQName):
            if isinstance(cxx_field.type.typename.segments[0], cxxheaderparser.types.AnonymousName):
                return True
    return False

def is_interface(cxx_type):
    interface_prefixes = ['IDStorage', 'IDML', 'ID3D', 'IDXGI', 'IUnknown']
    has_interface_prefix = False
    for prefix in interface_prefixes:
        if prefix in cxx_type:
            has_interface_prefix = True
    return has_interface_prefix

def parse_union(cxx_class):
    union = Union()
    if cxx_class.classes:
        pass # print(f'WARNING: union of structs not handled')
    else:
        for cxx_field in cxx_class.fields:
            field = parse_parameter(cxx_field.type)
            field.name = cxx_field.name
            union.fields.append(field)
    return union


def parse_structure(cxx_structure):
    struct = Struct()
    struct.type = cxx_structure.class_decl.typename.segments[0].name
    for cxx_field in cxx_structure.fields:
        field = None
        if is_union(cxx_field):
            field = parse_union(cxx_structure.classes[0])
        else:
            field = parse_parameter(cxx_field.type)
            field.name = cxx_field.name
        struct.fields.append(field)
    return struct


def parse_interfaces(header):
    interfaces = []

    for cxx_class in header.data.namespace.classes:
        if (cxx_class.class_decl.typename.classkey == 'class'):
            interface =  parse_interface(cxx_class)
            interface.api = header.api
            interfaces.append(interface)

    return interfaces

def postprocess(functions, interfaces, structures):
    for function in functions:
        for param in function.params:
            if is_interface(param.type):
                param.is_interface = True
                if param.sal.find('_Out_') >= 0 or param.sal.find('_Outptr_opt_result_maybenull_') >= 0:
                    param.is_interface_creation = True

    for interface in interfaces:
        for function in interface.functions:
            for param in function.params:
                if is_interface(param.type):
                    param.is_interface = True
                    if param.sal.find('_Out_') >= 0 or param.sal.find('_Outptr_opt_result_maybenull_') >= 0:
                        param.is_interface_creation = True


    structures_with_interfaces = {
        'D3D12_RESOURCE_BARRIER',
        'D3D12_RENDER_PASS_ENDING_ACCESS',
        'D3D12_PIPELINE_STATE_STREAM_DESC',
        'D3D12_STATE_OBJECT_DESC',
        'D3D12_RENDER_PASS_RENDER_TARGET_DESC',
        'D3D12_RENDER_PASS_DEPTH_STENCIL_DESC',        
        'D3D12_BARRIER_GROUP',
        'DML_BUFFER_BINDING',
        'DML_GRAPH_DESC',
        'DML_BINDING_DESC',
        'DML_BINDING_TABLE_DESC',
        'DSTORAGE_QUEUE_DESC',
        'DSTORAGE_REQUEST'
    }

    context_params = {
        'xess_context_handle_t'
    }

    for struct in structures:
        for field in struct.fields:
            if isinstance(field, Parameter):
                if is_interface(field.type):
                    field.is_interface = True
                    struct.has_interfaces = True
                    structures_with_interfaces.add(struct.name)

    for function in functions:
        for param in function.params:
            if param.type in structures_with_interfaces:
                param.structure_with_interfaces = True
            elif param.type in context_params:
                if param.is_pointer:
                    param.is_context_output = True
                else:
                    param.is_context = True

    for interface in interfaces:
        for function in interface.functions:
            for param in function.params:
                if param.type in structures_with_interfaces:
                    param.structure_with_interfaces = True

    latest_interfaces = {}
    for interface in interfaces:
        base_name = re.sub('[0-9]+$', '', interface.name)
        num = 0
        if base_name != interface.name:
            num = int(re.search('[0-9]+$', interface.name)[0])
            if not base_name in latest_interfaces or latest_interfaces[base_name] < num:
                latest_interfaces[base_name] = num

    for interface in interfaces:
        base_name = re.sub('[0-9]+$', '', interface.name)
        interface.latest_interface = base_name
        if base_name in latest_interfaces:
            interface.latest_interface += str(latest_interfaces[base_name])

    structure_removed = None
    for struct in structures:
        if struct.type == '__LUID':
            structure_removed = struct
            break
    if structure_removed:
        structures.remove(structure_removed)

    return interfaces


def parse_structures(header):
    structs = []

    for cxx_class in header.data.namespace.classes:
        if (cxx_class.class_decl.typename.classkey == 'struct'):
            struct = parse_structure(cxx_class)
            struct_name = find_typedef_name(header, struct.type)
            struct.name = struct_name if struct_name else struct.type
            struct.api = header.api
            structs.append(struct)

    return structs


def parse_functions(header):
    functions = []

    custom = {
      'DXGIDisableVBlankVirtualization',
      'DXGIGetDebugInterface'
    }
    
    skip = {
      'D3D12CreateVersionedRootSignatureDeserializerFromSubobjectInLibrary'
    }

    for cxx_function in header.data.namespace.functions:
        if cxx_function.name.segments[0].name in skip:
            continue
        function = parse_function(cxx_function)
        if function.name not in custom:
            function.api = header.api  
            functions.append(function)

    return functions

def parse_enums(header):
    enums = []

    custom = {
        'D3D12_MESSAGE_ID'
    }
    value_aliases = [
        'DML_OPERATOR_SCATTER'
    ]

    for cxx_enum in header.data.namespace.enums:
        cxx_name = cxx_enum.typename.segments[0].name
        seen_values = set()
        if not cxx_name in custom:
            enum = Enum()
            enum.api = header.api
            enum.name = cxx_name
            for cxx_value in cxx_enum.values:
                enum_value = cxx_value.value.tokens[0].value if cxx_value.value else None
                # Attempt to evaluate and convert enum_value to an integer to handle both numeric and hexadecimal values
                try:
                    # Need eval due to hexadecimal values
                    int_value = int(eval(enum_value))
                    is_convertible = True
                except (ValueError, SyntaxError, TypeError, NameError):
                    is_convertible = False
                if cxx_value.name not in value_aliases and enum_value not in seen_values:
                    enum.values.append(cxx_value.name)
                    seen_values.add(cxx_value.name)
                    if is_convertible is True:
                        seen_values.add(enum_value)
            enums.append(enum)

    return enums
