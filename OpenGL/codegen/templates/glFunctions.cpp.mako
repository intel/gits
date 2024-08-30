// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================


#include "glFunctions.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "streams.h"
#include "platform.h"
#include "openglTools.h"
#include "tools.h"
#include "stateTracking.h"
#include "playerRunWrap.h"
#include "playerRunWrapConditions.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

% for name, token_versions in gl_functions.items():
% for token in token_versions:
<%
    func_id: str = make_id(name, token.version)
    cname: str = make_cname(name, token.version)

    has_retval: bool = token.return_value.type != 'void'
    is_draw_func: bool = is_draw_function(token.function_type)

    args: list[Argument] = token.args
    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [retval_as_arg(token.return_value)] + args
    else:
        retval_and_args = args

    params = args_to_str(retval_and_args, '{type} {name}, ', ', ')
    deref_args = args_to_str(args, '*_{name}, ', ', ')
    retval_and_deref_args = args_to_str(retval_and_args, '*_{name}, ', ', ')
    normal_args = args_to_str(args, '_{name}, ', ', ')
    retval_and_normal_args = args_to_str(retval_and_args, '_{name}, ', ', ')

    member_init_list = make_member_initializer_list(retval_and_args)

    has_special_args: bool = is_any_arg_special(args)
    special_args = expand_special_args(args, only_add_wrap_params=False)
    special_args_for_init = expand_special_args(args, only_add_wrap_params=True)
    special_params: str = args_to_str(special_args, '{type} {name}, ', ', ')
    special_member_init_list: str = make_member_initializer_list(special_args_for_init)

    carg_macro = 'return get_cargument' if args else 'report_cargument_error'
    run_method_name = 'RunImpl' if is_draw_func else 'Run'

    run_call_name: str
    if token.run_wrap:
        run_call_name = f'{token.run_wrap}{version_suffix_from_token(token)}_WRAPRUN'
    else:
        run_call_name = f'drv.gl.{name}'

    run_call: str
    if has_retval and not token.run_wrap:
        run_call = f'_return_value.Assign({run_call_name}({deref_args}));'
    elif has_retval and token.run_wrap:
        run_call = f'{run_call_name}({retval_and_normal_args});'
    elif not has_retval and token.run_wrap:
        if token.pass_token:
            run_call = f'{run_call_name}(this, {normal_args});'
        else:
            run_call = f'{run_call_name}({normal_args});'
    else:
        run_call = f'{run_call_name}({deref_args});'

    remove_mapping_args = [arg for arg in args if arg.remove_mapping]
    remove_mappings = args_to_str(remove_mapping_args, '  _{name}.RemoveMapping();\n', '\n')

    remove_mappings_run: str
    if token.run_condition:
        if token.run_wrap:
            print(f"WARNING: run_condition on {name} is overwritten by run_wrap!")
        else:
            # TODO: Use wrap_in_if(str) for both.
            run_call = f'if ({token.run_condition})\n    {run_call}'
            remove_mappings_run = wrap_in_if(token.run_condition, remove_mappings)
    else:
        remove_mappings_run = remove_mappings
%>\

/* ***************************** ${func_id} *************************** */


gits::OpenGL::${cname}::${cname}()
{
}

% if params:

gits::OpenGL::${cname}::${cname}(${params})${member_init_list}
{
% if remove_mappings:
${remove_mappings}
% endif
}

% if has_special_args:
gits::OpenGL::${cname}::${cname}(${special_params})${special_member_init_list}
{
}

% endif  # has_special_args\

% endif  # params

gits::CArgument &gits::OpenGL::${cname}::Argument(unsigned idx)
{
  ${carg_macro}(__FUNCTION__, idx${f', {normal_args}'.rstrip(', ')});
}


void gits::OpenGL::${cname}::${run_method_name}()
{
  ${run_call}
% if token.state_track and not token.run_wrap:
  ${token.state_track}_SD(${retval_and_deref_args});
% endif
% if remove_mappings and not token.run_wrap:
${remove_mappings_run}
% endif
}
\
\
% if token.ccode_write_wrap:

void gits::OpenGL::${cname}::Write(CCodeOStream& stream) const {
  stream.select(stream.selectCCodeFile());
  ${cname}_CCODEWRITEWRAP(stream, *this);
}
% endif  # token.ccode_write_wrap
% endfor  # for token
% endfor  # for name, token_versions
