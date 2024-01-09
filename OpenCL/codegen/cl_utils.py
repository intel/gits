#!python
#!/usr/bin/env python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import copy
import re
import warnings

import cl_constants


def _extract_type(argument):
    return argument['type'].replace('const ', '').replace('*', '')


def _is_type(object_, type_, strict=False):
    """

    :param object: function or argument
    :param type: string or tuple
    :param strict:
    :return: True or False
    """
    def compare(op1, op2):
        return op1 == op2 if strict else op1 in op2

    if isinstance(type_, str):
        return compare(type_, object_['wrapType']) or compare(type_, object_['type'])
    elif isinstance(type_, (tuple, list)):
        return any([compare(t, object_['wrapType']) or compare(t, object_['type']) for t in type_])


def _is_zero_ended(argument):
    # cl_command_queue_properties does not represent
    # a zero ended array of properties, it's an enum
    return _is_type(argument, 'const char*', strict=True) or \
        (_is_pointer(argument) and _is_type(argument, ('properties', 'property')) and _extract_type(argument) != 'cl_command_queue_properties')


def _is_pointer(argument):
    return _is_type(argument, '*') and _is_type(argument, ('cl', 'size_t'))


def _init_list_argument(arg):
    if arg.get('wrapParams'):
        try:
            wrap_params = arg['wrapParams'].format(name=arg['name'])
        except (KeyError, ValueError):
            warnings.warn('Use {{ or }} in wrapParams', SyntaxWarning)
            wrap_params = arg['wrapParams']
        return '_{0}({1})'.format(arg['name'], wrap_params)

    lst = list()
    if _is_pointer(arg):
        lst.append('1')
    lst.append(arg['name'])
    if _is_zero_ended(arg):
        lst.append('0, 1')
    return '_{0}({1})'.format(arg['name'], ', '.join(lst))


def _constructor_init_list(function):
    lst = list()
    if has_return_value(function):
        lst.append('_{0}({0})'.format('return_value'))
    for arg in function['args']:
        lst.append(_init_list_argument(arg))
    return ',\n'.join(lst)


def _constructor_arguments(function, indent=-1):
    has_ret_val = True if function['type'] != 'void' else False
    arguments_list = arguments_list_from_function(function, retval=has_ret_val)
    return format_arguments(arguments_list, indent=indent)


def format_function_call(function_name, arguments_list):
    """

    :param function_name:
    :param arguments_list: list of tuples (type, name)
    :return: formatted C++ function call
    """
    return '{0}({1})'.format(
        function_name, format_arguments(arguments_list, with_types=False, newlines=False)
    )


def format_arguments(list_, separator=',', with_types=True, newlines=True, indent=-1, min_space=0, expand_callbacks=True):
    """

    :param list_: list of tuples (type, name)
    :param with_types:
    :param newlines:
    :param indent: block indentation
    :return: formatted arguments for function definition
    """
    if not list_:
        return ''

    def _argument(argument):
        if 'Callback' in argument[0] and expand_callbacks:
            return cl_constants.CALLBACKS['C' + argument[0]].format(name=argument[1])
        else:
            type_indent = max(max(max_arg_len, min_space) - len(argument[0]), 0) + 1
            return ''.join([argument[0], (' ' * type_indent), argument[1]])

    if newlines:
        glue = (separator + '\n') + (indent * ' ')
        max_arg_len = len(max([arg[0] for arg in list_], key=len))
    else:
        glue = (separator + ' ')
        max_arg_len = 0

    return glue.join([_argument(arg) if with_types else arg[1] for arg in list_])


def has_return_value(function):
    return function['type'] != 'void'

def remove_void_return_value(arguments_list):
    if arguments_list[0] == ('void', 'return_value') or arguments_list[0] == ('void', '_return_value'):
        return arguments_list[1:]
    return arguments_list

def _has_arguments(function):
    return len(function['args']) > 0


def arguments_list_from_function(function, process_types=False, name_prefix='', retval=False):
    def _return_type(function):
        if function['wrapType'] != function['type']:
            return function['wrapType']
        else:
            if function['type'] == 'cl_int':
                return 'CCLResult'
            elif function['type'] == 'void*':
                return 'CCLMappedPtr'
        return 'C' + function['type']

    def _argument_type(argument):
        name, type_ = argument['name'], argument['wrapType']
        if name == 'user_data':
            type_ = 'CLUserData'
        elif type_.endswith('int*') and name == 'errcode_ret':
            type_ = 'CLResult::CSArray'
        elif type_.endswith('*'):
            type_ = _extract_type(argument) + '::CSArray'
        if argument['type'] == argument['wrapType']:
            type_ = 'C' + type_
        return type_


    if process_types:
        arg_process = _argument_type
        ret_process = _return_type
    else:
        arg_process = lambda obj: obj['type']
        ret_process = lambda obj: obj['type']

    lst = list()
    if retval:# and has_return_value(function):
        lst.append((ret_process(function), name_prefix + 'return_value'))
    for arg in function['args']:
        lst.append((arg_process(arg), name_prefix + arg['name']))
    return lst


def constructor_declaration(function, indent=-1):
    indent_str = (indent * ' ') if indent > 0 else ''
    start = indent_str + 'C{name}('.format(name=function['name'])
    arguments = _constructor_arguments(function, indent=len(start))
    end = ');'

    return ''.join([start, arguments, end])


def constructor_definition(function):
    namespace = 'gits::OpenCL::'
    start = namespace + 'C{name}::C{name}('.format(name=function['name'])
    end = '):\n'
    arguments = _constructor_arguments(function, indent=len(start))
    body_start = '\n{'
    init_list = _constructor_init_list(function)
    body_end = '\n}\n'
    remove_mapping = format_remove_mapping(function)

    return ''.join([start, arguments, end, init_list, body_start, remove_mapping, body_end])


def token_separator(function):
    name = function['name']
    line_capacity = 72 # 80 - comment * 2 - ' ' * 4
    decoration = int(line_capacity / 2 - len(name) / 2) * '*'
    return ' '.join(['/*', decoration, name, decoration, '*/'])


def token_properties(function, has_ret_val=True):
    arguments_list = arguments_list_from_function(function, process_types=True, name_prefix='_', retval=has_ret_val)
    return format_arguments(arguments_list, separator=';', indent=6, expand_callbacks=False) + ';'


def argument_implementation(function):
    if _has_arguments(function):
        args = ', '.join(['_' + a['name'] for a in function['args']])
        return cl_constants.TOKENS_CARGUMENT.format(args=args)
    else:
        return cl_constants.TOKENS_SWITCH.format(name=function['name'], what='Argument')


def result_implementation(function):
    if has_return_value(function) and not function['name'].startswith('clGetExtensionFunctionAddress'):
        args = '_errcode_ret'
        if function['type'] == 'cl_int' or function['name'] == 'clSVMAlloc':
            args = '_return_value'
        return cl_constants.TOKENS_CARGUMENT.format(args=args)
    else:
        return cl_constants.TOKENS_SWITCH.format(name=function['name'], what='Result')


def format_remove_mapping(function):
    body = []
    for arg in function['args']:
        if arg.get('removeMapping'):
            body.append('_{0}.RemoveMapping();'.format(arg['name']))
    body_str = '\n  '.join(body)
    return ('\n  ' if body_str else '') + body_str


def _make_id(name, version):
    id_ = re.sub('([a-z])([A-Z])', r'\g<1>_\g<2>', name)
    id_ = re.sub('(D3D[0-9]+)([A-Z])', r'\g<1>_\g<2>', id_)
    id_ = re.sub('(DX[0-9])([A-Z])', r'\g<1>_\g<2>', id_)
    id_ = re.sub('([^_])(KHR|EXT|INTEL)', r'\g<1>_\g<2>', id_)

    id_final = 'ID_' + id_.upper().strip('_')
    if (version > 0):
        id_final = id_final + '_V' + str(version)
    return id_final


def make_version(function):
    return function['availableFrom'].replace('.', '_')


def get_platform_defs(function):
    prepend = ''
    if function.get('platform'):
        if not function['platform'].startswith('!'):
            prepend = cl_constants.IFDEF_PLATFORM + function['platform'].upper()
        else:
            prepend = cl_constants.IFNDEF_PLATFORM + function['platform'][1:].upper()
    if prepend:
        return (prepend + '\n', cl_constants.ENDIF_PLATFORM + '\n')
    return ('', '')


def process(functions):
    # add implicit properties
    for name, func in functions.items():
        functions[name]['version'] = func.get('version')
        functions[name]['recExecWrapName'] = name if not func['version'] else cut_version(func)
        functions[name]['runWrapName'] = name
        functions[name]['stateTrackName'] = name if not func['version'] else cut_version(func)

    # process inheritance
    for name, func in functions.items():
        if func.get('inheritFrom'):
            # preserve properties
            extension = ''
            enabled = functions[name]['enabled']
            version = functions[name]['availableFrom']
            if functions[name].get('extension'):
                extension = functions[name]['extension']
            functions[name] = copy.deepcopy(functions[func['inheritFrom']])
            functions[name]['enabled'] = enabled
            functions[name]['availableFrom'] = version
            functions[name]['extension'] = extension

        functions[name]['name'] = name
        functions[name]['recWrapName'] = name
        functions[name]['id'] = _make_id(name, 0)

        # this step must be performed after inheritance has been processed
        # check if these changes were not already done
        if not functions[name].get('functionType'):
            functions[name]['functionType'] = functions[name]['type']
            functions[name]['type'] = functions[name]['retV']['type']
            functions[name]['wrapType'] = functions[name]['retV']['type']
            if functions[name]['retV'].get('wrapType'):
                functions[name]['wrapType'] = functions[name]['retV']['wrapType']
            if functions[name]['retV'].get('wrapParams'):
                functions[name]['wrapParams'] = functions[name]['retV']['wrapParams']
            del functions[name]['retV']

        if not functions[name].get('args'):
            functions[name]['args'] = list()
            args = len([s for s in functions[name].keys() if s.startswith('arg')])
            for idx in range(1, args):
                idx = 'arg' + str(idx)
                if not functions[name][idx].get('wrapType'):
                    functions[name][idx]['wrapType'] = functions[name][idx]['type']
                functions[name]['args'].append(functions[name][idx])
                del functions[name][idx]

def cut_version(func):
    return func['name'].replace('_V' + str(func['version']), '')

def process_enums(enums):
    for name in enums.keys():
        if not enums[name].get('vars'):
            args = max([int(s.replace('var', '')) for s in enums[name].keys() if s.startswith('var')]) + 1
            enums[name]['vars'] = list()
            for idx in range(1, args):
                idx = 'var' + str(idx)
                if enums[name].get(idx):
                    enums[name]['vars'].append(enums[name][idx])
                    del enums[name][idx]
