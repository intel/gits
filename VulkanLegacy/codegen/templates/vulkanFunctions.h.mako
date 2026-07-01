// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include <array>

#include "vkFunction.h"
#include "vulkanArgumentsAuto.h"

namespace gits {
  namespace Vulkan{
    using std::uint64_t;

% for token in vk_functions:
<%
    cname: str = make_cname(token.name, token.version)
    id_str: str = make_id(token.name, token.version)

    has_retval: bool = token.return_value.type != 'void'
    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [token.return_value] + token.args
    else:
        retval_and_args = token.args

    params: str = args_to_str(retval_and_args, '{type} {name}{array}, ', ', ')
    arg_decls: str = args_to_str(retval_and_args, '      {ctype} _{name};\n', '\n')
    func_type_flags: str = make_func_type_flags(token.function_type)

    inherit_type: str = make_inherit_type(token.function_type)
    run_function_name: str = 'Run'
    if inherit_type != 'CFunction':
        run_function_name = 'RunImpl'
%>\
    class ${cname} : public ${inherit_type}, gits::noncopyable {
      ${arg_decls.lstrip(' ')}

      virtual CArgument &Argument(unsigned idx) override;
      virtual unsigned ArgumentCount() const override { return ${len(token.args)}; }
    % if has_retval:
      virtual const CArgument* Return() const { return (stream_older_than(GITS_VULKAN_RETURN_VALUE_FIX) && Configurator::IsPlayer()) ? CFunction::Return() : &_return_value; }
    % endif

    public:
      ${cname}();
% if params:
      ${cname}(${params});
% endif
      virtual unsigned Id() const override { return ${id_str}; }
      virtual unsigned Type() const { return ${func_type_flags};}
      virtual const char *Name() const override { return "${token.name}"; }
      virtual void ${run_function_name}() override;
    % if token.token_cache:
      virtual VkCommandBuffer CommandBuffer();
    % endif
      virtual void Exec();
      virtual void StateTrack();
      virtual void RemoveMapping();
      virtual std::set<uint64_t> GetMappedPointers();
      virtual void TokenBuffersUpdate();
    };

% endfor
  } // namespace Vulkan
} // namespace gits
