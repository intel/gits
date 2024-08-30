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

% for name, token_versions in gl_functions.items():
% for token in token_versions:
<%
    func_id: str = make_id(name, token.version)
    cname: str = make_cname(name, token.version)
    func_type_flags: str = make_func_type_flags(token.function_type)

    is_draw_func: bool = is_draw_function(token.function_type)
    has_retval: bool = token.return_value.type != 'void'

    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [token.return_value] + token.args
    else:
        retval_and_args = token.args

    arg_defs: str = args_to_str(retval_and_args, '{ctype} _{name};\n      ', '\n ')
    params: str = args_to_str(retval_and_args, '{type} {name}, ', ', ')

    has_special_args: bool = is_any_arg_special(token.args)
    special_args = expand_special_args(retval_and_args, only_add_wrap_params=False)
    special_params: str = args_to_str(special_args, '{type} {name}, ', ', ')

    run_method_name = 'RunImpl' if is_draw_func else 'Run'
###############################################################################
%>\
    class ${cname} : public ${'CDrawFunction' if is_draw_func else 'CFunction'} {
% if arg_defs:
      ${arg_defs}
% endif  # arg_defs

      virtual CArgument &Argument(unsigned idx);
      virtual unsigned ArgumentCount() const { return ${len(token.args)}; }
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
% if token.ccode_wrap:
      virtual const char* Suffix() const { return "_wrap"; }
% endif  # token.ccode_wrap
      virtual void ${run_method_name}();
% if token.ccode_write_wrap:
      virtual void Write(CCodeOStream& stream) const override;
      friend void ${cname}_CCODEWRITEWRAP(CCodeOStream& stream, const ${cname}& function);
% endif  # token.ccode_write_wrap
      };

% endfor  # for token
% endfor  # for name, token_versions
  } // namespace OpenGL
} // namespace gits
