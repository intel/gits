#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator import get_tokens, FuncType, Token, Argument, ReturnValue

import copy
import os.path
import re
import shutil
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


def version_suffix(version: int) -> str:
    """Return a version suffix (like '_V1'), empty for version 0."""
    if version <= 0:
        return ''
    else:
        return f'_V{version}'

def make_cname(name: str, version: int) -> str:
    """Return a Cname (like 'CglBegin_V1')."""
    return f'C{name}{version_suffix(version)}'

def make_id(name: str, version: int) -> str:
    """Return an ID (like 'ID_GL_BEGIN_V1')."""
    id_ = re.sub('([a-z])([A-Z])', r'\g<1>_\g<2>', name)
    id_ = re.sub('([0-9])D', r'_\g<1>D_', id_)
    id_ = id_.rstrip('_').upper()

    return f'ID_{id_}{version_suffix(version)}'

def make_func_type_flags(func_type: FuncType) -> str:
    """Convert FuncType into GITS' C++ representation string."""
    return ' | '.join(f'GITS_GL_{flag.name}_APITYPE' for flag in func_type)

def make_ctype(type_name: str, wrap_type: str = '', idx: int = 0) -> str:
    """Return a Ctype (like 'CGLdouble::CSArray')."""
    if wrap_type:  # Wrap types override regular types.
        if wrap_type == 'COutArgument':
            return f'COutArgument<{idx}>'
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
    """Return the base indentation of given code as a string."""
    # In case of multiline strings, first line should be the least indented.
    match = re.match(r'\s*', s)
    if match is None:
        return ''
    else:
        return match.group()

def wrap_in_if(condition: str, code: str, indent: str = '  ') -> str:
    """Wrap a multiline string in a C++ if statement."""

    orig_indent = get_indent(code)

    # So we don't double orig_indent when indenting everything at the end.
    # Dedent doesn't accept amount, but it should dedent exactly by orig_indent.
    dedented_code = textwrap.dedent(code)

    # Indent by one level.
    indented_code = textwrap.indent(dedented_code, indent)

    if_statement = f'if ({condition}) {{\n{indented_code}\n}}'

    # Indent everything by original indent amount.
    return textwrap.indent(if_statement, orig_indent)

def make_member_initializer_list(args: list[Argument]) -> str:
    """Build a C++ member initializer list string from arguments."""

    # Values of out arguments are not known before the API call.
    initializer_args = [a for a in args if a.wrap_type != 'COutArgument']
    if initializer_args:
        inits = args_to_str(initializer_args, '_{name}({wrap_params}), ', ', ')
        return f':\n  {inits}'
    else:
        return ''

def is_draw_function(func_type: FuncType) -> bool:
    """
    Check whether an OpenGL function is a CDrawFunction or a regular CFunction.

    Parameters:
        func_type: Bitflag of types the function is categorized as.

    Returns:
        A bool. True for a draw function, False otherwise.
    """

    return any([
        FuncType.RENDER in func_type,
        FuncType.FILL in func_type,
        FuncType.COPY in func_type,
    ])

def split_draw_functions(
    gl_functions: dict[str,list[Token]],
) -> tuple[list[Token],list[Token]]:
    """
    Separate OpenGL functions into ones that draw and ones that don't.

    Parameters:
        gl_functions: Dict of function versions: {'glFoo': [{glFoo data}, {glFoo_V1 data}], ...}

    Returns:
        A tuple: (draw functions, non-draw functions). Only latest token versions are kept.
    """

    draw_funcs: list[Token] = []
    nondraw_funcs: list[Token] = []

    for token_versions in gl_functions.values():
        # The result should not change depending on which version we use,
        # but we take the latest one just in case.
        token: Token = token_versions[-1]

        if is_draw_function(token.function_type):
            draw_funcs.append(token)
        else:
            nondraw_funcs.append(token)

    return (draw_funcs, nondraw_funcs)

def is_any_arg_special(args: list[Argument]) -> bool:
    """Return true if any of given arguments is special, false otherwise."""
    return any(arg.wrap_type in special_types for arg in args)

def expand_special_args(
    args: list[Argument],
    *,  # Force keyword arg, as calls with positional bools are cryptic.
    only_add_wrap_params: bool = False,
) -> list[Argument]:
    """
    Expand special Arguments to fit data to CFunction constructors.

    For some CArguments, C++ expects different or additional data to be passed
    to CFunction constructors. This function offers two ways of changing/adding
    that data: either replacing existing/adding new Arguments, or (when
    only_add_wrap_params is True) adding wrap_params to existing Arguments.

    Parameters:
        args: List of Arguments.
        only_add_wrap_params: Just add wrap_params to existing Arguments.

    Returns:
        A new list containing both actual and mock Arguments.
    """

    expanded_args: list[Argument] = []
    to_append: list[Argument] = []

    for arg in args:
        if arg.wrap_type in ('CGLTexResource', 'CGLCompressedTexResource'):
            if only_add_wrap_params:
                new_arg = copy.deepcopy(arg)  # TODO: Do we need to copy here and below, or can we just assign?
                new_arg.wrap_params = 'hash'
                expanded_args.append(new_arg)
            else:
                expanded_args.append(Argument(name='hash', type='hash_t'))
        elif arg.wrap_type in ('CAttribPtr', 'CIndexPtr'):
            if only_add_wrap_params:
                new_arg = copy.deepcopy(arg)
                new_arg.wrap_params = f'boundBuffer, {arg.name}'
                expanded_args.append(new_arg)
            else:
                expanded_args.append(arg)
                to_append.append(Argument(name='boundBuffer', type='GLint'))
        else:
            expanded_args.append(arg)

    expanded_args.extend(to_append)

    return expanded_args

def args_to_str(
    args: list[Argument],
    format_string: str,
    rstrip_string: str = '',
) -> str:
    """
    Format OpenGL function call arguments as string.

    Each argument will get square brackets and their contents removed, then
    `format_string.format(...)` will be run on it. The results will all get
    concatenated and returned.

    Placeholder strings supported for formatting are:
        name_with_array: Unedited name straight from generator, e.g. `baseAndCount [2]`.
        name: Name with the array part removed.
        type: Type straight from generator.
        wrap_type: wrap_type from generator (if present) or empty string.
        wrap_params: wrap_params from generator (if present) or name (described above).
        ctype: C type made from wrap_type (described above), e.g. `CGLenum`.

    Parameters:
        args: Parameters of the OpenGL function.
        format_string: Format of one arg with {} in place of arg name.
        rstrip_string: String to rstrip the result with; defaults to ''.

    Returns:
        A C++ arguments string. Examples:

            'foo, bar, baz'
            '*_foo, *_bar, *_baz'
    """

    args_str = ''

    idx: int = 0
    arg: Argument
    for arg in args:
        name_with_array: str = arg.name
        # Some arguments are arrays; remove square brackets from their names.
        # For example, `baseAndCount[2]` -> `baseAndCount`.
        name: str = re.sub(r'\[.+?\]', '', name_with_array).strip()
        type: str = arg.type
        wrap_type: str = arg.wrap_type or ''
        wrap_params: str = arg.wrap_params or name
        ctype: str = make_ctype(type, wrap_type, idx)

        # Number of this `COutArgument` within its CFunction.
        # Doing this outside of make_ctype is messy, but can we do it cleanly?
        if wrap_type == 'COutArgument':
            idx += 1

        args_str += format_string.format(
            name_with_array=name_with_array,
            name=name,
            type=type,
            wrap_type=wrap_type,
            wrap_params=wrap_params,
            ctype=ctype,
        )

    return args_str.rstrip(rstrip_string)

def arg_call(
    token: Token,
    *,  # Force keyword args, as calls with positional bools are cryptic.
    add_retval: bool,
    recording: bool = False,
    wrap: bool = False,
) -> str:
    """Return arguments formatted for a call, like '(return_value, a, b)'."""
    args_str = ''

    if add_retval and token.return_value.type != 'void':
        args_str += 'return_value, '

    args_str += args_to_str(token.args, '{name}, ')

    if recording:
        args_str += 'Recording(_recorder), '

    if wrap:
        args_str += '_recorder, '

    return f"({args_str.strip(', ')})"

def driver_call(token: Token) -> str:
    """
    Create a string containing a driver call (for gitsPluginPrePostAuto.cpp).

    Parameters:
        token: data for one version of a token

    Returns:
        A C++ driver call string. Examples:

        'wrapper.Drivers().gl.glAccum(op, value);'
        'execWrap_glGetIntegerv(pname, data);'
        'auto return_value = wrapper.Drivers().gl.glMapBuffer(target, access);'
    """

    has_retval: bool = token.return_value.type != 'void'
    retval_assignment = 'auto return_value = ' if has_retval else ''

    exec_wrap: bool = token.interceptor_exec_override is True
    function_prefix = 'execWrap_' if exec_wrap else 'wrapper.Drivers().gl.'

    driver_args: str = arg_call(token, add_retval=False)

    return f'{retval_assignment}{function_prefix}{token.name}{driver_args};'

def mako_write(inpath: str, outpath: str, **kwargs) -> int:
    """Render a Mako template into a file."""
    generator_types = {
        'FuncType': FuncType,
        'Token': Token,
        'Argument': Argument,
        'ReturnValue': ReturnValue,
    }

    try:
        print(f"Generating {outpath}...")
        template = mako.template.Template(filename=inpath)
        rendered = template.render(**(generator_types | kwargs))
        rendered = re.sub(r'\r\n', r'\n', rendered)

        with open('..' + os.path.sep + outpath, 'w') as fout:
            fout.write(rendered)
    except Exception:
        traceback = mako.exceptions.RichTraceback()
        for filename, lineno, function, line in traceback.traceback:
            print(f"{filename}({lineno}) : error in {function}")
            print(line, "\n")
        print(f"{traceback.error.__class__.__name__}: {traceback.error}")
        return -1
    return 0

def update_gl_ids(
    source: str,
    destination_subpath: str,
    gl_functions: dict[str, list[Token]]
) -> None:
    """
    Append new IDs (if any) to an ID file.

    Parameters:
        source: Path to the ID file.
        destination_subpath: Path to copy the file to; relative to `OpenGL/`.
        gl_functions: Dict of Tokens to generate IDs from.
    """
    gl_ids: set[str] = set()
    with open(source, 'r') as gl_ids_file:
        for line in gl_ids_file:
            if line.startswith('ID'):
                gl_id = line.strip(',\n')
                gl_ids.add(gl_id)

    new_ids = ''
    for token_versions in gl_functions.values():
        for token in token_versions:
            gl_id = make_id(token.name, token.version)
            if gl_id not in gl_ids:
                new_ids += gl_id + ',\n'

    if not new_ids:
        print(f"File {source} is up to date.")
    else:
        print(f"Adding new IDs to {source} ...")
        with open(source, 'a') as gl_ids_file:
            gl_ids_file.write(new_ids)

    destination = os.path.join('..', destination_subpath)
    print(f"Copying {source} to {destination_subpath} ...")
    shutil.copy2(source, destination)


def main() -> None:
    """Generate all the files."""
    # Keys are OpenGL function names, values are lists of token versions.
    # Example: {'glFoo': [glFoo, glFoo_v1], 'glBar': [glBar]}
    all_tokens: dict[str, list[Token]] = get_tokens(include_disabled=True)
    enabled_tokens: dict[str, list[Token]] = get_tokens(include_disabled=False)

    update_gl_ids('glIDs.h', 'common/include', gl_functions=enabled_tokens)

    mako_write('templates/glIDswitch.h.mako',
               'common/include/glIDswitch.h',
               make_cname=make_cname,
               make_id=make_id,
               gl_functions=enabled_tokens)

    mako_write('templates/GLIPluginXX.def.mako',
               'GLIPlugin/GLIPlugin32.def',
               egl_function_names=egl_function_names,
               wgl_function_names=wgl_function_names,
               gl_functions=all_tokens)

    # EGL should only be exported on 32-bit.
    mako_write('templates/GLIPluginXX.def.mako',
               'GLIPlugin/GLIPlugin64.def',
               egl_function_names='',
               wgl_function_names=wgl_function_names,
               gl_functions=all_tokens)

    mako_write('templates/gitsPluginPrePostAuto.h.mako',
               'GLIPlugin/include/gitsPluginPrePostAuto.h',
               args_to_str=args_to_str,
               gl_functions=all_tokens)

    mako_write('templates/gitsPluginPrePostAuto.cpp.mako',
               'GLIPlugin/gitsPluginPrePostAuto.cpp',
               args_to_str=args_to_str,
               arg_call=arg_call,
               driver_call=driver_call,
               gl_functions=all_tokens)

    (draw_functions, nondraw_functions) = split_draw_functions(all_tokens)
    mako_write('templates/glDrivers.h.mako',
               'common/include/glDrivers.h',
               args_to_str=args_to_str,
               arg_call=arg_call,
               driver_call=driver_call,
               draw_functions=draw_functions,
               nondraw_functions=nondraw_functions)

    mako_write('templates/glFunctions.cpp.mako',
               'common/glFunctions.cpp',
               version_suffix=version_suffix,
               make_cname=make_cname,
               make_id=make_id,
               wrap_in_if=wrap_in_if,
               make_member_initializer_list=make_member_initializer_list,
               is_draw_function=is_draw_function,
               is_any_arg_special=is_any_arg_special,
               expand_special_args=expand_special_args,
               args_to_str=args_to_str,
               gl_functions=enabled_tokens)

    mako_write('templates/glFunctions.h.mako',
               'common/include/glFunctions.h',
               make_cname=make_cname,
               make_id=make_id,
               make_func_type_flags=make_func_type_flags,
               is_draw_function=is_draw_function,
               is_any_arg_special=is_any_arg_special,
               expand_special_args=expand_special_args,
               args_to_str=args_to_str,
               gl_functions=enabled_tokens)

    mako_write('templates/openglRecorderWrapperXAuto.h.mako',
               'recorder/include/openglRecorderWrapperAuto.h',
               args_to_str=args_to_str,
               is_iface=False,
               gl_functions=all_tokens)

    mako_write('templates/openglRecorderWrapperXAuto.h.mako',
               'recorder/include/openglRecorderWrapperIfaceAuto.h',
               args_to_str=args_to_str,
               is_iface=True,
               gl_functions=all_tokens)

    mako_write('templates/openglRecorderWrapperAuto.cpp.mako',
               'recorder/openglRecorderWrapperAuto.cpp',
               make_cname=make_cname,
               is_draw_function=is_draw_function,
               args_to_str=args_to_str,
               gl_functions=all_tokens)

if __name__ == '__main__':
    main()
