#!python
#!/usr/bin/env python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import cl_constants
import cl_utils
import generator_cl
import re


def _token_default_body(func):
    ret_must_be_mapped = ((func['functionType'] == generator_cl.Creator and func['type'] != 'cl_int') or func['name'] == 'clLinkProgram' or 'EnqueueMap' in func['name'])
    has_ret_val = (not func['type'] == 'void')
    run = ''
    arguments_list = cl_utils.arguments_list_from_function(func, name_prefix='*_')
    function_call = cl_utils.format_function_call(func['name'] if not func['version'] else cl_utils.cut_version(func), arguments_list)
    if func.get('runCond'):
        run += 'if({0})\n    '.format(func['runCond'])
    if ret_must_be_mapped:
        run += '_return_value.Assign(drvOcl.{0});'.format(function_call)
    else:
        if func['type'] != 'void':
            run += '_return_value.Value() = drvOcl.{0};'.format(function_call)
        else:
            run += 'drvOcl.{0};'.format(function_call)
    if func.get('stateTrack'):
        arguments_with_ret_deref = cl_utils.arguments_list_from_function(func, name_prefix='*_', retval=has_ret_val)
        arguments_list_with_token = list(arguments_with_ret_deref)
        if func.get('passToken'):
            arguments_list_with_token.insert(0, ('CFunction*', 'this'))
        elif func.get('passNullToken'):
            arguments_list_with_token.insert(0, ('CFunction*', 'nullptr'))
        run += '\n  ' + cl_utils.format_function_call(func['stateTrackName'] + '_SD', arguments_list_with_token) + ';'
    return run


def generate_tokens(functions):
    with open(cl_constants.TOKENS_HEADER_NAME, 'w') as tokens_h, \
         open(cl_constants.TOKENS_SOURCE_NAME, 'w') as tokens_c:
        tokens_h.write(cl_constants.COPYRIGHT_HEADER)
        tokens_c.write(cl_constants.COPYRIGHT_HEADER)

        tokens_h.write(cl_constants.TOKENS_HEADER_START)
        tokens_c.write(cl_constants.TOKENS_SOURCE_START)

        for key in sorted(functions.keys()):
            func = functions[key]
            if not func['enabled']:
                continue
            ret_must_be_mapped = ((func['functionType'] == generator_cl.Creator and func['type'] != 'cl_int') or func['name'] == 'clLinkProgram' or 'EnqueueMap' in func['name'])
            header_template = cl_constants.TOKENS_HEADER_TEMPLATE
            has_ret_val = True
            if func['type'] == 'void':
                has_ret_val = False
                header_template = cl_constants.TOKENS_HEADER_TEMPLATE_VOID
            header_template = header_template.format(
                id=func['id'],
                name=key,
                version=cl_utils.make_version(func),
                arg_num=len(func['args']),
                result_num='0' if key.startswith('clGetExtensionFunctionAddress') or has_ret_val is False else '1',
                constructor='\n' + cl_utils.constructor_declaration(func, indent=6),
                properties=cl_utils.token_properties(func, has_ret_val),
                ccode=cl_constants.TOKENS_HEADER_CCODE if func.get('ccodeWrap') else '',
                write_post_call=cl_constants.TOKENS_HEADER_WRITE_POST_CALL if ret_must_be_mapped else '',
                key_name=cl_utils.cut_version(func) if func['version'] else key
            )

            prefix, postfix = cl_utils.get_platform_defs(func)

            tokens_h.write(prefix + header_template + postfix)

            ccode_func = cl_constants.TOKENS_SOURCE_CCODE.format(name=key, ccode=func.get('ccodeWrap'))
            write_post_call = cl_constants.TOKENS_SOURCE_WRITE_POST_CALL.format(name=key)
            run = ''
            arguments_with_ret = cl_utils.arguments_list_from_function(func, name_prefix='_', retval=True)
            if 'D3D' in key or 'DX9' in key:
                run += 'D3DWarning();\n  '
            if func.get('runWrap'):
                arguments_list_with_token = cl_utils.remove_void_return_value(arguments_with_ret)
                if func.get('passToken'):
                    arguments_list_with_token.insert(0, ('CFunction*', 'this'))
                run += cl_utils.format_function_call(func['runWrapName'] + '_RUNWRAP', arguments_list_with_token) + ';'
            else:
                run += _token_default_body(func)
            run += cl_utils.format_remove_mapping(func)

            func_def_template = cl_constants.TOKENS_SOURCE_FUNCTION.format(
                separator=cl_utils.token_separator(func),
                constructor=cl_utils.constructor_definition(func),
                name=key,
                run=run,
                argument_impl=cl_utils.argument_implementation(func),
                result_impl=cl_utils.result_implementation(func),
                ccode=ccode_func if func.get('ccodeWrap') else '',
                write_post_call=write_post_call if ret_must_be_mapped else ''
            )
            tokens_c.write(prefix + func_def_template + postfix)

        tokens_h.write(cl_constants.TOKENS_HEADER_END)


def _format_trace_arguments(list_, enums):
    if not list_:
        return ''

    def _get_tracing_wrapper_type(argument):
        if argument['name'] == 'errcode_ret':
            return 'TCLResult'
        arg_type = cl_utils._extract_type(argument)
        if arg_type in enums.keys():
            return 'T' + arg_type
        return arg_type

    def _get_tracing_params(argument):
        params = '{name}'
        if argument.get('wrapParams') and (argument['wrapParams'].count(',') == 1 or cl_utils._is_zero_ended(argument)) \
            and not argument['name'] == 'lengths' \
            and not argument['name'] == 'mapped_ptr':
            # if there are exactly two arguments in wrapParams and it's not
            # lengths and mapped_ptr
            params = argument.get('wrapParams')
        else:
            params = '1, {name}'
        return params.format(name=argument['name'])

    def _argument(argument):
        if cl_utils._is_pointer(argument) or 'void*' in argument['type'] \
            and not argument.get('wrapType') in ['CAsyncBinaryData', 'CSVMPtr', 'CUSMPtr', 'CUSMAreaPtr', 'CCLMappedPtr', 'CSVMPtr_V1']:
            array = 'TOclArray<{}, {}>'.format(cl_utils._extract_type(argument), _get_tracing_wrapper_type(argument))
            return '{}({})'.format(array, _get_tracing_params(argument))
        else:
            if argument['type'] in enums.keys():
                return 'T{}({})'.format(argument['type'], argument['name'])
        return argument['name']

    return ', '.join([_argument(arg) for arg in list_])


def generate_drivers(functions, enums):
    with open(cl_constants.DRIVERS_NAME, 'w') as drivers:
        drivers.write(cl_constants.COPYRIGHT_HEADER)
        drivers.write(cl_constants.DRIVERS_START)
        for key in sorted(functions.keys()):
            func = functions[key]
            if func['version']:
                continue
            arguments_list = cl_utils.arguments_list_from_function(func)
            drivers_function = cl_constants.DRIVERS_FUNCTION.format(
                rettype=func['type'] + ('_t' if func['type'] == 'void' else ''),
                name=key,
                args_type=cl_utils.format_arguments(arguments_list, newlines=False),
                args=cl_utils.format_arguments(arguments_list, with_types=False, newlines=False),
                trace_args=_format_trace_arguments(func['args'], enums)
            )
            drivers.write(' \\\n' + drivers_function)
        drivers.write('\n')
        drivers.write('\n')

        drivers.write(cl_constants.DRIVERS_ENUM_START)
        for key in sorted(enums.keys()):
            if enums[key].get('type'):
                continue
            drivers_enum = cl_constants.DRIVERS_ENUM.format(key)
            drivers.write(' \\\n' + drivers_enum)
        drivers.write('\n')


def generate_ids(functions):
    functions_ids = []
    with open(cl_constants.IDS_NAME, 'r') as ids_old:
        for line in ids_old:
            elem = line.strip(",\n")
            equal_sign = line.find(' ')
            if equal_sign != -1:
                elem = elem[:equal_sign]
            if 'ID' in elem:
                functions_ids.append(elem)

    with open(cl_constants.IDS_NAME, 'a') as ids:
        for key in sorted(functions.keys()):
            id_ = functions[key]['id']
            if id_ not in functions_ids:
                ids.write(id_ + ',\n')


def generate_prepost(functions):
    with open(cl_constants.PREPOST_NAME, 'w') as prepost:
        prepost.write(cl_constants.COPYRIGHT_HEADER)
        prepost.write(cl_constants.PREPOST_START)
        extension_functions = []
        for key in sorted(functions.keys()):
            func = functions[key]

            if func['version']:
                continue
            arguments_list = cl_utils.arguments_list_from_function(func, retval=True)
            drv_call = cl_utils.format_function_call(key, arguments_list[1:])
            arguments = cl_utils.format_arguments(arguments_list[1:], indent=2)
            args_without_void_return = cl_utils.remove_void_return_value(arguments_list)
            wrapper_call = cl_utils.format_function_call(key, args_without_void_return)

            auto = ''
            retval = ''
            return_recwrap = ''
            if cl_utils.has_return_value(func):
                auto = 'auto return_value = '
                retval = '\n  return return_value;'
                return_recwrap = '  return '

            if func.get('recExecWrap'):
                body = '\n' + return_recwrap + cl_utils.format_function_call(func['name'] + '_RECEXECWRAP', arguments_list[1:]) + ';\n'
            else:
                body = cl_constants.PREPOST_FUNCTION_BODY.format(
                    auto=auto, drv_call=drv_call, wrapper_call=wrapper_call, retval=retval
                )

            prepost_string = cl_constants.PREPOST_FUNCTION
            # because mac OS...
            if func['name'] == "clCreateFromGLTexture2D" or func['name'] == "clCreateFromGLTexture3D":
                prepost_string = cl_constants.PREPOST_FUNCTION_EXT

            prepost_function = prepost_string.format(
                rettype=func['type'], name=key, arguments=arguments,
                version=cl_utils.make_version(func), body=body
            )
            prefix, postfix = cl_utils.get_platform_defs(func)

            if func['extension']:
                function_string = prefix + cl_constants.PREPOST_EXTENSION_FUNCTION_ENTRY.format(key) + postfix
                extension_functions.append(function_string)

            prepost.write(prefix)
            prepost.write(prepost_function)
            prepost.write(postfix)
        prepost.write(cl_constants.PREPOST_END)

        prepost.write(cl_constants.PREPOST_EXTENSION_FUNCTION.format(''.join(extension_functions), '\"Function \"', '\" is not implemented in GITS\"'))



def generate_switch(functions):
    with open(cl_constants.SWITCH_NAME, 'w') as switch:
        switch.write(cl_constants.COPYRIGHT_HEADER)
        for key in sorted(functions.keys()):
            func = functions[key]

            prefix, postfix = cl_utils.get_platform_defs(func)

            if func['enabled']:
                switch.write(prefix)
                switch.write(''.join(['case ', func['id'], ':\n']))
                switch.write(''.join(['return new C', key, ';\n']))
                switch.write(postfix)


def generate_wrapper_headers(functions):
    base_line = 'void {name}({arguments}) const'
    interface_line = ' '.join(['virtual', base_line, '= 0;\n'])
    wrapper_line = ' '.join([base_line, 'override;\n'])

    with open(cl_constants.WRAPPER_HEADER_NAME, 'w') as wrapper, \
         open(cl_constants.WRAPPER_INTERFACE_HEADER_NAME, 'w') as wrapper_interface:
        wrapper.write(cl_constants.COPYRIGHT_HEADER)
        wrapper_interface.write(cl_constants.COPYRIGHT_HEADER)
        for key in sorted(functions.keys()):
            func = functions[key]
            if func['version']:
                continue
            arguments_list = cl_utils.arguments_list_from_function(func, retval=True)
            arguments_list = cl_utils.remove_void_return_value(arguments_list)

            prefix, postfix = cl_utils.get_platform_defs(func)

            arguments = cl_utils.format_arguments(arguments_list, newlines=False)
            wrapper.write(prefix)
            wrapper.write(wrapper_line.format(name=key, arguments=arguments))
            wrapper.write(postfix)
            wrapper_interface.write(prefix)
            wrapper_interface.write(interface_line.format(name=key, arguments=arguments))
            wrapper_interface.write(postfix)


def generate_wrapper_source(functions):
    with open(cl_constants.WRAPPER_SOURCE_NAME, 'w') as wrapper_source:
        wrapper_source.write(cl_constants.COPYRIGHT_HEADER)
        wrapper_source.write(cl_constants.WRAPPER_SOURCE_START)
        sorted_funcs = sorted(functions.keys())

        for idx, key in enumerate(sorted_funcs):
            func = functions[key]
            if re.search(key+'_V'+r'[0-9]+',' '.join([str(elem) for elem in sorted_funcs[idx:]])):
                continue
            arguments_list = cl_utils.arguments_list_from_function(func, retval=True)
            arguments_list = cl_utils.remove_void_return_value(arguments_list)

            rec_wrap = cl_constants.WRAPPER_NOT_IMPLEMENTED.format(name=key)
            if func['enabled']:
                if not func.get('recWrap'):
                    pre_schedule, pre_token, post_token, post_schedule, state_dynamic = ('',)*5
                    rec_cond = '_recorder.Running()'
                    if func.get('preSchedule'):
                        pre_schedule = func['preSchedule'] + ';'
                    if func.get('preToken'):
                        pre_token = cl_constants.WRAPPER_SCHEDULE.format(call=func['preToken'])
                    schedule = cl_constants.WRAPPER_SCHEDULE.format(call=cl_utils.format_function_call(key, arguments_list))
                    if func.get('postToken'):
                        post_token = cl_constants.WRAPPER_SCHEDULE.format(call=func['postToken'])
                    if func.get('postSchedule'):
                        post_schedule = func['postSchedule'] + ';'
                    if func.get('stateTrack'):
                        arguments_list_with_token = list(arguments_list)
                        if func.get('passToken'):
                            arguments_list_with_token.insert(0, ('CFunction*', '_token'))
                        state_dynamic = cl_utils.format_function_call(func['stateTrackName'] + '_SD', arguments_list_with_token) + ';'
                    if func.get('recCond'):
                        rec_cond = func['recCond']
                    body = [x for x in [pre_schedule, pre_token, schedule, post_token, post_schedule] if x != '']
                    kernel_wrap = 'KernelCallWrapperPrePost kernelWrapper(_recorder);\n' if func['functionType'] == generator_cl.NDRange else ''
                    rec_wrap = kernel_wrap + cl_constants.WRAPPER_DEFAULT.format(
                        condition=rec_cond,
                        body='\n    '.join(body),
                        state_dynamic=state_dynamic
                    )
                else:
                    rec_wrap = cl_utils.format_function_call(func['recWrapName'] + '_RECWRAP', [('CRecorder&', '_recorder')] + arguments_list) + ';'

            if ('D3D' in key or 'DX9' in key) and func.get('platform') == 'Windows':
                rec_wrap = '\n  D3DWarning();\n  ' + rec_wrap;

            wrapper_function = cl_constants.WRAPPER_FUNCTION.format(
                name=key if not func['version'] else cl_utils.cut_version(func),
                arguments=cl_utils.format_arguments(arguments_list, indent=2),
                rec_wrap=rec_wrap.strip()
            )
            prefix, postfix = cl_utils.get_platform_defs(func)
            wrapper_source.write(prefix + wrapper_function + postfix)
        wrapper_source.write(cl_constants.WRAPPER_SOURCE_END)


def generate_runwrap(functions):
    wrappers = open(cl_constants.PLAYER_RUNWRAP_NAME, 'r').read()
    wrappers = wrappers.replace(cl_constants.PLAYER_RUNWRAP_END, '')
    with open(cl_constants.PLAYER_RUNWRAP_NAME, 'w') as runwrap:
        runwrap.write(wrappers)
        for key in sorted(functions.keys()):
            func = functions[key]
            if not func['enabled']:
                continue
            if func['runWrapName'] + '_RUNWRAP' in wrappers:
                continue
            if func.get('runWrap'):
                arguments_list = cl_utils.arguments_list_from_function(func, process_types=True, name_prefix='&_', retval=True)
                if func.get('passToken'):
                    arguments_list.insert(0, ('CFunction*', '_token'))
                wrap_function = cl_constants.PLAYER_RUNWRAP_FUNCTION.format(name=key, args=cl_utils.format_arguments(arguments_list, newlines=False, expand_callbacks=False), body=_token_default_body(func))
                prefix, postfix = cl_utils.get_platform_defs(func)
                runwrap.write(prefix + wrap_function + postfix)

        runwrap.write(cl_constants.PLAYER_RUNWRAP_END)


def generate_state_tracking(functions):
    statetracking = open(cl_constants.PLAYER_STATETRACK_NAME, 'r').read()
    statetracking = statetracking.replace(cl_constants.PLAYER_STATETRACK_END, '')
    with open(cl_constants.PLAYER_STATETRACK_NAME, 'w') as sd:
        sd.write(statetracking)
        for key in sorted(functions.keys()):
            func = functions[key]
            if not func['enabled']:
                continue
            if func['stateTrackName'] + '_SD' in statetracking:
                continue
            if func.get('stateTrack'):
                arguments_list = cl_utils.arguments_list_from_function(func, retval=True)
                if func.get('passToken'):
                    arguments_list.insert(0, ('CFunction*', 'token'))
                wrap_function = cl_constants.PLAYER_STATETRACK_FUNCTION.format(name=key, args=cl_utils.format_arguments(arguments_list, newlines=False))
                prefix, postfix = cl_utils.get_platform_defs(func)
                sd.write(prefix + wrap_function + postfix)

        sd.write(cl_constants.PLAYER_STATETRACK_END)


def generate_recorder_subwrappers(functions):
    subwrappers = open(cl_constants.RECORDER_SUBWRAPPERS_NAME, 'r').read()
    subwrappers = subwrappers.replace(cl_constants.RECORDER_SUBWRAPPERS_END, '')
    with open(cl_constants.RECORDER_SUBWRAPPERS_NAME, 'w') as recwrap:
        recwrap.write(subwrappers)
        for key in sorted(functions.keys()):
            func = functions[key]
            if func['recWrapName'] + '_RECWRAP' in subwrappers:
                continue
            if re.search(func['recWrapName'] + '_V' + r'[0-9]+' + '_RECWRAP', subwrappers):
                continue
            if func.get('recWrap'):
                arguments_list = cl_utils.arguments_list_from_function(func, retval=True)
                arguments_list.insert(0, ('CRecorder&', 'recorder'))
                wrap_function = cl_constants.RECORDER_SUBWRAPPERS_FUNCTION.format(name=key, args=cl_utils.format_arguments(arguments_list, newlines=False))
                prefix, postfix = cl_utils.get_platform_defs(func)
                recwrap.write(prefix + wrap_function + postfix)

        recwrap.write(cl_constants.RECORDER_SUBWRAPPERS_END)


def generate_recexecwrap(functions):
    wrappers = open(cl_constants.RECEXECWRAP_NAME, 'r').read()
    wrappers = wrappers.replace(cl_constants.RECEXECWRAP_END, '')
    with open(cl_constants.RECEXECWRAP_NAME, 'w') as recexecwrap:
        recexecwrap.write(wrappers)
        for key in sorted(functions.keys()):
            func = functions[key]
            if not func['enabled']:
                continue
            if func['recExecWrapName'] + '_RECEXECWRAP' in wrappers:
                continue
            if func.get('recExecWrap'):
                arguments_list = cl_utils.arguments_list_from_function(func)
                wrap_function = cl_constants.RECEXECWRAP_FUNCTION.format(name=key, rettype=func['type'], args=cl_utils.format_arguments(arguments_list, newlines=False, expand_callbacks=False))
                prefix, postfix = cl_utils.get_platform_defs(func)
                recexecwrap.write(prefix + wrap_function + postfix)

        recexecwrap.write(cl_constants.RECEXECWRAP_END)


def generate_def(functions):
    with open(cl_constants.DEF_NAME, 'w') as exports:
        exports.write(cl_constants.COPYRIGHT_HEADER.replace('*', ';').replace('/', ';'))
        exports.write(cl_constants.DEF_START)

        for key in sorted(functions.keys()):
            func = functions[key]
            if func['version']:
                continue
            exports.write('  ')
            exports.write(func['name'] + '\n')


def generate_arguments(enums, arguments):
    with open(cl_constants.ARGUMENTS_HEADER_NAME, 'w') as header, \
         open(cl_constants.ARGUMENTS_SOURCE_NAME, 'w') as source:
        header.write(cl_constants.COPYRIGHT_HEADER)
        header.write(cl_constants.ARGUMENTS_HEADER_START)
        source.write(cl_constants.COPYRIGHT_HEADER)
        source.write(cl_constants.ARGUMENTS_SOURCE_START)

        declarations = []
        definitions = []
        for name in sorted(enums):
            enum = enums[name]
            type_ = enum['name'] if not enum.get('type') else enum['type']
            declarations.append(cl_constants.ARGUMENTS_TOSTRING_DECLARATION.format(name, type_))
            cases = []
            vars_arr = enum['vars'] # changes shouldn't be visible outside
            if type_ == 'cl_device_type':
                cases.append(cl_constants.ARGUMENTS_TOSTRING_BITFIELD_ALL)
                vars_arr = vars_arr[:-1] # cut off last value
            for var in vars_arr:
                if enum.get('bitfield'):
                    cases.append(cl_constants.ARGUMENTS_TOSTRING_BITFIELD_CASE.format('static_cast<cl_bitfield>('+var['value']+')', var['name']))
                else:
                    cases.append(cl_constants.ARGUMENTS_TOSTRING_CASE.format(var['value'], var['name']))
            if enum.get('bitfield'):
                body = cl_constants.ARGUMENTS_TOSTRING_BITFIELD.format(type_, '\n'.join(cases), name)
            else:
                body = cl_constants.ARGUMENTS_TOSTRING_SWITCH.format('\n'.join(cases), type_)
            definitions.append(cl_constants.ARGUMENTS_TOSTRING_DEFINITION.format(name, type_, body))

        args = []
        arg_names = []
        for name in sorted(enums):
            enum = enums[name]
            if enum.get('custom_argument'):
                continue
            type_ = enum['name'] if not enum.get('type') else enum['type']
            tostring_impl = '{{ return {}ToString(Value()); }}'.format(name)
            ccode = ''
            if enum.get('custom_tostring'):
                tostring_impl = 'override;'
            if enum.get('custom_ccode'):
                ccode = cl_constants.ARGUMENTS_ARGUMENT_DECLARATION_CCODE
            tostring = cl_constants.ARGUMENTS_ARGUMENT_DECLARATION_TOSTRING.format(tostring_impl)
            args.append(cl_constants.ARGUMENTS_ARGUMENT_DECLARATION.format(name, type_, tostring, ccode, ''))
            arg_names.append(cl_constants.ARGUMENTS_ARGUMENT_NAME.format(name, type_))

        for name in sorted(arguments):
            argument = arguments[name]
            type_ = argument['name'] if not argument.get('type') else argument['type']
            tostring = ''
            ccode = ''
            obj = ''
            if argument.get('custom_tostring'):
                tostring = cl_constants.ARGUMENTS_ARGUMENT_DECLARATION_TOSTRING.format('override;')
            if argument.get('custom_ccode'):
                ccode = cl_constants.ARGUMENTS_ARGUMENT_DECLARATION_CCODE
            if argument.get('obj'):
                obj = 'Obj'
            args.append(cl_constants.ARGUMENTS_ARGUMENT_DECLARATION.format(name, type_, tostring, ccode, obj))
            arg_names.append(cl_constants.ARGUMENTS_ARGUMENT_NAME.format(name, type_))

        header.write('\n'.join(declarations))
        header.write('\n\n')
        header.write('\n'.join(args))
        header.write(cl_constants.ARGUMENTS_HEADER_END)

        source.write('\n'.join(definitions))
        source.write('\n\n')
        source.write('\n'.join(arg_names))
        source.write(cl_constants.ARGUMENTS_SOURCE_END)


def generate_header(enums):
    with open(cl_constants.HEADER_NAME, 'w') as header:
        header.write(cl_constants.COPYRIGHT_HEADER)

        definitions = []
        added = []
        for name in sorted(enums):
            enum = enums[name]
            definitions.append('\n/* {} */'.format(enum['name']))
            defines = []
            for var in enum['vars']:
                if not var['name'] in added:
                    defines.append(('#define ' + var['name'], var['value']))
                    added.append(var['name'])

            defines_str = cl_utils.format_arguments(defines, separator='', expand_callbacks=False, min_space=60)
            definitions.append(defines_str)

        header.write('\n'.join(definitions))


def generate_lua_constants(enums):
    with open(cl_constants.LUA_CONSTANTS_NAME, 'w') as header:
        header.write(cl_constants.COPYRIGHT_HEADER.replace('* ', '-- ')
                                                  .replace('/', '-')
                                                  .replace('**', '--')
                                                  .replace('*', '--'))

        definitions = []
        added = []
        for name in sorted(enums):
            enum = enums[name]
            definitions.append('\n-- {}'.format(enum['name']))
            defines = []
            for var in enum['vars']:
                if not var['name'] in added:
                    if enum['name'].startswith('cl_') and enum['name'] != 'cl_build_status':
                        val = "{:#06x}".format(eval(var['value'])) # eval needed for bitshifts
                    else:
                        val = var['value']
                    defines.append((var['name'], '= ' + val))
                    added.append(var['name'])

            defines_str = cl_utils.format_arguments(defines, separator='', expand_callbacks=False, min_space=60)
            definitions.append(defines_str)

        header.write('\n'.join(definitions))
