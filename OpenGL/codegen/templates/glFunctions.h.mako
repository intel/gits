// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================


#pragma once

#include "gits.h"
#include "log.h"
#include "openglTypes.h"
#include "openglFunction.h"
#include "openglArguments.h"
#include "config.h"
#include "wglArguments.h"
#include "wglFunctions.h"
#include "platform.h"
#include "mapping.h"
#include "clientArrays.h"
#include "stateTracking.h"

#include "openglLibrary.h"
#include "stateDynamic.h"

namespace gits {
  namespace OpenGL {

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
    ccode_wrap: bool = token_version_data.get('ccodeWrap') or False
    ccode_write_wrap: bool = token_version_data.get('ccodeWriteWrap') or False
    is_draw_func: bool = is_draw_function(token_version_data)

    func_id: str = make_id(name, token_version_data['version'])
    cname: str = make_cname(name, token_version_data['version'])
    func_type_flags: str = make_func_type_flags(token_version_data['functionType'])

    ret_type: str = token_version_data['type']
    has_retval: bool = ret_type != 'void'

    args: list[dict[str,str]] = token_version_data['args']
    retval_and_args: list[dict[str,str]]
    if has_retval:
        retval_and_args = [retval_as_arg(token_version_data)] + args
    else:
        retval_and_args = args  # TODO: `.copy()` ?

    arg_defs: str = args_to_str(retval_and_args, '{ctype} _{name};\n      ', '\n ')
    params: str = args_to_str(retval_and_args, '{type} {name}, ', ', ')

    has_special_args: bool = is_any_arg_special(args)
    special_args: list[dict[str,str]] = expand_special_args(retval_and_args, False)
    special_params: str = args_to_str(special_args, '{type} {name}, ', ', ')

    run_method_name = 'RunImpl' if is_draw_func else 'Run'
###############################################################################
%>\
    class ${cname} : public ${'CDrawFunction' if is_draw_func else 'CFunction'} {
% if arg_defs:
      ${arg_defs}
% endif  # arg_defs

      virtual CArgument &Argument(unsigned idx);
      virtual unsigned ArgumentCount() const { return ${len(args)}; }
% if has_retval:
      virtual const CArgument* Return() const { return &_return_value; }
% endif  # has_retval

    public:
      ${cname}();
% if params:
      ${cname}(${params});
% endif  # params
% if has_special_args:
      ${cname}(${special_params});
% endif  # has_special_args
      virtual unsigned Id() const { return ${func_id}; }
      virtual unsigned Type() const { return ${func_type_flags};}
      virtual const char* Name() const { return "${name}"; }
% if ccode_wrap:
      virtual const char* Suffix() const { return "_wrap"; }
% endif  # ccode_wrap
      virtual void ${run_method_name}();
% if ccode_write_wrap:
      virtual void Write(CCodeOStream& stream) const override;
      friend void ${cname}_CCODEWRITEWRAP(CCodeOStream& stream, const ${cname}& function);
% endif  # ccode_write_wrap
      };

% endfor  # for token_version_data
% endfor  # for name, token_versions_data
  } // namespace OpenGL
} // namespace gits
