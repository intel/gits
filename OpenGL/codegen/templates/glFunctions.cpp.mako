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

<%
    from typing import Any

    name: str  # Keys are OpenGL function names.
    token_versions_data: list[dict[str,Any]]  # Values are complicated.
    # Each dict in the list contains data for one version of a token.
    # Example:
    # 'glFoo': [{glFoo data}, {glFoo_V1 data}]
    token_version_data: dict[str,Any]  # Data for one version.
%>\
% for name, token_versions_data in gl_functions.items():
% for token_version_data in token_versions_data:
<%
    track_state: bool = token_version_data.get('stateTrack') or False
    wrap_run: bool = token_version_data.get('runWrap') or False
    pass_token: bool = token_version_data.get('passToken') or False
    ccode_write_wrap: bool = token_version_data.get('ccodeWriteWrap') or False
    is_draw_func: bool = is_draw_function(token_version_data)

    func_id: str = make_id(name, token_version_data['version'])
    cname: str = make_cname(name, token_version_data['version'])
    run_cond: str = token_version_data.get('runCond') or ''
    state_track_name: str|None = token_version_data.get('stateTrackName')

    ret_type: str = token_version_data['type']
    has_retval: bool = ret_type != 'void'

    args: list[dict[str,str]] = token_version_data['args']
    retval_and_args: list[dict[str,str]]
    if has_retval:
        retval_and_args = [retval_as_arg(token_version_data)] + args
    else:
        retval_and_args = args  # TODO: `.copy()` ?

    params: str = args_to_str(retval_and_args, '{type} {name}, ', ', ')
    deref_args = args_to_str(args, '*_{name}, ', ', ')
    retval_and_deref_args = args_to_str(retval_and_args, '*_{name}, ', ', ')
    normal_args = args_to_str(args, '_{name}, ', ', ')
    retval_and_normal_args = args_to_str(retval_and_args, '_{name}, ', ', ')

    member_init_list = make_member_initializer_list(retval_and_args)

    has_special_args: bool = is_any_arg_special(args)
    special_args: list[dict[str,str]] = expand_special_args(args, False)
    special_args_for_init: list[dict[str,str]] = expand_special_args(args, True)
    special_params: str = args_to_str(special_args, '{type} {name}, ', ', ')
    special_member_init_list: str = make_member_initializer_list(special_args_for_init)

    carg_macro = 'return get_cargument' if args else 'report_cargument_error'
    run_method_name = 'RunImpl' if is_draw_func else 'Run'

    run_call_name: str
    if wrap_run:
        run_wrap_name: str = token_version_data.get('runWrapName')
        assert run_wrap_name is not None, "When using wrapRun, runWrapName is mandatory."
        run_wrap_name += version_suffix_from_token(token_version_data)

        run_call_name = f'{run_wrap_name}_WRAPRUN'
    else:
        run_call_name = f'drv.gl.{name}'

    run_call: str
    if has_retval and not wrap_run:
        run_call = f'_return_value.Assign({run_call_name}({deref_args}));'
    elif has_retval and wrap_run:
        run_call = f'{run_call_name}({retval_and_normal_args});'
    elif not has_retval and wrap_run:
        if pass_token:
            run_call = f'{run_call_name}(this, {normal_args});'
        else:
            run_call = f'{run_call_name}({normal_args});'
    else:
        run_call = f'{run_call_name}({deref_args});'

    remove_mapping_args = [arg for arg in args if arg.get('removeMapping')]
    remove_mappings = args_to_str(remove_mapping_args, '  _{name}.RemoveMapping();\n', '\n')

    remove_mappings_run: str
    if run_cond:
        if wrap_run:
            # TODO: Warn the user that runCond is useless.
            pass
        else:
            # TODO: Use wrap_in_if(str) for both.
            run_call = f'if ({run_cond})\n    {run_call}'
            remove_mappings_run = wrap_in_if(run_cond, remove_mappings)
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
% if track_state and not wrap_run:
  ${state_track_name}_SD(${retval_and_deref_args});
% endif
% if remove_mappings and not wrap_run:
${remove_mappings_run}
% endif
}
\
\
% if ccode_write_wrap:

void gits::OpenGL::${cname}::Write(CCodeOStream& stream) const {
  stream.select(stream.selectCCodeFile());
  ${cname}_CCODEWRITEWRAP(stream, *this);
}
% endif  # ccode_write_wrap
% endfor  # for token_version_data
% endfor  # for name, token_versions_data
