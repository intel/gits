## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2026 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
% for registry, function_list in functions_by_api.items():
# ${registry} functions
% for func in function_list:
<%
    # In OGL generator, two concepts are named "API".
    # This is the prefix indicating a registry (base GL or a native/windowing API).
    # Here we name it "registry" instead of "api" to avoid confusion. Type is `Api`.
    registry_prefix: str = registry.lower()
    # Don't confuse it with the XML 'api' annotation found on enums.

    # FuncType representation, e.g., `FuncType.PARAM|FuncType.QUERY`.
    func_type = '|'.join(f'FuncType.{flag.name}' for flag in func.function_type) or "FuncType.NONE"

    # These are bool
    exec_post_recorder_wrap = " exec_post_recorder_wrap=True," if func.exec_post_recorder_wrap else ""
    interceptor_exec_override = " interceptor_exec_override=True," if func.interceptor_exec_override else ""
    pass_token = " pass_token=True," if func.pass_token else ""
    remove_mapping = " remove_mapping=True," if func.remove_mapping else ""

    # These are int
    version = f" version={func.version}," if func.version else ""

    # These are str | None
    alias = f" alias='{func.alias}'," if func.alias else ""
    # Comments might need escaping. `!r` means repr and it provides its own (single) quotes.
    comment = f" comment={func.comment!r}," if func.comment else ""
    rec_condition = f" rec_condition='{func.rec_condition}'," if func.rec_condition else ""
    run_condition = f" run_condition='{func.run_condition}'," if func.run_condition else ""
    wrap_type =   f" wrap_type='{func.wrap_type}',"     if func.wrap_type   else ""
    wrap_params = f" wrap_params='{func.wrap_params}'," if func.wrap_params else ""
    prefix = f" prefix='{func.prefix}'," if func.prefix else ""
    suffix = f" suffix='{func.suffix}'," if func.suffix else ""
    pre_token =    f" pre_token='{func.pre_token}',"       if func.pre_token    else ""
    pre_schedule = f" pre_schedule='{func.pre_schedule}'," if func.pre_schedule else ""

    # These are bool | str

    state_track: str
    if func.state_track is True:
        state_track = " state_track=True,"
    elif func.state_track:
        state_track = f" state_track='{func.state_track}',"
    else:
        state_track = ""

    recorder_wrap: str
    if func.recorder_wrap is True:
        recorder_wrap = " recorder_wrap=True,"
    elif func.recorder_wrap:
        recorder_wrap = f" recorder_wrap='{func.recorder_wrap}',"
    else:
        recorder_wrap = ""

    run_wrap: str
    if func.run_wrap is True:
        run_wrap = " run_wrap=True,"
    elif func.run_wrap:
        run_wrap = f" run_wrap='{func.run_wrap}',"
    else:
        run_wrap = ""


    # Return value data
    retval: ReturnValue = func.return_value
    ret_group = f", group='{retval.group}'" if retval.group else ""
    ret_class = f", class_='{retval.class_}'" if retval.class_ else ""
    ret_kinds = f", kinds={retval.kinds}" if retval.kinds else ""  # It's a tuple.
    ret_wrap_type = f", wrap_type='{retval.wrap_type}'" if retval.wrap_type else ""
    ret_wrap_params = f", wrap_params='{retval.wrap_params}'" if retval.wrap_params else ""
%>\
${registry_prefix}_function(name='${func.name}', enabled=${func.enabled}, function_type=${func_type},${alias}${version}${comment}${state_track}${rec_condition}${run_condition}${wrap_type}${wrap_params}${recorder_wrap}${exec_post_recorder_wrap}${run_wrap}${pass_token}${interceptor_exec_override}${prefix}${suffix}${pre_token}${pre_schedule}${remove_mapping}
    return_value=ReturnValue(type='${retval.type}'${ret_group}${ret_class}${ret_kinds}${ret_wrap_type}${ret_wrap_params}),
    args=[
    % for arg in func.args:
    <%
        arg_group = f", group='{arg.group}'" if arg.group else ""
        arg_class = f", class_='{arg.class_}'" if arg.class_ else ""
        arg_kinds = f", kinds={arg.kinds}" if arg.kinds else ""  # It's a tuple.
        if arg.len == '':
            raise ValueError(f"Empty len (<param len=\"\">) in command '{func.name}'")
        arg_len = f", len='{arg.len}'" if arg.len else ""
        arg_wrap_type = f", wrap_type='{arg.wrap_type}'" if arg.wrap_type else ""
        arg_wrap_params = f", wrap_params='{arg.wrap_params}'" if arg.wrap_params else ""
        arg_remove_mapping = ", remove_mapping=True" if arg.remove_mapping else ""
    %>\
        Argument(name='${arg.name}', type='${arg.type}'${arg_group}${arg_class}${arg_kinds}${arg_len}${arg_wrap_type}${arg_wrap_params}${arg_remove_mapping}),
    % endfor  # for arg
    ],
)
% endfor  # for func

% endfor  # for registry, function_list
