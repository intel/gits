// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "vulkanFunctions.h"
#include "vulkanPlayerRunWrap.h"
#include "vulkanStateTracking.h"
\
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
    normal_args: str = args_to_str(token.args, '_{name}, ', ', ')
    normal_retval_and_args: str = args_to_str(retval_and_args, '_{name}, ', ', ')
    deref_args: str = args_to_str(token.args, '*_{name}, ', ', ')
    deref_retval_and_args: str = args_to_str(retval_and_args, '*_{name}, ', ', ')
    original_args: str = args_to_str(token.args, '_{name}.Original(), ', ', ')
    member_initializer_list: str = args_to_str(retval_and_args, '_{name}({wrap_params}), ', ', ')
    argument_infos: str = args_to_str(
        token.args,
        '  {{ gits::Vulkan::ArgType::{category}, {num_ptr}, {needs_ampersand} }}, // {type} ({ctype})\n',
        '\n',
    )
    func_type_flags: str = make_func_type_flags(token.function_type)

    inherit_type: str = make_inherit_type(token.function_type)
    run_function_name: str = 'Run'
    if inherit_type != 'CFunction':
        run_function_name = 'RunImpl'
%>\

/* ***************************** ${id_str} *************************** */

const std::array<gits::Vulkan::ArgInfo, ${len(token.args)}> gits::Vulkan::${cname}::argumentInfos_ = {{
  ${argument_infos.lstrip(' ')}
}};

gits::Vulkan::${cname}::${cname}()
{
}


gits::Vulkan::${cname}::${cname}(${params}):
  ${member_initializer_list}
{
}

gits::CArgument &gits::Vulkan::${cname}::Argument(unsigned idx)
{
% if normal_args:
  return get_cargument(__FUNCTION__, idx, ${normal_args});
% else:
  return get_cargument(__FUNCTION__, idx);
% endif
}

gits::Vulkan::ArgInfo gits::Vulkan::${cname}::ArgumentInfo(unsigned idx) const
{
  return argumentInfos_[idx];
}

std::set<uint64_t> gits::Vulkan::${cname}::GetMappedPointers()
{
  std::set<uint64_t> returnMap;
% for argument in token.args:
  % if undecorated_type(argument.type) not in primitive_types:
  for (auto obj : _${argument.name}.GetMappedPointers())
    returnMap.insert((uint64_t)obj);
  % endif
% endfor
  return returnMap;
}

void gits::Vulkan::${cname}::${run_function_name}()
{
% if token.run_wrap:
  ${token.name}_WRAPRUN(${normal_retval_and_args});
% elif token.token_cache:
  if (Config::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
    RemoveMapping();
  }
% else:
  Exec();
  StateTrack();
  RemoveMapping();
% endif
}

void gits::Vulkan::${cname}::Exec()
{
% if token.return_value.type == 'VkResult':
  _return_value.Assign(drvVk.${token.name}(${deref_args}));
% else:
  drvVk.${token.name}(${deref_args});
% endif
}

void gits::Vulkan::${cname}::StateTrack()
{
% if token.state_track:
  ${token.name}_SD(${deref_retval_and_args});
% endif
}

void gits::Vulkan::${cname}::TokenBuffersUpdate()
{
% if token.token_cache:
  SD()._commandbufferstates[*_commandBuffer]->tokensBuffer.Add(new ${cname}(${original_args}));
% endif
}

void gits::Vulkan::${cname}::RemoveMapping()
{
% for argument in token.args:
  % if argument.remove_mapping:
  _${argument.name}.RemoveMapping();
  % endif
% endfor
}
% if token.ccode_write_wrap:

void gits::Vulkan::${cname}::Write(CCodeOStream& stream) const {
  stream.select(stream.selectCCodeFile());
  ${cname}_CCODEWRITEWRAP(stream, *this);
}
% endif
% if token.token_cache:

VkCommandBuffer gits::Vulkan::${cname}::CommandBuffer() {
  return _commandBuffer.Original();
}
% endif
% endfor
