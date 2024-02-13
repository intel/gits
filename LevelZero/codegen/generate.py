#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import copy
import os
import sys
import re

import mako.template
import mako.exceptions

import generator_l0

def _make_id(name):
    id_ = re.sub('([a-z])([A-Z])', r'\g<1>_\g<2>', name)
    id_ = re.sub('(D3D[0-9]+)([A-Z])', r'\g<1>_\g<2>', id_)
    id_ = re.sub('(DX[0-9])([A-Z])', r'\g<1>_\g<2>', id_)
    id_ = re.sub('([A-Z])([A-Z])([a-z])', r'\g<1>_\g<2>\g<3>', id_)
    id_ = re.sub('([^_])(KHR|EXT|INTEL)', r'\g<1>_\g<2>', id_)
    id_ = re.sub('^ze', 'l0', id_)

    id_final = 'ID_' + id_.upper().strip('_')
    return id_final

def cut_version(name, version):
    return name.replace('_V' + str(version), '')

def is_latest_version(all_items, item):
    if item.get('name') + '_V' + str(item.get('version') + 1) in all_items.keys():
        return False
    return True

def get_object_versions(arguments, name):
    object_list = {}
    for k, v in arguments.items():
        if v.get('name') == name:
            object_list[k] = v
    return object_list

def get_namespace(name):
    return name.split("_")[0]

def get_api_version_from_string(api_version: str):
    return api_version.replace('ZE_API_VERSION_', '').replace('_', '.')

def is_function_included(api_version:str, function_version :str):
    if function_version == '0':
        return True
    api_version = get_api_version_from_string(api_version)
    if api_version >= function_version:
        return True
    return False

def sort_dditable(functions_to_group, functions):
    sorted_dditable = {}
    for function in functions_to_group:
        for name, func in functions.items():
            if function == name:
                if "ddi_pos" not in func:
                    raise Exception("Please fill 'ddi_pos' inside generator_l0.py for function name:", function)
                sorted_dditable[func['ddi_pos']] = function
                continue
    return dict(sorted(sorted_dditable.items(), key=lambda x:x[0])).values()


def get_ddi_table_functions(func, ddi_helper_functions, api_version: str):
    ddi_struct_type = ""
    for arg in func["args"]:
        if arg["name"] == "pDdiTable":
            ddi_struct_type = arg["type"].replace("*", "")
    component = ddi_struct_type.replace("_dditable_t", "")
    component_functions = set()
    for name, tmp_func in ddi_helper_functions.items():
        if not is_latest_version(ddi_helper_functions, tmp_func):
            continue
        if tmp_func.get("component") == component and is_function_included(
            api_version, tmp_func.get("api_version", "0")
        ):
            component_functions.add(cut_version(name, tmp_func.get("version")))
    return component_functions


def process(functions):
    # add implicit properties
    for name, func in functions.items():
        if func.get('unprotectLogic', False) and func.get('recExecWrap', False):
            raise Exception("'unprotectLogic' and 'recExecWrap' are mutually exclusive")
        functions[name]['version'] = func.get('version', 0)
        functions[name]['recExecWrapName'] = (name if not func['version'] else func.get('name')) + '_RECEXECWRAP'
        functions[name]['runWrapName'] = name + '_RUNWRAP'
        functions[name]['stateTrackName'] = (name if not func['version'] else func.get('name')) + '_SD'
        functions[name]['recWrapName'] = (name if not func['version'] else func.get('name')) + '_RECWRAP'
        functions[name]['id'] = _make_id(name)

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

        # this step must be performed after inheritance has been processed
        # check if these changes were not already done
        if not functions[name].get('functionType'):
            #functions[name]['functionType'] = functions[name]['type']
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

def process_arguments(arguments):
    for name in arguments.keys():
        if arguments[name].get('obj'):
            arguments[name]["mutual_mapping"] = []
        arguments[name]['version'] = arguments[name].get('version', 0)
        if arguments[name].get('alias'):
            arguments[arguments[name].get('alias')]['mutual_mapping'].append(name)
            arguments[name]['mutual_mapping'].append(arguments[name].get('alias'))
        if not arguments[name].get('vars'):
            vars = [int(s.replace('var', '')) for s in arguments[name].keys() if s.startswith('var')]
            if vars:
                arguments[name]['vars'] = list()
                for idx in range(1, max(vars) + 1):
                    idx = 'var' + str(idx)
                    if arguments[name].get(idx):
                        arguments[name]['vars'].append(arguments[name][idx])
                        del arguments[name][idx]

def get_arg_name(arg_name: str):
    if '[' in arg_name:
        return arg_name.split('[')[0]
    return arg_name

def get_arg_type(arg_name: str, arg_type: str):
    if '[' in arg_name:
        return arg_type + '*'
    return arg_type

def make_params(
    func, prefix=str(), with_retval=False, with_types=False, prepend_comma=False, with_array=False
):
    contain_args = len(func["args"]) > 0
    str_params = ", " if prepend_comma and contain_args else ""
    if with_retval and func.get("type") != "void":
        if with_types:
            str_params += f"{func['type']} "
        str_params += prefix + "return_value"
    if contain_args:
        if with_retval and func.get("type") != "void":
            str_params += ", "
        str_params += ", ".join(
            [
                prefix + ((arg["type"] + " ") if with_types else "") + (arg["name"] if with_array else get_arg_name(arg["name"]))
                for arg in func["args"]
            ]
        )
    return str_params


def make_lua_params(func):
    return make_params(func).replace("end", "end_")

def has_vars(argType, arguments):
    if "const " in argType:
        argType = argType.split("const ")[1]
    if "*" in argType:
        argType = argType.split("*")[0]
    argType = argType.replace(" ", "")
    return 'vars' in arguments[argType] if argType in arguments else False

def get_used_types(functions, arguments, enums):
    used_types = set()
    for func in functions.values():
        for arg in func['args']:
            used_types.add(arg['type'])
    for arg in arguments.values():
        if 'vars' in arg:
            for var in arg['vars']:
                used_types.add(var['type'])
    for name, enum in enums.items():
        if name == "ze_structure_type_t":
            for item in enum.get('vars'):
                used_types.add("const " + item.get('struct') + "*")
    return used_types

def split_field_name(name):
    return name[:-1].split('[') if '[' in name else [name, '']

def get_field_name(argument, prefix='', wrap_params=None):
    if wrap_params:
        name = wrap_params.replace("{name}", prefix + split_field_name(argument['name'])[0])
        return prefix + name
    return prefix + split_field_name(argument['name'])[0]

def get_field_array_size(argument):
    return split_field_name(argument['name'])[1]

def extract_type(type):
    return (
        type.replace("const ", "")
        .replace("*", "")
        .replace("ze_bool_t", "uint8_t")
        .replace("ze_rtas_builder_packed_input_data_format_exp_t", "uint8_t")
        .replace("ze_rtas_builder_packed_geometry_exp_flags_t", "uint8_t")
        .replace("ze_rtas_builder_packed_geometry_type_exp_t", "uint8_t")
        .replace("ze_rtas_builder_packed_instance_exp_flags_t", "uint8_t")
    )


def get_field_type(argument):
    type = argument['type']
    raw_type = extract_type(type)
    if 'wrapType' in argument:
        return argument['wrapType']
    elif '[' in argument['name'] or type == 'const char*':
        return 'C' + raw_type + '::CSArray'
    elif 'void*' in type:
        return 'CvoidPtr'
    return 'C' + raw_type

def get_wrap_type(argument):
    type = argument['type']
    tag = argument['tag']
    if argument['wrapType'] != argument['type']:
        return argument['wrapType']
    raw_type = extract_type(type)
    if not 'handle' in type and 'out' == tag:
        return 'COutArgument'
    elif 'callbacks' in type or 'void' in type:
        return 'CvoidPtr'
    elif '*' in type and not argument.get('custom'):
        return 'C' + raw_type + '::' + ('CSMapArray' if 'handle' in type and 'out' in tag else 'CSArray')
    elif argument.get('version') != None:
        return 'C' + raw_type + '_V' + argument.get('version')
    return 'C' + raw_type


def makoWrite(inpath, outpath, **args):
    try:
        template = mako.template.Template(filename=inpath)
        rendered = template.render(
            make_params=make_params,
            make_lua_params=make_lua_params,
            has_vars=has_vars,
            get_wrap_type=get_wrap_type,
            get_field_type=get_field_type,
            get_field_name=get_field_name,
            get_field_array_size=get_field_array_size,
            is_latest_version=is_latest_version,
            get_object_versions=get_object_versions,
            get_namespace=get_namespace,
            cut_version=cut_version,
            get_ddi_table_functions=get_ddi_table_functions,
            sort_dditable=sort_dditable,
            get_arg_name=get_arg_name,
            get_arg_type=get_arg_type,
            **args)
        rendered = re.sub(r"\r\n", r"\n", rendered)

        with open(outpath, 'w') as fout:
            fout.write(rendered)
    except:
        traceback = mako.exceptions.RichTraceback()
        for (filename, lineno, function, line) in traceback.traceback:
            print("%s(%s) : error in %s" % (filename, lineno, function))
            print(line, "\n")
        print("%s: %s" % (str(traceback.error.__class__.__name__), traceback.error))
        return -1
    return 0

def prepare_dev_files(dir):
    files = [
        'recorder/include/l0RecorderSubWrappers.h',
        'common/include/l0PlayerRunWrap.h',
        'common/include/l0StateTracking.h',
        'interceptor/include/l0ExecWrap.h'

    ]
    for f in files:
        source_read = open(os.path.join(dir, f), 'r').read()
        source_read = source_read.replace('\n} // namespace l0\n} // namespace gits', '')
        with open(os.path.join(dir, f), 'w') as source_write:
            source_write.write(source_read)

def main():
    if len(sys.argv) < 2:
        print('Usage: {} outpath'.format(sys.argv[0]))
        sys.exit(1)

    pathin = ''
    pathout = sys.argv[1]

    functions = generator_l0.get_functions()
    process(functions)
    enums = generator_l0.get_enums()
    process_enums(enums)
    arguments = generator_l0.get_arguments()
    process_arguments(arguments)
    constants = generator_l0.get_constants()

    if sys.argv[1] == 'update':
        dir = os.path.dirname(os.path.dirname(__file__))
        prepare_dev_files(dir)
        dev_files = {}
        dev_files['common'] = [
            'l0PlayerRunWrap.h',
            'l0StateTracking.h'
        ]
        dev_files['recorder'] = [
            'l0RecorderSubWrappers.h'
        ]
        dev_files['interceptor'] = [
            'l0ExecWrap.h'
        ]
        for proj in dev_files:
            for f in dev_files[proj]:
                dev_path = os.path.join(dir, proj, f)
                if f.endswith('.h'):
                    dev_path = os.path.join(dir, proj, 'include', f)
                ret_val = makoWrite(
                    os.path.join(dir, 'codegen/templates', f + ".mako"),
                    dev_path,
                    output_path=dev_path,
                    functions=functions,
                    enums=enums,
                    arguments=arguments,
                    constants=constants)
                if ret_val != 0:
                    sys.exit(ret_val)
        sys.exit(0)

    print("Generating %s..."%os.path.join(pathout, 'l0IDs.h'))
    functions_ids = []
    with open('l0IDs.h', 'r') as ids_old:
        for line in ids_old:
            elem = line.strip(",\n")
            equal_sign = line.find(' ')
            if equal_sign != -1:
                elem = elem[:equal_sign]
            if 'ID' in elem:
                functions_ids.append(elem)

    with open('l0IDs.h', 'a') as ids:
        for _, func in sorted(functions.items()):
            id_ = func['id']
            if id_ not in functions_ids:
                ids.write(id_ + ',\n')

    with open('l0IDs.h', 'r') as ids, open(os.path.join(pathout, 'l0IDs.h'), 'w') as ids_gen:
        ids_gen.write(ids.read())

    files = [
        'l0EntryPoints.cpp',
        'l0WrapperFunctions.cpp',
        'l0WrapperFunctions.h',
        'l0WrapperFunctionsIface.h',
        'l0DriversInit.cpp',
        'l0ArgumentsAuto.h',
        'l0ArgumentsAuto.cpp',
        'l0Function.cpp',
        'l0Functions.h',
        'l0Functions.cpp',
        'l0Header.h',
        'l0Lua.h',
        'l0Lua.cpp',
        'l0Log.h',
        'helperL0.h',
        'helperL0.cpp',
        'l0APITest.lua',
        'l0Constants.lua'
    ]

    for f in files:
        print("Generating %s..."%os.path.join(pathout, f))
        ret_val = makoWrite(
            os.path.join(pathin, 'templates', f + ".mako"),
            os.path.join(pathout, f),
            functions=functions,
            enums=enums,
            arguments=arguments,
            constants=constants,
            used_types=get_used_types(functions, arguments, enums))
        if ret_val != 0:
            sys.exit(ret_val)


if __name__ == '__main__':
    main()
