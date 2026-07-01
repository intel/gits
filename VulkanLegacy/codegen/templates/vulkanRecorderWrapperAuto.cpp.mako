// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "vulkanRecorderWrapper.h"
#include "vulkanRecorderSubwrappers.h"

namespace gits {
namespace Vulkan {

% for token in vk_functions:
<%
    cname: str = make_cname(token.name, token.version)
    pre_token: str = token.pre_token or ''
    post_token: str = token.post_token or ''

    additional_recorder_conditions = ''
    if token.token_cache:
        additional_recorder_conditions = (
            ' && !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit')

    wrapper_pre_post_macro: str = ''
    if token.function_type in (
        FuncType.QUEUE_SUBMIT,
        FuncType.CREATE_IMAGE,
        FuncType.CREATE_BUFFER,
    ):
        wrapper_pre_post_macro = f'{token.function_type.name}_WRAPPER_PRE_POST'

    has_retval: bool = token.return_value.type != 'void'

    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [token.return_value] + token.args
    else:
        retval_and_args = token.args

    params: str = args_to_str(retval_and_args, '{type} {name}{array}, ', ', ')
    constructor_arguments: str = args_to_str(retval_and_args, '{name}, ')
    rec_wrap_args: str = constructor_arguments + '_recorder'
    constructor_arguments = constructor_arguments.strip(', ')
%>\
void CRecorderWrapper::${token.name}(${params}) const
{
% if token.enabled and not token.recorder_wrap:  # ----------------------------
  % if wrapper_pre_post_macro:
  ${wrapper_pre_post_macro}
  % endif  # wrapper_pre_post_macro
  if (_recorder.Running()${additional_recorder_conditions}) {
  % if pre_token:
    _recorder.Schedule(new ${pre_token});
  % endif  # pre_token
    _recorder.Schedule(new ${cname}(${constructor_arguments}));
  % if post_token:
    _recorder.Schedule(new ${post_token});
  % endif  # post_token
  % if token.token_cache:
  } else {
    ${token.token_cache}.Add(new ${cname}(${constructor_arguments}));
  % endif  # token.token_cache
  }
  % if token.state_track:
  ${token.name}_SD(${constructor_arguments});
  % endif  # token.state_track
% elif token.recorder_wrap:  # ------------------------------------------------
  % if wrapper_pre_post_macro:
  ${wrapper_pre_post_macro}
  % endif  # wrapper_pre_post_macro
  ${token.name}_RECWRAP(${rec_wrap_args});
% else:  # --------------------------------------------------------------------
  CALL_ONCE [] { LOG_ERROR << "function ${token.name} not implemented"; };
% endif  # --------------------------------------------------------------------
}

% endfor  # for token
} // namespace Vulkan
} // namespace gits
