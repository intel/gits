#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator import GetFunctions, FuncType

from typing import Any
import operator
import copy
import re
import textwrap

import mako.template
import mako.exceptions


special_types = ['CIndexPtr', 'CAttribPtr', 'CGLTexResource', 'CGLCompressedTexResource']

# TODO: turn these strings into lists (how to handle semicolons?)
egl_function_names = '''
	eglGetError
	eglGetDisplay
	eglInitialize
	eglTerminate
	eglQueryString
	eglGetConfigs
	eglChooseConfig
	eglGetConfigAttrib
	eglCreateWindowSurface
	eglCreatePbufferSurface
	eglCreatePixmapSurface
	eglDestroySurface
	eglQuerySurface
	eglBindAPI
	eglQueryAPI
	eglWaitClient
	eglReleaseThread
	eglCreatePbufferFromClientBuffer
	eglSurfaceAttrib
	eglBindTexImage
	eglReleaseTexImage
	eglSwapInterval
	eglCreateContext
	eglDestroyContext
	eglMakeCurrent
	eglGetCurrentContext
	eglGetCurrentSurface
	eglGetCurrentDisplay
	eglQueryContext
	eglWaitGL
	eglWaitNative
	eglSwapBuffers
	eglCopyBuffers
	eglGetProcAddress
	eglClientWaitSyncKHR
	eglClientWaitSyncNV
	eglCreateDRMImageMESA
	eglCreateFenceSyncNV
	eglCreateImageKHR
	eglCreatePixmapSurfaceHI
	eglCreateStreamFromFileDescriptorKHR
	eglCreateStreamKHR
	eglCreateStreamProducerSurfaceKHR
	eglCreateSyncKHR
	eglDestroyImageKHR
	eglDestroyStreamKHR
	eglDestroySyncKHR
	eglDestroySyncNV
	eglExportDRMImageMESA
	eglFenceNV
	eglGetStreamFileDescriptorKHR
	eglGetSyncAttribKHR
	eglGetSyncAttribNV
;	eglGetSystemTimeFrequencyNV
;	eglGetSystemTimeNV
	eglLockSurfaceKHR
	eglPostSubBufferNV
	eglQueryStreamKHR
	eglQueryStreamTimeKHR
	eglQueryStreamu64KHR
	eglQuerySurfacePointerANGLE
	eglSignalSyncKHR
	eglSignalSyncNV
	eglStreamAttribKHR
	eglStreamConsumerAcquireKHR
	eglStreamConsumerGLTextureExternalKHR
	eglStreamConsumerReleaseKHR
	eglUnlockSurfaceKHR
	eglWaitSyncKHR
	eglSetSwapRectangleANDROID
	eglGetRenderBufferANDROID
	eglDupNativeFenceFDANDROID
	eglWaitSyncANDROID
'''.strip('\r\n')

wgl_function_names = '''
	wglCopyContext
	wglCreateContext
	wglCreateLayerContext
	wglDeleteContext
	wglGetCurrentContext
	wglGetCurrentDC
	wglGetDefaultProcAddress
	wglGetPixelFormat
	wglGetProcAddress
	wglMakeCurrent
	wglRealizeLayerPalette
	wglShareLists
	wglSwapBuffers
	wglSwapMultipleBuffers
	wglSwapLayerBuffers
	wglUseFontBitmapsA
	wglUseFontBitmapsW
	wglChoosePixelFormat
	wglDescribeLayerPlane
	wglDescribePixelFormat
	wglGetLayerPaletteEntries
	wglSetLayerPaletteEntries
	wglSetPixelFormat
	wglUseFontOutlinesA
	wglUseFontOutlinesW
	wglCreateBufferRegionARB
	wglDeleteBufferRegionARB
	wglSaveBufferRegionARB
	wglRestoreBufferRegionARB
	wglGetExtensionsStringARB
	wglGetPixelFormatAttribivARB
	wglGetPixelFormatAttribfvARB
	wglChoosePixelFormatARB
	wglMakeContextCurrentARB
	wglGetCurrentReadDCARB
	wglCreatePbufferARB
	wglGetPbufferDCARB
	wglReleasePbufferDCARB
	wglDestroyPbufferARB
	wglQueryPbufferARB
	wglBindTexImageARB
	wglReleaseTexImageARB
	wglSetPbufferAttribARB
	wglCreateContextAttribsARB
	wglCreateDisplayColorTableEXT
	wglLoadDisplayColorTableEXT
	wglBindDisplayColorTableEXT
	wglDestroyDisplayColorTableEXT
	wglGetExtensionsStringEXT
	wglMakeContextCurrentEXT
	wglGetCurrentReadDCEXT
	wglCreatePbufferEXT
	wglGetPbufferDCEXT
	wglReleasePbufferDCEXT
	wglDestroyPbufferEXT
	wglQueryPbufferEXT
	wglGetPixelFormatAttribivEXT
	wglGetPixelFormatAttribfvEXT
	wglChoosePixelFormatEXT
	wglSwapIntervalEXT
	wglGetSwapIntervalEXT
	wglAllocateMemoryNV
	wglFreeMemoryNV
	wglGetSyncValuesOML
	wglGetMscRateOML
	wglSwapBuffersMscOML
	wglSwapLayerBuffersMscOML
	wglWaitForMscOML
	wglWaitForSbcOML
	wglGetDigitalVideoParametersI3D
	wglSetDigitalVideoParametersI3D
	wglGetGammaTableParametersI3D
	wglSetGammaTableParametersI3D
	wglGetGammaTableI3D
	wglSetGammaTableI3D
	wglEnableGenlockI3D
	wglDisableGenlockI3D
	wglIsEnabledGenlockI3D
	wglGenlockSourceI3D
	wglGetGenlockSourceI3D
	wglGenlockSourceEdgeI3D
	wglGetGenlockSourceEdgeI3D
	wglGenlockSampleRateI3D
	wglGetGenlockSampleRateI3D
	wglGenlockSourceDelayI3D
	wglGetGenlockSourceDelayI3D
	wglQueryGenlockMaxSourceDelayI3D
	wglCreateImageBufferI3D
	wglDestroyImageBufferI3D
	wglAssociateImageBufferEventsI3D
	wglReleaseImageBufferEventsI3D
	wglEnableFrameLockI3D
	wglDisableFrameLockI3D
	wglIsEnabledFrameLockI3D
	wglQueryFrameLockMasterI3D
	wglGetFrameUsageI3D
	wglBeginFrameTrackingI3D
	wglEndFrameTrackingI3D
	wglQueryFrameTrackingI3D
	wglSetStereoEmitterState3DL
	wglEnumerateVideoDevicesNV
	wglBindVideoDeviceNV
	wglQueryCurrentContextNV
	wglGetVideoDeviceNV
	wglReleaseVideoDeviceNV
	wglBindVideoImageNV
	wglReleaseVideoImageNV
	wglSendPbufferToVideoNV
	wglGetVideoInfoNV
	wglJoinSwapGroupNV
	wglBindSwapBarrierNV
	wglQuerySwapGroupNV
	wglQueryMaxSwapGroupsNV
	wglQueryFrameCountNV
	wglResetFrameCountNV
	wglEnumGpusNV
	wglEnumGpuDevicesNV
	wglCreateAffinityDCNV
	wglEnumGpusFromAffinityDCNV
	wglDeleteDCNV
;	wglGetGPUIDsAMD
	wglGetGPUInfoAMD
	wglGetContextGPUIDAMD
	wglCreateAssociatedContextAMD
	wglCreateAssociatedContextAttribsAMD
	wglDeleteAssociatedContextAMD
	wglMakeAssociatedContextCurrentAMD
	wglGetCurrentAssociatedContextAMD
	wglBlitContextFramebufferAMD
	wglBindVideoCaptureDeviceNV
	wglEnumerateVideoCaptureDevicesNV
	wglLockVideoCaptureDeviceNV
	wglQueryVideoCaptureDeviceNV
	wglReleaseVideoCaptureDeviceNV
	wglCopyImageSubDataNV
'''.strip('\r\n')


def version_suffix_from_int(version: int) -> str:
    if version <= 0:
        return ''
    else:
        return f'_V{str(version)}'

def version_suffix_from_token(token_version_data: dict[str, Any]) -> str:
    version: int = token_version_data.get('version') or 0
    return version_suffix_from_int(version)

def make_cname(name: str, version: int) -> str:
    version_suffix: str = version_suffix_from_int(version)
    return f'C{name}{version_suffix}'

def make_id(name: str, version: int) -> str:
    id_ = re.sub('([a-z])([A-Z])', r'\g<1>_\g<2>', name)
    id_ = re.sub('([0-9])D', r'_\g<1>D_', id_)
    id_ = id_.rstrip('_').upper()

    version_suffix: str = version_suffix_from_int(version)

    return f'ID_{id_}{version_suffix}'

def make_func_type_flags(func_type: FuncType) -> str:
    """Convert FuncType into GITS' C++ representation string."""
    return ' | '.join(f'GITS_GL_{flag.name}_APITYPE' for flag in func_type)

def make_ctype(type_name: str, wrap_type: str = '', idx: int = 0) -> str:
    if wrap_type:  # Wrap types override regular types.
        if wrap_type == 'COutArgument':
            return f'COutArgument<{str(idx)}>'
        else:
            return wrap_type
    else:
        type_name = type_name.replace('const ', '')
        if '*' in type_name:
            type_name = type_name.strip(' *')
            if type_name in ('void', 'GLvoid'):
                return 'CGLvoid_ptr'
            else:
                return f'C{type_name}::CSArray'

    return f'C{type_name}'

def get_indent(s: str) -> str:
    # In case of multiline strings, first line should be the least indented.
    match = re.match(r'\s*', s)
    if match is None:
        return ''
    else:
        return match.group()

def wrap_in_scope(code: str, indent: str = '  ') -> str:
    """Wrap a multiline string in a C++ scope."""

    orig_indent = get_indent(code)
    indented_code = textwrap.indent(code, indent)

    return f"{orig_indent}{{\n{indented_code}\n{orig_indent}}}"

def wrap_in_if(condition: str, code: str, indent: str = '  ') -> str:
    """Wrap a multiline string in a C++ if statement."""

    orig_indent = get_indent(code)

    return f'{orig_indent}if ({condition})\n{wrap_in_scope(code, indent)}'

def make_member_initializer_list(args: list[dict[str,str]]) -> str:
    # Values of out arguments are not known before the API call.
    initializer_args = [a for a in args if a.get('wrapType') != 'COutArgument']
    if initializer_args:
        inits = args_to_str(initializer_args, '_{name}({wrap_params}), ', ', ')
        return f':\n  {inits}'
    else:
        return ''

def is_draw_func_type(func_type: int) -> bool:
    """
    Check whether an OpenGL function is a CDrawFunction or a regular CFunction.

    Parameters:
        func_type: Bitflag of types the function is categorized as.
    Returns:
        A bool. True for a draw function, False otherwise.
    """
    return any([
        func_type & FuncType.Render,
        func_type & FuncType.Fill,
        func_type & FuncType.Copy
    ])

def is_draw_function(token_version_data: dict[str, Any]) -> bool:
    """
    Check whether an OpenGL function is a CDrawFunction or a regular CFunction.

    Parameters:
        token_version_data: Data for one version of a token.
    Returns:
        A bool. True for a draw function, False otherwise.
    """

    # `func_type` is a bit flag.
    func_type: int = token_version_data.get('functionType') or 0

    return is_draw_func_type(func_type)

def split_draw_functions(
    gl_functions: dict[str,list[dict[str,Any]]]
) -> tuple[dict[str,dict[str,Any]],dict[str,dict[str,Any]]]:
    """
    Separate OpenGL functions into ones that draw and ones that don't.

    Parameters:
        gl_functions: Dict of function versions: {'glFoo': [{glFoo data}, {glFoo_V1 data}], ...}

    Returns:
        A tuple: (draw functions, other functions). Only latest token versions are kept.
    """

    draw_funcs: dict[str,dict[str,Any]] = {}
    other_funcs: dict[str,dict[str,Any]] = {}

    for name, token_versions_data in gl_functions.items():
        # The result should not change depending on which version we use,
        # but we take the latest one just in case.
        token_version_data: dict[str, Any] = token_versions_data[-1]

        if is_draw_function(token_version_data):
            draw_funcs[name] = token_version_data
        else:
            other_funcs[name] = token_version_data

    return (draw_funcs, other_funcs)

def retval_as_arg(token_version_data: dict[str, Any]) -> dict[str,str]:
    """Returns function's return value metadata in the form of an argument."""

    retval = {}
    retval['name'] = 'return_value'
    retval['type'] = token_version_data['type']

    wrap_type = token_version_data.get('retVwrapType')
    if wrap_type:
        retval['wrapType'] = wrap_type

    wrap_type = token_version_data.get('retVwrapParams')
    if wrap_type:
        retval['wrapParams'] = wrap_type

    return retval

def is_any_arg_special(args: list[dict[str,str]]) -> bool:
    return any(arg.get('wrapType') in special_types for arg in args)

def expand_special_args(
    args: list[dict[str,str]],
    wrap: bool = False  # TODO: make it an enum?
) -> list[dict[str,str]]:
    expanded_args: list[dict[str,str]] = []
    to_append: list[dict[str,str]] = []

    for arg in args:
        wrap_type: str = arg.get('wrapType') or ''
        if wrap_type in ('CGLTexResource', 'CGLCompressedTexResource'):
            if wrap:
                new_arg = copy.deepcopy(arg)  # TODO: Do we need to copy here and below, or can we just assign?
                new_arg['wrapParams'] = 'hash'
                expanded_args.append(new_arg)
            else:
                expanded_args.append({'name': 'hash', 'type': 'hash_t'})
        elif wrap_type in ('CAttribPtr', 'CIndexPtr'):
            if wrap:
                name: str = arg['name']
                new_arg = copy.deepcopy(arg)
                new_arg['wrapParams'] = f'boundBuffer, {name}'
                expanded_args.append(new_arg)
            else:
                expanded_args.append(arg)
                to_append.append({'name': 'boundBuffer', 'type': 'GLint'})
        else:
            expanded_args.append(arg)

    expanded_args.extend(to_append)

    return expanded_args

def args_to_str(
    args: list[dict[str,str]],
    format_string: str,
    rstrip_string: str = ''
) -> str:
    """
    Format OpenGL function call arguments as string.

    Each argument will get square brackets and their contents removed, then
    `format_string.format(arg_name)` will be run on it. They will all get
    concatenated and the result returned.

    Parameters:
        args: parameters of the OpenGL function
        format_string: format of one arg with {} in place of arg name.
        rstrip_string: string to rstrip the result with; defaults to ''

    Returns:
        A C++ arguments string. Examples:

        'foo, bar, baz'
        '*_foo, *_bar, *_baz'
    """

    args_str = ''

    idx: int = 0
    arg: dict[str,str]
    for arg in args:
        name_with_array: str = arg['name']
        # Some arguments are arrays; remove square brackets from their names.
        # For example, `baseAndCount[2]` -> `baseAndCount`.
        name: str = re.sub(r'\[.+?\]', '', name_with_array).strip()
        type: str = arg['type']
        wrap_type: str = arg.get('wrapType') or ''
        wrap_params: str = arg.get('wrapParams') or name
        ctype: str = make_ctype(type, wrap_type, idx)

        # Doing this outside of make_ctype is messy, but can we do it cleanly?
        if wrap_type == 'COutArgument':
            idx += 1

        args_str += format_string.format(
            name_with_array=name_with_array,
            name=name,
            # name_array=name_array,
            type=type,
            wrap_type=wrap_type,
            wrap_params=wrap_params,
            ctype=ctype,
        )

    return args_str.rstrip(rstrip_string)

def arg_call(
    token_version_data: dict[str, Any],
    add_retval: bool,
    recording: bool = False,
    wrap: bool = False
) -> str:
    args: list[dict[str,str]] = token_version_data['args']
    args_str = ''

    if add_retval and token_version_data['type'] != 'void':
        args_str += 'return_value, '

    args_str += args_to_str(args, '{name}, ')

    # arg: dict[str,str]
    # for arg in args:
    #     name: str = arg['name']
    #     # Some arguments are arrays; remove square brackets from their names.
    #     # For example, `baseAndCount[2]` -> `baseAndCount`.
    #     name = re.sub(r'\[.+?\]', '', name)
    #     args_str += f'{name}, '

    if recording:
        args_str += 'Recording(_recorder), '

    if wrap:
        args_str += '_recorder, '

    return f"({args_str.strip(', ')})"

# TODO: Replace it with expand_special_args/retval_and_args & args_to_str
def arg_decl(
    token_version_data: dict[str, Any],
    add_retval: bool,
    add_names: bool = False,
    mode: str = 'default',  # TODO: Use an enum instead.
) -> str:
    params: list[dict[str,str]] = token_version_data['args']
    params_str = ''

    return_type: str = token_version_data['type']
    if add_retval and return_type != 'void':
        params_str += f'{return_type} return_value, '

    for param in params:
        if (mode in ('CGLTexResource', 'CGLCompressedTexResource') and
            param.get('wrapType') in ('CGLTexResource', 'CGLCompressedTexResource')):
            params_str += 'hash_t hash, '
        elif add_names:
            params_str += f"{param['type']} {param['name']}, "
        else:
            params_str += f"{param['type']}, "

    # if mode in ('CGLTexResource', 'CGLCompressedTexResource'):
    #   for param in params:
    #     if param.get('wrapType') in ('CGLTexResource', 'CGLCompressedTexResource'):
    #         param = 'hash_t hash, '
    #
    # if add_names:
    #   params_str += args_to_str(params, '{type} {name}, ')
    # else:
    #   params_str += args_to_str(params, '{type}, ')

    if mode in ('CAttribPtr', 'CIndexPtr'):
        params_str += 'GLint boundBuffer, '

    return f"({params_str.strip(', ')})"

def driver_call(name: str, token_version_data: dict[str, Any], has_retval: bool) -> str:
    """
    Creates a string containing a driver call (for gitsPluginPrePostAuto.cpp).

    Parameters:
        name: name of the OpenGL function, e.g. 'glEnd'
        token_version_data: dict of data for one version of a token
        has_retval: whether the OpenGL function returns something

    Returns:
        A C++ driver call string. Examples:

        'wrapper.Drivers().gl.glAccum(op, value);'
        'execWrap_glGetIntegerv(pname, data);'
        'auto return_value = wrapper.Drivers().gl.glMapBuffer(target, access);'
    """

    retval_assignment = 'auto return_value = ' if has_retval else ''

    exec_wrap: bool = token_version_data.get('interceptorExecOverride') is True
    function_prefix = 'execWrap_' if exec_wrap else 'wrapper.Drivers().gl.'

    driver_args: str = arg_call(token_version_data, add_retval=False)

    return f'{retval_assignment}{function_prefix}{name}{driver_args};'

def mako_write(inpath: str, outpath: str, **args) -> int:
    try:
        print(f"Generating {outpath}...")
        template = mako.template.Template(filename=inpath)
        rendered = template.render(**args)
        rendered = re.sub(r'\r\n', r'\n', rendered)

        with open(outpath, 'w') as fout:
            fout.write(rendered)
    except Exception:
        traceback = mako.exceptions.RichTraceback()
        for filename, lineno, function, line in traceback.traceback:
            print("%s(%s) : error in %s" % (filename, lineno, function))
            print(line, "\n")
        print("%s: %s" % (str(traceback.error.__class__.__name__), traceback.error))
        return -1
    return 0


def main() -> None:
    table = GetFunctions()
    try:
        black = open('blacklist.txt').readlines()
        black = list(map(lambda a: a.strip('\n'), black))
        print("Using blacklist file")
    except Exception:
        pass
    functions_all: dict[str, list[dict[str, Any]]] = {}
    functions_enabled: dict[str, list[dict[str, Any]]] = {}
    for f in table:
        function: dict[str, Any] = {}
        function['args'] = []
        function['type'] = ''
        function['functionType'] = None
        function['enabled'] = f.get('enabled')
        function['custom'] = False
        function['version'] = 0
        if f.get('inheritFrom'):
            inherit_name = f.get('inheritFrom')
            function['inheritName'] = inherit_name
            for g in table:
                if g.get('name') == inherit_name:
                    if g.get('retV'):
                        function['type'] = g.get('retV').get('type')
                        if g.get('retV').get('wrapType'):
                            function['retVwrapType'] = g.get('retV').get('wrapType')
                        if g.get('retV').get('wrapParams'):
                            function['retVwrapParams'] = g.get('retV').get('wrapParams')
                    if g.get('type'):
                        function['functionType'] = g.get('type')
                    if g.get('custom'):
                        function['custom'] = g.get('custom')
                    if g.get('version'):
                        function['version'] = g.get('version')
                    i = 1
                    while g.get('arg'+str(i)):
                        arg = {}
                        arg['type'] = g.get('arg'+str(i)).get('type')
                        arg['name'] = g.get('arg'+str(i)).get('name')
                        if g.get('arg'+str(i)).get('wrapType'):
                            arg['wrapType'] = g.get('arg'+str(i)).get('wrapType')
                        if g.get('arg'+str(i)).get('wrapParams'):
                            arg['wrapParams'] = g.get('arg'+str(i)).get('wrapParams')
                        if g.get('arg'+str(i)).get('removeMapping'):
                            arg['removeMapping'] = g.get('arg'+str(i)).get('removeMapping')
                        function['args'].append(arg)
                        i += 1
                    if g.get('preToken') is not None:
                        function['preToken'] = g.get('preToken')
                        function['preTokenName'] = inherit_name
                    if g.get('postToken') is not None:
                        function['postToken'] = g.get('postToken')
                        function['postTokenName'] = inherit_name
                    if g.get('stateTrack') is not None:
                        function['stateTrack'] = g.get('stateTrack')
                        function['stateTrackName'] = inherit_name
                    if g.get('recCond') is not None:
                        function['recCond'] = g.get('recCond')
                    if g.get('runCond') is not None:
                        function['runCond'] = g.get('runCond')
                    if g.get('preSchedule') is not None:
                        function['preSchedule'] = g.get('preSchedule')
                    if g.get('recWrap') is not None:
                        function['recWrap'] = g.get('recWrap')
                        function['recWrapName'] = inherit_name
                    if g.get('runWrap') is not None:
                        function['runWrap'] = g.get('runWrap')
                        function['runWrapName'] = inherit_name
                    if g.get('ccodeWrap') is not None:
                        function['ccodeWrap'] = g.get('ccodeWrap')
                        function['ccodeWrapName'] = inherit_name
                    if g.get('ccodeWriteWrap') is not None:
                        function['ccodeWriteWrap'] = g.get('ccodeWriteWrap')
                        function['ccodeWriteWrapName'] = inherit_name
                    if g.get('execPostRecWrap') is not None:
                        function['execPostRecWrap'] = g.get('execPostRecWrap')
                        function['execPostRecWrapName'] = inherit_name
                    if g.get('prefix') is not None:
                        function['prefix'] = g.get('prefix')
                        function['prefixName'] = inherit_name
                    if g.get('interceptorExecOverride') is not None:
                        function['interceptorExecOverride'] = g.get('interceptorExecOverride')
                        function['interceptorExecOverrideName'] = inherit_name
                    if g.get('suffix') is not None:
                        function['suffix'] = g.get('suffix')
                        function['suffixName'] = inherit_name
        if f.get('retV'):
            function['type'] = f.get('retV').get('type')
            if f.get('retV').get('wrapType'):
                function['retVwrapType'] = f.get('retV').get('wrapType')
            if f.get('retV').get('wrapParams'):
                function['retVwrapParams'] = f.get('retV').get('wrapParams')
        if f.get('type'):
            function['functionType'] = f.get('type')
        if f.get('custom'):
            function['custom'] = f.get('custom')
        if f.get('version'):
            function['version'] = f.get('version')
        i = 1

        while f.get('arg'+str(i)) or (i <= len(function['args'])):
            if f.get('arg'+str(i)):
                arg = {}
                arg['type'] = f.get('arg'+str(i)).get('type')
                arg['name'] = f.get('arg'+str(i)).get('name')
                if f.get('arg'+str(i)).get('wrapType'):
                    arg['wrapType'] = f.get('arg'+str(i)).get('wrapType')
                if f.get('arg'+str(i)).get('wrapParams'):
                    arg['wrapParams'] = f.get('arg'+str(i)).get('wrapParams')
                if f.get('arg'+str(i)).get('removeMapping'):
                    arg['removeMapping'] = f.get('arg'+str(i)).get('removeMapping')
                if i <= len(function['args']):
                    function['args'][i-1] = arg
                else:
                    function['args'].append(arg)
            i += 1
        if f.get('preToken') is not None:
            function['preToken'] = f.get('preToken')
            function['preTokenName'] = f.get('name')
        if f.get('postToken') is not None:
            function['postToken'] = f.get('postToken')
            function['postTokenName'] = f.get('name')
        if f.get('stateTrack') is not None:
            function['stateTrack'] = f.get('stateTrack')
            function['stateTrackName'] = f.get('name')
        if f.get('recCond') is not None:
            function['recCond'] = f.get('recCond')
        if f.get('runCond') is not None:
            function['runCond'] = f.get('runCond')
        if f.get('preSchedule') is not None:
            function['preSchedule'] = f.get('preSchedule')
        if f.get('recWrap') is not None:
            function['recWrap'] = f.get('recWrap')
            function['recWrapName'] = f.get('name')
        if f.get('runWrap') is not None:
            function['runWrap'] = f.get('runWrap')
            function['runWrapName'] = f.get('name')
            if f.get('passToken') is not None:
                function['passToken'] = f.get('passToken')
        if f.get('ccodeWrap') is not None:
            function['ccodeWrap'] = f.get('ccodeWrap')
            function['ccodeWrapName'] = f.get('name')
        if f.get('ccodeWriteWrap') is not None:
            function['ccodeWriteWrap'] = f.get('ccodeWriteWrap')
            function['ccodeWriteWrapName'] = f.get('name')
        if f.get('execPostRecWrap') is not None:
            function['execPostRecWrap'] = f.get('execPostRecWrap')
            function['execPostRecWrapName'] = f.get('name')
        if f.get('prefix') is not None:
            function['prefix'] = f.get('prefix')
            function['prefixName'] = f.get('name')
        if f.get('interceptorExecOverride') is not None:
            function['interceptorExecOverride'] = f.get('interceptorExecOverride')
            function['interceptorExecOverrideName'] = f.get('name')
        if f.get('suffix') is not None:
            function['suffix'] = f.get('suffix')
            function['suffixName'] = f.get('name')
        if (f.get('name') not in black):
            if functions_all.get(f.get('name')) is None:
                functions_all[f.get('name')] = []
            if (functions_enabled.get(f.get('name')) is None) and (function['enabled'] is True):
                functions_enabled[f.get('name')] = []
            functions_all[f.get('name')].append(function)
            functions_all[f.get('name')].sort(key=operator.itemgetter('version'))
            if function['enabled'] is True:
                functions_enabled[f.get('name')].append(function)
                functions_enabled[f.get('name')].sort(key=operator.itemgetter('version'))

    # TODO: remove it:
    # input("Press Enter to continue...")

    mako_write('templates/glIDswitch.h.mako',
               'temp/glIDswitch.h',
               make_cname=make_cname,
               make_id=make_id,
               gl_functions=functions_enabled)

    mako_write('templates/GLIPluginXX.def.mako',
               'temp/GLIPlugin32.def',
               egl_function_names=egl_function_names,
               wgl_function_names=wgl_function_names,
               gl_functions=functions_all)

    # EGL should only be exported on 32-bit.
    mako_write('templates/GLIPluginXX.def.mako',
               'temp/GLIPlugin64.def',
               egl_function_names='',
               wgl_function_names=wgl_function_names,
               gl_functions=functions_all)

    mako_write('templates/gitsPluginPrePostAuto.h.mako',
               'temp/gitsPluginPrePostAuto.h',
               arg_decl=arg_decl,
               gl_functions=functions_all)

    mako_write('templates/gitsPluginPrePostAuto.cpp.mako',
               'temp/gitsPluginPrePostAuto.cpp',
               arg_decl=arg_decl,
               arg_call=arg_call,
               driver_call=driver_call,
               gl_functions=functions_all)

    (draw_functions, nondraw_functions) = split_draw_functions(functions_all)
    mako_write('templates/glDrivers.h.mako',
               'temp/glDrivers.h',
               arg_decl=arg_decl,
               arg_call=arg_call,
               driver_call=driver_call,
               draw_functions=draw_functions,
               nondraw_functions=nondraw_functions)

    mako_write('templates/glFunctions.cpp.mako',
               'temp/glFunctions.cpp',
               version_suffix_from_token=version_suffix_from_token,
               make_cname=make_cname,
               make_id=make_id,
               wrap_in_if=wrap_in_if,
               make_member_initializer_list=make_member_initializer_list,
               is_draw_function=is_draw_function,
               retval_as_arg=retval_as_arg,
               is_any_arg_special=is_any_arg_special,
               expand_special_args=expand_special_args,
               args_to_str=args_to_str,
               # arg_call=arg_call,
               # arg_decl=arg_decl,
               # driver_call=driver_call,
               gl_functions=functions_enabled)

    mako_write('templates/glFunctions.h.mako',
               'temp/glFunctions.h',
               make_cname=make_cname,
               make_id=make_id,
               make_func_type_flags=make_func_type_flags,
               is_draw_function=is_draw_function,
               retval_as_arg=retval_as_arg,
               is_any_arg_special=is_any_arg_special,
               expand_special_args=expand_special_args,
               args_to_str=args_to_str,
               gl_functions=functions_enabled)

    mako_write('templates/openglRecorderWrapperXAuto.h.mako',
               'temp/openglRecorderWrapperAuto.h',
               retval_as_arg=retval_as_arg,
               args_to_str=args_to_str,
               is_iface=False,
               gl_functions=functions_all)

    mako_write('templates/openglRecorderWrapperXAuto.h.mako',
               'temp/openglRecorderWrapperIfaceAuto.h',
               retval_as_arg=retval_as_arg,
               args_to_str=args_to_str,
               is_iface=True,
               gl_functions=functions_all)

    mako_write('templates/openglRecorderWrapperAuto.cpp.mako',
               'temp/openglRecorderWrapperAuto.cpp',
               make_cname=make_cname,
               is_draw_function=is_draw_function,
               retval_as_arg=retval_as_arg,
               args_to_str=args_to_str,
               gl_functions=functions_all)

if __name__ == '__main__':
    main()
