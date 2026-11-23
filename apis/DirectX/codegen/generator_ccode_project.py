#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================
from generator_helpers import *

def function_parameters(params):
    arguments = ""
    for i, param in enumerate(params):
        if i > 0:
            arguments += ", "
        arguments += param
    return arguments

def function_signature(function, interface_name=None):
    parameters = generate_params(function)
    if interface_name:
        # Add a pointer to the interface as the first parameter for interface methods
        object = interface_name + "* object"
        parameters.insert(0, object)
    func_return = generate_return(function)
    func_parameters = function_parameters(parameters)
    
    # ReturnType CC_FunctionName(Parameters)
    return f"{func_return} CC_{function.name}({func_parameters})"

def function_call(function, is_interface_method=False):
    # Regular functions: Function(param1, param2)
    # Interface methods: object->Function(param1, param2)
    str = ""
    if not function.ret.is_void:    
        str += "return "
    if is_interface_method:
        str += "object->"
    str += f"{function.name}("
    str += function_parameters([param.name for param in function.params])
    str += ");"
    return str

def generate_ccode_project_files(context, out_path):
    additional_context = {
        'function_signature': function_signature,
        'function_call': function_call
    }
    files_to_generate = [
        'ccodeApiWrappersAuto.h',
        'ccodeApiWrappersAuto.cpp',
    ]

    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)