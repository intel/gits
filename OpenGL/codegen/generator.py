#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from collections import defaultdict
from dataclasses import dataclass, field
from enum import IntFlag
from typing import Any, Self



class FuncType(IntFlag):
    """Flag for API call classification."""

    NONE = 0
    BIND = 1
    CREATE = 2
    DELETE = 4
    FILL = 8
    GEN = 16
    GET = 32
    MAP = 64
    PARAM = 128
    RENDER = 256
    RESOURCE = 512
    QUERY = 1024
    COPY = 2048
    EXEC = 4096
    GET_ESSENTIAL = 8192


@dataclass(kw_only=True)
class Argument:
    """Argument of a Token."""

    name: str
    type: str
    wrap_type: str | None = None
    wrap_params: str | None = None
    remove_mapping: bool = False


@dataclass(kw_only=True)
class ReturnValue(Argument):
    """Return value of a Token."""

    name: str = field(init=False, default='return_value')
    type: str
    wrap_type: str | None = None
    wrap_params: str | None = None
    remove_mapping: bool = field(init=False, default=False)


@dataclass(kw_only=True)
class Token:
    """API call (or an internal GITS action)."""

    name: str
    enabled: bool
    function_type: FuncType
    # TODO: Do we need to keep inherit_from in Token?
    inherit_from: Self | None = None
    version: int = 0
    custom: bool = False  # TODO: It's unused in OpenGL, should we remove it?
    state_track: bool | str = False
    rec_condition: str | None = None
    run_condition: str | None = None
    wrap_type: str | None = None
    wrap_params: str | None = None
    recorder_wrap: bool | str = False
    exec_post_recorder_wrap: bool = False
    run_wrap: bool | str = False
    ccode_wrap: bool = False
    ccode_write_wrap: bool = False
    pass_token: bool = False
    interceptor_exec_override: bool = False
    prefix: str | None = None
    suffix: str | None = None
    pre_token: str | None = None
    pre_schedule: str | None = None
    remove_mapping: bool = False
    return_value: ReturnValue
    args: list[Argument]


# {'glFoo', [{'name': 'glFoo', 'version': 0}, {'name': 'glFoo', 'version': 1}]}
functions_dict: dict[str, list[dict[str, Any]]] = defaultdict(list)
functions_dict_processed: bool = False  # To avoid reprocessing/damaging data once it's ready.

# Tokens to ignore (not just disable).
blocklist: list[str] = ['glCreateSyncFromCLeventARB']

def apply_blocklist(functions_dict: dict[str, list[dict[str, Any]]]) -> None:
    """Remove blocked API calls entirely."""
    for func in blocklist:
        if func in functions_dict:
            del functions_dict[func]


def calculate_wrap_names(functions_dict: dict[str, list[dict[str, Any]]]) -> None:
    """
    Assign wrap/state_track names.

    When a wrap is inherited, the child token should call the parent token's
    wrap/state_track (e.g. glUnmapBufferARB calls glUnmapBuffer_SD, not
    glUnmapBufferARB_SD). On the other hand, if the child has its own wrap
    explicitly set to true, then it calls its own wrap, even though the parent
    also has this wrap.

    This function does a pass assigning wrap names where wraps are explicitly
    set. Inheritance handling should come after it.
    """
    funcs: list[dict[str, Any]]
    for funcs in functions_dict.values():
        func: dict[str, Any]
        for func in funcs:
            if func.get('state_track') is True:
                func['state_track'] = func['name']
            if func.get('recorder_wrap') is True:
                func['recorder_wrap'] = func['name']
            if func.get('run_wrap') is True:
                func['run_wrap'] = func['name']
            # TODO: Likely other wraps (like CCode) need the same treatment.


def handle_inheritance(functions_dict: dict[str, list[dict[str, Any]]]) -> None:
    """
    Copy data from parents to children, applying child overrides.

    Since tokens can inherit from tokens defined later in the file, we have to
    first gather data for all tokens and only then make a separate pass over
    the data to handle inheritance.
    """
    funcs: list[dict[str, Any]]
    for funcs in functions_dict.values():
        func: dict[str, Any]
        for func in funcs:
            parent_name: str | None = func.get('inherit_from')
            if parent_name:  # This token inherits from another.
                parents: list[dict[str, Any]] = functions_dict[parent_name]
                if len(parents) > 1:
                    raise RuntimeError("Ambiguous inheritance in generator.")
                parent: dict[str, Any] = parents[0]
                child = parent.copy()
                child |= func
                # Replace func with child.
                func.clear()
                func.update(child)


def Function(**kwargs) -> None:
    """Process function data and save it to a dict."""
    name: str = kwargs['name']
    functions_dict[name].append(kwargs)


def ArgDef(**kwargs):
    return Argument(**kwargs)


def RetDef(**kwargs):
    return ReturnValue(**kwargs)


def get_tokens(*, include_disabled: bool) -> dict[str, list[Token]]:
    """Return a dict with either all tokens or enabled ones only."""
    global functions_dict_processed
    if not functions_dict_processed:
        apply_blocklist(functions_dict)
        calculate_wrap_names(functions_dict)
        handle_inheritance(functions_dict)
        functions_dict_processed = True

    # Example: {'glFoo': [glFoo, glFoo_V1], 'glBar': [glBar]}
    tokens: defaultdict[str, list[Token]] = defaultdict(list)

    funcs: list[dict[str, Any]]
    for name, funcs in functions_dict.items():
        func: dict[str, Any]
        for func in funcs:
            if func.get('args') is None:
                raise RuntimeError(f"Function {func['name']} is missing the args array.")

            if include_disabled or func['enabled'] is True:
                tokens[name].append(Token(**func))

    # Lock the dict to prevent users from accidentally adding new Tokens.
    tokens.default_factory = None

    return tokens


Function(name='glAccum', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='value', type='GLfloat'),
    ],
)

Function(name='glAccumxOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='value', type='GLfixed'),
    ],
)

Function(name='glAcquireKeyedMutexWin32EXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='key', type='GLuint64'),
        Argument(name='timeout', type='GLuint'),
    ],
)

Function(name='glActiveProgramEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glActiveShaderProgram', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glActiveShaderProgramEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glActiveShaderProgram')

Function(name='glActiveStencilFaceEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
    ],
)

Function(name='glActiveTexture', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
    ],
)

Function(name='glActiveTextureARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glActiveTexture')

Function(name='glActiveVaryingNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glAddSwapHintRectWIN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glAlphaFragmentOp1ATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='dst', type='GLuint'),
        Argument(name='dstMod', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg1Rep', type='GLuint'),
        Argument(name='arg1Mod', type='GLuint'),
    ],
)

Function(name='glAlphaFragmentOp2ATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='dst', type='GLuint'),
        Argument(name='dstMod', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg1Rep', type='GLuint'),
        Argument(name='arg1Mod', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
        Argument(name='arg2Rep', type='GLuint'),
        Argument(name='arg2Mod', type='GLuint'),
    ],
)

Function(name='glAlphaFragmentOp3ATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='dst', type='GLuint'),
        Argument(name='dstMod', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg1Rep', type='GLuint'),
        Argument(name='arg1Mod', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
        Argument(name='arg2Rep', type='GLuint'),
        Argument(name='arg2Mod', type='GLuint'),
        Argument(name='arg3', type='GLuint'),
        Argument(name='arg3Rep', type='GLuint'),
        Argument(name='arg3Mod', type='GLuint'),
    ],
)

Function(name='glAlphaFunc', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLfloat'),
    ],
)

Function(name='glAlphaFuncQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLclampf'),
    ],
)

Function(name='glAlphaFuncx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLfixed'),
    ],
)

Function(name='glAlphaFuncxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glAlphaFuncx')

Function(name='glAlphaToCoverageDitherControlNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glApplyFramebufferAttachmentCMAAINTEL', enabled=True, function_type=FuncType.FILL,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glApplyFramebufferAttachmentCmaaINTEL', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target',type='GLenum'),
        Argument(name='attachment',type='GLenum'),
    ],
)

Function(name='glApplyTextureEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glAreProgramsResidentNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='const GLuint*'),
        Argument(name='residences', type='GLboolean*'),
    ],
)

Function(name='glAreTexturesResident', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='residences', type='GLboolean*'),
    ],
)

Function(name='glAreTexturesResidentEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glAreTexturesResident')

Function(name='glArrayElement', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(i)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='i', type='GLint'),
    ],
)

Function(name='glArrayElementEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glArrayElement')

Function(name='glArrayObjectATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='buffer', type='GLuint'),
        Argument(name='offset', type='GLuint'),
    ],
)

Function(name='glAsyncMarkerSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLuint'),
    ],
)

Function(name='glAttachObjectARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='containerObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

Function(name='glAttachShader', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glBegin', enabled=True, function_type=FuncType.PARAM, run_wrap=True, pass_token=True, state_track=True, prefix="""  // glBegin/glEnd are special, in that flattened playback in one thread
  // will switch contexts during vertex specification, which causes problems -
  // make current fails (at least on Windows). Here we lock, API mutex,
  // which is recursive, and effectively makes glBegin/glEnd block atomic
  // with respect to GL commands in other threads. This will cause big issues
  // if application fails to match glBegin and glEnd everywhere.
  globalMutex.lock();""",
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glBeginConditionalRender', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glBeginConditionalRenderNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glBeginConditionalRender')

Function(name='glBeginConditionalRenderNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBeginFragmentShaderATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glBeginOcclusionQueryNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBeginPerfMonitorAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
    ],
)

Function(name='glBeginPerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
    ],
)

Function(name='glBeginQuery', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBeginQueryARB', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glBeginQuery')

Function(name='glBeginQueryEXT', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glBeginQuery')

Function(name='glBeginQueryIndexed', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBeginTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='primitiveMode', type='GLenum'),
    ],
)

Function(name='glBeginTransformFeedbackEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBeginTransformFeedback')

Function(name='glBeginTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glBeginTransformFeedback')

Function(name='glBeginVertexShaderEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glBeginVideoCaptureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
    ],
)

Function(name='glBindAttribLocation', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glBindAttribLocationARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBindAttribLocation')

Function(name='glBindBuffer', enabled=True, function_type=FuncType.BIND, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glBindBufferARB', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBuffer')

Function(name='glBindBufferBase', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glBindBufferBaseEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferBase')

Function(name='glBindBufferBaseNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferBase')

Function(name='glBindBufferOffsetEXT', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glBindBufferOffsetNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferOffsetEXT')

Function(name='glBindBufferRange', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glBindBufferRangeEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferRange')

Function(name='glBindBufferRangeNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferRange')

Function(name='glBindBuffersBase', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*', wrap_type='CGLBuffer::CSArray', wrap_params='count, buffers'),
    ],
)

Function(name='glBindBuffersRange', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*'),
        Argument(name='offsets', type='const GLintptr*'),
        Argument(name='sizes', type='const GLsizeiptr*'),
    ],
)

Function(name='glBindFragDataLocation', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='color', type='GLuint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glBindFragDataLocationEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindFragDataLocation')

Function(name='glBindFragDataLocationIndexed', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='colorNumber', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glBindFragDataLocationIndexedEXT', enabled=False, function_type=FuncType.BIND, inherit_from='glBindFragDataLocationIndexed')

Function(name='glBindFragmentShaderATI', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBindFramebuffer', enabled=True, function_type=FuncType.BIND, state_track=True, run_wrap=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
    ],
)

Function(name='glBindFramebufferEXT', enabled=True, function_type=FuncType.BIND, state_track=True, run_wrap=False, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
    ],
)

Function(name='glBindFramebufferOES', enabled=True, function_type=FuncType.BIND, inherit_from='glBindFramebufferEXT', run_wrap=False, recorder_wrap=True)

Function(name='glBindImageTexture', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='unit', type='GLuint'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layered', type='GLboolean'),
        Argument(name='layer', type='GLint'),
        Argument(name='access', type='GLenum'),
        Argument(name='format', type='GLenum'),
    ],
)

Function(name='glBindImageTextureEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layered', type='GLboolean'),
        Argument(name='layer', type='GLint'),
        Argument(name='access', type='GLenum'),
        Argument(name='format', type='GLint'),
    ],
)

Function(name='glBindImageTextures', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='textures', type='const GLuint*', wrap_type='CGLTexture::CSArray', wrap_params='count, textures'),
    ],
)

Function(name='glBindLightParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

Function(name='glBindMaterialParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

Function(name='glBindMultiTextureEXT', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glBindParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='value', type='GLenum'),
    ],
)

Function(name='glBindProgramARB', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='program', type='GLuint'),
    ],
)

Function(name='glBindProgramNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBindProgramPipeline', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
    ],
)

Function(name='glBindProgramPipelineEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindProgramPipeline')

Function(name='glBindRenderbuffer', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

Function(name='glBindRenderbufferEXT', enabled=True, function_type=FuncType.BIND, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

Function(name='glBindRenderbufferOES', enabled=True, function_type=FuncType.BIND, inherit_from='glBindRenderbuffer')

Function(name='glBindSampler', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='unit', type='GLuint'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

Function(name='glBindSamplers', enabled=True, function_type=FuncType.BIND, ccode_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='samplers', type='const GLuint*', wrap_type='CGLSampler::CSArray', wrap_params='count, samplers'),
    ],
)

Function(name='glBindTexGenParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='unit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

Function(name='glBindTexture', enabled=True, function_type=FuncType.BIND, state_track=True, rec_condition='ConditionTextureES(_recorder)', run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glBindTextureEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindTexture', rec_condition=False, run_condition=False)

Function(name='glBindTextureUnit', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='unit', type='GLuint'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glBindTextureUnitParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='unit', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

Function(name='glBindTextures', enabled=True, function_type=FuncType.BIND, ccode_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='textures', type='const GLuint*', wrap_type='CGLTexture::CSArray', wrap_params='count, textures'),
    ],
)

Function(name='glBindTransformFeedback', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint', wrap_type='CGLTransformFeedback'),
    ],
)

Function(name='glBindTransformFeedbackNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindTransformFeedback')

Function(name='glBindVertexArray', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLuint', wrap_type='CGLVertexArray'),
    ],
)

Function(name='glBindVertexArrayAPPLE', enabled=False, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLuint'),
    ],
)

Function(name='glBindVertexArrayOES', enabled=True, function_type=FuncType.BIND, inherit_from='glBindVertexArray')

Function(name='glBindVertexBuffer', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glBindVertexBuffers', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*'),
        Argument(name='offsets', type='const GLintptr*'),
        Argument(name='strides', type='const GLsizei*'),
    ],
)

Function(name='glBindVertexShaderEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glBindVideoCaptureStreamBufferNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='frame_region', type='GLenum'),
        Argument(name='offset', type='GLintptrARB'),
    ],
)

Function(name='glBindVideoCaptureStreamTextureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='frame_region', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glBinormal3bEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLbyte'),
        Argument(name='by', type='GLbyte'),
        Argument(name='bz', type='GLbyte'),
    ],
)

Function(name='glBinormal3bvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*'),
    ],
)

Function(name='glBinormal3dEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLdouble'),
        Argument(name='by', type='GLdouble'),
        Argument(name='bz', type='GLdouble'),
    ],
)

Function(name='glBinormal3dvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*'),
    ],
)

Function(name='glBinormal3fEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLfloat'),
        Argument(name='by', type='GLfloat'),
        Argument(name='bz', type='GLfloat'),
    ],
)

Function(name='glBinormal3fvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glBinormal3iEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLint'),
        Argument(name='by', type='GLint'),
        Argument(name='bz', type='GLint'),
    ],
)

Function(name='glBinormal3ivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*'),
    ],
)

Function(name='glBinormal3sEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLshort'),
        Argument(name='by', type='GLshort'),
        Argument(name='bz', type='GLshort'),
    ],
)

Function(name='glBinormal3svEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*'),
    ],
)

Function(name='glBinormalPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glBitmap', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='xorig', type='GLfloat'),
        Argument(name='yorig', type='GLfloat'),
        Argument(name='xmove', type='GLfloat'),
        Argument(name='ymove', type='GLfloat'),
        Argument(name='bitmap', type='const GLubyte*', wrap_type='CGLBitmapResource', wrap_params='width, height, bitmap'),
    ],
)

Function(name='glBitmapxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='xorig', type='GLfixed'),
        Argument(name='yorig', type='GLfixed'),
        Argument(name='xmove', type='GLfixed'),
        Argument(name='ymove', type='GLfixed'),
        Argument(name='bitmap', type='const GLubyte*', wrap_type='CGLBitmapResource'),
    ],
)

Function(name='glBlendBarrier', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glBlendBarrierKHR', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendBarrier')

Function(name='glBlendBarrierNV', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendBarrier')

Function(name='glBlendColor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

Function(name='glBlendColorEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendColor')

Function(name='glBlendColorxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

Function(name='glBlendEquation', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glBlendEquationEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquation')

Function(name='glBlendEquationIndexedAMD', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationi')

Function(name='glBlendEquationOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquation')

Function(name='glBlendEquationSeparate', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='modeRGB', type='GLenum'),
        Argument(name='modeAlpha', type='GLenum'),
    ],
)

Function(name='glBlendEquationSeparateATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparate')

Function(name='glBlendEquationSeparateEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparate')

Function(name='glBlendEquationSeparateIndexedAMD', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparatei')

Function(name='glBlendEquationSeparateOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparate')

Function(name='glBlendEquationSeparatei', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='modeRGB', type='GLenum'),
        Argument(name='modeAlpha', type='GLenum'),
    ],
)

Function(name='glBlendEquationSeparateiARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparatei')

Function(name='glBlendEquationSeparateiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparatei')

Function(name='glBlendEquationSeparateiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendEquationSeparatei')

Function(name='glBlendEquationi', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glBlendEquationiARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationi')

Function(name='glBlendEquationiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationi')

Function(name='glBlendEquationiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendEquationi')

Function(name='glBlendFunc', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sfactor', type='GLenum'),
        Argument(name='dfactor', type='GLenum'),
    ],
)

Function(name='glBlendFuncIndexedAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='src', type='GLenum'),
        Argument(name='dst', type='GLenum'),
    ],
)

Function(name='glBlendFuncSeparate', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sfactorRGB', type='GLenum'),
        Argument(name='dfactorRGB', type='GLenum'),
        Argument(name='sfactorAlpha', type='GLenum'),
        Argument(name='dfactorAlpha', type='GLenum'),
    ],
)

Function(name='glBlendFuncSeparateEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparate')

Function(name='glBlendFuncSeparateINGR', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparate')

Function(name='glBlendFuncSeparateIndexedAMD', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparatei')

Function(name='glBlendFuncSeparateOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparate')

Function(name='glBlendFuncSeparatei', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='srcRGB', type='GLenum'),
        Argument(name='dstRGB', type='GLenum'),
        Argument(name='srcAlpha', type='GLenum'),
        Argument(name='dstAlpha', type='GLenum'),
    ],
)

Function(name='glBlendFuncSeparateiARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparatei')

Function(name='glBlendFuncSeparateiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparatei')

Function(name='glBlendFuncSeparateiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendFuncSeparatei')

Function(name='glBlendFunci', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='src', type='GLenum'),
        Argument(name='dst', type='GLenum'),
    ],
)

Function(name='glBlendFunciARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFunci')

Function(name='glBlendFunciEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFunci')

Function(name='glBlendFunciOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendFunci')

Function(name='glBlendParameteriNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

Function(name='glBlitFramebuffer', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='srcX0', type='GLint'),
        Argument(name='srcY0', type='GLint'),
        Argument(name='srcX1', type='GLint'),
        Argument(name='srcY1', type='GLint'),
        Argument(name='dstX0', type='GLint'),
        Argument(name='dstY0', type='GLint'),
        Argument(name='dstX1', type='GLint'),
        Argument(name='dstY1', type='GLint'),
        Argument(name='mask', type='GLbitfield'),
        Argument(name='filter', type='GLenum'),
    ],
)

Function(name='glBlitFramebufferANGLE', enabled=True, function_type=FuncType.FILL, inherit_from='glBlitFramebuffer')

Function(name='glBlitFramebufferEXT', enabled=True, function_type=FuncType.FILL, inherit_from='glBlitFramebuffer')

Function(name='glBlitFramebufferNV', enabled=False, function_type=FuncType.FILL, inherit_from='glBlitFramebuffer')

Function(name='glBlitNamedFramebuffer', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readFramebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='drawFramebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='srcX0', type='GLint'),
        Argument(name='srcY0', type='GLint'),
        Argument(name='srcX1', type='GLint'),
        Argument(name='srcY1', type='GLint'),
        Argument(name='dstX0', type='GLint'),
        Argument(name='dstY0', type='GLint'),
        Argument(name='dstX1', type='GLint'),
        Argument(name='dstY1', type='GLint'),
        Argument(name='mask', type='GLbitfield'),
        Argument(name='filter', type='GLenum'),
    ],
)

Function(name='glBufferAddressRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='address', type='GLuint64EXT'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

Function(name='glBufferData', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionBufferData(_recorder, target, size, data, usage)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='usage', type='GLenum'),
    ],
)

Function(name='glBufferDataARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glBufferData', rec_condition=False)

Function(name='glBufferPageCommitmentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='commit', type='GLboolean'),
    ],
)

Function(name='glBufferParameteriAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glBufferStorage', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionBufferStorage(_recorder, target, size, data, flags)', interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='flags', type='GLbitfield', wrap_type='CBufferStorageFlags'),
    ],
)

Function(name='glBufferStorageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glBufferStorage', interceptor_exec_override=False)

Function(name='glBufferStorageExternalEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='clientBuffer', type='GLeglClientBufferEXT'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glBufferStorageMemEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
    ],
)

Function(name='glBufferSubDataARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glBufferSubData', rec_condition=False)

Function(name='glCallCommandListNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

Function(name='glCallList', enabled=True, function_type=FuncType.RENDER, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

Function(name='glCallLists', enabled=True, function_type=FuncType.RENDER, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='lists', type='const void*', wrap_type='CGLubyte::CSArray', wrap_params='DataTypeSize(type) * n, static_cast<const GLubyte*>(lists)'),
    ],
)

Function(name='glCheckFramebufferStatus', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glCheckFramebufferStatusEXT', enabled=True, function_type=FuncType.GET, inherit_from='glCheckFramebufferStatus', recorder_wrap=True)

Function(name='glCheckFramebufferStatusOES', enabled=True, function_type=FuncType.GET, inherit_from='glCheckFramebufferStatus')

Function(name='glCheckNamedFramebufferStatus', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glCheckNamedFramebufferStatusEXT', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glClampColor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='clamp', type='GLenum'),
    ],
)

Function(name='glClampColorARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glClampColor')

Function(name='glClear', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glClearAccum', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

Function(name='glClearAccumxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

Function(name='glClearBufferData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

Function(name='glClearBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

Function(name='glClearBufferfi', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='depth', type='GLfloat'),
        Argument(name='stencil', type='GLint'),
    ],
)

Function(name='glClearBufferfv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='buffer, value'),
    ],
)

Function(name='glClearBufferiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='buffer, value'),
    ],
)

Function(name='glClearBufferuiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='buffer, value'),
    ],
)

Function(name='glClearColor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

Function(name='glClearColorIiEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
        Argument(name='alpha', type='GLint'),
    ],
)

Function(name='glClearColorIuiEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
        Argument(name='alpha', type='GLuint'),
    ],
)

Function(name='glClearColorx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

Function(name='glClearColorxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClearColorx')

Function(name='glClearDepth', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='depth', type='GLdouble'),
    ],
)

Function(name='glClearDepthdNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='depth', type='GLdouble'),
    ],
)

Function(name='glClearDepthf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='d', type='GLfloat'),
    ],
)

Function(name='glClearDepthfOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClearDepthf',
    args=[
        Argument(name='depth', type='GLclampf'),
    ],
)

Function(name='glClearDepthx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='depth', type='GLfixed'),
    ],
)

Function(name='glClearDepthxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClearDepthx',
    args=[
        Argument(name='depth', type='GLclampx'),
    ],
)

Function(name='glClearIndex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLfloat'),
    ],
)

Function(name='glClearNamedBufferData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

Function(name='glClearNamedBufferDataEXT', enabled=False, function_type=FuncType.RESOURCE, inherit_from='glClearNamedBufferData')

Function(name='glClearNamedBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

# TODO: Shouldn't it just inherit from the non-EXT function instead of defining its own arguments?
Function(name='glClearNamedBufferSubDataEXT', enabled=False, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='offset', type='GLsizeiptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*'),
    ],
)

Function(name='glClearNamedFramebufferfi', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='depth', type='GLfloat'),
        Argument(name='stencil', type='GLint'),
    ],
)

Function(name='glClearNamedFramebufferfv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='buffer, value'),
    ],
)

Function(name='glClearNamedFramebufferiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='buffer, value'),
    ],
)

Function(name='glClearNamedFramebufferuiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='buffer, value'),
    ],
)

Function(name='glClearPixelLocalStorageuiEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='offset', type='GLsizei'),
        Argument(name='n', type='GLsizei'),
        Argument(name='values', type='const GLuint*'),
    ],
)

Function(name='glClearStencil', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
    ],
)

Function(name='glClearTexImage', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CGLClearTexResource', wrap_params='data, texture, level, format, type'),
    ],
)

Function(name='glClearTexImageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glClearTexImage')

Function(name='glClearTexSubImage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource'),
    ],
)

Function(name='glClearTexSubImageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glClearTexSubImage')

Function(name='glClientActiveTexture', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
    ],
)

Function(name='glClientActiveTextureARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glClientActiveTexture')

Function(name='glClientActiveVertexStreamATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
    ],
)

Function(name='glClientAttribDefaultEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glClientWaitSync', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync'),
        Argument(name='flags', type='GLbitfield'),
        Argument(name='timeout', type='GLuint64'),
    ],
)

Function(name='glClientWaitSyncAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='sync', type='GLsync'),
        Argument(name='flags', type='GLbitfield'),
        Argument(name='timeout', type='GLuint64'),
    ],
)

Function(name='glClipControl', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='origin', type='GLenum'),
        Argument(name='depth', type='GLenum'),
    ],
)

Function(name='glClipControlEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glClipControl')

Function(name='glClipPlane', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='const GLdouble*', wrap_params='4, equation'),
    ],
)

Function(name='glClipPlanef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='p', type='GLenum'),
        Argument(name='eqn', type='const GLfloat*', wrap_params='4, eqn'),
    ],
)

Function(name='glClipPlanefIMG', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanef')

Function(name='glClipPlanefOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanef')

Function(name='glClipPlanex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='const GLfixed*', wrap_params='4, equation'),
    ],
)

Function(name='glClipPlanexIMG', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanex')

Function(name='glClipPlanexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanex')

Function(name='glColor3b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLbyte'),
        Argument(name='green', type='GLbyte'),
        Argument(name='blue', type='GLbyte'),
    ],
)

Function(name='glColor3bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

Function(name='glColor3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLdouble'),
        Argument(name='green', type='GLdouble'),
        Argument(name='blue', type='GLdouble'),
    ],
)

Function(name='glColor3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glColor3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
    ],
)

Function(name='glColor3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glColor3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLfloat*', wrap_params='3, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glColor3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glColor3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLhalfNV'),
        Argument(name='green', type='GLhalfNV'),
        Argument(name='blue', type='GLhalfNV'),
    ],
)

Function(name='glColor3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

Function(name='glColor3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
    ],
)

Function(name='glColor3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glColor3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLshort'),
        Argument(name='green', type='GLshort'),
        Argument(name='blue', type='GLshort'),
    ],
)

Function(name='glColor3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glColor3ub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLubyte'),
        Argument(name='green', type='GLubyte'),
        Argument(name='blue', type='GLubyte'),
    ],
)

Function(name='glColor3ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLubyte*', wrap_params='3, v'),
    ],
)

Function(name='glColor3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
    ],
)

Function(name='glColor3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLuint*', wrap_params='3, v'),
    ],
)

Function(name='glColor3us', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLushort'),
        Argument(name='green', type='GLushort'),
        Argument(name='blue', type='GLushort'),
    ],
)

Function(name='glColor3usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLushort*', wrap_params='3, v'),
    ],
)

Function(name='glColor3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
    ],
)

Function(name='glColor3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='components', type='const GLfixed*'),
    ],
)

Function(name='glColor4b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLbyte'),
        Argument(name='green', type='GLbyte'),
        Argument(name='blue', type='GLbyte'),
        Argument(name='alpha', type='GLbyte'),
    ],
)

Function(name='glColor4bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

Function(name='glColor4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLdouble'),
        Argument(name='green', type='GLdouble'),
        Argument(name='blue', type='GLdouble'),
        Argument(name='alpha', type='GLdouble'),
    ],
)

Function(name='glColor4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glColor4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

Function(name='glColor4fNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='a', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glColor4fNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLfloat*', wrap_params='4, c'),
        Argument(name='n', type='const GLfloat*', wrap_params='3, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glColor4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glColor4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLhalfNV'),
        Argument(name='green', type='GLhalfNV'),
        Argument(name='blue', type='GLhalfNV'),
        Argument(name='alpha', type='GLhalfNV'),
    ],
)

Function(name='glColor4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

Function(name='glColor4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
        Argument(name='alpha', type='GLint'),
    ],
)

Function(name='glColor4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glColor4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLshort'),
        Argument(name='green', type='GLshort'),
        Argument(name='blue', type='GLshort'),
        Argument(name='alpha', type='GLshort'),
    ],
)

Function(name='glColor4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glColor4ub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLubyte'),
        Argument(name='green', type='GLubyte'),
        Argument(name='blue', type='GLubyte'),
        Argument(name='alpha', type='GLubyte'),
    ],
)

Function(name='glColor4ubVertex2fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='r', type='GLubyte'),
        Argument(name='g', type='GLubyte'),
        Argument(name='b', type='GLubyte'),
        Argument(name='a', type='GLubyte'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glColor4ubVertex2fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLubyte*', wrap_params='4, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glColor4ubVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='r', type='GLubyte'),
        Argument(name='g', type='GLubyte'),
        Argument(name='b', type='GLubyte'),
        Argument(name='a', type='GLubyte'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glColor4ubVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLubyte*', wrap_params='4, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glColor4ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

Function(name='glColor4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
        Argument(name='alpha', type='GLuint'),
    ],
)

Function(name='glColor4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

Function(name='glColor4us', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLushort'),
        Argument(name='green', type='GLushort'),
        Argument(name='blue', type='GLushort'),
        Argument(name='alpha', type='GLushort'),
    ],
)

Function(name='glColor4usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

Function(name='glColor4x', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

Function(name='glColor4xOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glColor4x')

Function(name='glColor4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='components', type='const GLfixed*'),
    ],
)

Function(name='glColorFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glColorFragmentOp1ATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='dst', type='GLuint'),
        Argument(name='dstMask', type='GLuint'),
        Argument(name='dstMod', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg1Rep', type='GLuint'),
        Argument(name='arg1Mod', type='GLuint'),
    ],
)

Function(name='glColorFragmentOp2ATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='dst', type='GLuint'),
        Argument(name='dstMask', type='GLuint'),
        Argument(name='dstMod', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg1Rep', type='GLuint'),
        Argument(name='arg1Mod', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
        Argument(name='arg2Rep', type='GLuint'),
        Argument(name='arg2Mod', type='GLuint'),
    ],
)

Function(name='glColorFragmentOp3ATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='dst', type='GLuint'),
        Argument(name='dstMask', type='GLuint'),
        Argument(name='dstMod', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg1Rep', type='GLuint'),
        Argument(name='arg1Mod', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
        Argument(name='arg2Rep', type='GLuint'),
        Argument(name='arg2Mod', type='GLuint'),
        Argument(name='arg3', type='GLuint'),
        Argument(name='arg3Rep', type='GLuint'),
        Argument(name='arg3Mod', type='GLuint'),
    ],
)

Function(name='glColorMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLboolean'),
        Argument(name='green', type='GLboolean'),
        Argument(name='blue', type='GLboolean'),
        Argument(name='alpha', type='GLboolean'),
    ],
)

Function(name='glColorMaskIndexedEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glColorMaski')

Function(name='glColorMaski', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='r', type='GLboolean'),
        Argument(name='g', type='GLboolean'),
        Argument(name='b', type='GLboolean'),
        Argument(name='a', type='GLboolean'),
    ],
)

Function(name='glColorMaskiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glColorMaski')

Function(name='glColorMaskiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glColorMaski')

Function(name='glColorMaterial', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glColorP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='GLuint'),
    ],
)

Function(name='glColorP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='const GLuint*', wrap_params='3, color'),
    ],
)

Function(name='glColorP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='GLuint'),
    ],
)

Function(name='glColorP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='const GLuint*', wrap_params='4, color'),
    ],
)

Function(name='glColorPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glColorPointerBounds', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='ptr', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glColorPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glColorPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glColorPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

Function(name='glColorSubTable', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource'),
    ],
)

Function(name='glColorSubTableEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glColorSubTable')

Function(name='glColorTable', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='table', type='const void*'),
    ],
)

Function(name='glColorTableEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTable')

Function(name='glColorTableParameterfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glColorTableParameterfvSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTableParameterfv')

Function(name='glColorTableParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glColorTableParameterivSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTableParameteriv')

Function(name='glColorTableSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTable')

Function(name='glCombinerInputNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='variable', type='GLenum'),
        Argument(name='input', type='GLenum'),
        Argument(name='mapping', type='GLenum'),
        Argument(name='componentUsage', type='GLenum'),
    ],
)

Function(name='glCombinerOutputNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='abOutput', type='GLenum'),
        Argument(name='cdOutput', type='GLenum'),
        Argument(name='sumOutput', type='GLenum'),
        Argument(name='scale', type='GLenum'),
        Argument(name='bias', type='GLenum'),
        Argument(name='abDotProduct', type='GLboolean'),
        Argument(name='cdDotProduct', type='GLboolean'),
        Argument(name='muxSum', type='GLboolean'),
    ],
)

Function(name='glCombinerParameterfNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glCombinerParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glCombinerParameteriNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glCombinerParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glCombinerStageParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glCommandListSegmentsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='segments', type='GLuint'),
    ],
)

Function(name='glCompileCommandListNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

Function(name='glCompileShader', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glCompileShaderARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glCompileShader',
    args=[
        Argument(name='shaderObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

Function(name='glCompileShaderIncludeARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei'),
        Argument(name='path', type='const GLchar*const*'),
        Argument(name='length', type='const GLint*'),
    ],
)

Function(name='glCompressedMultiTexImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const GLvoid*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, bits'),
    ],
)

Function(name='glCompressedMultiTexImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, imageSize, bits'),
    ],
)

Function(name='glCompressedMultiTexImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, bits'),
    ],
)

Function(name='glCompressedMultiTexSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, bits'),
    ],
)

Function(name='glCompressedMultiTexSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, imageSize, bits'),
    ],
)

Function(name='glCompressedMultiTexSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, bits'),
    ],
)

Function(name='glCompressedTexImage1D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, data'),
    ],
)

Function(name='glCompressedTexImage1DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage1D')

Function(name='glCompressedTexImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, imageSize, data'),
    ],
)

Function(name='glCompressedTexImage2DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage2D', rec_condition=False)

Function(name='glCompressedTexImage3D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, data'),
    ],
)

Function(name='glCompressedTexImage3DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage3D')

Function(name='glCompressedTexImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage3D')

Function(name='glCompressedTexSubImage1D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, data'),
    ],
)

Function(name='glCompressedTexSubImage1DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexSubImage1D')

Function(name='glCompressedTexSubImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, imageSize, data'),
    ],
)

# FIXME: This function had `RecCond=False` - a typo of recCond that used to be ignored.
# FIXME: Should this function have `rec_condition=False` or not?
# This rec_condition causes openglRecorderWrapperAuto.cpp to use `Recording(_recorder)` instead of `ConditionTextureES(_recorder)`.
Function(name='glCompressedTexSubImage2DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexSubImage2D', rec_condition=False)

Function(name='glCompressedTexSubImage3D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, data'),
    ],
)

Function(name='glCompressedTexSubImage3DARB', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', inherit_from='glCompressedTexSubImage3D')

Function(name='glCompressedTexSubImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexSubImage3D')

Function(name='glCompressedTextureImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, bits'),
    ],
)

Function(name='glCompressedTextureImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, imageSize, bits'),
    ],
)

Function(name='glCompressedTextureImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, bits'),
    ],
)

Function(name='glCompressedTextureSubImage1D', enabled=False, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, data'),
    ],
)

Function(name='glCompressedTextureSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, imageSize, bits'),
    ],
)

Function(name='glCompressedTextureSubImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='texture, width, height, imageSize, data'),
    ],
)

Function(name='glCompressedTextureSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, imageSize, bits'),
    ],
)

Function(name='glCompressedTextureSubImage3D', enabled=False, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='data', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, data'),
    ],
)

Function(name='glCompressedTextureSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='imageSize', type='GLsizei'),
        Argument(name='bits', type='const void*', wrap_type='CGLCompressedTexResource', wrap_params='target, width, height, depth, imageSize, bits'),
    ],
)

Function(name='glConservativeRasterParameterfNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat'),
    ],
)

Function(name='glConservativeRasterParameteriNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glConvolutionFilter1D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='image', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, image'),
    ],
)

Function(name='glConvolutionFilter1DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glConvolutionFilter1D')

Function(name='glConvolutionFilter2D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='image', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, image'),
    ],
)

Function(name='glConvolutionFilter2DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glConvolutionFilter2D')

Function(name='glConvolutionParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat'),
    ],
)

Function(name='glConvolutionParameterfEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameterf')

Function(name='glConvolutionParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glConvolutionParameterfvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameterfv')

Function(name='glConvolutionParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint'),
    ],
)

Function(name='glConvolutionParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameteri')

Function(name='glConvolutionParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glConvolutionParameterivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameteriv')

Function(name='glConvolutionParameterxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glConvolutionParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*'),
    ],
)

Function(name='glCopyBufferSubData', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readTarget', type='GLenum'),
        Argument(name='writeTarget', type='GLenum'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glCopyBufferSubDataNV', enabled=False, function_type=FuncType.COPY, inherit_from='glCopyBufferSubData')

Function(name='glCopyColorSubTable', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLsizei'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyColorSubTableEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyColorSubTable')

Function(name='glCopyColorTable', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyColorTableSGI', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyColorTable')

Function(name='glCopyConvolutionFilter1D', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyConvolutionFilter1DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyConvolutionFilter1D')

Function(name='glCopyConvolutionFilter2D', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyConvolutionFilter2DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyConvolutionFilter2D')

Function(name='glCopyImageSubData', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='srcName', type='GLuint'),
        Argument(name='srcTarget', type='GLenum'),
        Argument(name='srcLevel', type='GLint'),
        Argument(name='srcX', type='GLint'),
        Argument(name='srcY', type='GLint'),
        Argument(name='srcZ', type='GLint'),
        Argument(name='dstName', type='GLuint'),
        Argument(name='dstTarget', type='GLenum'),
        Argument(name='dstLevel', type='GLint'),
        Argument(name='dstX', type='GLint'),
        Argument(name='dstY', type='GLint'),
        Argument(name='dstZ', type='GLint'),
        Argument(name='srcWidth', type='GLsizei'),
        Argument(name='srcHeight', type='GLsizei'),
        Argument(name='srcDepth', type='GLsizei'),
    ],
)

Function(name='glCopyImageSubDataEXT', enabled=False, function_type=FuncType.COPY, inherit_from='glCopyImageSubData')

Function(name='glCopyImageSubDataNV', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyImageSubData')

Function(name='glCopyImageSubDataOES', enabled=False, function_type=FuncType.COPY, inherit_from='glCopyImageSubData')

Function(name='glCopyMultiTexImage1DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
    ],
)

Function(name='glCopyMultiTexImage2DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
    ],
)

Function(name='glCopyMultiTexSubImage1DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyMultiTexSubImage2DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyMultiTexSubImage3DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyNamedBufferSubData', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='writeBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glCopyPathNV', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='srcPath', type='GLuint'),
    ],
)

Function(name='glCopyPixels', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='type', type='GLenum'),
    ],
)

Function(name='glCopyTexImage1D', enabled=True, function_type=FuncType.COPY, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
    ],
)

Function(name='glCopyTexImage1DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexImage1D')

Function(name='glCopyTexImage2D', enabled=True, function_type=FuncType.COPY, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
    ],
)

Function(name='glCopyTexImage2DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexImage2D')

Function(name='glCopyTexSubImage1D', enabled=True, function_type=FuncType.COPY, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyTexSubImage1DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage1D')

Function(name='glCopyTexSubImage2D', enabled=True, function_type=FuncType.COPY, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyTexSubImage2DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage2D')

Function(name='glCopyTexSubImage3D', enabled=True, function_type=FuncType.COPY, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyTexSubImage3DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage3D')

Function(name='glCopyTexSubImage3DOES', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage3D')

Function(name='glCopyTextureImage1DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
    ],
)

Function(name='glCopyTextureImage2DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
    ],
)

Function(name='glCopyTextureLevelsAPPLE', enabled=False, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='destinationTexture', type='GLuint'),
        Argument(name='sourceTexture', type='GLuint'),
        Argument(name='sourceBaseLevel', type='GLint'),
        Argument(name='sourceLevelCount', type='GLsizei'),
    ],
)

Function(name='glCopyTextureSubImage1D', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyTextureSubImage1DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glCopyTextureSubImage2D', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyTextureSubImage2DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyTextureSubImage3D', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCopyTextureSubImage3DEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glCoverFillPathInstancedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*'),
    ],
)

Function(name='glCoverFillPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

Function(name='glCoverStrokePathInstancedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*'),
    ],
)

Function(name='glCoverStrokePathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

Function(name='glCoverageMaskNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLboolean'),
    ],
)

Function(name='glCoverageModulationNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='components', type='GLenum'),
    ],
)

Function(name='glCoverageModulationTableNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glCoverageOperationNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='operation', type='GLenum'),
    ],
)

Function(name='glCreateBuffers', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='buffers', type='GLuint*', wrap_type='CGLBuffer::CSMapArray', wrap_params='n, buffers'),
    ],
)

Function(name='glCreateCommandListsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='lists', type='GLuint*'),
    ],
)

Function(name='glCreateFramebuffers', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='GLuint*', wrap_type='CGLFramebuffer::CSMapArray', wrap_params='n, framebuffers'),
    ],
)

Function(name='glCreateMemoryObjectsEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='memoryObjects', type='GLuint*'),
    ],
)

Function(name='glCreatePerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint'),
        Argument(name='queryHandle', type='GLuint*'),
    ],
)

Function(name='glCreateProgram', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[],
)

Function(name='glCreateProgramObjectARB', enabled=True, function_type=FuncType.CREATE, inherit_from='glCreateProgram',
    return_value=ReturnValue(type='GLhandleARB', wrap_type='CGLProgram'),
    args=[],
)

Function(name='glCreateProgramPipelines', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='pipelines', type='GLuint*', wrap_type='CGLPipeline::CSMapArray', wrap_params='n, pipelines'),
    ],
)

Function(name='glCreateQueries', enabled=True, function_type=FuncType.GEN|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLQuery::CSMapArray', wrap_params='n, ids'),
    ],
)

Function(name='glCreateRenderbuffers', enabled=True, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='GLuint*', wrap_type='CGLRenderbuffer::CSMapArray', wrap_params='n, renderbuffers'),
    ],
)

Function(name='glCreateSamplers', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='samplers', type='GLuint*', wrap_type='CGLSampler::CSMapArray', wrap_params='n, samplers'),
    ],
)

Function(name='glCreateShader', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[
        Argument(name='type', type='GLenum'),
    ],
)

Function(name='glCreateShaderObjectARB', enabled=True, function_type=FuncType.CREATE, inherit_from='glCreateShader',
    return_value=ReturnValue(type='GLhandleARB', wrap_type='CGLProgram'),
    args=[
        Argument(name='shaderType', type='GLenum'),
    ],
)

Function(name='glCreateShaderProgramEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='string', type='const GLchar*'),
    ],
)

Function(name='glCreateShaderProgramv', enabled=True, function_type=FuncType.CREATE|FuncType.RESOURCE, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='count', type='GLsizei', wrap_params='1'),
        Argument(name='strings', type='const GLchar*const*', wrap_type='CShaderSource', wrap_params='count, strings, _strings.SHADER_PROGRAM'),
    ],
)

Function(name='glCreateShaderProgramvEXT', enabled=True, function_type=FuncType.CREATE|FuncType.RESOURCE, run_wrap=True, inherit_from='glCreateShaderProgramv')

Function(name='glCreateStatesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='states', type='GLuint*'),
    ],
)

Function(name='glCreateSyncFromCLeventARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLsync'),
    args=[
        Argument(name='context', type='struct _cl_context*'),
        Argument(name='event', type='struct _cl_event*'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glCreateTextures', enabled=True, function_type=FuncType.CREATE, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='GLuint*', wrap_type='CGLTexture::CSMapArray', wrap_params='n, textures'),
    ],
)

Function(name='glCreateTransformFeedbacks', enabled=True, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLTransformFeedback::CSMapArray', wrap_params='n, ids'),
    ],
)

Function(name='glCreateVertexArrays', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='GLuint*', wrap_type='CGLVertexArray::CSMapArray', wrap_params='n, arrays'),
    ],
)

Function(name='glCullFace', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glCullParameterdvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glCullParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glCurrentPaletteMatrixARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLint'),
    ],
)

Function(name='glCurrentPaletteMatrixOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixpaletteindex', type='GLuint'),
    ],
)

Function(name='glDebugMessageCallback', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='callback', type='GLDEBUGPROC'),
        Argument(name='userParam', type='const void*'),
    ],
)

Function(name='glDebugMessageCallbackAMD', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageCallback',
    args=[
        Argument(name='callback', type='GLDEBUGPROCAMD'),
        Argument(name='userParam', type='void*'),
    ],
)

Function(name='glDebugMessageCallbackARB', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageCallback',
    args=[
        Argument(name='callback', type='GLDEBUGPROCARB'),
        Argument(name='userParam', type='const void*'),
    ],
)

Function(name='glDebugMessageCallbackKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageCallback',
    args=[
        Argument(name='callback',type='GLDEBUGPROCKHR'),
        Argument(name='userParam', type='const void*'),
    ],
)

Function(name='glDebugMessageControl', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='source', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='severity', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='ids', type='const GLuint*'),
        Argument(name='enabled', type='GLboolean'),
    ],
)

Function(name='glDebugMessageControlARB', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageControl')

Function(name='glDebugMessageControlKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageControl')

Function(name='glDebugMessageEnableAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='category', type='GLenum'),
        Argument(name='severity', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='ids', type='const GLuint*'),
        Argument(name='enabled', type='GLboolean'),
    ],
)

Function(name='glDebugMessageInsert', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='source', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='severity', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='buf', type='const GLchar*'),
    ],
)

Function(name='glDebugMessageInsertAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='category', type='GLenum'),
        Argument(name='severity', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='buf', type='const GLchar*'),
    ],
)

Function(name='glDebugMessageInsertARB', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageInsert')

Function(name='glDebugMessageInsertKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageInsert')

Function(name='glDeformSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glDeformationMap3dSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='w1', type='GLdouble'),
        Argument(name='w2', type='GLdouble'),
        Argument(name='wstride', type='GLint'),
        Argument(name='worder', type='GLint'),
        Argument(name='points', type='const GLdouble*'),
    ],
)

Function(name='glDeformationMap3fSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='w1', type='GLfloat'),
        Argument(name='w2', type='GLfloat'),
        Argument(name='wstride', type='GLint'),
        Argument(name='worder', type='GLint'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

Function(name='glDeleteAsyncMarkersSGIX', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLuint'),
        Argument(name='range', type='GLsizei'),
    ],
)

Function(name='glDeleteBuffers', enabled=True, function_type=FuncType.DELETE, state_track=True, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*', wrap_type='CGLBuffer::CSUnmapArray', wrap_params='n, buffers', remove_mapping=True),
    ],
)

Function(name='glDeleteBuffersARB', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteBuffers')

Function(name='glDeleteCommandListsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='lists', type='const GLuint*'),
    ],
)

Function(name='glDeleteFencesAPPLE', enabled=False, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='const GLuint*'),
    ],
)

Function(name='glDeleteFencesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='const GLuint*'),
    ],
)

Function(name='glDeleteFragmentShaderATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glDeleteFramebuffers', enabled=True, function_type=FuncType.DELETE, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='const GLuint*', wrap_type='CGLFramebuffer::CSUnmapArray', wrap_params='n, framebuffers', remove_mapping=True),
    ],
)

Function(name='glDeleteFramebuffersEXT', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='const GLuint*', wrap_type='CGLFramebufferEXT::CSUnmapArray', wrap_params='n, framebuffers', remove_mapping=True),
    ],
)

Function(name='glDeleteFramebuffersOES', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteFramebuffers')

Function(name='glDeleteLists', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='range', type='GLsizei'),
    ],
)

Function(name='glDeleteMemoryObjectsEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='memoryObjects', type='const GLuint*'),
    ],
)

Function(name='glDeleteNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*'),
    ],
)

Function(name='glDeleteNamesAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='num', type='GLuint'),
        Argument(name='names', type='const GLuint*'),
    ],
)

Function(name='glDeleteObjectARB', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

Function(name='glDeleteOcclusionQueriesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='const GLuint*'),
    ],
)

Function(name='glDeletePathsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='range', type='GLsizei'),
    ],
)

Function(name='glDeletePerfMonitorsAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='monitors', type='GLuint*'),
    ],
)

Function(name='glDeletePerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
    ],
)

Function(name='glDeleteProgram', enabled=True, function_type=FuncType.RESOURCE, state_track=True, run_wrap=True, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glDeleteProgramPipelines', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='pipelines', type='const GLuint*', wrap_type='CGLPipeline::CSUnmapArray', wrap_params='n, pipelines', remove_mapping=True),
    ],
)

Function(name='glDeleteProgramPipelinesEXT', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteProgramPipelines')

Function(name='glDeleteProgramsARB', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='const GLuint*', wrap_type='CGLProgram::CSUnmapArray', wrap_params='n, programs'),
    ],
)

Function(name='glDeleteProgramsNV', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteProgramsARB')

Function(name='glDeleteQueries', enabled=True, function_type=FuncType.DELETE|FuncType.QUERY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='const GLuint*', wrap_type='CGLQuery::CSUnmapArray', wrap_params='n, ids', remove_mapping=True),
    ],
)

Function(name='glDeleteQueriesARB', enabled=True, function_type=FuncType.DELETE|FuncType.QUERY, inherit_from='glDeleteQueries')

Function(name='glDeleteQueriesEXT', enabled=True, function_type=FuncType.DELETE|FuncType.QUERY, inherit_from='glDeleteQueries')

Function(name='glDeleteQueryResourceTagNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='tagIds', type='const GLint*'),
    ],
)

Function(name='glDeleteRenderbuffers', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='const GLuint*', wrap_type='CGLRenderbuffer::CSUnmapArray', wrap_params='n, renderbuffers', remove_mapping=True),
    ],
)

Function(name='glDeleteRenderbuffersEXT', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True, inherit_from='glDeleteRenderbuffers',
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='const GLuint*', wrap_type='CGLRenderbufferEXT::CSUnmapArray', wrap_params='n, renderbuffers', remove_mapping=True),
    ],
)

Function(name='glDeleteRenderbuffersOES', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteRenderbuffers')

Function(name='glDeleteSamplers', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='samplers', type='const GLuint*', wrap_type='CGLSampler::CSUnmapArray', wrap_params='count, samplers', remove_mapping=True),
    ],
)

Function(name='glDeleteSemaphoresEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='semaphores', type='const GLuint*'),
    ],
)

Function(name='glDeleteShader', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glDeleteStatesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='states', type='const GLuint*'),
    ],
)

Function(name='glDeleteSync', enabled=True, function_type=FuncType.DELETE, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync', remove_mapping=True),
    ],
)

Function(name='glDeleteSyncAPPLE', enabled=False, function_type=FuncType.DELETE, inherit_from='glDeleteSync')

Function(name='glDeleteTextures', enabled=True, function_type=FuncType.DELETE, state_track=True, run_condition='ConditionCurrentContextZero()', rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*', wrap_type='CGLTexture::CSUnmapArray', wrap_params='n, textures', remove_mapping=True),
    ],
)

Function(name='glDeleteTexturesEXT', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteTextures', run_condition=False)

Function(name='glDeleteTransformFeedbacks', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='const GLuint*',wrap_type='CGLTransformFeedback::CSUnmapArray', wrap_params='n, ids', remove_mapping=True),
    ],
)

Function(name='glDeleteTransformFeedbacksNV', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteTransformFeedbacks')

Function(name='glDeleteVertexArrays', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='const GLuint*', wrap_type='CGLVertexArray::CSUnmapArray', wrap_params='n, arrays', remove_mapping=True),
    ],
)

Function(name='glDeleteVertexArraysAPPLE', enabled=False, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='const GLuint*'),
    ],
)

Function(name='glDeleteVertexArraysOES', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteVertexArrays')

Function(name='glDeleteVertexShaderEXT', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glDepthBoundsEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='zmin', type='GLclampd'),
        Argument(name='zmax', type='GLclampd'),
    ],
)

Function(name='glDepthBoundsdNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='zmin', type='GLdouble'),
        Argument(name='zmax', type='GLdouble'),
    ],
)

Function(name='glDepthFunc', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
    ],
)

Function(name='glDepthMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='flag', type='GLboolean'),
    ],
)

Function(name='glDepthRange', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLdouble'),
        Argument(name='f', type='GLdouble'),
    ],
)

Function(name='glDepthRangeArrayfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glDepthRangeArrayfvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glDepthRangeArrayv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='count, v'),
    ],
)

Function(name='glDepthRangeIndexed', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLdouble'),
        Argument(name='f', type='GLdouble'),
    ],
)

Function(name='glDepthRangeIndexedfNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

Function(name='glDepthRangeIndexedfOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

Function(name='glDepthRangedNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='zNear', type='GLdouble'),
        Argument(name='zFar', type='GLdouble'),
    ],
)

Function(name='glDepthRangef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

Function(name='glDepthRangefOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glDepthRangef',
    args=[
        Argument(name='zNear', type='GLclampf'),
        Argument(name='zFar', type='GLclampf'),
    ],
)

Function(name='glDepthRangex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLfixed'),
        Argument(name='f', type='GLfixed'),
    ],
)

Function(name='glDepthRangexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glDepthRangex')

Function(name='glDetachObjectARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='containerObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='attachedObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

Function(name='glDetachShader', enabled=True, function_type=FuncType.PARAM, run_condition='ConditionCurrentContextZero()', state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glDetailTexFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

Function(name='glDisable', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='cap', type='GLenum'),
    ],
)

Function(name='glDisableClientState', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
    ],
)

Function(name='glDisableClientStateIndexedEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glDisableClientStateiEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glDisableDriverControlQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='driverControl', type='GLuint'),
    ],
)

Function(name='glDisableIndexedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glDisableVariantClientStateEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glDisableVertexArrayAttrib', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glDisableVertexArrayAttribEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glDisableVertexArrayAttrib')

Function(name='glDisableVertexArrayEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='array', type='GLenum'),
    ],
)

Function(name='glDisableVertexAttribAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
    ],
)

Function(name='glDisableVertexAttribArray', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glDisableVertexAttribArrayARB', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glDisableVertexAttribArray')

Function(name='glDisablei', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glDisableiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glDisablei')

Function(name='glDisableiNV', enabled=False, function_type=FuncType.NONE, inherit_from='glDisablei')

Function(name='glDisableiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glDisablei')

Function(name='glDiscardFramebufferEXT', enabled=True, function_type=FuncType.PARAM, run_condition='ConditionExtension("GL_EXT_discard_framebuffer")',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*', wrap_params='numAttachments, attachments'),
    ],
)

Function(name='glDispatchCompute', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='num_groups_x', type='GLuint'),
        Argument(name='num_groups_y', type='GLuint'),
        Argument(name='num_groups_z', type='GLuint'),
    ],
)

Function(name='glDispatchComputeGroupSizeARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='num_groups_x', type='GLuint'),
        Argument(name='num_groups_y', type='GLuint'),
        Argument(name='num_groups_z', type='GLuint'),
        Argument(name='group_size_x', type='GLuint'),
        Argument(name='group_size_y', type='GLuint'),
        Argument(name='group_size_z', type='GLuint'),
    ],
)

Function(name='glDispatchComputeIndirect', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='indirect', type='GLintptr'),
    ],
)

Function(name='glDrawArrays', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, 0, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glDrawArraysEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawArrays')

Function(name='glDrawArraysIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, indirect, 1, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
    ],
)

Function(name='glDrawArraysInstanced', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

Function(name='glDrawArraysInstancedANGLE', enabled=True, function_type=FuncType.RENDER, exec_post_recorder_wrap=True, inherit_from='glDrawArraysInstanced')

Function(name='glDrawArraysInstancedARB', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawArraysInstanced')

Function(name='glDrawArraysInstancedBaseInstance', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, baseinstance)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='instancecount', type='GLsizei'),
        Argument(name='baseinstance', type='GLuint'),
    ],
)

Function(name='glDrawArraysInstancedBaseInstanceEXT', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, baseinstance)', inherit_from='glDrawArraysInstancedBaseInstance')

Function(name='glDrawArraysInstancedEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawArraysInstanced')

Function(name='glDrawArraysInstancedNV', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, 0)', inherit_from='glDrawArraysInstanced')

Function(name='glDrawBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLenum'),
    ],
)

Function(name='glDrawBuffers', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='bufs', type='const GLenum*', wrap_params='n,bufs'),
    ],
)

Function(name='glDrawBuffersARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glDrawBuffers')

Function(name='glDrawBuffersATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glDrawBuffers')

Function(name='glDrawBuffersEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glDrawBuffers')

Function(name='glDrawBuffersIndexedEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLint'),
        Argument(name='location', type='const GLenum*'),
        Argument(name='indices', type='const GLint*'),
    ],
)

Function(name='glDrawBuffersNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glDrawBuffers')

Function(name='glDrawCommandsAddressNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='primitiveMode', type='GLenum'),
        Argument(name='indirects', type='const GLuint64*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glDrawCommandsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='primitiveMode', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='indirects', type='const GLintptr*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glDrawCommandsStatesAddressNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='indirects', type='const GLuint64*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='states', type='const GLuint*'),
        Argument(name='fbos', type='const GLuint*'),
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glDrawCommandsStatesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='indirects', type='const GLintptr*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='states', type='const GLuint*'),
        Argument(name='fbos', type='const GLuint*'),
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glDrawElementArrayAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glDrawElementArrayATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glDrawElements', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, 0)', rec_condition='count>=0 && Recording(_recorder)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
    ],
)

Function(name='glDrawElementsBaseVertex', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='basevertex', type='GLint'),
    ],
)

Function(name='glDrawElementsBaseVertexEXT', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', inherit_from='glDrawElementsBaseVertex')

Function(name='glDrawElementsBaseVertexOES', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', inherit_from='glDrawElementsBaseVertex')

Function(name='glDrawElementsIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, type, indirect, 1, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
    ],
)

Function(name='glDrawElementsInstanced', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, 0, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

Function(name='glDrawElementsInstancedANGLE', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

Function(name='glDrawElementsInstancedARB', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

Function(name='glDrawElementsInstancedBaseInstance', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, 0)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='instancecount', type='GLsizei'),
        Argument(name='baseinstance', type='GLuint'),
    ],
)

Function(name='glDrawElementsInstancedBaseInstanceEXT', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, 0)', inherit_from='glDrawElementsInstancedBaseInstance')

Function(name='glDrawElementsInstancedBaseVertex', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, 0, basevertex)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='instancecount', type='GLsizei'),
        Argument(name='basevertex', type='GLint'),
    ],
)

Function(name='glDrawElementsInstancedBaseVertexBaseInstance', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, basevertex)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='instancecount', type='GLsizei'),
        Argument(name='basevertex', type='GLint'),
        Argument(name='baseinstance', type='GLuint'),
    ],
)

Function(name='glDrawElementsInstancedBaseVertexBaseInstanceEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstancedBaseVertexBaseInstance')

Function(name='glDrawElementsInstancedBaseVertexEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstancedBaseVertex')

Function(name='glDrawElementsInstancedBaseVertexOES', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstancedBaseVertex')

Function(name='glDrawElementsInstancedEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

Function(name='glDrawElementsInstancedNV', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

Function(name='glDrawMeshArraysSUN', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glDrawPixels', enabled=True, function_type=FuncType.FILL, pre_schedule='coherentBufferUpdate_PS(_recorder)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='GL_TEXTURE_2D, format, type, width, height, pixels'),
    ],
)

Function(name='glDrawRangeElementArrayAPPLE', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glDrawRangeElementArrayATI', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glDrawRangeElements', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
    ],
)

Function(name='glDrawRangeElementsBaseVertex', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='basevertex', type='GLint'),
    ],
)

Function(name='glDrawRangeElementsBaseVertexEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawRangeElementsBaseVertex')

Function(name='glDrawRangeElementsBaseVertexOES', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawRangeElementsBaseVertex')

Function(name='glDrawRangeElementsEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawRangeElements')

Function(name='glDrawTexfOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='width', type='GLfloat'),
        Argument(name='height', type='GLfloat'),
    ],
)

Function(name='glDrawTexfvOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfloat*', wrap_params='5, coords'),
    ],
)

Function(name='glDrawTexiOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='width', type='GLint'),
        Argument(name='height', type='GLint'),
    ],
)

Function(name='glDrawTexivOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLint*', wrap_params='5, coords'),
    ],
)

Function(name='glDrawTexsOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='width', type='GLshort'),
        Argument(name='height', type='GLshort'),
    ],
)

Function(name='glDrawTexsvOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLshort*', wrap_params='5, coords'),
    ],
)

Function(name='glDrawTextureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='x0', type='GLfloat'),
        Argument(name='y0', type='GLfloat'),
        Argument(name='x1', type='GLfloat'),
        Argument(name='y1', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='s0', type='GLfloat'),
        Argument(name='t0', type='GLfloat'),
        Argument(name='s1', type='GLfloat'),
        Argument(name='t1', type='GLfloat'),
    ],
)

Function(name='glDrawTexxOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
        Argument(name='width', type='GLfixed'),
        Argument(name='height', type='GLfixed'),
    ],
)

Function(name='glDrawTexxvOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*', wrap_params='5, coords'),
    ],
)

Function(name='glDrawTransformFeedback', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glDrawTransformFeedbackEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawTransformFeedback')

Function(name='glDrawTransformFeedbackInstanced', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

Function(name='glDrawTransformFeedbackInstancedEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawTransformFeedbackInstanced')

Function(name='glDrawTransformFeedbackNV', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawTransformFeedback')

Function(name='glDrawTransformFeedbackStream', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='stream', type='GLuint'),
    ],
)

Function(name='glDrawTransformFeedbackStreamInstanced', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

Function(name='glDrawVkImageNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkImage', type='GLuint64'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='x0', type='GLfloat'),
        Argument(name='y0', type='GLfloat'),
        Argument(name='x1', type='GLfloat'),
        Argument(name='y1', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='s0', type='GLfloat'),
        Argument(name='t0', type='GLfloat'),
        Argument(name='s1', type='GLfloat'),
        Argument(name='t1', type='GLfloat'),
    ],
)

Function(name='glEGLImageTargetRenderbufferStorageOES', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
    ],
)

Function(name='glEGLImageTargetTexStorageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
        Argument(name='attrib_list', type='const GLint*'),
    ],
)

Function(name='glEGLImageTargetTexture2DOES', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
    ],
)

Function(name='glEGLImageTargetTextureStorageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
        Argument(name='attrib_list', type='const GLint*'),
    ],
)

Function(name='glEdgeFlag', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='flag', type='GLboolean'),
    ],
)

Function(name='glEdgeFlagFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glEdgeFlagPointer', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glEdgeFlagPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const GLboolean*'),
    ],
)

Function(name='glEdgeFlagPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const GLboolean**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glEdgeFlagv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='flag', type='const GLboolean*'),
    ],
)

Function(name='glElementPointerAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glElementPointerATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glEnable', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='cap', type='GLenum'),
    ],
)

Function(name='glEnableClientState', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
    ],
)

Function(name='glEnableClientStateIndexedEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEnableClientStateiEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEnableDriverControlQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='driverControl', type='GLuint'),
    ],
)

Function(name='glEnableIndexedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEnableVariantClientStateEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glEnableVertexArrayAttrib', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEnableVertexArrayAttribEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glEnableVertexArrayAttrib')

Function(name='glEnableVertexArrayEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='array', type='GLenum'),
    ],
)

Function(name='glEnableVertexAttribAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
    ],
)

Function(name='glEnableVertexAttribArray', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEnableVertexAttribArrayARB', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glEnableVertexAttribArray')

Function(name='glEnablei', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEnableiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glEnablei')

Function(name='glEnableiNV', enabled=False, function_type=FuncType.NONE, inherit_from='glEnablei')

Function(name='glEnableiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glEnablei')

Function(name='glEnd', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True, suffix="""// See comment in glBegin for rationale of following line.
  globalMutex.unlock();""",
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndConditionalRender', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndConditionalRenderNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glEndConditionalRender')

Function(name='glEndConditionalRenderNVX', enabled=False, function_type=FuncType.NONE, inherit_from='glEndConditionalRender')

Function(name='glEndFragmentShaderATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndList', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndOcclusionQueryNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndPerfMonitorAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
    ],
)

Function(name='glEndPerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
    ],
)

Function(name='glEndQuery', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glEndQueryARB', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glEndQuery')

Function(name='glEndQueryEXT', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glEndQuery')

Function(name='glEndQueryIndexed', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glEndTilingQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='preserveMask', type='GLbitfield'),
    ],
)

Function(name='glEndTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndTransformFeedbackEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glEndTransformFeedback')

Function(name='glEndTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glEndTransformFeedback')

Function(name='glEndVertexShaderEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glEndVideoCaptureNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
    ],
)

Function(name='glEvalCoord1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLdouble'),
    ],
)

Function(name='glEvalCoord1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLdouble*', wrap_params='1, u'),
    ],
)

Function(name='glEvalCoord1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfloat'),
    ],
)

Function(name='glEvalCoord1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLfloat*', wrap_params='1, u'),
    ],
)

Function(name='glEvalCoord1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfixed'),
    ],
)

Function(name='glEvalCoord1xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glEvalCoord2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLdouble'),
        Argument(name='v', type='GLdouble'),
    ],
)

Function(name='glEvalCoord2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLdouble*', wrap_params='2, u'),
    ],
)

Function(name='glEvalCoord2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfloat'),
        Argument(name='v', type='GLfloat'),
    ],
)

Function(name='glEvalCoord2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLfloat*', wrap_params='2, u'),
    ],
)

Function(name='glEvalCoord2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfixed'),
        Argument(name='v', type='GLfixed'),
    ],
)

Function(name='glEvalCoord2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glEvalMapsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glEvalMesh1', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='i1', type='GLint'),
        Argument(name='i2', type='GLint'),
    ],
)

Function(name='glEvalMesh2', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='i1', type='GLint'),
        Argument(name='i2', type='GLint'),
        Argument(name='j1', type='GLint'),
        Argument(name='j2', type='GLint'),
    ],
)

Function(name='glEvalPoint1', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='i', type='GLint'),
    ],
)

Function(name='glEvalPoint2', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='i', type='GLint'),
        Argument(name='j', type='GLint'),
    ],
)

Function(name='glEvaluateDepthValuesARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glExecuteProgramNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glExtGetBufferPointervQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='params', type='void**'),
    ],
)

Function(name='glExtGetBuffersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffers', type='GLuint*'),
        Argument(name='maxBuffers', type='GLint'),
        Argument(name='numBuffers', type='GLint*'),
    ],
)

Function(name='glExtGetFramebuffersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffers', type='GLuint*'),
        Argument(name='maxFramebuffers', type='GLint'),
        Argument(name='numFramebuffers', type='GLint*'),
    ],
)

Function(name='glExtGetProgramBinarySourceQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='source', type='GLchar*'),
        Argument(name='length', type='GLint*'),
    ],
)

Function(name='glExtGetProgramsQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='programs', type='GLuint*'),
        Argument(name='maxPrograms', type='GLint'),
        Argument(name='numPrograms', type='GLint*'),
    ],
)

Function(name='glExtGetRenderbuffersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffers', type='GLuint*'),
        Argument(name='maxRenderbuffers', type='GLint'),
        Argument(name='numRenderbuffers', type='GLint*'),
    ],
)

Function(name='glExtGetShadersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shaders', type='GLuint*'),
        Argument(name='maxShaders', type='GLint'),
        Argument(name='numShaders', type='GLint*'),
    ],
)

Function(name='glExtGetTexLevelParameterivQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='face', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glExtGetTexSubImageQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='texels', type='void*'),
    ],
)

Function(name='glExtGetTexturesQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='textures', type='GLuint*'),
        Argument(name='maxTextures', type='GLint'),
        Argument(name='numTextures', type='GLint*'),
    ],
)

Function(name='glExtIsProgramBinaryQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glExtTexObjectStateOverrideiQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glExtractComponentEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='res', type='GLuint'),
        Argument(name='src', type='GLuint'),
        Argument(name='num', type='GLuint'),
    ],
)

Function(name='glFeedbackBuffer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='buffer', type='GLfloat*'),
    ],
)

Function(name='glFeedbackBufferxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='buffer', type='const GLfixed*'),
    ],
)

Function(name='glFenceSync', enabled=True, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='GLsync', wrap_type='CGLsync'),
    args=[
        Argument(name='condition', type='GLenum'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glFenceSyncAPPLE', enabled=False, function_type=FuncType.CREATE, inherit_from='glFenceSync')

Function(name='glFinalCombinerInputNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='variable', type='GLenum'),
        Argument(name='input', type='GLenum'),
        Argument(name='mapping', type='GLenum'),
        Argument(name='componentUsage', type='GLenum'),
    ],
)

Function(name='glFinish', enabled=True, function_type=FuncType.EXEC, run_wrap=True, ccode_wrap=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFinishAsyncSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='markerp', type='GLuint*'),
    ],
)

Function(name='glFinishFenceAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

Function(name='glFinishFenceNV', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

Function(name='glFinishObjectAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='object', type='GLenum'),
        Argument(name='name', type='GLint'),
    ],
)

Function(name='glFinishRenderAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFinishTextureSUNX', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFlush', enabled=True, function_type=FuncType.EXEC, run_wrap=True, ccode_wrap=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFlushMappedBufferRange', enabled=True, function_type=FuncType.EXEC, state_track=True, pre_token='CgitsFlushMappedBufferRange(target,offset,length)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

Function(name='glFlushMappedBufferRangeAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glFlushMappedBufferRangeEXT', enabled=True, function_type=FuncType.EXEC, inherit_from='glFlushMappedBufferRange', pre_token=False )

Function(name='glFlushMappedNamedBufferRange', enabled=True, function_type=FuncType.EXEC, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

Function(name='glFlushMappedNamedBufferRangeEXT', enabled=True, function_type=FuncType.EXEC, inherit_from='glFlushMappedNamedBufferRange')

Function(name='glFlushPixelDataRangeNV', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glFlushRasterSGIX', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFlushRenderAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFlushStaticDataIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glFlushVertexArrayRangeAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glFlushVertexArrayRangeNV', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFogCoordFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glFogCoordPointer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glFogCoordPointerEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordPointer')

Function(name='glFogCoordPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glFogCoordd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLdouble'),
    ],
)

Function(name='glFogCoorddEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordd')

Function(name='glFogCoorddv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='const GLdouble*', wrap_params='1, coord'),
    ],
)

Function(name='glFogCoorddvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoorddv')

Function(name='glFogCoordf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLfloat'),
    ],
)

Function(name='glFogCoordfEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordf')

Function(name='glFogCoordfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='const GLfloat*', wrap_params='1, coord'),
    ],
)

Function(name='glFogCoordfvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordfv')

Function(name='glFogCoordhNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fog', type='GLhalfNV'),
    ],
)

Function(name='glFogCoordhvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fog', type='const GLhalfNV*', wrap_params='1, fog'),
    ],
)

Function(name='glFogFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

Function(name='glFogf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glFogfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glFogi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glFogiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_params='1, params'),
    ],
)

Function(name='glFogx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glFogxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogx')

Function(name='glFogxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_params='1, params'),
    ],
)

Function(name='glFogxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogxv')

Function(name='glFragmentColorMaterialSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glFragmentCoverageColorNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLuint'),
    ],
)

Function(name='glFragmentLightModelfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glFragmentLightModelfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glFragmentLightModeliSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glFragmentLightModelivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glFragmentLightfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glFragmentLightfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glFragmentLightiSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glFragmentLightivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glFragmentMaterialfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glFragmentMaterialfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glFragmentMaterialiSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glFragmentMaterialivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glFrameTerminatorGREMEDY', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFrameZoomSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLint'),
    ],
)

Function(name='glFramebufferDrawBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glFramebufferDrawBuffersEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='n', type='GLsizei'),
        Argument(name='bufs', type='const GLenum*'),
    ],
)

Function(name='glFramebufferFetchBarrierEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFramebufferFetchBarrierQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glFramebufferFoveationConfigQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numLayers', type='GLuint'),
        Argument(name='focalPointsPerLayer', type='GLuint'),
        Argument(name='requestedFeatures', type='GLuint'),
        Argument(name='providedFeatures', type='GLuint*'),
    ],
)

Function(name='glFramebufferFoveationParametersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='layer', type='GLuint'),
        Argument(name='focalPoint', type='GLuint'),
        Argument(name='focalX', type='GLfloat'),
        Argument(name='focalY', type='GLfloat'),
        Argument(name='gainX', type='GLfloat'),
        Argument(name='gainY', type='GLfloat'),
        Argument(name='foveaArea', type='GLfloat'),
    ],
)

Function(name='glFramebufferParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glFramebufferPixelLocalStorageSizeEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLuint'),
        Argument(name='size', type='GLsizei'),
    ],
)

Function(name='glFramebufferReadBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glFramebufferRenderbuffer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='renderbuffertarget', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

Function(name='glFramebufferRenderbufferEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferRenderbuffer',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='renderbuffertarget', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

Function(name='glFramebufferRenderbufferOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferRenderbuffer')

Function(name='glFramebufferSampleLocationsfvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glFramebufferSampleLocationsfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glFramebufferSamplePositionsfvAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numsamples', type='GLuint'),
        Argument(name='pixelindex', type='GLuint'),
        Argument(name='values', type='const GLfloat*'),
    ],
)

Function(name='glFramebufferTexture', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glFramebufferTexture1D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glFramebufferTexture1DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture1D')

Function(name='glFramebufferTexture2D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glFramebufferTexture2DDownsampleIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xscale', type='GLint'),
        Argument(name='yscale', type='GLint'),
    ],
)

Function(name='glFramebufferTexture2DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture2D')

Function(name='glFramebufferTexture2DMultisampleEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='samples', type='GLsizei'),
    ],
)

Function(name='glFramebufferTexture2DMultisampleIMG', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='samples', type='GLsizei'),
    ],
)

Function(name='glFramebufferTexture2DOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTexture2D')

Function(name='glFramebufferTexture3D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='zoffset', type='GLint'),
    ],
)

Function(name='glFramebufferTexture3DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture3D')

Function(name='glFramebufferTexture3DOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTexture3D')

Function(name='glFramebufferTextureARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTexture')

Function(name='glFramebufferTextureEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture')

Function(name='glFramebufferTextureFaceARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='face', type='GLenum'),
    ],
)

Function(name='glFramebufferTextureFaceEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTextureFaceARB')

Function(name='glFramebufferTextureLayer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layer', type='GLint'),
    ],
)

Function(name='glFramebufferTextureLayerARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTextureLayer')

Function(name='glFramebufferTextureLayerDownsampleIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layer', type='GLint'),
        Argument(name='xscale', type='GLint'),
        Argument(name='yscale', type='GLint'),
    ],
)

Function(name='glFramebufferTextureLayerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTextureLayer')

Function(name='glFramebufferTextureMultisampleMultiviewOVR', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='baseViewIndex', type='GLint'),
        Argument(name='numViews', type='GLsizei'),
    ],
)

Function(name='glFramebufferTextureMultiviewOVR', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='baseViewIndex', type='GLint'),
        Argument(name='numViews', type='GLsizei'),
    ],
)

Function(name='glFramebufferTextureOES', enabled=False, function_type=FuncType.NONE, inherit_from='glFramebufferTexture')

Function(name='glFreeObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glFrontFace', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glFrustum', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='left', type='GLdouble'),
        Argument(name='right', type='GLdouble'),
        Argument(name='bottom', type='GLdouble'),
        Argument(name='top', type='GLdouble'),
        Argument(name='zNear', type='GLdouble'),
        Argument(name='zFar', type='GLdouble'),
    ],
)

Function(name='glFrustumf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='l', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

Function(name='glFrustumfOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFrustumf')

Function(name='glFrustumx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='l', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
        Argument(name='b', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='n', type='GLfixed'),
        Argument(name='f', type='GLfixed'),
    ],
)

Function(name='glFrustumxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFrustumx')

Function(name='glGenAsyncMarkersSGIX', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLsizei'),
    ],
)

Function(name='glGenBuffers', enabled=True, function_type=FuncType.GEN, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='buffers', type='GLuint*', wrap_type='CGLBuffer::CSMapArray', wrap_params='n, buffers'),
    ],
)

Function(name='glGenBuffersARB', enabled=True, function_type=FuncType.GEN, inherit_from='glGenBuffers')

Function(name='glGenFencesAPPLE', enabled=False, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='GLuint*'),
    ],
)

Function(name='glGenFencesNV', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='GLuint*', wrap_type='CGLfence::CSMapArray', wrap_params='n, fences'),
    ],
)

Function(name='glGenFragmentShadersATI', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLuint'),
    ],
)

Function(name='glGenFramebuffers', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='GLuint*', wrap_type='CGLFramebuffer::CSMapArray', wrap_params='n, framebuffers'),
    ],
)

Function(name='glGenFramebuffersEXT', enabled=True, function_type=FuncType.GEN, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='GLuint*', wrap_type='CGLFramebufferEXT::CSMapArray', wrap_params='n, framebuffers'),
    ],
)

Function(name='glGenFramebuffersOES', enabled=True, function_type=FuncType.GEN, recorder_wrap=False, inherit_from='glGenFramebuffersEXT')

Function(name='glGenLists', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLsizei'),
    ],
)

Function(name='glGenNamesAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='num', type='GLuint'),
        Argument(name='names', type='GLuint*'),
    ],
)

Function(name='glGenOcclusionQueriesNV', enabled=False, function_type=FuncType.GEN|FuncType.QUERY, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_params='n, ids'),
    ],
)

Function(name='glGenPathsNV', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLsizei'),
    ],
)

Function(name='glGenPerfMonitorsAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='monitors', type='GLuint*'),
    ],
)

Function(name='glGenProgramPipelines', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='pipelines', type='GLuint*', wrap_type='CGLPipeline::CSMapArray', wrap_params='n, pipelines'),
    ],
)

Function(name='glGenProgramPipelinesEXT', enabled=True, function_type=FuncType.GEN, inherit_from='glGenProgramPipelines')

Function(name='glGenProgramsARB', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='GLuint*', wrap_type='CGLProgram::CSMapArray', wrap_params='n, programs'),
    ],
)

Function(name='glGenProgramsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='GLuint*'),
    ],
)

Function(name='glGenQueries', enabled=True, function_type=FuncType.GEN|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLQuery::CSMapArray', wrap_params='n, ids'),
    ],
)

Function(name='glGenQueriesARB', enabled=True, function_type=FuncType.GEN|FuncType.QUERY, inherit_from='glGenQueries')

Function(name='glGenQueriesEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGenQueries')

Function(name='glGenQueryResourceTagNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='tagIds', type='GLint*'),
    ],
)

Function(name='glGenRenderbuffers', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='GLuint*', wrap_type='CGLRenderbuffer::CSMapArray', wrap_params='n, renderbuffers'),
    ],
)

Function(name='glGenRenderbuffersEXT', enabled=True, function_type=FuncType.GEN, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='GLuint*', wrap_type='CGLRenderbufferEXT::CSMapArray', wrap_params='n, renderbuffers'),
    ],
)

Function(name='glGenRenderbuffersOES', enabled=True, function_type=FuncType.GEN, recorder_wrap=False, inherit_from='glGenRenderbuffersEXT')

Function(name='glGenSamplers', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='samplers', type='GLuint*', wrap_type='CGLSampler::CSMapArray', wrap_params='count, samplers'),
    ],
)

Function(name='glGenSemaphoresEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='semaphores', type='GLuint*'),
    ],
)

Function(name='glGenSymbolsEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='datatype', type='GLenum'),
        Argument(name='storagetype', type='GLenum'),
        Argument(name='range', type='GLenum'),
        Argument(name='components', type='GLuint'),
    ],
)

Function(name='glGenTextures', enabled=True, function_type=FuncType.GEN, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='GLuint*', wrap_type='CGLTexture::CSMapArray', wrap_params='n, textures'),
    ],
)

Function(name='glGenTexturesEXT', enabled=True, function_type=FuncType.GEN, inherit_from='glGenTextures')

Function(name='glGenTransformFeedbacks', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLTransformFeedback::CSMapArray', wrap_params='n, ids'),
    ],
)

Function(name='glGenTransformFeedbacksNV', enabled=True, function_type=FuncType.GEN, inherit_from='glGenTransformFeedbacks')

Function(name='glGenVertexArrays', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='GLuint*', wrap_type='CGLVertexArray::CSMapArray', wrap_params='n, arrays'),
    ],
)

Function(name='glGenVertexArraysAPPLE', enabled=False, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='GLuint*'),
    ],
)

Function(name='glGenVertexArraysOES', enabled=True, function_type=FuncType.GEN, inherit_from='glGenVertexArrays')

Function(name='glGenVertexShadersEXT', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLuint'),
    ],
)

Function(name='glGenerateMipmap', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glGenerateMipmapEXT', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glGenerateMipmap')

Function(name='glGenerateMipmapOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glGenerateMipmap')

Function(name='glGenerateMultiTexMipmapEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glGenerateTextureMipmap', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
    ],
)

Function(name='glGenerateTextureMipmapEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glGetActiveAtomicCounterBufferiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='bufferIndex', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveAttrib', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='size', type='GLint*', wrap_type='COutArgument'),
        Argument(name='type', type='GLenum*', wrap_type='COutArgument'),
        Argument(name='name', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveAttribARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetActiveAttrib',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='size', type='GLint*', wrap_type='COutArgument'),
        Argument(name='type', type='GLenum*', wrap_type='COutArgument'),
        Argument(name='name', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveSubroutineName', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='name', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveSubroutineUniformName', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='name', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveSubroutineUniformiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='values', type='GLint*'),
    ],
)

Function(name='glGetActiveUniform', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='size', type='GLint*', wrap_type='COutArgument'),
        Argument(name='type', type='GLenum*', wrap_type='COutArgument'),
        Argument(name='name', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveUniformARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetActiveUniform',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='size', type='GLint*', wrap_type='COutArgument'),
        Argument(name='type', type='GLenum*', wrap_type='COutArgument'),
        Argument(name='name', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveUniformBlockName', enabled=True, function_type=FuncType.GET, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockIndex', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_params='1, length'),
        Argument(name='uniformBlockName', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveUniformBlockiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockIndex', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveUniformName', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformIndex', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='uniformName', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveUniformsiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformCount', type='GLsizei'),
        Argument(name='uniformIndices', type='const GLuint*', wrap_params='uniformCount, uniformIndices'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetActiveVaryingNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='size', type='GLsizei*'),
        Argument(name='type', type='GLenum*'),
        Argument(name='name', type='GLchar*'),
    ],
)

Function(name='glGetArrayObjectfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetArrayObjectivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetAttachedObjectsARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='containerObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='maxCount', type='GLsizei'),
        Argument(name='count', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='obj', type='GLhandleARB*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetAttachedShaders', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='maxCount', type='GLsizei'),
        Argument(name='count', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='shaders', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetAttribLocation', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetAttribLocationARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetAttribLocation',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLcharARB*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetBooleanIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLboolean*'),
    ],
)

Function(name='glGetBooleani_v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLboolean*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetBooleanv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLboolean*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetBufferParameteri64v', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferParameteriv',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetBufferParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetBufferParameterivARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferParameteriv')

Function(name='glGetBufferParameterui64vNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferParameteriv',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetBufferPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**', wrap_type='COutArgument'),
    ],
)

Function(name='glGetBufferPointervARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferPointerv')

Function(name='glGetBufferPointervOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferPointerv')

Function(name='glGetBufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='void*'),
    ],
)

Function(name='glGetBufferSubDataARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetBufferSubData',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptrARB'),
        Argument(name='size', type='GLsizeiptrARB'),
        Argument(name='data', type='void*'),
    ],
)

Function(name='glGetClipPlane', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetClipPlanef', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='eqn', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetClipPlanefOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='eqn', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetClipPlanex', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='eqn', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetClipPlanexOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='eqn', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetColorTable', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='table', type='void*'),
    ],
)

Function(name='glGetColorTableEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetColorTable')

Function(name='glGetColorTableParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetColorTableParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameterfv')

Function(name='glGetColorTableParameterfvSGI', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameterfv')

Function(name='glGetColorTableParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetColorTableParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameteriv')

Function(name='glGetColorTableParameterivSGI', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameteriv')

Function(name='glGetColorTableSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glGetColorTable')

Function(name='glGetCombinerInputParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetCombinerInputParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetCombinerOutputParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetCombinerOutputParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetCombinerStageParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetCommandHeaderNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='tokenID', type='GLenum'),
        Argument(name='size', type='GLuint'),
    ],
)

Function(name='glGetCompressedMultiTexImageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='lod', type='GLint'),
        Argument(name='img', type='void*'),
    ],
)

Function(name='glGetCompressedTexImage', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='img', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glGetCompressedTexImageARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetCompressedTexImage')

Function(name='glGetCompressedTextureImage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*'),
    ],
)

Function(name='glGetCompressedTextureImageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='lod', type='GLint'),
        Argument(name='img', type='GLvoid*'),
    ],
)

Function(name='glGetCompressedTextureSubImage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glGetConvolutionFilter', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='image', type='void*'),
    ],
)

Function(name='glGetConvolutionFilterEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetConvolutionFilter')

Function(name='glGetConvolutionParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetConvolutionParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetConvolutionParameterfv')

Function(name='glGetConvolutionParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetConvolutionParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetConvolutionParameteriv')

Function(name='glGetConvolutionParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

Function(name='glGetCoverageModulationTableNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='v', type='GLfloat*'),
    ],
)

Function(name='glGetDebugMessageLog', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='count', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='sources', type='GLenum*'),
        Argument(name='types', type='GLenum*'),
        Argument(name='ids', type='GLuint*'),
        Argument(name='severities', type='GLenum*'),
        Argument(name='lengths', type='GLsizei*'),
        Argument(name='messageLog', type='GLchar*'),
    ],
)

Function(name='glGetDebugMessageLogAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='count', type='GLuint'),
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='categories', type='GLenum*'),
        Argument(name='severities', type='GLuint*'),
        Argument(name='ids', type='GLuint*'),
        Argument(name='lengths', type='GLsizei*'),
        Argument(name='message', type='GLchar*'),
    ],
)

Function(name='glGetDebugMessageLogARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetDebugMessageLog')

Function(name='glGetDebugMessageLogKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetDebugMessageLog')

Function(name='glGetDetailTexFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='points', type='GLfloat*'),
    ],
)

Function(name='glGetDoubleIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLdouble*'),
    ],
)

Function(name='glGetDoublei_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLdouble*'),
    ],
)

Function(name='glGetDoublei_vEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetDoublei_v')

Function(name='glGetDoublev', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetDriverControlStringQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='driverControl', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='driverControlString', type='GLchar*'),
    ],
)

Function(name='glGetDriverControlsQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='num', type='GLint*'),
        Argument(name='size', type='GLsizei'),
        Argument(name='driverControls', type='GLuint*'),
    ],
)

Function(name='glGetError', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[],
)

Function(name='glGetFenceivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetFinalCombinerInputParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetFinalCombinerInputParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetFirstPerfQueryIdINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint*'),
    ],
)

Function(name='glGetFixedv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

Function(name='glGetFixedvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFixedv')

Function(name='glGetFloatIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLfloat*'),
    ],
)

Function(name='glGetFloati_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLfloat*'),
    ],
)

Function(name='glGetFloati_vEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFloati_v')

Function(name='glGetFloati_vNV', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFloati_v')

Function(name='glGetFloati_vOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFloati_v')

Function(name='glGetFloatv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetFogFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='points', type='GLfloat*'),
    ],
)

Function(name='glGetFragDataIndex', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetFragDataIndexEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFragDataIndex')

Function(name='glGetFragDataLocation', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetFragDataLocationEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetFragDataLocation')

Function(name='glGetFragmentLightfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetFragmentLightivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetFragmentMaterialfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetFragmentMaterialivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetFramebufferAttachmentParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetFramebufferAttachmentParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetFramebufferAttachmentParameteriv')

Function(name='glGetFramebufferAttachmentParameterivOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetFramebufferAttachmentParameteriv')

Function(name='glGetFramebufferParameterfvAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='numsamples', type='GLuint'),
        Argument(name='pixelindex', type='GLuint'),
        Argument(name='size', type='GLsizei'),
        Argument(name='values', type='GLfloat*'),
    ],
)

Function(name='glGetFramebufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetFramebufferParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetFramebufferPixelLocalStorageSizeEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLsizei'),
    args=[
        Argument(name='target', type='GLuint'),
    ],
)

Function(name='glGetGraphicsResetStatus', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[],
)

Function(name='glGetGraphicsResetStatusARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetGraphicsResetStatus')

Function(name='glGetGraphicsResetStatusEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetGraphicsResetStatus')

Function(name='glGetGraphicsResetStatusKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetGraphicsResetStatus')

Function(name='glGetHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLhandleARB'),
    args=[
        Argument(name='pname', type='GLenum'),
    ],
)

Function(name='glGetHistogram', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='reset', type='GLboolean'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='values', type='void*'),
    ],
)

Function(name='glGetHistogramEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetHistogram')

Function(name='glGetHistogramParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetHistogramParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetHistogramParameterfv')

Function(name='glGetHistogramParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetHistogramParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetHistogramParameteriv')

Function(name='glGetHistogramParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

Function(name='glGetImageHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64', wrap_type='CGLTextureHandle'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layered', type='GLboolean'),
        Argument(name='layer', type='GLint'),
        Argument(name='format', type='GLenum'),
    ],
)

Function(name='glGetImageHandleNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layered', type='GLboolean'),
        Argument(name='layer', type='GLint'),
        Argument(name='format', type='GLenum'),
    ],
)

Function(name='glGetImageTransformParameterfvHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetImageTransformParameterivHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetInfoLogARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='maxLength', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='infoLog', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetInstrumentsSGIX', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[],
)

Function(name='glGetInteger64i_v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLint64*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetInteger64v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLint64*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetInteger64vAPPLE', enabled=False, function_type=FuncType.GET, inherit_from='glGetInteger64v')

Function(name='glGetIntegerIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLint*'),
    ],
)

Function(name='glGetIntegeri_v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetIntegeri_vEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetIntegeri_v')

Function(name='glGetIntegerui64i_vNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='result', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetIntegerui64vNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLenum'),
        Argument(name='result', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetIntegerv', enabled=True, function_type=FuncType.GET, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetInternalformatSampleivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetInternalformati64v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint64*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetInternalformativ', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetInvariantBooleanvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLboolean*'),
    ],
)

Function(name='glGetInvariantFloatvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLfloat*'),
    ],
)

Function(name='glGetInvariantIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLint*'),
    ],
)

Function(name='glGetLightfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetLightiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetLightxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

Function(name='glGetLightxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetLightxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetLightxv')

Function(name='glGetListParameterfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetListParameterivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetLocalConstantBooleanvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLboolean*'),
    ],
)

Function(name='glGetLocalConstantFloatvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLfloat*'),
    ],
)

Function(name='glGetLocalConstantIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLint*'),
    ],
)

Function(name='glGetMapAttribParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetMapAttribParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMapControlPointsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='ustride', type='GLsizei'),
        Argument(name='vstride', type='GLsizei'),
        Argument(name='packed', type='GLboolean'),
        Argument(name='points', type='void*'),
    ],
)

Function(name='glGetMapParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetMapParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMapdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMapfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMapiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMapxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLfixed*'),
    ],
)

Function(name='glGetMaterialfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMaterialiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMaterialxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glGetMaterialxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMaterialxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetMaterialxv')

Function(name='glGetMemoryObjectParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memoryObject', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMinmax', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='reset', type='GLboolean'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='values', type='void*'),
    ],
)

Function(name='glGetMinmaxEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetMinmax')

Function(name='glGetMinmaxParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMinmaxParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetMinmaxParameterfv')

Function(name='glGetMinmaxParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMinmaxParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetMinmaxParameteriv')

Function(name='glGetMultiTexEnvfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetMultiTexEnvivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMultiTexGendvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glGetMultiTexGenfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetMultiTexGenivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMultiTexImageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='void*'),
    ],
)

Function(name='glGetMultiTexLevelParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetMultiTexLevelParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMultiTexParameterIivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMultiTexParameterIuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetMultiTexParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetMultiTexParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetMultisamplefv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='val', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetMultisamplefvNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetMultisamplefv')

Function(name='glGetNamedBufferParameteri64v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*'),
    ],
)

Function(name='glGetNamedBufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetNamedBufferParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedBufferParameteriv')

Function(name='glGetNamedBufferParameterui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

Function(name='glGetNamedBufferPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**', wrap_type='COutArgument'),
    ],
)

Function(name='glGetNamedBufferPointervEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetNamedBufferPointerv')

Function(name='glGetNamedBufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='void*'),
    ],
)

Function(name='glGetNamedBufferSubDataEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedBufferSubData')

Function(name='glGetNamedFramebufferAttachmentParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetNamedFramebufferAttachmentParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedFramebufferAttachmentParameteriv')

Function(name='glGetNamedFramebufferParameterfvAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='numsamples', type='GLuint'),
        Argument(name='pixelindex', type='GLuint'),
        Argument(name='size', type='GLsizei'),
        Argument(name='values', type='GLfloat*'),
    ],
)

Function(name='glGetNamedFramebufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

Function(name='glGetNamedFramebufferParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedFramebufferParameteriv')

Function(name='glGetNamedProgramLocalParameterIivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetNamedProgramLocalParameterIuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetNamedProgramLocalParameterdvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glGetNamedProgramLocalParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetNamedProgramStringEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='string', type='void*'),
    ],
)

Function(name='glGetNamedProgramivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetNamedRenderbufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetNamedRenderbufferParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedRenderbufferParameteriv',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='stringlen', type='GLint*'),
        Argument(name='string', type='GLchar*'),
    ],
)

Function(name='glGetNamedStringivARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetNextPerfQueryIdINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint'),
        Argument(name='nextQueryId', type='GLuint*'),
    ],
)

Function(name='glGetObjectBufferfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetObjectBufferivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetObjectLabel', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='label', type='GLchar*'),
    ],
)

Function(name='glGetObjectLabelEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetObjectLabel')

Function(name='glGetObjectLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetObjectLabel')

Function(name='glGetObjectParameterfvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetObjectParameterivAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='objectType', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetObjectParameterivARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetObjectPtrLabel', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='ptr', type='const void*'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='label', type='GLchar*'),
    ],
)

Function(name='glGetObjectPtrLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetObjectPtrLabel')

Function(name='glGetOcclusionQueryivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetOcclusionQueryuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetPathColorGenfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat*'),
    ],
)

Function(name='glGetPathColorGenivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint*'),
    ],
)

Function(name='glGetPathCommandsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='commands', type='GLubyte*'),
    ],
)

Function(name='glGetPathCoordsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coords', type='GLfloat*'),
    ],
)

Function(name='glGetPathDashArrayNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='dashArray', type='GLfloat*'),
    ],
)

Function(name='glGetPathLengthNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLfloat'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='startSegment', type='GLsizei'),
        Argument(name='numSegments', type='GLsizei'),
    ],
)

Function(name='glGetPathMetricRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='metricQueryMask', type='GLbitfield'),
        Argument(name='firstPathName', type='GLuint'),
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='metrics', type='GLfloat*'),
    ],
)

Function(name='glGetPathMetricsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='metricQueryMask', type='GLbitfield'),
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='metrics', type='GLfloat*'),
    ],
)

Function(name='glGetPathParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat*'),
    ],
)

Function(name='glGetPathParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint*'),
    ],
)

Function(name='glGetPathSpacingNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pathListMode', type='GLenum'),
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='advanceScale', type='GLfloat'),
        Argument(name='kerningScale', type='GLfloat'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='returnedSpacing', type='GLfloat*'),
    ],
)

Function(name='glGetPathTexGenfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texCoordSet', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat*'),
    ],
)

Function(name='glGetPathTexGenivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texCoordSet', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint*'),
    ],
)

Function(name='glGetPerfCounterInfoINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint'),
        Argument(name='counterId', type='GLuint'),
        Argument(name='counterNameLength', type='GLuint'),
        Argument(name='counterName', type='GLchar*'),
        Argument(name='counterDescLength', type='GLuint'),
        Argument(name='counterDesc', type='GLchar*'),
        Argument(name='counterOffset', type='GLuint*'),
        Argument(name='counterDataSize', type='GLuint*'),
        Argument(name='counterTypeEnum', type='GLuint*'),
        Argument(name='counterDataTypeEnum', type='GLuint*'),
        Argument(name='rawCounterMaxValue', type='GLuint64*'),
    ],
)

Function(name='glGetPerfMonitorCounterDataAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='dataSize', type='GLsizei'),
        Argument(name='data', type='GLuint*'),
        Argument(name='bytesWritten', type='GLint*'),
    ],
)

Function(name='glGetPerfMonitorCounterInfoAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='counter', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='void*'),
    ],
)

Function(name='glGetPerfMonitorCounterStringAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='counter', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='counterString', type='GLchar*'),
    ],
)

Function(name='glGetPerfMonitorCountersAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='numCounters', type='GLint*'),
        Argument(name='maxActiveCounters', type='GLint*'),
        Argument(name='counterSize', type='GLsizei'),
        Argument(name='counters', type='GLuint*'),
    ],
)

Function(name='glGetPerfMonitorGroupStringAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='groupString', type='GLchar*'),
    ],
)

Function(name='glGetPerfMonitorGroupsAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numGroups', type='GLint*'),
        Argument(name='groupsSize', type='GLsizei'),
        Argument(name='groups', type='GLuint*'),
    ],
)

Function(name='glGetPerfQueryDataINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
        Argument(name='flags', type='GLuint'),
        Argument(name='dataSize', type='GLsizei'),
        Argument(name='data', type='void*'),
        Argument(name='bytesWritten', type='GLuint*'),
    ],
)

Function(name='glGetPerfQueryIdByNameINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryName', type='GLchar*'),
        Argument(name='queryId', type='GLuint*'),
    ],
)

Function(name='glGetPerfQueryInfoINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint'),
        Argument(name='queryNameLength', type='GLuint'),
        Argument(name='queryName', type='GLchar*'),
        Argument(name='dataSize', type='GLuint*'),
        Argument(name='noCounters', type='GLuint*'),
        Argument(name='noInstances', type='GLuint*'),
        Argument(name='capsMask', type='GLuint*'),
    ],
)

Function(name='glGetPixelMapfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='values', type='GLfloat*'),
    ],
)

Function(name='glGetPixelMapuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='values', type='GLuint*'),
    ],
)

Function(name='glGetPixelMapusv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='values', type='GLushort*'),
    ],
)

Function(name='glGetPixelMapxv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='values', type='GLfixed*'),
    ],
)

Function(name='glGetPixelTexGenParameterfvSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetPixelTexGenParameterivSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetPixelTransformParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetPixelTransformParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetPointerIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='void**'),
    ],
)

Function(name='glGetPointeri_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='void**'),
    ],
)

Function(name='glGetPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**', wrap_type='COutArgument'),
    ],
)

Function(name='glGetPointervEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetPointerv')

Function(name='glGetPointervKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetPointerv')

Function(name='glGetPolygonStipple', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLubyte*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramBinary', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='binaryFormat', type='GLenum*'),
        Argument(name='binary', type='void*'),
    ],
)

Function(name='glGetProgramBinaryOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramBinary')

Function(name='glGetProgramEnvParameterIivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetProgramEnvParameterIuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetProgramEnvParameterdvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramEnvParameterfvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramInfoLog', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='infoLog', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramInterfaceiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetProgramLocalParameterIivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetProgramLocalParameterIuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetProgramLocalParameterdvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramLocalParameterfvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramNamedParameterdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glGetProgramNamedParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetProgramParameterdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glGetProgramParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetProgramPipelineInfoLog', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='infoLog', type='GLchar*'),
    ],
)

Function(name='glGetProgramPipelineInfoLogEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramPipelineInfoLog')

Function(name='glGetProgramPipelineiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetProgramPipelineivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramPipelineiv')

Function(name='glGetProgramResourceIndex', enabled=True, function_type=FuncType.GET, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetProgramResourceLocation', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True,
    return_value=ReturnValue(type='GLint', wrap_type='CRecUniformLocation', wrap_params='programInterface == GL_UNIFORM ? return_value : -1, program, name'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetProgramResourceLocationIndex', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetProgramResourceLocationIndexEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramResourceLocationIndex')

Function(name='glGetProgramResourceName', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='name', type='GLchar*'),
    ],
)

Function(name='glGetProgramResourcefvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='propCount', type='GLsizei'),
        Argument(name='props', type='const GLenum*'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetProgramResourceiv', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True, ccode_write_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='index', type='GLuint', wrap_type='CGLResourceIndex', wrap_params='program, programInterface, index'),
        Argument(name='propCount', type='GLsizei'),
        Argument(name='props', type='const GLenum*', wrap_params='propCount, props'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_params='1, length'), # TODO: This is an output sizei, not an input array.
        Argument(name='params', type='GLint*', wrap_type='CGLProgramResourceivHelper', wrap_params='program, programInterface, index, propCount, props, bufSize, params'),
    ],
)

Function(name='glGetProgramStageiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='values', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='string', type='void*'),
    ],
)

Function(name='glGetProgramStringNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='program', type='GLubyte*'),
    ],
)

Function(name='glGetProgramSubroutineParameteruivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='param', type='GLuint*'),
    ],
)

Function(name='glGetProgramiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramivARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetProgramivNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryBufferObjecti64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glGetQueryBufferObjectiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glGetQueryBufferObjectui64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glGetQueryBufferObjectuiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glGetQueryIndexediv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjecti64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjecti64vEXT', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjecti64v',
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64EXT*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjecti64vINTEL', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjecti64v')

Function(name='glGetQueryObjectiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjectivARB', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectiv')

Function(name='glGetQueryObjectivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetQueryObjectiv')

Function(name='glGetQueryObjectui64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjectui64vEXT', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjectui64vINTEL', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectui64v')

Function(name='glGetQueryObjectuiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryObjectuivARB', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectuiv')

Function(name='glGetQueryObjectuivEXT', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectuiv')

Function(name='glGetQueryiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetQueryivARB', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryiv')

Function(name='glGetQueryivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetQueryiv')

Function(name='glGetRenderbufferParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetRenderbufferParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetRenderbufferParameteriv')

Function(name='glGetRenderbufferParameterivOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetRenderbufferParameteriv')

Function(name='glGetSamplerParameterIiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetSamplerParameterIivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIiv')

Function(name='glGetSamplerParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIiv')

Function(name='glGetSamplerParameterIuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetSamplerParameterIuivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIuiv')

Function(name='glGetSamplerParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIuiv')

Function(name='glGetSamplerParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetSamplerParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetSemaphoreParameterui64vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64*'),
    ],
)

Function(name='glGetSeparableFilter', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='row', type='void*'),
        Argument(name='column', type='void*'),
        Argument(name='span', type='void*'),
    ],
)

Function(name='glGetSeparableFilterEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSeparableFilter')

Function(name='glGetShaderInfoLog', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='infoLog', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetShaderPrecisionFormat', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shadertype', type='GLenum'),
        Argument(name='precisiontype', type='GLenum'),
        Argument(name='range', type='GLint*', wrap_type='COutArgument'),
        Argument(name='precision', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetShaderSource', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='source', type='GLchar*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetShaderSourceARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='maxLength', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='source', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetShaderiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetSharpenTexFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='points', type='GLfloat*'),
    ],
)

Function(name='glGetStageIndexNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLushort'),
    args=[
        Argument(name='shadertype', type='GLenum'),
    ],
)

Function(name='glGetString', enabled=True, function_type=FuncType.GET, interceptor_exec_override=True,
    return_value=ReturnValue(type='const GLubyte*', wrap_type='CGLconstuchar_ptr'),
    args=[
        Argument(name='name', type='GLenum'),
    ],
)

Function(name='glGetStringi', enabled=True, function_type=FuncType.GET, interceptor_exec_override=True,
    return_value=ReturnValue(type='const GLubyte*', wrap_type='CGLconstuchar_ptr'),
    args=[
        Argument(name='name', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glGetSubroutineIndex', enabled=True, function_type=FuncType.GET, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetSubroutineUniformLocation', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetSynciv', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sync', type='GLsync'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='values', type='GLint*', wrap_params='bufSize/sizeof(GLint), values'),
    ],
)

Function(name='glGetSyncivAPPLE', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSynciv')

Function(name='glGetTexBumpParameterfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat*'),
    ],
)

Function(name='glGetTexBumpParameterivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

Function(name='glGetTexEnvfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexEnviv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexEnvxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexEnvxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexEnvxv')

Function(name='glGetTexFilterFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='filter', type='GLenum'),
        Argument(name='weights', type='GLfloat*'),
    ],
)

Function(name='glGetTexGendv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexGenfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexGenfvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexGenfv')

Function(name='glGetTexGeniv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexGenivOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexGeniv')

Function(name='glGetTexGenxvOES', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexImage', enabled=True, function_type=FuncType.GET, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glGetTexLevelParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexLevelParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexLevelParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

Function(name='glGetTexParameterIiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexParameterIivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexParameterIiv')

Function(name='glGetTexParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetTexParameterIiv')

Function(name='glGetTexParameterIuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexParameterIuivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexParameterIuiv')

Function(name='glGetTexParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetTexParameterIuiv')

Function(name='glGetTexParameterPointervAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**'),
    ],
)

Function(name='glGetTexParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexParameterxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetTexParameterxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexParameterxv')

Function(name='glGetTextureHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64', wrap_type='CGLTextureHandle'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glGetTextureHandleIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glGetTextureHandleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glGetTextureImage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*'),
    ],
)

Function(name='glGetTextureImageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='GLvoid*'),
    ],
)

Function(name='glGetTextureLevelParameterfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetTextureLevelParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetTextureLevelParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTextureLevelParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTextureParameterIiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTextureParameterIivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTextureParameterIuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetTextureParameterIuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetTextureParameterfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetTextureParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetTextureParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTextureParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTextureSamplerHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64', wrap_type='CGLTextureHandle'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

Function(name='glGetTextureSamplerHandleIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

Function(name='glGetTextureSamplerHandleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

Function(name='glGetTextureSubImage', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glGetTrackMatrixivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='address', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetTransformFeedbackVarying', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='size', type='GLsizei*'),
        Argument(name='type', type='GLenum*'),
        Argument(name='name', type='GLchar*'),
    ],
)

Function(name='glGetTransformFeedbackVaryingEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetTransformFeedbackVarying')

Function(name='glGetTransformFeedbackVaryingNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='location', type='GLint*'),
    ],
)

Function(name='glGetTransformFeedbacki64_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='param', type='GLint64*'),
    ],
)

Function(name='glGetTransformFeedbacki_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='param', type='GLint*'),
    ],
)

Function(name='glGetTransformFeedbackiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

Function(name='glGetTranslatedShaderSourceANGLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='source', type='GLchar*'),
    ],
)

Function(name='glGetUniformBlockIndex', enabled=True, function_type=FuncType.GET, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockName', type='const GLchar*', wrap_params='uniformBlockName, \'\\0\', 1'),
    ],
)

Function(name='glGetUniformBufferSizeEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
    ],
)

Function(name='glGetUniformIndices', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformCount', type='GLsizei'),
        Argument(name='uniformNames', type='const GLchar*const*', wrap_type='CStringArray', wrap_params='uniformNames, uniformCount'),
        Argument(name='uniformIndices', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformLocation', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='GLint', wrap_type='CRecUniformLocation', wrap_params='return_value, program, name'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetUniformLocationARB', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='GLint', wrap_type='CRecUniformLocation', wrap_params='return_value, programObj, name'),
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLcharARB*', wrap_type='CGLchar::CSArray', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetUniformOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLintptr'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
    ],
)

Function(name='glGetUniformSubroutineuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shadertype', type='GLenum'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetUniformdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformfvARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetUniformfv',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='programObj, location'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformi64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLint64*'),
    ],
)

Function(name='glGetUniformi64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLint64EXT*'),
    ],
)

Function(name='glGetUniformiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformivARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetUniformiv',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='programObj, location'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLuint64*'),
    ],
)

Function(name='glGetUniformui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

Function(name='glGetUniformuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetUniformuivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetUniformuiv')

Function(name='glGetUnsignedBytei_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLubyte*'),
    ],
)

Function(name='glGetUnsignedBytevEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLubyte*'),
    ],
)

Function(name='glGetVariantArrayObjectfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetVariantArrayObjectivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetVariantBooleanvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLboolean*'),
    ],
)

Function(name='glGetVariantFloatvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLfloat*'),
    ],
)

Function(name='glGetVariantIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLint*'),
    ],
)

Function(name='glGetVariantPointervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='void**'),
    ],
)

Function(name='glGetVaryingLocationNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetVertexArrayIndexed64iv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint64*', wrap_type='CGLint64_ptr'),
    ],
)

Function(name='glGetVertexArrayIndexediv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexArrayIntegeri_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

Function(name='glGetVertexArrayIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

Function(name='glGetVertexArrayPointeri_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='void**'),
    ],
)

Function(name='glGetVertexArrayPointervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='void**'),
    ],
)

Function(name='glGetVertexArrayiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribArrayObjectfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetVertexAttribArrayObjectivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetVertexAttribIiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribIivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribIiv')

Function(name='glGetVertexAttribIuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribIuivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribIuiv')

Function(name='glGetVertexAttribLdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribLdvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribLdv')

Function(name='glGetVertexAttribLi64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64EXT*'),
    ],
)

Function(name='glGetVertexAttribLui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

Function(name='glGetVertexAttribLui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

Function(name='glGetVertexAttribPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='pointer', type='void**', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribPointervARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribPointerv')

Function(name='glGetVertexAttribPointervNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribPointerv')

Function(name='glGetVertexAttribdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribdvARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribdv')

Function(name='glGetVertexAttribdvNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribdv')

Function(name='glGetVertexAttribfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribfvARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribfv')

Function(name='glGetVertexAttribfvNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribfv')

Function(name='glGetVertexAttribiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

Function(name='glGetVertexAttribivARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribiv')

Function(name='glGetVertexAttribivNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribiv')

Function(name='glGetVideoCaptureStreamdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glGetVideoCaptureStreamfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetVideoCaptureStreamivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetVideoCaptureivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetVideoi64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64EXT*'),
    ],
)

Function(name='glGetVideoivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetVideoui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

Function(name='glGetVideouivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetVkProcAddrNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLVULKANPROCNV'),
    args=[
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glGetnColorTable', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='table', type='void*'),
    ],
)

Function(name='glGetnColorTableARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnColorTable')

Function(name='glGetnCompressedTexImage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='lod', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glGetnCompressedTexImageARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnCompressedTexImage')

Function(name='glGetnConvolutionFilter', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='image', type='void*'),
    ],
)

Function(name='glGetnConvolutionFilterARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnConvolutionFilter')

Function(name='glGetnHistogram', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='reset', type='GLboolean'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='void*'),
    ],
)

Function(name='glGetnHistogramARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnHistogram')

Function(name='glGetnMapdv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='v', type='GLdouble*'),
    ],
)

Function(name='glGetnMapdvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMapdv')

Function(name='glGetnMapfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='v', type='GLfloat*'),
    ],
)

Function(name='glGetnMapfvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMapfv')

Function(name='glGetnMapiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='v', type='GLint*'),
    ],
)

Function(name='glGetnMapivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMapiv')

Function(name='glGetnMinmax', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='reset', type='GLboolean'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='void*'),
    ],
)

Function(name='glGetnMinmaxARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMinmax')

Function(name='glGetnPixelMapfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='GLfloat*'),
    ],
)

Function(name='glGetnPixelMapfvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPixelMapfv')

Function(name='glGetnPixelMapuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='GLuint*'),
    ],
)

Function(name='glGetnPixelMapuivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPixelMapuiv')

Function(name='glGetnPixelMapusv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='GLushort*'),
    ],
)

Function(name='glGetnPixelMapusvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPixelMapusv')

Function(name='glGetnPolygonStipple', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pattern', type='GLubyte*'),
    ],
)

Function(name='glGetnPolygonStippleARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPolygonStipple')

Function(name='glGetnSeparableFilter', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='rowBufSize', type='GLsizei'),
        Argument(name='row', type='void*'),
        Argument(name='columnBufSize', type='GLsizei'),
        Argument(name='column', type='void*'),
        Argument(name='span', type='void*'),
    ],
)

Function(name='glGetnSeparableFilterARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnSeparableFilter')

Function(name='glGetnTexImage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glGetnTexImageARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnTexImage')

Function(name='glGetnUniformdv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLdouble*'),
    ],
)

Function(name='glGetnUniformdvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformdv')

Function(name='glGetnUniformfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLfloat*'),
    ],
)

Function(name='glGetnUniformfvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformfv')

Function(name='glGetnUniformfvEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformfv')

Function(name='glGetnUniformfvKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformfv')

Function(name='glGetnUniformi64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint64*'),
    ],
)

Function(name='glGetnUniformiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glGetnUniformivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformiv')

Function(name='glGetnUniformivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformiv')

Function(name='glGetnUniformivKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformiv')

Function(name='glGetnUniformui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLuint64*'),
    ],
)

Function(name='glGetnUniformuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glGetnUniformuivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformuiv')

Function(name='glGetnUniformuivKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformuiv')

Function(name='glGlobalAlphaFactorbSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLbyte'),
    ],
)

Function(name='glGlobalAlphaFactordSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLdouble'),
    ],
)

Function(name='glGlobalAlphaFactorfSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
    ],
)

Function(name='glGlobalAlphaFactoriSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLint'),
    ],
)

Function(name='glGlobalAlphaFactorsSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLshort'),
    ],
)

Function(name='glGlobalAlphaFactorubSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLubyte'),
    ],
)

Function(name='glGlobalAlphaFactoruiSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLuint'),
    ],
)

Function(name='glGlobalAlphaFactorusSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLushort'),
    ],
)

Function(name='glHint', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glHintPGI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='mode', type='GLint'),
    ],
)

Function(name='glHistogram', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='sink', type='GLboolean'),
    ],
)

Function(name='glHistogramEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glHistogram')

Function(name='glIglooInterfaceSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const void*'),
    ],
)

Function(name='glImageTransformParameterfHP', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glImageTransformParameterfvHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glImageTransformParameteriHP', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glImageTransformParameterivHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glImportMemoryFdEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='size', type='GLuint64'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='fd', type='GLint'),
    ],
)

Function(name='glImportMemoryWin32HandleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='size', type='GLuint64'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='handle', type='void*'),
    ],
)

Function(name='glImportMemoryWin32NameEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='size', type='GLuint64'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='name', type='const void*'),
    ],
)

Function(name='glImportSemaphoreFdEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='fd', type='GLint'),
    ],
)

Function(name='glImportSemaphoreWin32HandleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='handle', type='void*'),
    ],
)

Function(name='glImportSemaphoreWin32NameEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='name', type='const void*'),
    ],
)

Function(name='glImportSyncEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLsync'),
    args=[
        Argument(name='external_sync_type', type='GLenum'),
        Argument(name='external_sync', type='GLintptr'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glIndexFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glIndexFuncEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLclampf'),
    ],
)

Function(name='glIndexMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glIndexMaterialEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glIndexPointer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glIndexPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glIndexPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glIndexd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLdouble'),
    ],
)

Function(name='glIndexdv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLdouble*'),
    ],
)

Function(name='glIndexf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLfloat'),
    ],
)

Function(name='glIndexfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLfloat*'),
    ],
)

Function(name='glIndexi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLint'),
    ],
)

Function(name='glIndexiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLint*'),
    ],
)

Function(name='glIndexs', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLshort'),
    ],
)

Function(name='glIndexsv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLshort*'),
    ],
)

Function(name='glIndexub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLubyte'),
    ],
)

Function(name='glIndexubv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLubyte*'),
    ],
)

Function(name='glIndexxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='component', type='GLfixed'),
    ],
)

Function(name='glIndexxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='component', type='const GLfixed*'),
    ],
)

Function(name='glInitNames', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glInsertComponentEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='res', type='GLuint'),
        Argument(name='src', type='GLuint'),
        Argument(name='num', type='GLuint'),
    ],
)

Function(name='glInsertEventMarkerEXT', enabled=False, function_type=FuncType.NONE, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='marker', type='const GLchar*'),
    ],
)

Function(name='glInstrumentsBufferSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='buffer', type='GLint*'),
    ],
)

Function(name='glInterleavedArrays', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='format', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glInterpolatePathsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='pathA', type='GLuint'),
        Argument(name='pathB', type='GLuint'),
        Argument(name='weight', type='GLfloat'),
    ],
)

Function(name='glInvalidateBufferData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glInvalidateBufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

Function(name='glInvalidateFramebuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*', wrap_params='numAttachments, attachments'),
    ],
)

Function(name='glInvalidateNamedFramebufferData', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*', wrap_params='numAttachments, attachments'),
    ],
)

Function(name='glInvalidateNamedFramebufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glInvalidateSubFramebuffer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glInvalidateTexImage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glInvalidateTexSubImage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
    ],
)

Function(name='glIsAsyncMarkerSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='marker', type='GLuint'),
    ],
)

Function(name='glIsBuffer', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glIsBufferARB', enabled=True, function_type=FuncType.GET, inherit_from='glIsBuffer')

Function(name='glIsBufferResidentNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glIsCommandListNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

Function(name='glIsEnabled', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='cap', type='GLenum'),
    ],
)

Function(name='glIsEnabledIndexedEXT', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glIsEnabledi', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glIsEnablediEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsEnabledi')

Function(name='glIsEnablediNV', enabled=False, function_type=FuncType.NONE, inherit_from='glIsEnabledi')

Function(name='glIsEnablediOES', enabled=False, function_type=FuncType.NONE, inherit_from='glIsEnabledi')

Function(name='glIsFenceAPPLE', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

# FIXME: Argument `fence` had a `wrapParam='CGLfence'`, which was a typo of wrapType.
# FIXME: Should we add that wrap_type or not?
# Adding `wrapParam='CGLfence'` causes CglIsFenceNV in glFunctions.h to contain CGLfence instead of CGLuint.
Function(name='glIsFenceNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)


Function(name='glIsFramebuffer', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
    ],
)

Function(name='glIsFramebufferEXT', enabled=True, function_type=FuncType.GET, recorder_wrap=True,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
    ],
)

Function(name='glIsFramebufferOES', enabled=True, function_type=FuncType.GET, inherit_from='glIsFramebuffer')

Function(name='glIsImageHandleResidentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glIsImageHandleResidentNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glIsList', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

Function(name='glIsMemoryObjectEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='memoryObject', type='GLuint'),
    ],
)

Function(name='glIsNameAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='name', type='GLuint'),
    ],
)

Function(name='glIsNamedBufferResidentNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glIsNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

Function(name='glIsObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glIsOcclusionQueryNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

Function(name='glIsPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
    ],
)

Function(name='glIsPointInFillPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='mask', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glIsPointInStrokePathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glIsProgram', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glIsProgramARB', enabled=True, function_type=FuncType.GET, inherit_from='glIsProgram')

Function(name='glIsProgramNV', enabled=True, function_type=FuncType.GET, inherit_from='glIsProgram')

Function(name='glIsProgramPipeline', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
    ],
)

Function(name='glIsProgramPipelineEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsProgramPipeline')

Function(name='glIsQuery', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
    ],
)

Function(name='glIsQueryARB', enabled=True, function_type=FuncType.GET, inherit_from='glIsQuery')

Function(name='glIsQueryEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsQuery')

Function(name='glIsRenderbuffer', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

Function(name='glIsRenderbufferEXT', enabled=True, function_type=FuncType.GET, recorder_wrap=True, inherit_from='glIsRenderbuffer',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

Function(name='glIsRenderbufferOES', enabled=True, function_type=FuncType.GET, inherit_from='glIsRenderbuffer')

Function(name='glIsSampler', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

Function(name='glIsSemaphoreEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='semaphore', type='GLuint'),
    ],
)

Function(name='glIsShader', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glIsStateNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='state', type='GLuint'),
    ],
)

Function(name='glIsSync', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync'),
    ],
)

Function(name='glIsSyncAPPLE', enabled=False, function_type=FuncType.GET, inherit_from='glIsSync')

Function(name='glIsTexture', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glIsTextureEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsTexture', run_wrap=False, recorder_wrap=False)

Function(name='glIsTextureHandleResidentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glIsTextureHandleResidentNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glIsTransformFeedback', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLTransformFeedback'),
    ],
)

Function(name='glIsTransformFeedbackNV', enabled=True, function_type=FuncType.GET, inherit_from='glIsTransformFeedback')

Function(name='glIsVariantEnabledEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='cap', type='GLenum'),
    ],
)

Function(name='glIsVertexArray', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='array', type='GLuint', wrap_type='CGLVertexArray'),
    ],
)

Function(name='glIsVertexArrayAPPLE', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='array', type='GLuint'),
    ],
)

Function(name='glIsVertexArrayOES', enabled=True, function_type=FuncType.GET, inherit_from='glIsVertexArray')

Function(name='glIsVertexAttribEnabledAPPLE', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
    ],
)

Function(name='glLGPUCopyImageSubDataNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sourceGpu', type='GLuint'),
        Argument(name='destinationGpuMask', type='GLbitfield'),
        Argument(name='srcName', type='GLuint'),
        Argument(name='srcTarget', type='GLenum'),
        Argument(name='srcLevel', type='GLint'),
        Argument(name='srcX', type='GLint'),
        Argument(name='srxY', type='GLint'),
        Argument(name='srcZ', type='GLint'),
        Argument(name='dstName', type='GLuint'),
        Argument(name='dstTarget', type='GLenum'),
        Argument(name='dstLevel', type='GLint'),
        Argument(name='dstX', type='GLint'),
        Argument(name='dstY', type='GLint'),
        Argument(name='dstZ', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
    ],
)

Function(name='glLGPUInterlockNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glLGPUNamedBufferSubDataNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpuMask', type='GLbitfield'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource'),
    ],
)

Function(name='glLabelObjectEXT', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='object', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='label', type='const GLchar*', wrap_params='length, label'),
    ],
)

Function(name='glLabelObjectWithResponsibleProcessAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='pid', type='GLint'),
    ],
)

Function(name='glLightEnviSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glLightModelf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glLightModelfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glLightModeli', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glLightModeliv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname,params'),
    ],
)

Function(name='glLightModelx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glLightModelxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightModelx')

Function(name='glLightModelxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname,params'),
    ],
)

Function(name='glLightModelxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightModelxv')

Function(name='glLightf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glLightfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glLighti', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glLightiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glLightx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glLightxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightx')

Function(name='glLightxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glLightxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightxv')

Function(name='glLineStipple', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLint'),
        Argument(name='pattern', type='GLushort'),
    ],
)

Function(name='glLineWidth', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLfloat'),
    ],
)

Function(name='glLineWidthx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLfixed'),
    ],
)

Function(name='glLineWidthxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLineWidthx')

Function(name='glLinkProgram', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glLinkProgramARB', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, run_wrap=True, state_track=True, inherit_from='glLinkProgram')

Function(name='glListBase', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='base', type='GLuint'),
    ],
)

Function(name='glListDrawCommandsStatesClientNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='segment', type='GLuint'),
        Argument(name='indirects', type='const void**'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='states', type='const GLuint*'),
        Argument(name='fbos', type='const GLuint*'),
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glListParameterfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glListParameterfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glListParameteriSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glListParameterivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glLoadIdentity', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glLoadIdentityDeformationMapSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glLoadMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

Function(name='glLoadMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

Function(name='glLoadMatrixx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*', wrap_params='16, m'),
    ],
)

Function(name='glLoadMatrixxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLoadMatrixx')

Function(name='glLoadName', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='name', type='GLuint'),
    ],
)

Function(name='glLoadPaletteFromModelViewMatrixOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glLoadProgramNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='program', type='const GLubyte*'),
    ],
)

Function(name='glLoadTransposeMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

Function(name='glLoadTransposeMatrixdARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glLoadTransposeMatrixd')

Function(name='glLoadTransposeMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

Function(name='glLoadTransposeMatrixfARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glLoadTransposeMatrixf')

Function(name='glLoadTransposeMatrixxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*'),
    ],
)

Function(name='glLockArraysEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glLogicOp', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='opcode', type='GLenum'),
    ],
)

Function(name='glMakeBufferNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glMakeBufferResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glMakeImageHandleNonResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

Function(name='glMakeImageHandleNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glMakeImageHandleResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glMakeImageHandleResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glMakeNamedBufferNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glMakeNamedBufferResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glMakeTextureHandleNonResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

Function(name='glMakeTextureHandleNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glMakeTextureHandleResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

Function(name='glMakeTextureHandleResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

Function(name='glMap1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
        Argument(name='stride', type='GLint'),
        Argument(name='order', type='GLint'),
        Argument(name='points', type='const GLdouble*', wrap_type='CGLdouble::CGLMapArray', wrap_params='target, order, points'),
    ],
)

Function(name='glMap1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
        Argument(name='stride', type='GLint'),
        Argument(name='order', type='GLint'),
        Argument(name='points', type='const GLfloat*', wrap_type='CGLfloat::CGLMapArray', wrap_params='target, order, points'),
    ],
)

Function(name='glMap1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLfixed'),
        Argument(name='u2', type='GLfixed'),
        Argument(name='stride', type='GLint'),
        Argument(name='order', type='GLint'),
        Argument(name='points', type='GLfixed'),
    ],
)

Function(name='glMap2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='points', type='const GLdouble*', wrap_type='CGLdouble::CGLMapArray', wrap_params='target, uorder, vorder, points'),
    ],
)

Function(name='glMap2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='points', type='const GLfloat*', wrap_type='CGLfloat::CGLMapArray', wrap_params='target, uorder, vorder, points'),
    ],
)

Function(name='glMap2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='u1', type='GLfixed'),
        Argument(name='u2', type='GLfixed'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLfixed'),
        Argument(name='v2', type='GLfixed'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='points', type='GLfixed'),
    ],
)

Function(name='glMapBuffer', enabled=True, function_type=FuncType.MAP, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void*', wrap_type='CGLvoid_ptr'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glMapBufferARB', enabled=True, function_type=FuncType.MAP, inherit_from='glMapBuffer', state_track=True)

Function(name='glMapBufferOES', enabled=True, function_type=FuncType.MAP, inherit_from='glMapBuffer', state_track=True)

Function(name='glMapBufferRange', enabled=True, function_type=FuncType.MAP, state_track=True, rec_condition='ConditionBufferES(_recorder)', interceptor_exec_override=True,
    return_value=ReturnValue(type='void*', wrap_type='CGLvoid_ptr'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
        Argument(name='access', type='GLbitfield', wrap_type='CMapAccess'),
    ],
)

Function(name='glMapBufferRangeEXT', enabled=True, function_type=FuncType.MAP, interceptor_exec_override=True, inherit_from='glMapBufferRange')

Function(name='glMapControlPointsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='ustride', type='GLsizei'),
        Argument(name='vstride', type='GLsizei'),
        Argument(name='uorder', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='packed', type='GLboolean'),
        Argument(name='points', type='const void*'),
    ],
)

Function(name='glMapGrid1d', enabled=True, function_type=FuncType.MAP,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='un', type='GLint'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
    ],
)

Function(name='glMapGrid1f', enabled=True, function_type=FuncType.MAP,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='un', type='GLint'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
    ],
)

Function(name='glMapGrid1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLint'),
        Argument(name='u1', type='GLfixed'),
        Argument(name='u2', type='GLfixed'),
    ],
)

Function(name='glMapGrid2d', enabled=True, function_type=FuncType.MAP,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='un', type='GLint'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
        Argument(name='vn', type='GLint'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
    ],
)

Function(name='glMapGrid2f', enabled=True, function_type=FuncType.MAP,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='un', type='GLint'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
        Argument(name='vn', type='GLint'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
    ],
)

Function(name='glMapGrid2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLint'),
        Argument(name='u1', type='GLfixed'),
        Argument(name='u2', type='GLfixed'),
        Argument(name='v1', type='GLfixed'),
        Argument(name='v2', type='GLfixed'),
    ],
)

Function(name='glMapNamedBuffer', enabled=True, function_type=FuncType.MAP, state_track=True,
    return_value=ReturnValue(type='void*'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glMapNamedBufferEXT', enabled=True, function_type=FuncType.MAP, inherit_from='glMapNamedBuffer')

Function(name='glMapNamedBufferRange', enabled=True, function_type=FuncType.MAP, state_track=True, interceptor_exec_override=True,
    return_value=ReturnValue(type='void*'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
        Argument(name='access', type='GLbitfield', wrap_type='CMapAccess'),
    ],
)

Function(name='glMapNamedBufferRangeEXT', enabled=True, function_type=FuncType.MAP, interceptor_exec_override=True, inherit_from='glMapNamedBufferRange')

Function(name='glMapObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void*'),
    args=[
        Argument(name='buffer', type='GLuint'),
    ],
)

Function(name='glMapParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glMapParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glMapTexture2DINTEL', enabled=True, function_type=FuncType.PARAM, ccode_wrap=True, state_track=True, interceptor_exec_override=True,
    return_value=ReturnValue(type='void*', wrap_type='CGLMappedTexturePtr'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='access', type='GLbitfield', wrap_params='GL_MAP_WRITE_BIT | GL_MAP_READ_BIT'),
        Argument(name='stride', type='GLint*', wrap_type='COutArgument'),
        Argument(name='layout', type='GLenum*', wrap_type='COutArgument'),
    ],
)

Function(name='glMapVertexAttrib1dAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLuint'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
        Argument(name='stride', type='GLint'),
        Argument(name='order', type='GLint'),
        Argument(name='points', type='const GLdouble*'),
    ],
)

Function(name='glMapVertexAttrib1fAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLuint'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
        Argument(name='stride', type='GLint'),
        Argument(name='order', type='GLint'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

Function(name='glMapVertexAttrib2dAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLuint'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='points', type='const GLdouble*'),
    ],
)

Function(name='glMapVertexAttrib2fAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLuint'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
        Argument(name='ustride', type='GLint'),
        Argument(name='uorder', type='GLint'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
        Argument(name='vstride', type='GLint'),
        Argument(name='vorder', type='GLint'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

Function(name='glMaterialf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glMaterialfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMateriali', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glMaterialiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMaterialx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glMaterialxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMaterialx')

Function(name='glMaterialxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMaterialxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMaterialxv')

Function(name='glMatrixFrustumEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='left', type='GLdouble'),
        Argument(name='right', type='GLdouble'),
        Argument(name='bottom', type='GLdouble'),
        Argument(name='top', type='GLdouble'),
        Argument(name='zNear', type='GLdouble'),
        Argument(name='zFar', type='GLdouble'),
    ],
)

Function(name='glMatrixIndexPointerARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glMatrixIndexPointerOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glMatrixIndexPointerOESBounds', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glMatrixIndexubvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='indices', type='const GLubyte*'),
    ],
)

Function(name='glMatrixIndexuivARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='indices', type='const GLuint*'),
    ],
)

Function(name='glMatrixIndexusvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='indices', type='const GLushort*'),
    ],
)

Function(name='glMatrixLoad3x2fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixLoad3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixLoadIdentityEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glMatrixLoadTranspose3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixLoadTransposedEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*'),
    ],
)

Function(name='glMatrixLoadTransposefEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixLoaddEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

Function(name='glMatrixLoadfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

Function(name='glMatrixMode', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glMatrixMult3x2fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixMult3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixMultTranspose3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixMultTransposedEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*'),
    ],
)

Function(name='glMatrixMultTransposefEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixMultdEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*'),
    ],
)

Function(name='glMatrixMultfEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

Function(name='glMatrixOrthoEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='left', type='GLdouble'),
        Argument(name='right', type='GLdouble'),
        Argument(name='bottom', type='GLdouble'),
        Argument(name='top', type='GLdouble'),
        Argument(name='zNear', type='GLdouble'),
        Argument(name='zFar', type='GLdouble'),
    ],
)

Function(name='glMatrixPopEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glMatrixPushEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glMatrixRotatedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='angle', type='GLdouble'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glMatrixRotatefEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='angle', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glMatrixScaledEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glMatrixScalefEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glMatrixTranslatedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glMatrixTranslatefEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glMaxShaderCompilerThreadsARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glMaxShaderCompilerThreadsKHR', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLuint'),
    ],
)

Function(name='glMemoryBarrier', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='barriers', type='GLbitfield'),
    ],
)

Function(name='glMemoryBarrierByRegion', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='barriers', type='GLbitfield'),
    ],
)

Function(name='glMemoryBarrierEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glMemoryBarrier')

Function(name='glMemoryObjectParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memoryObject', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glMinSampleShading', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLfloat'),
    ],
)

Function(name='glMinSampleShadingARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMinSampleShading')

Function(name='glMinSampleShadingOES', enabled=False, function_type=FuncType.NONE, inherit_from='glMinSampleShading')

Function(name='glMinmax', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='sink', type='GLboolean'),
    ],
)

Function(name='glMinmaxEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glMinmax')

Function(name='glMultMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

Function(name='glMultMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

Function(name='glMultMatrixx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*', wrap_params='16, m'),
    ],
)

Function(name='glMultMatrixxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultMatrixx')

Function(name='glMultTransposeMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

Function(name='glMultTransposeMatrixdARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultTransposeMatrixd')

Function(name='glMultTransposeMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

Function(name='glMultTransposeMatrixfARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultTransposeMatrixf')

Function(name='glMultTransposeMatrixxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*'),
    ],
)

Function(name='glMultiDrawArrays', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, drawcount)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='const GLint*', wrap_params='drawcount, first'),
        Argument(name='count', type='const GLsizei*', wrap_params='drawcount, count'),
        Argument(name='drawcount', type='GLsizei'),
    ],
)

Function(name='glMultiDrawArraysEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawArrays')

Function(name='glMultiDrawArraysIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, indirect, drawcount, stride)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glMultiDrawArraysIndirectAMD', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawArraysIndirect')

Function(name='glMultiDrawArraysIndirectBindlessCountNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawCount', type='GLsizei'),
        Argument(name='maxDrawCount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='vertexBufferCount', type='GLint'),
    ],
)

Function(name='glMultiDrawArraysIndirectBindlessNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawCount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='vertexBufferCount', type='GLint'),
    ],
)

Function(name='glMultiDrawArraysIndirectCount', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLintptr'),
        Argument(name='maxdrawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glMultiDrawArraysIndirectCountARB', enabled=False, function_type=FuncType.NONE, inherit_from='glMultiDrawArraysIndirectCount')

Function(name='glMultiDrawArraysIndirectEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glMultiDrawArraysIndirect')

Function(name='glMultiDrawElementArrayAPPLE', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='const GLint*'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='primcount', type='GLsizei'),
    ],
)

Function(name='glMultiDrawElements', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(type, count, indices, drawcount)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='const GLsizei*', wrap_params='drawcount, count'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*const*', wrap_type='CDataPtrArray', wrap_params='GL_ELEMENT_ARRAY_BUFFER, indices, drawcount'),
        Argument(name='drawcount', type='GLsizei'),
    ],
)

Function(name='glMultiDrawElementsBaseVertex', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*const*'),
        Argument(name='drawcount', type='GLsizei'),
        Argument(name='basevertex', type='const GLint*'),
    ],
)

Function(name='glMultiDrawElementsBaseVertexEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glMultiDrawElementsBaseVertex')

Function(name='glMultiDrawElementsBaseVertexOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode',type='GLenum'),
        Argument(name='count',type='const GLsizei*'),
        Argument(name='type',type='GLenum'),
        Argument(name='indices',type='const void**'),
        Argument(name='primcount',type='GLsizei'),
        Argument(name='basevertex',type='const GLint*'),
    ],
)

Function(name='glMultiDrawElementsEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawElements')

Function(name='glMultiDrawElementsIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, type, indirect, drawcount, stride)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glMultiDrawElementsIndirectAMD', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawElementsIndirect')

Function(name='glMultiDrawElementsIndirectBindlessCountNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawCount', type='GLsizei'),
        Argument(name='maxDrawCount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='vertexBufferCount', type='GLint'),
    ],
)

Function(name='glMultiDrawElementsIndirectBindlessNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawCount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='vertexBufferCount', type='GLint'),
    ],
)

Function(name='glMultiDrawElementsIndirectCount', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLintptr'),
        Argument(name='maxdrawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glMultiDrawElementsIndirectCountARB', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawElementsIndirectCount')

Function(name='glMultiDrawElementsIndirectEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glMultiDrawElementsIndirect')

Function(name='glMultiDrawRangeElementArrayAPPLE', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='first', type='const GLint*'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='primcount', type='GLsizei'),
    ],
)

Function(name='glMultiModeDrawArraysIBM', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='const GLenum*'),
        Argument(name='first', type='const GLint*'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='primcount', type='GLsizei'),
        Argument(name='modestride', type='GLint'),
    ],
)

Function(name='glMultiModeDrawElementsIBM', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='const GLenum*'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*const*'),
        Argument(name='primcount', type='GLsizei'),
        Argument(name='modestride', type='GLint'),
    ],
)

Function(name='glMultiTexBufferEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glMultiTexCoord1bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
    ],
)

Function(name='glMultiTexCoord1bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glMultiTexCoord1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
    ],
)

Function(name='glMultiTexCoord1dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1d')

Function(name='glMultiTexCoord1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

Function(name='glMultiTexCoord1dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1dv')

Function(name='glMultiTexCoord1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
    ],
)

Function(name='glMultiTexCoord1fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1f')

Function(name='glMultiTexCoord1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='1, v'),
    ],
)

Function(name='glMultiTexCoord1fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1fv')

Function(name='glMultiTexCoord1hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
    ],
)

Function(name='glMultiTexCoord1hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

Function(name='glMultiTexCoord1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
    ],
)

Function(name='glMultiTexCoord1iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1i')

Function(name='glMultiTexCoord1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='1, v'),
    ],
)

Function(name='glMultiTexCoord1ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1iv')

Function(name='glMultiTexCoord1s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
    ],
)

Function(name='glMultiTexCoord1sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1s')

Function(name='glMultiTexCoord1sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='1, v'),
    ],
)

Function(name='glMultiTexCoord1svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1sv')

Function(name='glMultiTexCoord1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
    ],
)

Function(name='glMultiTexCoord1xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glMultiTexCoord2bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
    ],
)

Function(name='glMultiTexCoord2bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glMultiTexCoord2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
    ],
)

Function(name='glMultiTexCoord2dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2d')

Function(name='glMultiTexCoord2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glMultiTexCoord2dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2dv')

Function(name='glMultiTexCoord2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
    ],
)

Function(name='glMultiTexCoord2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2f')

Function(name='glMultiTexCoord2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glMultiTexCoord2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2fv')

Function(name='glMultiTexCoord2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
    ],
)

Function(name='glMultiTexCoord2hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

Function(name='glMultiTexCoord2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
    ],
)

Function(name='glMultiTexCoord2iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2i')

Function(name='glMultiTexCoord2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

Function(name='glMultiTexCoord2ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2iv')

Function(name='glMultiTexCoord2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
    ],
)

Function(name='glMultiTexCoord2sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2s')

Function(name='glMultiTexCoord2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

Function(name='glMultiTexCoord2svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2sv')

Function(name='glMultiTexCoord2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
    ],
)

Function(name='glMultiTexCoord2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glMultiTexCoord3bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
    ],
)

Function(name='glMultiTexCoord3bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glMultiTexCoord3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
    ],
)

Function(name='glMultiTexCoord3dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3d')

Function(name='glMultiTexCoord3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glMultiTexCoord3dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3dv')

Function(name='glMultiTexCoord3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
    ],
)

Function(name='glMultiTexCoord3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3f')

Function(name='glMultiTexCoord3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glMultiTexCoord3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3fv')

Function(name='glMultiTexCoord3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
    ],
)

Function(name='glMultiTexCoord3hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

Function(name='glMultiTexCoord3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
    ],
)

Function(name='glMultiTexCoord3iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3i')

Function(name='glMultiTexCoord3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glMultiTexCoord3ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3iv')

Function(name='glMultiTexCoord3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
    ],
)

Function(name='glMultiTexCoord3sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3s')

Function(name='glMultiTexCoord3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glMultiTexCoord3svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3sv')

Function(name='glMultiTexCoord3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
    ],
)

Function(name='glMultiTexCoord3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glMultiTexCoord4bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
        Argument(name='q', type='GLbyte'),
    ],
)

Function(name='glMultiTexCoord4bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glMultiTexCoord4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
        Argument(name='q', type='GLdouble'),
    ],
)

Function(name='glMultiTexCoord4dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4d')

Function(name='glMultiTexCoord4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glMultiTexCoord4dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4dv')

Function(name='glMultiTexCoord4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='q', type='GLfloat'),
    ],
)

Function(name='glMultiTexCoord4fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4f')

Function(name='glMultiTexCoord4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glMultiTexCoord4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4fv')

Function(name='glMultiTexCoord4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
        Argument(name='q', type='GLhalfNV'),
    ],
)

Function(name='glMultiTexCoord4hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

Function(name='glMultiTexCoord4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
        Argument(name='q', type='GLint'),
    ],
)

Function(name='glMultiTexCoord4iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4i')

Function(name='glMultiTexCoord4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glMultiTexCoord4ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4iv')

Function(name='glMultiTexCoord4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
        Argument(name='q', type='GLshort'),
    ],
)

Function(name='glMultiTexCoord4sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4s')

Function(name='glMultiTexCoord4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glMultiTexCoord4svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4sv')

Function(name='glMultiTexCoord4x', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
        Argument(name='q', type='GLfixed'),
    ],
)

Function(name='glMultiTexCoord4xOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4x')

Function(name='glMultiTexCoord4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glMultiTexCoordP1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glMultiTexCoordP1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

Function(name='glMultiTexCoordP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glMultiTexCoordP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='2, coords'),
    ],
)

Function(name='glMultiTexCoordP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glMultiTexCoordP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='3, coords'),
    ],
)

Function(name='glMultiTexCoordP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glMultiTexCoordP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='4, coords'),
    ],
)

Function(name='glMultiTexCoordPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glMultiTexEnvfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glMultiTexEnvfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMultiTexEnviEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glMultiTexEnvivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMultiTexGendEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLdouble'),
    ],
)

Function(name='glMultiTexGendvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLdouble*'),
    ],
)

Function(name='glMultiTexGenfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glMultiTexGenfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glMultiTexGeniEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glMultiTexGenivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glMultiTexImage1DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glMultiTexImage2DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glMultiTexImage3DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glMultiTexParameterIivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMultiTexParameterIuivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMultiTexParameterfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glMultiTexParameterfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMultiTexParameteriEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glMultiTexParameterivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glMultiTexRenderbufferEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

Function(name='glMultiTexSubImage1DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glMultiTexSubImage2DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glMultiTexSubImage3DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glMulticastBarrierNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glMulticastBlitFramebufferNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='srcGpu', type='GLuint'),
        Argument(name='dstGpu', type='GLuint'),
        Argument(name='srcX0', type='GLint'),
        Argument(name='srcY0', type='GLint'),
        Argument(name='srcX1', type='GLint'),
        Argument(name='srcY1', type='GLint'),
        Argument(name='dstX0', type='GLint'),
        Argument(name='dstY0', type='GLint'),
        Argument(name='dstX1', type='GLint'),
        Argument(name='dstY1', type='GLint'),
        Argument(name='mask', type='GLbitfield'),
        Argument(name='filter', type='GLenum'),
    ],
)

Function(name='glMulticastBufferSubDataNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpuMask', type='GLbitfield'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource'),
    ],
)

Function(name='glMulticastCopyBufferSubDataNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readGpu', type='GLuint'),
        Argument(name='writeGpuMask', type='GLbitfield'),
        Argument(name='readBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='writeBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glMulticastCopyImageSubDataNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='srcGpu', type='GLuint'),
        Argument(name='dstGpuMask', type='GLbitfield'),
        Argument(name='srcName', type='GLuint'),
        Argument(name='srcTarget', type='GLenum'),
        Argument(name='srcLevel', type='GLint'),
        Argument(name='srcX', type='GLint'),
        Argument(name='srcY', type='GLint'),
        Argument(name='srcZ', type='GLint'),
        Argument(name='dstName', type='GLuint'),
        Argument(name='dstTarget', type='GLenum'),
        Argument(name='dstLevel', type='GLint'),
        Argument(name='dstX', type='GLint'),
        Argument(name='dstY', type='GLint'),
        Argument(name='dstZ', type='GLint'),
        Argument(name='srcWidth', type='GLsizei'),
        Argument(name='srcHeight', type='GLsizei'),
        Argument(name='srcDepth', type='GLsizei'),
    ],
)

Function(name='glMulticastFramebufferSampleLocationsfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glMulticastGetQueryObjecti64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*'),
    ],
)

Function(name='glMulticastGetQueryObjectivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

Function(name='glMulticastGetQueryObjectui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64*'),
    ],
)

Function(name='glMulticastGetQueryObjectuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

Function(name='glMulticastWaitSyncNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='signalGpu', type='GLuint'),
        Argument(name='waitGpuMask', type='GLbitfield'),
    ],
)

Function(name='glNamedBufferData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='usage', type='GLenum'),
    ],
)

Function(name='glNamedBufferDataEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glNamedBufferData')

Function(name='glNamedBufferPageCommitmentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='commit', type='GLboolean'),
    ],
)

Function(name='glNamedBufferPageCommitmentEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='commit', type='GLboolean'),
    ],
)

Function(name='glNamedBufferStorage', enabled=True, function_type=FuncType.RESOURCE, state_track=True, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='flags', type='GLbitfield', wrap_type='CBufferStorageFlags'),
    ],
)

Function(name='glNamedBufferStorageEXT', enabled=True, function_type=FuncType.RESOURCE, interceptor_exec_override=True, inherit_from='glNamedBufferStorage')

Function(name='glNamedBufferStorageExternalEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='clientBuffer', type='GLeglClientBufferEXT'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glNamedBufferStorageMemEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glNamedBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
    ],
)

Function(name='glNamedBufferSubDataEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glNamedBufferSubData')

Function(name='glNamedCopyBufferSubDataEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='writeBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glNamedFramebufferDrawBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='buf', type='GLenum'),
    ],
)

Function(name='glNamedFramebufferDrawBuffers', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='n', type='GLsizei'),
        Argument(name='bufs', type='const GLenum*', wrap_params='n, bufs'),
    ],
)

Function(name='glNamedFramebufferParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glNamedFramebufferParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glNamedFramebufferParameteri')

Function(name='glNamedFramebufferReadBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='src', type='GLenum'),
    ],
)

Function(name='glNamedFramebufferRenderbuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='renderbuffertarget', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

Function(name='glNamedFramebufferRenderbufferEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glNamedFramebufferRenderbuffer')

Function(name='glNamedFramebufferSampleLocationsfvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glNamedFramebufferSampleLocationsfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glNamedFramebufferSamplePositionsfvAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numsamples', type='GLuint'),
        Argument(name='pixelindex', type='GLuint'),
        Argument(name='values', type='const GLfloat*'),
    ],
)

Function(name='glNamedFramebufferTexture', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glNamedFramebufferTexture1DEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glNamedFramebufferTexture2DEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glNamedFramebufferTexture3DEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='zoffset', type='GLint'),
    ],
)

Function(name='glNamedFramebufferTextureEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glNamedFramebufferTexture')

Function(name='glNamedFramebufferTextureFaceEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='face', type='GLenum'),
    ],
)

Function(name='glNamedFramebufferTextureLayer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layer', type='GLint'),
    ],
)

Function(name='glNamedFramebufferTextureLayerEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glNamedFramebufferTextureLayer')

Function(name='glNamedProgramLocalParameter4dEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glNamedProgramLocalParameter4dvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLdouble*', wrap_params='4, params'),
    ],
)

Function(name='glNamedProgramLocalParameter4fEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glNamedProgramLocalParameter4fvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLfloat*', wrap_params='4, params'),
    ],
)

Function(name='glNamedProgramLocalParameterI4iEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glNamedProgramLocalParameterI4ivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLint*', wrap_params='4, params'),
    ],
)

Function(name='glNamedProgramLocalParameterI4uiEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
        Argument(name='w', type='GLuint'),
    ],
)

Function(name='glNamedProgramLocalParameterI4uivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLuint*', wrap_params='4, params'),
    ],
)

Function(name='glNamedProgramLocalParameters4fvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*', wrap_params='count*4, params'),
    ],
)

Function(name='glNamedProgramLocalParametersI4ivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*', wrap_params='count*4, params'),
    ],
)

Function(name='glNamedProgramLocalParametersI4uivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*', wrap_params='count*4, params'),
    ],
)

Function(name='glNamedProgramStringEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='len', type='GLsizei'),
        Argument(name='string', type='const void*'),
    ],
)

Function(name='glNamedRenderbufferStorage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glNamedRenderbufferStorageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glNamedRenderbufferStorage',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glNamedRenderbufferStorageMultisample', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glNamedRenderbufferStorageMultisampleCoverageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='coverageSamples', type='GLsizei'),
        Argument(name='colorSamples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glNamedRenderbufferStorageMultisampleEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glNamedRenderbufferStorageMultisample',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
        Argument(name='stringlen', type='GLint'),
        Argument(name='string', type='const GLchar*'),
    ],
)

Function(name='glNewList', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glNewObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
        Argument(name='usage', type='GLenum'),
    ],
)

Function(name='glNormal3b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLbyte'),
        Argument(name='ny', type='GLbyte'),
        Argument(name='nz', type='GLbyte'),
    ],
)

Function(name='glNormal3bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLdouble'),
        Argument(name='ny', type='GLdouble'),
        Argument(name='nz', type='GLdouble'),
    ],
)

Function(name='glNormal3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
    ],
)

Function(name='glNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='const GLfloat*', wrap_params='3, v'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLhalfNV'),
        Argument(name='ny', type='GLhalfNV'),
        Argument(name='nz', type='GLhalfNV'),
    ],
)

Function(name='glNormal3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLint'),
        Argument(name='ny', type='GLint'),
        Argument(name='nz', type='GLint'),
    ],
)

Function(name='glNormal3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLshort'),
        Argument(name='ny', type='GLshort'),
        Argument(name='nz', type='GLshort'),
    ],
)

Function(name='glNormal3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glNormal3x', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLfixed'),
        Argument(name='ny', type='GLfixed'),
        Argument(name='nz', type='GLfixed'),
    ],
)

Function(name='glNormal3xOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glNormal3x')

Function(name='glNormal3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glNormalFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glNormalP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glNormalP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='3, coords'),
    ],
)

Function(name='glNormalPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glNormalPointerBounds', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glNormalPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glNormalPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glNormalPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

Function(name='glNormalStream3bATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLbyte'),
        Argument(name='ny', type='GLbyte'),
        Argument(name='nz', type='GLbyte'),
    ],
)

Function(name='glNormalStream3bvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glNormalStream3dATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLdouble'),
        Argument(name='ny', type='GLdouble'),
        Argument(name='nz', type='GLdouble'),
    ],
)

Function(name='glNormalStream3dvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*'),
    ],
)

Function(name='glNormalStream3fATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
    ],
)

Function(name='glNormalStream3fvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*'),
    ],
)

Function(name='glNormalStream3iATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLint'),
        Argument(name='ny', type='GLint'),
        Argument(name='nz', type='GLint'),
    ],
)

Function(name='glNormalStream3ivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*'),
    ],
)

Function(name='glNormalStream3sATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLshort'),
        Argument(name='ny', type='GLshort'),
        Argument(name='nz', type='GLshort'),
    ],
)

Function(name='glNormalStream3svATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*'),
    ],
)

Function(name='glObjectLabel', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='label', type='const GLchar*', wrap_params='label, \'\\0\', 1'),
    ],
)

Function(name='glObjectLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glObjectLabel')

Function(name='glObjectPtrLabel', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='ptr', type='const void*'),
        Argument(name='length', type='GLsizei'),
        Argument(name='label', type='const GLchar*'),
    ],
)

Function(name='glObjectPtrLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glObjectPtrLabel')

Function(name='glObjectPurgeableAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='objectType', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='option', type='GLenum'),
    ],
)

Function(name='glObjectUnpurgeableAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='objectType', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='option', type='GLenum'),
    ],
)

Function(name='glOrtho', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='left', type='GLdouble'),
        Argument(name='right', type='GLdouble'),
        Argument(name='bottom', type='GLdouble'),
        Argument(name='top', type='GLdouble'),
        Argument(name='zNear', type='GLdouble'),
        Argument(name='zFar', type='GLdouble'),
    ],
)

Function(name='glOrthof', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='l', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

Function(name='glOrthofOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glOrthof')

Function(name='glOrthox', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='l', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
        Argument(name='b', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='n', type='GLfixed'),
        Argument(name='f', type='GLfixed'),
    ],
)

Function(name='glOrthoxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glOrthox')

Function(name='glPNTrianglesfATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glPNTrianglesiATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glPassTexCoordATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='dst', type='GLuint'),
        Argument(name='coord', type='GLuint'),
        Argument(name='swizzle', type='GLenum'),
    ],
)

Function(name='glPassThrough', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='token', type='GLfloat'),
    ],
)

Function(name='glPassThroughxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='token', type='GLfixed'),
    ],
)

# FIXME: Argument `values` had a `wrapParam='CGLfloat::CSParamArray'`, which was a typo of wrapType.
# FIXME: Should we add that wrap_type or not?
Function(name='glPatchParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='values', type='const GLfloat*', wrap_params='GetPatchParameterValuesCount(pname), values'),
    ],
)

Function(name='glPatchParameterfvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameterfv')

Function(name='glPatchParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

Function(name='glPatchParameteriARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameteri')

Function(name='glPatchParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameteri')

Function(name='glPatchParameteriOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameteri')

Function(name='glPathColorGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLenum'),
        Argument(name='genMode', type='GLenum'),
        Argument(name='colorFormat', type='GLenum'),
        Argument(name='coeffs', type='const GLfloat*'),
    ],
)

Function(name='glPathCommandsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='numCommands', type='GLsizei'),
        Argument(name='commands', type='const GLubyte*'),
        Argument(name='numCoords', type='GLsizei'),
        Argument(name='coordType', type='GLenum'),
        Argument(name='coords', type='const void*'),
    ],
)

Function(name='glPathCoordsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='numCoords', type='GLsizei'),
        Argument(name='coordType', type='GLenum'),
        Argument(name='coords', type='const void*'),
    ],
)

Function(name='glPathCoverDepthFuncNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
    ],
)

Function(name='glPathDashArrayNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='dashCount', type='GLsizei'),
        Argument(name='dashArray', type='const GLfloat*'),
    ],
)

Function(name='glPathFogGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='genMode', type='GLenum'),
    ],
)

Function(name='glPathGlyphIndexArrayNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='firstPathName', type='GLuint'),
        Argument(name='fontTarget', type='GLenum'),
        Argument(name='fontName', type='const void*'),
        Argument(name='fontStyle', type='GLbitfield'),
        Argument(name='firstGlyphIndex', type='GLuint'),
        Argument(name='numGlyphs', type='GLsizei'),
        Argument(name='pathParameterTemplate', type='GLuint'),
        Argument(name='emScale', type='GLfloat'),
    ],
)

Function(name='glPathGlyphIndexRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='fontTarget', type='GLenum'),
        Argument(name='fontName', type='const void*'),
        Argument(name='fontStyle', type='GLbitfield'),
        Argument(name='pathParameterTemplate', type='GLuint'),
        Argument(name='emScale', type='GLfloat'),
        Argument(name='baseAndCount [2]', type='GLuint'),
    ],
)

Function(name='glPathGlyphRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='firstPathName', type='GLuint'),
        Argument(name='fontTarget', type='GLenum'),
        Argument(name='fontName', type='const void*'),
        Argument(name='fontStyle', type='GLbitfield'),
        Argument(name='firstGlyph', type='GLuint'),
        Argument(name='numGlyphs', type='GLsizei'),
        Argument(name='handleMissingGlyphs', type='GLenum'),
        Argument(name='pathParameterTemplate', type='GLuint'),
        Argument(name='emScale', type='GLfloat'),
    ],
)

Function(name='glPathGlyphsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='firstPathName', type='GLuint'),
        Argument(name='fontTarget', type='GLenum'),
        Argument(name='fontName', type='const void*'),
        Argument(name='fontStyle', type='GLbitfield'),
        Argument(name='numGlyphs', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='charcodes', type='const void*'),
        Argument(name='handleMissingGlyphs', type='GLenum'),
        Argument(name='pathParameterTemplate', type='GLuint'),
        Argument(name='emScale', type='GLfloat'),
    ],
)

Function(name='glPathMemoryGlyphIndexArrayNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='firstPathName', type='GLuint'),
        Argument(name='fontTarget', type='GLenum'),
        Argument(name='fontSize', type='GLsizeiptr'),
        Argument(name='fontData', type='const void*'),
        Argument(name='faceIndex', type='GLsizei'),
        Argument(name='firstGlyphIndex', type='GLuint'),
        Argument(name='numGlyphs', type='GLsizei'),
        Argument(name='pathParameterTemplate', type='GLuint'),
        Argument(name='emScale', type='GLfloat'),
    ],
)

Function(name='glPathParameterfNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat'),
    ],
)

Function(name='glPathParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='const GLfloat*'),
    ],
)

Function(name='glPathParameteriNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

Function(name='glPathParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='const GLint*'),
    ],
)

Function(name='glPathStencilDepthOffsetNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='units', type='GLfloat'),
    ],
)

Function(name='glPathStencilFuncNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glPathStringNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='format', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='pathString', type='const void*'),
    ],
)

Function(name='glPathSubCommandsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='commandStart', type='GLsizei'),
        Argument(name='commandsToDelete', type='GLsizei'),
        Argument(name='numCommands', type='GLsizei'),
        Argument(name='commands', type='const GLubyte*'),
        Argument(name='numCoords', type='GLsizei'),
        Argument(name='coordType', type='GLenum'),
        Argument(name='coords', type='const void*'),
    ],
)

Function(name='glPathSubCoordsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coordStart', type='GLsizei'),
        Argument(name='numCoords', type='GLsizei'),
        Argument(name='coordType', type='GLenum'),
        Argument(name='coords', type='const void*'),
    ],
)

Function(name='glPathTexGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texCoordSet', type='GLenum'),
        Argument(name='genMode', type='GLenum'),
        Argument(name='components', type='GLint'),
        Argument(name='coeffs', type='const GLfloat*'),
    ],
)

Function(name='glPauseTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPauseTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPauseTransformFeedback')

Function(name='glPixelDataRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glPixelMapfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='mapsize', type='GLsizei'),
        Argument(name='values', type='const GLfloat*', wrap_params='mapsize, values'),
    ],
)

Function(name='glPixelMapuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='mapsize', type='GLsizei'),
        Argument(name='values', type='const GLuint*'),
    ],
)

Function(name='glPixelMapusv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='mapsize', type='GLsizei'),
        Argument(name='values', type='const GLushort*'),
    ],
)

Function(name='glPixelMapx', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='values', type='const GLfixed*'),
    ],
)

Function(name='glPixelStoref', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glPixelStorei', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glPixelStorex', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glPixelTexGenParameterfSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glPixelTexGenParameterfvSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glPixelTexGenParameteriSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glPixelTexGenParameterivSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glPixelTexGenSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glPixelTransferf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glPixelTransferi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glPixelTransferxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glPixelTransformParameterfEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glPixelTransformParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glPixelTransformParameteriEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glPixelTransformParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glPixelZoom', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfactor', type='GLfloat'),
        Argument(name='yfactor', type='GLfloat'),
    ],
)

Function(name='glPixelZoomxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfactor', type='GLfixed'),
        Argument(name='yfactor', type='GLfixed'),
    ],
)

Function(name='glPointAlongPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='startSegment', type='GLsizei'),
        Argument(name='numSegments', type='GLsizei'),
        Argument(name='distance', type='GLfloat'),
        Argument(name='x', type='GLfloat*'),
        Argument(name='y', type='GLfloat*'),
        Argument(name='tangentX', type='GLfloat*'),
        Argument(name='tangentY', type='GLfloat*'),
    ],
)

Function(name='glPointParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glPointParameterfARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterf')

Function(name='glPointParameterfEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterf')

Function(name='glPointParameterfSGIS', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterf')

Function(name='glPointParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glPointParameterfvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterfv')

Function(name='glPointParameterfvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterfv')

Function(name='glPointParameterfvSGIS', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterfv')

Function(name='glPointParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glPointParameteriNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameteri')

Function(name='glPointParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glPointParameterivNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameteriv')

Function(name='glPointParameterx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glPointParameterxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterx')

Function(name='glPointParameterxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glPointParameterxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterxv')

Function(name='glPointSize', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLfloat'),
    ],
)

Function(name='glPointSizePointerAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
    ],
)

Function(name='glPointSizePointerOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glPointSizePointerOESBounds', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glPointSizex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLfixed'),
    ],
)

Function(name='glPointSizexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointSizex')

Function(name='glPollAsyncSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='markerp', type='GLuint*'),
    ],
)

Function(name='glPollInstrumentsSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='marker_p', type='GLint*'),
    ],
)

Function(name='glPolygonMode', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glPolygonModeNV', enabled=False, function_type=FuncType.NONE, inherit_from='glPolygonMode')

Function(name='glPolygonOffset', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='units', type='GLfloat'),
    ],
)

Function(name='glPolygonOffsetClamp', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='units', type='GLfloat'),
        Argument(name='clamp', type='GLfloat'),
    ],
)

Function(name='glPolygonOffsetClampEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPolygonOffsetClamp')

Function(name='glPolygonOffsetEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='bias', type='GLfloat'),
    ],
)

Function(name='glPolygonOffsetx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfixed'),
        Argument(name='units', type='GLfixed'),
    ],
)

Function(name='glPolygonOffsetxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPolygonOffsetx')

Function(name='glPolygonStipple', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='const GLubyte*', wrap_params='32 * 4, mask'),
    ],
)

Function(name='glPopAttrib', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPopClientAttrib', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPopDebugGroup', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPopDebugGroupKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glPopDebugGroup')

Function(name='glPopGroupMarkerEXT', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPopMatrix', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPopName', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPresentFrameDualFillNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='minPresentTime', type='GLuint64EXT'),
        Argument(name='beginPresentTimeId', type='GLuint'),
        Argument(name='presentDurationId', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='target0', type='GLenum'),
        Argument(name='fill0', type='GLuint'),
        Argument(name='target1', type='GLenum'),
        Argument(name='fill1', type='GLuint'),
        Argument(name='target2', type='GLenum'),
        Argument(name='fill2', type='GLuint'),
        Argument(name='target3', type='GLenum'),
        Argument(name='fill3', type='GLuint'),
    ],
)

Function(name='glPresentFrameKeyedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='minPresentTime', type='GLuint64EXT'),
        Argument(name='beginPresentTimeId', type='GLuint'),
        Argument(name='presentDurationId', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='target0', type='GLenum'),
        Argument(name='fill0', type='GLuint'),
        Argument(name='key0', type='GLuint'),
        Argument(name='target1', type='GLenum'),
        Argument(name='fill1', type='GLuint'),
        Argument(name='key1', type='GLuint'),
    ],
)

Function(name='glPrimitiveBoundingBox', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='minX', type='GLfloat'),
        Argument(name='minY', type='GLfloat'),
        Argument(name='minZ', type='GLfloat'),
        Argument(name='minW', type='GLfloat'),
        Argument(name='maxX', type='GLfloat'),
        Argument(name='maxY', type='GLfloat'),
        Argument(name='maxZ', type='GLfloat'),
        Argument(name='maxW', type='GLfloat'),
    ],
)

Function(name='glPrimitiveBoundingBoxARB', enabled=False, function_type=FuncType.NONE, inherit_from='glPrimitiveBoundingBox')

Function(name='glPrimitiveBoundingBoxEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glPrimitiveBoundingBox')

Function(name='glPrimitiveBoundingBoxOES', enabled=False, function_type=FuncType.NONE, inherit_from='glPrimitiveBoundingBox')

Function(name='glPrimitiveRestartIndex', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
    ],
)

Function(name='glPrimitiveRestartIndexNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPrimitiveRestartIndex')

Function(name='glPrimitiveRestartNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPrioritizeTextures', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='priorities', type='const GLfloat*'),
    ],
)

Function(name='glPrioritizeTexturesEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glPrioritizeTextures')

Function(name='glPrioritizeTexturesxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='priorities', type='const GLfixed*'),
    ],
)

Function(name='glProgramBinary', enabled=True, function_type=FuncType.RESOURCE, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='binaryFormat', type='GLenum'),
        Argument(name='binary', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, binary, length'),
        Argument(name='length', type='GLsizei'),
    ],
)

Function(name='glProgramBinaryOES', enabled=True, function_type=FuncType.RESOURCE, interceptor_exec_override=True, inherit_from='glProgramBinary')

Function(name='glProgramBufferParametersIivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='bindingIndex', type='GLuint'),
        Argument(name='wordIndex', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glProgramBufferParametersIuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='bindingIndex', type='GLuint'),
        Argument(name='wordIndex', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

Function(name='glProgramBufferParametersfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='bindingIndex', type='GLuint'),
        Argument(name='wordIndex', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glProgramEnvParameter4dARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glProgramEnvParameter4dvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLdouble*', wrap_params='4, params'),
    ],
)

Function(name='glProgramEnvParameter4fARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glProgramEnvParameter4fvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLfloat*', wrap_params='4, params'),
    ],
)

Function(name='glProgramEnvParameterI4iNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glProgramEnvParameterI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glProgramEnvParameterI4uiNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
        Argument(name='w', type='GLuint'),
    ],
)

Function(name='glProgramEnvParameterI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLuint*'),
    ],
)

Function(name='glProgramEnvParameters4fvEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*', wrap_params='4 * count, params'),
    ],
)

Function(name='glProgramEnvParametersI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glProgramEnvParametersI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

Function(name='glProgramLocalParameter4dARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glProgramLocalParameter4dvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLdouble*', wrap_params='4, params'),
    ],
)

Function(name='glProgramLocalParameter4fARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glProgramLocalParameter4fvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLfloat*', wrap_params='4, params'),
    ],
)

Function(name='glProgramLocalParameterI4iNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glProgramLocalParameterI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glProgramLocalParameterI4uiNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
        Argument(name='w', type='GLuint'),
    ],
)

Function(name='glProgramLocalParameterI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLuint*'),
    ],
)

Function(name='glProgramLocalParameters4fvEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*', wrap_params='count * 4, params'),
    ],
)

Function(name='glProgramLocalParametersI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glProgramLocalParametersI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

Function(name='glProgramNamedParameter4dNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glProgramNamedParameter4dvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='v', type='const GLdouble*'),
    ],
)

Function(name='glProgramNamedParameter4fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glProgramNamedParameter4fvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glProgramParameter4dNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glProgramParameter4dvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*'),
    ],
)

Function(name='glProgramParameter4fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glProgramParameter4fvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glProgramParameteri', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

Function(name='glProgramParameteriARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramParameteri')

Function(name='glProgramParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramParameteri')

Function(name='glProgramParameters4dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='count*4, v'),
    ],
)

Function(name='glProgramParameters4fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='count*4, v'),
    ],
)

Function(name='glProgramPathFragmentInputGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='genMode', type='GLenum'),
        Argument(name='components', type='GLint'),
        Argument(name='coeffs', type='const GLfloat*'),
    ],
)

Function(name='glProgramStringARB', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='len', type='GLsizei'),
        Argument(name='string', type='const void*', wrap_type='CShaderSource', wrap_params='(const char *)string, len, CShaderSource::PROGRAM_STRING, target, format'),
    ],
)

Function(name='glProgramSubroutineParametersuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

Function(name='glProgramUniform1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
    ],
)

Function(name='glProgramUniform1dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1d')

Function(name='glProgramUniform1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1dv')

Function(name='glProgramUniform1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
    ],
)

Function(name='glProgramUniform1fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1f')

Function(name='glProgramUniform1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1fv')

Function(name='glProgramUniform1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
    ],
)

Function(name='glProgramUniform1i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
    ],
)

Function(name='glProgramUniform1i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
    ],
)

Function(name='glProgramUniform1i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1i')

Function(name='glProgramUniform1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1iv')

Function(name='glProgramUniform1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
    ],
)

Function(name='glProgramUniform1ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
    ],
)

Function(name='glProgramUniform1ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

Function(name='glProgramUniform1ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1ui')

Function(name='glProgramUniform1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count, value'),
    ],
)

Function(name='glProgramUniform1uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1uiv')

Function(name='glProgramUniform2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
        Argument(name='v1', type='GLdouble'),
    ],
)

Function(name='glProgramUniform2dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2d')

Function(name='glProgramUniform2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2dv')

Function(name='glProgramUniform2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
    ],
)

Function(name='glProgramUniform2fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2f')

Function(name='glProgramUniform2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2fv')

Function(name='glProgramUniform2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
    ],
)

Function(name='glProgramUniform2i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
    ],
)

Function(name='glProgramUniform2i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
    ],
)

Function(name='glProgramUniform2i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2i')

Function(name='glProgramUniform2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2iv')

Function(name='glProgramUniform2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
    ],
)

Function(name='glProgramUniform2ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
    ],
)

Function(name='glProgramUniform2ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
    ],
)

Function(name='glProgramUniform2ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2ui')

Function(name='glProgramUniform2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 2, value'),
    ],
)

Function(name='glProgramUniform2uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2uiv')

Function(name='glProgramUniform3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
    ],
)

Function(name='glProgramUniform3dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3d')

Function(name='glProgramUniform3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3dv')

Function(name='glProgramUniform3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
    ],
)

Function(name='glProgramUniform3fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3f')

Function(name='glProgramUniform3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3fv')

Function(name='glProgramUniform3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
    ],
)

Function(name='glProgramUniform3i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
    ],
)

Function(name='glProgramUniform3i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
    ],
)

Function(name='glProgramUniform3i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3i')

Function(name='glProgramUniform3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3iv')

Function(name='glProgramUniform3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
    ],
)

Function(name='glProgramUniform3ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
    ],
)

Function(name='glProgramUniform3ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
    ],
)

Function(name='glProgramUniform3ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3ui')

Function(name='glProgramUniform3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 3, value'),
    ],
)

Function(name='glProgramUniform3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3uiv')

Function(name='glProgramUniform4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
        Argument(name='v3', type='GLdouble'),
    ],
)

Function(name='glProgramUniform4dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4d')

Function(name='glProgramUniform4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4dv')

Function(name='glProgramUniform4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
        Argument(name='v3', type='GLfloat'),
    ],
)

Function(name='glProgramUniform4fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4f')

Function(name='glProgramUniform4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4fv')

Function(name='glProgramUniform4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
        Argument(name='v3', type='GLint'),
    ],
)

Function(name='glProgramUniform4i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
        Argument(name='w', type='GLint64'),
    ],
)

Function(name='glProgramUniform4i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
        Argument(name='w', type='GLint64EXT'),
    ],
)

Function(name='glProgramUniform4i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4i')

Function(name='glProgramUniform4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4iv')

Function(name='glProgramUniform4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
        Argument(name='v3', type='GLuint'),
    ],
)

Function(name='glProgramUniform4ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
        Argument(name='w', type='GLuint64'),
    ],
)

Function(name='glProgramUniform4ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
        Argument(name='w', type='GLuint64EXT'),
    ],
)

Function(name='glProgramUniform4ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4ui')

Function(name='glProgramUniform4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniform4uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4uiv')

Function(name='glProgramUniformHandleui64ARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='value', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

Function(name='glProgramUniformHandleui64IMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='value', type='GLuint64'),
    ],
)

Function(name='glProgramUniformHandleui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='value', type='GLuint64'),
    ],
)

Function(name='glProgramUniformHandleui64vARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='values', type='const GLuint64*', wrap_type='CGLTextureHandle::CSArray', wrap_params='count, values'),
    ],
)

Function(name='glProgramUniformHandleui64vIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='values', type='const GLuint64*'),
    ],
)

Function(name='glProgramUniformHandleui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='values', type='const GLuint64*', wrap_params='count, values'),
    ],
)

Function(name='glProgramUniformMatrix2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniformMatrix2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2dv')

Function(name='glProgramUniformMatrix2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

Function(name='glProgramUniformMatrix2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2fv')

Function(name='glProgramUniformMatrix2x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

Function(name='glProgramUniformMatrix2x3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x3dv')

Function(name='glProgramUniformMatrix2x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

Function(name='glProgramUniformMatrix2x3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x3fv')

Function(name='glProgramUniformMatrix2x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

Function(name='glProgramUniformMatrix2x4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x4dv')

Function(name='glProgramUniformMatrix2x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

Function(name='glProgramUniformMatrix2x4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x4fv')

Function(name='glProgramUniformMatrix3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 9, value'),
    ],
)

Function(name='glProgramUniformMatrix3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3dv')

Function(name='glProgramUniformMatrix3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 9, value'),
    ],
)

Function(name='glProgramUniformMatrix3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3fv')

Function(name='glProgramUniformMatrix3x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

Function(name='glProgramUniformMatrix3x2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x2dv')

Function(name='glProgramUniformMatrix3x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

Function(name='glProgramUniformMatrix3x2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x2fv')

Function(name='glProgramUniformMatrix3x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

Function(name='glProgramUniformMatrix3x4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x4dv')

Function(name='glProgramUniformMatrix3x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

Function(name='glProgramUniformMatrix3x4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x4fv')

Function(name='glProgramUniformMatrix4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 16, value'),
    ],
)

Function(name='glProgramUniformMatrix4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4dv')

Function(name='glProgramUniformMatrix4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 16, value'),
    ],
)

Function(name='glProgramUniformMatrix4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4fv')

Function(name='glProgramUniformMatrix4x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

Function(name='glProgramUniformMatrix4x2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x2dv')

Function(name='glProgramUniformMatrix4x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

Function(name='glProgramUniformMatrix4x2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x2fv')

Function(name='glProgramUniformMatrix4x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

Function(name='glProgramUniformMatrix4x3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x3dv')

Function(name='glProgramUniformMatrix4x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

Function(name='glProgramUniformMatrix4x3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x3fv')

Function(name='glProgramUniformui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='value', type='GLuint64EXT'),
    ],
)

Function(name='glProgramUniformui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

Function(name='glProgramVertexLimitNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='limit', type='GLint'),
    ],
)

Function(name='glProvokingVertex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glProvokingVertexEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProvokingVertex')

Function(name='glPushAttrib', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glPushClientAttrib', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glPushClientAttribDefaultEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glPushDebugGroup', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='source', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='message', type='const GLchar*', wrap_params='message, \'\\0\', 1'),
    ],
)

Function(name='glPushDebugGroupKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glPushDebugGroup')

Function(name='glPushGroupMarkerEXT', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='marker', type='const GLchar*', wrap_params='length, marker'),
    ],
)

Function(name='glPushMatrix', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glPushName', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='name', type='GLuint'),
    ],
)

Function(name='glQueryCounter', enabled=True, function_type=FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glQueryCounterEXT', enabled=True, function_type=FuncType.QUERY, inherit_from='glQueryCounter')

Function(name='glQueryCounterINTEL', enabled=True, function_type=FuncType.QUERY, inherit_from='glQueryCounter')

Function(name='glQueryMatrixxOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLbitfield'),
    args=[
        Argument(name='mantissa', type='GLfixed*', wrap_params='16, mantissa'),
        Argument(name='exponent', type='GLint*', wrap_params='16, exponent'),
    ],
)

Function(name='glQueryObjectParameteruiAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLuint'),
    ],
)

Function(name='glQueryResourceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='queryType', type='GLenum'),
        Argument(name='tagId', type='GLint'),
        Argument(name='bufSize', type='GLuint'),
        Argument(name='buffer', type='GLint*'),
    ],
)

Function(name='glQueryResourceTagNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tagId', type='GLint'),
        Argument(name='tagString', type='const GLchar*'),
    ],
)

Function(name='glRasterPos2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glRasterPos2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glRasterPos2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glRasterPos2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glRasterPos2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

Function(name='glRasterPos2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

Function(name='glRasterPos2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

Function(name='glRasterPos2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

Function(name='glRasterPos2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
    ],
)

Function(name='glRasterPos2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glRasterPos3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glRasterPos3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glRasterPos3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glRasterPos3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glRasterPos3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

Function(name='glRasterPos3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glRasterPos3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

Function(name='glRasterPos3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glRasterPos3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

Function(name='glRasterPos3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glRasterPos4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glRasterPos4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glRasterPos4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glRasterPos4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glRasterPos4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glRasterPos4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glRasterPos4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

Function(name='glRasterPos4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glRasterPos4xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
        Argument(name='w', type='GLfixed'),
    ],
)

Function(name='glRasterPos4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glRasterSamplesEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='samples', type='GLuint'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glReadBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='src', type='GLenum'),
    ],
)

Function(name='glReadBufferIndexedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='src', type='GLenum'),
        Argument(name='index', type='GLint'),
    ],
)

Function(name='glReadBufferNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glReadBuffer')

Function(name='glReadInstrumentsSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLint'),
    ],
)

Function(name='glReadPixels', enabled=True, function_type=FuncType.PARAM, run_wrap=True, ccode_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

Function(name='glReadnPixels', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='data', type='void*'),
    ],
)

Function(name='glReadnPixelsARB', enabled=False, function_type=FuncType.NONE, inherit_from='glReadnPixels')

Function(name='glReadnPixelsEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glReadnPixels')

Function(name='glReadnPixelsKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glReadnPixels')

Function(name='glRectd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLdouble'),
        Argument(name='y1', type='GLdouble'),
        Argument(name='x2', type='GLdouble'),
        Argument(name='y2', type='GLdouble'),
    ],
)

Function(name='glRectdv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLdouble*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLdouble*', wrap_params='2, v2'),
    ],
)

Function(name='glRectf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLfloat'),
        Argument(name='y1', type='GLfloat'),
        Argument(name='x2', type='GLfloat'),
        Argument(name='y2', type='GLfloat'),
    ],
)

Function(name='glRectfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLfloat*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLfloat*', wrap_params='2, v2'),
    ],
)

Function(name='glRecti', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLint'),
        Argument(name='y1', type='GLint'),
        Argument(name='x2', type='GLint'),
        Argument(name='y2', type='GLint'),
    ],
)

Function(name='glRectiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLint*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLint*', wrap_params='2, v2'),
    ],
)

Function(name='glRects', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLshort'),
        Argument(name='y1', type='GLshort'),
        Argument(name='x2', type='GLshort'),
        Argument(name='y2', type='GLshort'),
    ],
)

Function(name='glRectsv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLshort*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLshort*', wrap_params='2, v2'),
    ],
)

Function(name='glRectxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLfixed'),
        Argument(name='y1', type='GLfixed'),
        Argument(name='x2', type='GLfixed'),
        Argument(name='y2', type='GLfixed'),
    ],
)

Function(name='glRectxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLfixed*'),
        Argument(name='v2', type='const GLfixed*'),
    ],
)

Function(name='glReferencePlaneSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='equation', type='const GLdouble*'),
    ],
)

Function(name='glReleaseKeyedMutexWin32EXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='key', type='GLuint64'),
    ],
)

Function(name='glReleaseShaderCompiler', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glRenderGpuMaskNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glRenderMode', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glRenderbufferStorage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glRenderbufferStorageEXT', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glRenderbufferStorage')

Function(name='glRenderbufferStorageMultisample', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glRenderbufferStorageMultisampleANGLE', enabled=True, function_type=FuncType.PARAM, run_wrap=True, inherit_from='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleAPPLE', enabled=False, function_type=FuncType.PARAM, inherit_from='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='coverageSamples', type='GLsizei'),
        Argument(name='colorSamples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glRenderbufferStorageMultisampleEXT', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True, recorder_wrap=True, inherit_from='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleIMG', enabled=True, function_type=FuncType.PARAM, run_wrap=True, inherit_from='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageMultisampleNV', enabled=False, function_type=FuncType.NONE, inherit_from='glRenderbufferStorageMultisample')

Function(name='glRenderbufferStorageOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glRenderbufferStorage')

Function(name='glReplacementCodePointerSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void**'),
    ],
)

Function(name='glReplacementCodeubSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='GLubyte'),
    ],
)

Function(name='glReplacementCodeubvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='const GLubyte*'),
    ],
)

Function(name='glReplacementCodeuiColor3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiColor3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='c', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiColor4fNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='a', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiColor4fNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='c', type='const GLfloat*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiColor4ubVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='r', type='GLubyte'),
        Argument(name='g', type='GLubyte'),
        Argument(name='b', type='GLubyte'),
        Argument(name='a', type='GLubyte'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiColor4ubVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='c', type='const GLubyte*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='GLuint'),
    ],
)

Function(name='glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='a', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='tc', type='const GLfloat*'),
        Argument(name='c', type='const GLfloat*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='tc', type='const GLfloat*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiTexCoord2fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiTexCoord2fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='tc', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuiVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glReplacementCodeuiVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

Function(name='glReplacementCodeuivSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='const GLuint*'),
    ],
)

Function(name='glReplacementCodeusSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='GLushort'),
    ],
)

Function(name='glReplacementCodeusvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='const GLushort*'),
    ],
)

Function(name='glRequestResidentProgramsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='const GLuint*', wrap_params='n, programs'),
    ],
)

Function(name='glResetHistogram', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glResetHistogramEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glResetHistogram')

Function(name='glResetMinmax', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glResetMinmaxEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glResetMinmax')

Function(name='glResizeBuffersMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glResolveDepthValuesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glResolveMultisampleFramebufferAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glResumeTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glResumeTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glResumeTransformFeedback')

Function(name='glRotated', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='angle', type='GLdouble'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glRotatef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='angle', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glRotatex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='angle', type='GLfixed'),
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

Function(name='glRotatexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glRotatex')

Function(name='glSampleCoverage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLfloat'),
        Argument(name='invert', type='GLboolean'),
    ],
)

Function(name='glSampleCoverageARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glSampleCoverage')

Function(name='glSampleCoverageOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLfixed'),
        Argument(name='invert', type='GLboolean'),
    ],
)

Function(name='glSampleCoveragex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLclampx'),
        Argument(name='invert', type='GLboolean'),
    ],
)

Function(name='glSampleCoveragexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glSampleCoveragex')

Function(name='glSampleMapATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='dst', type='GLuint'),
        Argument(name='interp', type='GLuint'),
        Argument(name='swizzle', type='GLenum'),
    ],
)

Function(name='glSampleMaskEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLclampf'),
        Argument(name='invert', type='GLboolean'),
    ],
)

Function(name='glSampleMaskIndexedNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glSampleMaskSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLclampf'),
        Argument(name='invert', type='GLboolean'),
    ],
)

Function(name='glSampleMaski', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='maskNumber', type='GLuint'),
        Argument(name='mask', type='GLbitfield'),
    ],
)

Function(name='glSamplePass', enabled=False, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glSamplePatternEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pattern', type='GLenum'),
    ],
)

Function(name='glSamplePatternSGIS', enabled=True, function_type=FuncType.PARAM, inherit_from='glSamplePatternEXT')

Function(name='glSamplerParameterIiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glSamplerParameterIivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIiv')

Function(name='glSamplerParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIiv')

Function(name='glSamplerParameterIuiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glSamplerParameterIuivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIuiv')

Function(name='glSamplerParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIuiv')

Function(name='glSamplerParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glSamplerParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glSamplerParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glSamplerParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glScaled', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glScalef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glScalex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

Function(name='glScalexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glScalex')

Function(name='glScissor', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glScissorArrayv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLint*', wrap_params='4 * count, v'),
    ],
)

Function(name='glScissorArrayvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorArrayv')

Function(name='glScissorArrayvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorArrayv')

Function(name='glScissorIndexed', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='left', type='GLint'),
        Argument(name='bottom', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glScissorIndexedNV', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexed')

Function(name='glScissorIndexedOES', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexed')

Function(name='glScissorIndexedv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glScissorIndexedvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexedv')

Function(name='glScissorIndexedvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexedv')

Function(name='glSecondaryColor3b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLbyte'),
        Argument(name='green', type='GLbyte'),
        Argument(name='blue', type='GLbyte'),
    ],
)

Function(name='glSecondaryColor3bEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3b')

Function(name='glSecondaryColor3bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3bvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3bv')

Function(name='glSecondaryColor3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLdouble'),
        Argument(name='green', type='GLdouble'),
        Argument(name='blue', type='GLdouble'),
    ],
)

Function(name='glSecondaryColor3dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3d')

Function(name='glSecondaryColor3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3dv')

Function(name='glSecondaryColor3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
    ],
)

Function(name='glSecondaryColor3fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3f')

Function(name='glSecondaryColor3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3fv')

Function(name='glSecondaryColor3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLhalfNV'),
        Argument(name='green', type='GLhalfNV'),
        Argument(name='blue', type='GLhalfNV'),
    ],
)

Function(name='glSecondaryColor3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
    ],
)

Function(name='glSecondaryColor3iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3i')

Function(name='glSecondaryColor3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3iv')

Function(name='glSecondaryColor3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLshort'),
        Argument(name='green', type='GLshort'),
        Argument(name='blue', type='GLshort'),
    ],
)

Function(name='glSecondaryColor3sEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3s')

Function(name='glSecondaryColor3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3svEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3sv')

Function(name='glSecondaryColor3ub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLubyte'),
        Argument(name='green', type='GLubyte'),
        Argument(name='blue', type='GLubyte'),
    ],
)

Function(name='glSecondaryColor3ubEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3ub')

Function(name='glSecondaryColor3ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLubyte*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3ubvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3ubv')

Function(name='glSecondaryColor3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
    ],
)

Function(name='glSecondaryColor3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3ui')

Function(name='glSecondaryColor3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLuint*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3uiv')

Function(name='glSecondaryColor3us', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLushort'),
        Argument(name='green', type='GLushort'),
        Argument(name='blue', type='GLushort'),
    ],
)

Function(name='glSecondaryColor3usEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3us')

Function(name='glSecondaryColor3usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLushort*', wrap_params='3, v'),
    ],
)

Function(name='glSecondaryColor3usvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3usv')

Function(name='glSecondaryColorFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glSecondaryColorP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='GLuint'),
    ],
)

Function(name='glSecondaryColorP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='const GLuint*', wrap_params='1, color'),
    ],
)

Function(name='glSecondaryColorPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glSecondaryColorPointerEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColorPointer')

Function(name='glSecondaryColorPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glSelectBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='buffer', type='GLuint*', wrap_params='size, buffer'),
    ],
)

Function(name='glSelectPerfMonitorCountersAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
        Argument(name='enable', type='GLboolean'),
        Argument(name='group', type='GLuint'),
        Argument(name='numCounters', type='GLint'),
        Argument(name='counterList', type='GLuint*', wrap_params='numCounters, counterList'),
    ],
)

Function(name='glSemaphoreParameterui64vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint64*'),
    ],
)

Function(name='glSeparableFilter2D', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='row', type='const void*'),
        Argument(name='column', type='const void*'),
    ],
)

Function(name='glSeparableFilter2DEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glSeparableFilter2D')

Function(name='glSetFenceAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

Function(name='glSetFenceNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
        Argument(name='condition', type='GLenum'),
    ],
)

Function(name='glSetFragmentShaderConstantATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='dst', type='GLuint'),
        Argument(name='value', type='const GLfloat*', wrap_params='4, value'),
    ],
)

Function(name='glSetInvariantEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='addr', type='const void*'),
    ],
)

Function(name='glSetLocalConstantEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='addr', type='const void*'),
    ],
)

Function(name='glSetMultisamplefvAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='val', type='const GLfloat*', wrap_params='1, val'),
    ],
)

Function(name='glShadeModel', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glShaderBinary', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='shaders', type='const GLuint*', wrap_params='count, shaders'),
        Argument(name='binaryformat', type='GLenum'),
        Argument(name='binary', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, binary, length'),
        Argument(name='length', type='GLsizei'),
    ],
)

Function(name='glShaderOp1EXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='res', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
    ],
)

Function(name='glShaderOp2EXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='res', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
    ],
)

Function(name='glShaderOp3EXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='res', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
        Argument(name='arg3', type='GLuint'),
    ],
)

Function(name='glShaderSource', enabled=True, function_type=FuncType.RESOURCE, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei', wrap_params='1'),
        Argument(name='string', type='const GLchar*const*', wrap_type='CShaderSource', wrap_params='shader, count, string, length, _string.SHADER_SOURCE'),
        Argument(name='length', type='const GLint*', wrap_type='CGLintptrZero', wrap_params=' '),
    ],
)

Function(name='glShaderSourceARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glShaderSource', run_wrap=True,
    args=[
        Argument(name='shaderObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei', wrap_params='1'),
        Argument(name='string', type='const GLcharARB*const*', wrap_type='CShaderSource', wrap_params='shaderObj, count, string, length, _string.SHADER_SOURCE'),
        Argument(name='length', type='const GLint*', wrap_type='CGLintptrZero', wrap_params=' '),
    ],
)

Function(name='glShaderStorageBlockBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='storageBlockIndex', type='GLuint'),
        Argument(name='storageBlockBinding', type='GLuint'),
    ],
)

Function(name='glShaderStorageBlockBinding', enabled=True, function_type=FuncType.PARAM, version=1,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='storageBlockIndex', type='GLuint',wrap_type='CGLStorageBlockIndex', wrap_params='program, storageBlockIndex'),
        Argument(name='storageBlockBinding', type='GLuint'),
    ],
)

Function(name='glSharpenTexFuncSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='points', type='const GLfloat*', wrap_params='n * 2, points'),
    ],
)

Function(name='glSignalSemaphoreEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='numBufferBarriers', type='GLuint'),
        Argument(name='buffers', type='const GLuint*'),
        Argument(name='numTextureBarriers', type='GLuint'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='dstLayouts', type='const GLenum*'),
    ],
)

Function(name='glSignalVkFenceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkFence', type='GLuint64'),
    ],
)

Function(name='glSignalVkSemaphoreNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkSemaphore', type='GLuint64'),
    ],
)

Function(name='glSpecializeShader', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='pEntryPoint', type='const GLchar*'),
        Argument(name='numSpecializationConstants', type='GLuint'),
        Argument(name='pConstantIndex', type='const GLuint*'),
        Argument(name='pConstantValue', type='const GLuint*'),
    ],
)

Function(name='glSpecializeShaderARB', enabled=False, function_type=FuncType.NONE, inherit_from='glSpecializeShader')

Function(name='glSpriteParameterfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glSpriteParameterfvSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glSpriteParameteriSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glSpriteParameterivSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glStartInstrumentsSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glStartTilingQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='width', type='GLuint'),
        Argument(name='height', type='GLuint'),
        Argument(name='preserveMask', type='GLbitfield'),
    ],
)

Function(name='glStateCaptureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='state', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glStencilClearTagEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stencilTagBits', type='GLsizei'),
        Argument(name='stencilClearTag', type='GLuint'),
    ],
)

Function(name='glStencilFillPathInstancedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='fillMode', type='GLenum'),
        Argument(name='mask', type='GLuint'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*'),
    ],
)

Function(name='glStencilFillPathNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='fillMode', type='GLenum'),
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glStencilFunc', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glStencilFuncSeparate', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glStencilFuncSeparateATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glStencilFuncSeparate')

Function(name='glStencilMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glStencilMaskSeparate', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glStencilOp', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fail', type='GLenum'),
        Argument(name='zfail', type='GLenum'),
        Argument(name='zpass', type='GLenum'),
    ],
)

Function(name='glStencilOpSeparate', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='sfail', type='GLenum'),
        Argument(name='dpfail', type='GLenum'),
        Argument(name='dppass', type='GLenum'),
    ],
)

Function(name='glStencilOpSeparateATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glStencilOpSeparate')

Function(name='glStencilOpValueAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glStencilStrokePathInstancedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='reference', type='GLint'),
        Argument(name='mask', type='GLuint'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*'),
    ],
)

Function(name='glStencilStrokePathNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='reference', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glStencilThenCoverFillPathInstancedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='fillMode', type='GLenum'),
        Argument(name='mask', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*'),
    ],
)

Function(name='glStencilThenCoverFillPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='fillMode', type='GLenum'),
        Argument(name='mask', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

Function(name='glStencilThenCoverStrokePathInstancedNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='pathNameType', type='GLenum'),
        Argument(name='paths', type='const void*'),
        Argument(name='pathBase', type='GLuint'),
        Argument(name='reference', type='GLint'),
        Argument(name='mask', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*'),
    ],
)

Function(name='glStencilThenCoverStrokePathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='reference', type='GLint'),
        Argument(name='mask', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

Function(name='glStopInstrumentsSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLint'),
    ],
)

Function(name='glStringMarkerGREMEDY', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='len', type='GLsizei'),
        Argument(name='string', type='const void*'),
    ],
)

Function(name='glSubpixelPrecisionBiasNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xbits', type='GLuint'),
        Argument(name='ybits', type='GLuint'),
    ],
)

Function(name='glSwapAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glSwizzleEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='res', type='GLuint'),
        Argument(name='in', type='GLuint'),
        Argument(name='outX', type='GLenum'),
        Argument(name='outY', type='GLenum'),
        Argument(name='outZ', type='GLenum'),
        Argument(name='outW', type='GLenum'),
    ],
)

Function(name='glSyncTextureINTEL', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

Function(name='glTagSampleBufferSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glTangent3bEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLbyte'),
        Argument(name='ty', type='GLbyte'),
        Argument(name='tz', type='GLbyte'),
    ],
)

Function(name='glTangent3bvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

Function(name='glTangent3dEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLdouble'),
        Argument(name='ty', type='GLdouble'),
        Argument(name='tz', type='GLdouble'),
    ],
)

Function(name='glTangent3dvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glTangent3fEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLfloat'),
        Argument(name='ty', type='GLfloat'),
        Argument(name='tz', type='GLfloat'),
    ],
)

Function(name='glTangent3fvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTangent3iEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLint'),
        Argument(name='ty', type='GLint'),
        Argument(name='tz', type='GLint'),
    ],
)

Function(name='glTangent3ivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glTangent3sEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLshort'),
        Argument(name='ty', type='GLshort'),
        Argument(name='tz', type='GLshort'),
    ],
)

Function(name='glTangent3svEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glTangentPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glTbufferMask3DFX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLuint'),
    ],
)

Function(name='glTessellationFactorAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
    ],
)

Function(name='glTessellationModeAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glTestFenceAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

Function(name='glTestFenceNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

Function(name='glTestObjectAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='object', type='GLenum'),
        Argument(name='name', type='GLuint'),
    ],
)

Function(name='glTexBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glTexBufferARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexBuffer')

Function(name='glTexBufferEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexBuffer')

Function(name='glTexBufferOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexBuffer')

Function(name='glTexBufferRange', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glTexBufferRangeEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glTexBufferRange')

Function(name='glTexBufferRangeOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexBufferRange')

Function(name='glTexBumpParameterfvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glTexBumpParameterivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glTexCoord1bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
    ],
)

Function(name='glTexCoord1bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glTexCoord1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
    ],
)

Function(name='glTexCoord1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

Function(name='glTexCoord1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
    ],
)

Function(name='glTexCoord1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='1, v'),
    ],
)

Function(name='glTexCoord1hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
    ],
)

Function(name='glTexCoord1hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='1, v'),
    ],
)

Function(name='glTexCoord1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
    ],
)

Function(name='glTexCoord1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='1, v'),
    ],
)

Function(name='glTexCoord1s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
    ],
)

Function(name='glTexCoord1sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='1, v'),
    ],
)

Function(name='glTexCoord1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
    ],
)

Function(name='glTexCoord1xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glTexCoord2bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
    ],
)

Function(name='glTexCoord2bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glTexCoord2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
    ],
)

Function(name='glTexCoord2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glTexCoord2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
    ],
)

Function(name='glTexCoord2fColor3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glTexCoord2fColor3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='c', type='const GLfloat*', wrap_params='3, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord2fColor4fNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='a', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glTexCoord2fColor4fNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='c', type='const GLfloat*', wrap_params='3, c'),
        Argument(name='n', type='const GLfloat*', wrap_params='3, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord2fColor4ubVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLubyte'),
        Argument(name='g', type='GLubyte'),
        Argument(name='b', type='GLubyte'),
        Argument(name='a', type='GLubyte'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glTexCoord2fColor4ubVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='c', type='const GLubyte*', wrap_params='3, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord2fNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glTexCoord2fNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='n', type='const GLfloat*', wrap_params='3, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord2fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glTexCoord2fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glTexCoord2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
    ],
)

Function(name='glTexCoord2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='2, v'),
    ],
)

Function(name='glTexCoord2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
    ],
)

Function(name='glTexCoord2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

Function(name='glTexCoord2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
    ],
)

Function(name='glTexCoord2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

Function(name='glTexCoord2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
    ],
)

Function(name='glTexCoord2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glTexCoord3bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
    ],
)

Function(name='glTexCoord3bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glTexCoord3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
    ],
)

Function(name='glTexCoord3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
    ],
)

Function(name='glTexCoord3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
    ],
)

Function(name='glTexCoord3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
    ],
)

Function(name='glTexCoord3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
    ],
)

Function(name='glTexCoord3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glTexCoord3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
    ],
)

Function(name='glTexCoord3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glTexCoord4bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
        Argument(name='q', type='GLbyte'),
    ],
)

Function(name='glTexCoord4bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glTexCoord4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
        Argument(name='q', type='GLdouble'),
    ],
)

Function(name='glTexCoord4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='q', type='GLfloat'),
    ],
)

Function(name='glTexCoord4fColor4fNormal3fVertex4fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='p', type='GLfloat'),
        Argument(name='q', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='g', type='GLfloat'),
        Argument(name='b', type='GLfloat'),
        Argument(name='a', type='GLfloat'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glTexCoord4fColor4fNormal3fVertex4fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='4, tc'),
        Argument(name='c', type='const GLfloat*', wrap_params='4, c'),
        Argument(name='n', type='const GLfloat*', wrap_params='4, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4fVertex4fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='p', type='GLfloat'),
        Argument(name='q', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glTexCoord4fVertex4fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='4, tc'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
        Argument(name='q', type='GLhalfNV'),
    ],
)

Function(name='glTexCoord4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
        Argument(name='q', type='GLint'),
    ],
)

Function(name='glTexCoord4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
        Argument(name='q', type='GLshort'),
    ],
)

Function(name='glTexCoord4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glTexCoord4xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
        Argument(name='q', type='GLfixed'),
    ],
)

Function(name='glTexCoord4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glTexCoordFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glTexCoordP1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glTexCoordP1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

Function(name='glTexCoordP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glTexCoordP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

Function(name='glTexCoordP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glTexCoordP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

Function(name='glTexCoordP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

Function(name='glTexCoordP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

Function(name='glTexCoordPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glTexCoordPointerBounds', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glTexCoordPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glTexCoordPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glTexCoordPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

Function(name='glTexEnvf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glTexEnvfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexEnvi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glTexEnviv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexEnvx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glTexEnvxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexEnvx')

Function(name='glTexEnvxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexEnvxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexEnvxv')

Function(name='glTexFilterFuncSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='filter', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='weights', type='const GLfloat*', wrap_params='n, weights'),
    ],
)

Function(name='glTexGend', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLdouble'),
    ],
)

Function(name='glTexGendv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLdouble*', wrap_type='CGLdouble::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexGenf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glTexGenfOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGenf')

Function(name='glTexGenfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexGenfvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGenfv')

Function(name='glTexGeni', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glTexGeniOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGeni')

Function(name='glTexGeniv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexGenivOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGeniv')

Function(name='glTexGenxOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glTexGenxvOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexImage1D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)', rec_condition='ConditionTexImageES(_recorder, format, type)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, pixels'),
    ],
)

Function(name='glTexImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTexImageES(_recorder, format, type)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, pixels'),
    ],
)

Function(name='glTexImage2DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTexImage2DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='coverageSamples', type='GLsizei'),
        Argument(name='colorSamples', type='GLsizei'),
        Argument(name='internalFormat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
    ],
)

Function(name='glTexImage3D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTexImageES(_recorder, format, type)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, depth, pixels'),
    ],
)

Function(name='glTexImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexImage3D', rec_condition='ConditionTexImageES(_recorder, format, type)')

Function(name='glTexImage3DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTexImage3DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='coverageSamples', type='GLsizei'),
        Argument(name='colorSamples', type='GLsizei'),
        Argument(name='internalFormat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
    ],
)

Function(name='glTexImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexImage3D')

Function(name='glTexImage4DSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='size4d', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glTexPageCommitmentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='commit', type='GLboolean'),
    ],
)

Function(name='glTexPageCommitmentEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='commit', type='GLboolean'),
    ],
)

Function(name='glTexParameterIiv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexParameterIivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterIiv')

Function(name='glTexParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexParameterIiv')

Function(name='glTexParameterIuiv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexParameterIuivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterIuiv')

Function(name='glTexParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexParameterIuiv')

Function(name='glTexParameterf', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glTexParameterfv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexParameteri', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glTexParameteriv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexParameterx', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

Function(name='glTexParameterxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterx')

Function(name='glTexParameterxv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTexParameterxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterxv')

Function(name='glTexRenderbufferNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

Function(name='glTexStorage1D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glTexStorage1DEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexStorage1D')

Function(name='glTexStorage2D', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glTexStorage2DEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexStorage2D')

Function(name='glTexStorage2DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTexStorage3D', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
    ],
)

Function(name='glTexStorage3DEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexStorage3D')

Function(name='glTexStorage3DMultisample', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTexStorage3DMultisampleOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexStorage3DMultisample')

Function(name='glTexStorageMem1DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTexStorageMem2DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTexStorageMem2DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTexStorageMem3DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTexStorageMem3DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTexStorageSparseAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='layers', type='GLsizei'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glTexSubImage1D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, pixels'),
    ],
)

Function(name='glTexSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexSubImage1D')

Function(name='glTexSubImage2D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, pixels'),
    ],
)

# FIXME: It's possible that this recorder_wrap was meant to be rec_condition; see commit d5a29b4e75 "changes for recCond in generator.py"
Function(name='glTexSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, recorder_wrap=False, inherit_from='glTexSubImage2D')

Function(name='glTexSubImage3D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, depth, pixels'),
    ],
)

# FIXME: It's possible that this recorder_wrap was meant to be rec_condition; see commit d5a29b4e75 "changes for recCond in generator.py"
Function(name='glTexSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexSubImage3D', recorder_wrap=False)

Function(name='glTexSubImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexSubImage3D')

Function(name='glTexSubImage4DSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='woffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='size4d', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*'),
    ],
)

Function(name='glTextureBarrier', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glTextureBarrierNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glTextureBarrier')

Function(name='glTextureBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glTextureBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glTextureBufferRange', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glTextureBufferRangeEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glTextureColorMaskSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLboolean'),
        Argument(name='green', type='GLboolean'),
        Argument(name='blue', type='GLboolean'),
        Argument(name='alpha', type='GLboolean'),
    ],
)

Function(name='glTextureFoveationParametersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='layer', type='GLuint'),
        Argument(name='focalPoint', type='GLuint'),
        Argument(name='focalX', type='GLfloat'),
        Argument(name='focalY', type='GLfloat'),
        Argument(name='gainX', type='GLfloat'),
        Argument(name='gainY', type='GLfloat'),
        Argument(name='foveaArea', type='GLfloat'),
    ],
)

Function(name='glTextureImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, pixels'),
    ],
)

Function(name='glTextureImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, pixels'),
    ],
)

Function(name='glTextureImage2DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='coverageSamples', type='GLsizei'),
        Argument(name='colorSamples', type='GLsizei'),
        Argument(name='internalFormat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
    ],
)

Function(name='glTextureImage2DMultisampleNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalFormat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
    ],
)

Function(name='glTextureImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='internalformat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='border', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, depth, pixels'),
    ],
)

Function(name='glTextureImage3DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='coverageSamples', type='GLsizei'),
        Argument(name='colorSamples', type='GLsizei'),
        Argument(name='internalFormat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
    ],
)

Function(name='glTextureImage3DMultisampleNV', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalFormat', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
    ],
)

Function(name='glTextureLightEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
    ],
)

Function(name='glTextureMaterialEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glTextureNormalEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

Function(name='glTexturePageCommitmentEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='commit', type='GLboolean'),
    ],
)

Function(name='glTextureParameterIiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTextureParameterIivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTextureParameterIuiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTextureParameterIuivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTextureParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glTextureParameterfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glTextureParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glTextureParameterfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTextureParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glTextureParameteriEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glTextureParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

Function(name='glTextureParameterivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

Function(name='glTextureRangeAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glTextureRenderbufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

Function(name='glTextureStorage1D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glTextureStorage1DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
    ],
)

Function(name='glTextureStorage2D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glTextureStorage2DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glTextureStorage2DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTextureStorage2DMultisampleEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTextureStorage3D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
    ],
)

Function(name='glTextureStorage3DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
    ],
)

Function(name='glTextureStorage3DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTextureStorage3DMultisampleEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

Function(name='glTextureStorageMem1DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTextureStorageMem2DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTextureStorageMem2DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTextureStorageMem3DEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTextureStorageMem3DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='fixedSampleLocations', type='GLboolean'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

Function(name='glTextureStorageSparseAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='internalFormat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='layers', type='GLsizei'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

Function(name='glTextureSubImage1D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='GetTargetOfTextureOrCrash(texture), format, type, width, pixels'),
    ],
)

Function(name='glTextureSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, pixels'),
    ],
)

Function(name='glTextureSubImage2D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='GetTargetOfTextureOrCrash(texture), format, type, width, height, pixels'),
    ],
)

Function(name='glTextureSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, pixels'),
    ],
)

Function(name='glTextureSubImage3D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='GetTargetOfTextureOrCrash(texture), format, type, width, height, depth, pixels'),
    ],
)

Function(name='glTextureSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='xoffset', type='GLint'),
        Argument(name='yoffset', type='GLint'),
        Argument(name='zoffset', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='depth', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='target, format, type, width, height, depth, pixels'),
    ],
)

Function(name='glTextureView', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='origtexture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='minlevel', type='GLuint'),
        Argument(name='numlevels', type='GLuint'),
        Argument(name='minlayer', type='GLuint'),
        Argument(name='numlayers', type='GLuint'),
    ],
)

Function(name='glTextureViewEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glTextureView')

Function(name='glTextureViewOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTextureView')

Function(name='glTrackMatrixNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='address', type='GLuint'),
        Argument(name='matrix', type='GLenum'),
        Argument(name='transform', type='GLenum'),
    ],
)

Function(name='glTransformFeedbackAttribsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='attribs', type='const GLint*'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

Function(name='glTransformFeedbackBufferBase', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glTransformFeedbackBufferRange', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

Function(name='glTransformFeedbackStreamAttribsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='attribs', type='const GLint*', wrap_params='count, attribs'),
        Argument(name='nbuffers', type='GLsizei'),
        Argument(name='bufstreams', type='const GLint*', wrap_params='nbuffers, bufstreams'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

Function(name='glTransformFeedbackVaryings', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei'),
        Argument(name='varyings', type='const GLchar*const*', wrap_type='CStringArray', wrap_params='varyings, count'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

Function(name='glTransformFeedbackVaryingsEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTransformFeedbackVaryings')

Function(name='glTransformFeedbackVaryingsNV', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='locations', type='const GLint*'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

Function(name='glTransformPathNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='srcPath', type='GLuint'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*', wrap_params='1, transformValues'),
    ],
)

Function(name='glTranslated', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glTranslatef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glTranslatex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

Function(name='glTranslatexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTranslatex')

Function(name='glUniform1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
    ],
)

Function(name='glUniform1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
    ],
)

Function(name='glUniform1fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1f')

Function(name='glUniform1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1fv')

Function(name='glUniform1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
    ],
)

Function(name='glUniform1i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
    ],
)

Function(name='glUniform1i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
    ],
)

Function(name='glUniform1i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1i')

Function(name='glUniform1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1iv')

Function(name='glUniform1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
    ],
)

Function(name='glUniform1ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
    ],
)

Function(name='glUniform1ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

Function(name='glUniform1ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1ui')

Function(name='glUniform1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count, value'),
    ],
)

Function(name='glUniform1uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1uiv')

Function(name='glUniform2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glUniform2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
    ],
)

Function(name='glUniform2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2f')

Function(name='glUniform2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count* 2, value'),
    ],
)

Function(name='glUniform2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2fv')

Function(name='glUniform2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
    ],
)

Function(name='glUniform2i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
    ],
)

Function(name='glUniform2i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
    ],
)

Function(name='glUniform2i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2i')

Function(name='glUniform2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2iv')

Function(name='glUniform2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
    ],
)

Function(name='glUniform2ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
    ],
)

Function(name='glUniform2ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
    ],
)

Function(name='glUniform2ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2ui')

Function(name='glUniform2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 2, value'),
    ],
)

Function(name='glUniform2uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2uiv')

Function(name='glUniform3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glUniform3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
    ],
)

Function(name='glUniform3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3f')

Function(name='glUniform3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3fv')

Function(name='glUniform3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
    ],
)

Function(name='glUniform3i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
    ],
)

Function(name='glUniform3i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
    ],
)

Function(name='glUniform3i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3i')

Function(name='glUniform3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3iv')

Function(name='glUniform3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
    ],
)

Function(name='glUniform3ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
    ],
)

Function(name='glUniform3ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
    ],
)

Function(name='glUniform3ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3ui')

Function(name='glUniform3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 3, value'),
    ],
)

Function(name='glUniform3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3uiv')

Function(name='glUniform4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glUniform4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
        Argument(name='v3', type='GLfloat'),
    ],
)

Function(name='glUniform4fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4f')

Function(name='glUniform4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4fv')

Function(name='glUniform4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
        Argument(name='v3', type='GLint'),
    ],
)

Function(name='glUniform4i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
        Argument(name='w', type='GLint64'),
    ],
)

Function(name='glUniform4i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
        Argument(name='w', type='GLint64EXT'),
    ],
)

Function(name='glUniform4i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4i')

Function(name='glUniform4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4iv')

Function(name='glUniform4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
        Argument(name='v3', type='GLuint'),
    ],
)

Function(name='glUniform4ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
        Argument(name='w', type='GLuint64'),
    ],
)

Function(name='glUniform4ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
        Argument(name='w', type='GLuint64EXT'),
    ],
)

Function(name='glUniform4ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4ui')

Function(name='glUniform4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniform4uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4uiv')

Function(name='glUniformBlockBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockIndex', type='GLuint', wrap_type='CGLUniformBlockIndex', wrap_params='program, uniformBlockIndex'),
        Argument(name='uniformBlockBinding', type='GLuint'),
    ],
)

Function(name='glUniformBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glUniformHandleui64ARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='value', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

Function(name='glUniformHandleui64IMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint'),
        Argument(name='value', type='GLuint64'),
    ],
)

Function(name='glUniformHandleui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='value', type='GLuint64'),
    ],
)

Function(name='glUniformHandleui64vARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_type='CGLTextureHandle::CSArray', wrap_params='count, value'),
    ],
)

Function(name='glUniformHandleui64vIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*'),
    ],
)

Function(name='glUniformHandleui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count, value'),
    ],
)

Function(name='glUniformMatrix2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniformMatrix2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

Function(name='glUniformMatrix2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniformMatrix2fv')

Function(name='glUniformMatrix2x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

Function(name='glUniformMatrix2x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

Function(name='glUniformMatrix2x3fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix2x3fv')

Function(name='glUniformMatrix2x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

Function(name='glUniformMatrix2x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

Function(name='glUniformMatrix2x4fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix2x4fv')

Function(name='glUniformMatrix3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 9, value'),
    ],
)

Function(name='glUniformMatrix3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 9, value'),
    ],
)

Function(name='glUniformMatrix3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniformMatrix3fv')

Function(name='glUniformMatrix3x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

Function(name='glUniformMatrix3x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

Function(name='glUniformMatrix3x2fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix3x2fv')

Function(name='glUniformMatrix3x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

Function(name='glUniformMatrix3x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

Function(name='glUniformMatrix3x4fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix3x4fv')

Function(name='glUniformMatrix4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 16, value'),
    ],
)

Function(name='glUniformMatrix4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 16, value'),
    ],
)

Function(name='glUniformMatrix4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniformMatrix4fv')

Function(name='glUniformMatrix4x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

Function(name='glUniformMatrix4x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

Function(name='glUniformMatrix4x2fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix4x2fv')

Function(name='glUniformMatrix4x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

Function(name='glUniformMatrix4x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

Function(name='glUniformMatrix4x3fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix4x3fv')

Function(name='glUniformSubroutinesuiv', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shadertype', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='indices', type='const GLuint*', wrap_params='count, indices'),
    ],
)

Function(name='glUniformui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='value', type='GLuint64EXT'),
    ],
)

Function(name='glUniformui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

Function(name='glUnlockArraysEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glUnmapBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True, pre_token='CgitsUnmapBuffer(target)', rec_condition='ConditionBufferES(_recorder)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

Function(name='glUnmapBufferARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUnmapBuffer')

Function(name='glUnmapBufferOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glUnmapBuffer')

Function(name='glUnmapNamedBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True, pre_token='CgitsUnmapBuffer(0, buffer)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glUnmapNamedBufferEXT', enabled=True, function_type=FuncType.PARAM, pre_token='CgitsUnmapBuffer(0, buffer)', inherit_from='glUnmapNamedBuffer')

Function(name='glUnmapObjectBufferATI', enabled=True, function_type=FuncType.PARAM, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glUnmapTexture2DINTEL', enabled=True, function_type=FuncType.PARAM, ccode_wrap=True, state_track=True, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

Function(name='glUpdateObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLuint'),
        Argument(name='size', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
        Argument(name='preserve', type='GLenum'),
    ],
)

Function(name='glUseProgram', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glUseProgramObjectARB', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

Function(name='glUseProgramStages', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='stages', type='GLbitfield'),
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glUseProgramStagesEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glUseProgramStages')

Function(name='glUseShaderProgramEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glVDPAUFiniNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

Function(name='glVDPAUGetSurfaceivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='values', type='GLint*'),
    ],
)

Function(name='glVDPAUInitNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vdpDevice', type='const void*'),
        Argument(name='getProcAddress', type='const void*'),
    ],
)

Function(name='glVDPAUIsSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
    ],
)

Function(name='glVDPAUMapSurfacesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numSurfaces', type='GLsizei'),
        Argument(name='surfaces', type='const GLvdpauSurfaceNV*'),
    ],
)

Function(name='glVDPAURegisterOutputSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLvdpauSurfaceNV'),
    args=[
        Argument(name='vdpSurface', type='const void*'),
        Argument(name='target', type='GLenum'),
        Argument(name='numTextureNames', type='GLsizei'),
        Argument(name='textureNames', type='const GLuint*'),
    ],
)

Function(name='glVDPAURegisterVideoSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLvdpauSurfaceNV'),
    args=[
        Argument(name='vdpSurface', type='const void*'),
        Argument(name='target', type='GLenum'),
        Argument(name='numTextureNames', type='GLsizei'),
        Argument(name='textureNames', type='const GLuint*'),
    ],
)

Function(name='glVDPAUSurfaceAccessNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
        Argument(name='access', type='GLenum'),
    ],
)

Function(name='glVDPAUUnmapSurfacesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numSurface', type='GLsizei'),
        Argument(name='surfaces', type='const GLvdpauSurfaceNV*'),
    ],
)

Function(name='glVDPAUUnregisterSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
    ],
)

Function(name='glValidateProgram', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

Function(name='glValidateProgramARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glValidateProgram',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

Function(name='glValidateProgramPipeline', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
    ],
)

Function(name='glValidateProgramPipelineEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glValidateProgramPipeline')

Function(name='glVariantArrayObjectATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLuint'),
    ],
)

Function(name='glVariantPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLuint'),
        Argument(name='addr', type='const void*'),
    ],
)

Function(name='glVariantbvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLbyte*'),
    ],
)

Function(name='glVariantdvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLdouble*'),
    ],
)

Function(name='glVariantfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLfloat*'),
    ],
)

Function(name='glVariantivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLint*'),
    ],
)

Function(name='glVariantsvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLshort*'),
    ],
)

Function(name='glVariantubvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLubyte*'),
    ],
)

Function(name='glVariantuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLuint*'),
    ],
)

Function(name='glVariantusvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLushort*'),
    ],
)

Function(name='glVertex2bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLbyte'),
        Argument(name='y', type='GLbyte'),
    ],
)

Function(name='glVertex2bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glVertex2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glVertex2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glVertex2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glVertex2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glVertex2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
    ],
)

Function(name='glVertex2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='2, v'),
    ],
)

Function(name='glVertex2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

Function(name='glVertex2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

Function(name='glVertex2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

Function(name='glVertex2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

Function(name='glVertex2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
    ],
)

Function(name='glVertex2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glVertex3bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLbyte'),
        Argument(name='y', type='GLbyte'),
        Argument(name='z', type='GLbyte'),
    ],
)

Function(name='glVertex3bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glVertex3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glVertex3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glVertex3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glVertex3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glVertex3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
    ],
)

Function(name='glVertex3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

Function(name='glVertex3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

Function(name='glVertex3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glVertex3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

Function(name='glVertex3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glVertex3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
    ],
)

Function(name='glVertex3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glVertex4bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLbyte'),
        Argument(name='y', type='GLbyte'),
        Argument(name='z', type='GLbyte'),
        Argument(name='w', type='GLbyte'),
    ],
)

Function(name='glVertex4bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

Function(name='glVertex4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glVertex4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glVertex4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glVertex4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glVertex4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
        Argument(name='w', type='GLhalfNV'),
    ],
)

Function(name='glVertex4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

Function(name='glVertex4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glVertex4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glVertex4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

Function(name='glVertex4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glVertex4xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

Function(name='glVertex4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

Function(name='glVertexArrayAttribBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
    ],
)

Function(name='glVertexArrayAttribFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexArrayAttribIFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexArrayAttribLFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexArrayBindVertexBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glVertexArrayBindingDivisor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

Function(name='glVertexArrayColorOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayEdgeFlagOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayElementBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

Function(name='glVertexArrayFogCoordOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayIndexOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayMultiTexCoordOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='texunit', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayNormalOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayParameteriAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glVertexArrayRangeAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glVertexArrayRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glVertexArraySecondaryColorOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayTexCoordOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayVertexAttribBindingEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
    ],
)

Function(name='glVertexArrayVertexAttribDivisorEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

Function(name='glVertexArrayVertexAttribFormatEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexArrayVertexAttribIFormatEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexArrayVertexAttribIOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayVertexAttribLFormatEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexArrayVertexAttribLOffsetEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayVertexAttribOffsetEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexArrayVertexBindingDivisorEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

Function(name='glVertexArrayVertexBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glVertexArrayVertexBuffers', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*'),
        Argument(name='offsets', type='const GLintptr*'),
        Argument(name='strides', type='const GLsizei*'),
    ],
)

Function(name='glVertexArrayVertexOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

Function(name='glVertexAttrib1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
    ],
)

Function(name='glVertexAttrib1dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1d')

Function(name='glVertexAttrib1dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1d')

Function(name='glVertexAttrib1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttrib1dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1dv')

Function(name='glVertexAttrib1dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1dv')

Function(name='glVertexAttrib1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
    ],
)

Function(name='glVertexAttrib1fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1f')

Function(name='glVertexAttrib1fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1f')

Function(name='glVertexAttrib1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttrib1fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1fv')

Function(name='glVertexAttrib1fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1fv')

Function(name='glVertexAttrib1hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
    ],
)

Function(name='glVertexAttrib1hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttrib1s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
    ],
)

Function(name='glVertexAttrib1sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1s')

Function(name='glVertexAttrib1sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1s')

Function(name='glVertexAttrib1sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttrib1svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1sv')

Function(name='glVertexAttrib1svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1sv')

Function(name='glVertexAttrib2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glVertexAttrib2dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2d')

Function(name='glVertexAttrib2dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2d')

Function(name='glVertexAttrib2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttrib2dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2dv')

Function(name='glVertexAttrib2dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2dv')

Function(name='glVertexAttrib2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glVertexAttrib2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2f')

Function(name='glVertexAttrib2fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2f')

Function(name='glVertexAttrib2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttrib2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2fv')

Function(name='glVertexAttrib2fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2fv')

Function(name='glVertexAttrib2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
    ],
)

Function(name='glVertexAttrib2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttrib2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

Function(name='glVertexAttrib2sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2s')

Function(name='glVertexAttrib2sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2s')

Function(name='glVertexAttrib2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttrib2svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2sv')

Function(name='glVertexAttrib2svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2sv')

Function(name='glVertexAttrib3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glVertexAttrib3dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3d')

Function(name='glVertexAttrib3dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3d')

Function(name='glVertexAttrib3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttrib3dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3dv')

Function(name='glVertexAttrib3dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3dv')

Function(name='glVertexAttrib3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glVertexAttrib3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3f')

Function(name='glVertexAttrib3fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3f')

Function(name='glVertexAttrib3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttrib3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3fv')

Function(name='glVertexAttrib3fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3fv')

Function(name='glVertexAttrib3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
    ],
)

Function(name='glVertexAttrib3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttrib3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

Function(name='glVertexAttrib3sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3s')

Function(name='glVertexAttrib3sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3s')

Function(name='glVertexAttrib3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttrib3svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3sv')

Function(name='glVertexAttrib3svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3sv')

Function(name='glVertexAttrib4Nbv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4NbvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nbv')

Function(name='glVertexAttrib4Niv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4NivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Niv')

Function(name='glVertexAttrib4Nsv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4NsvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nsv')

Function(name='glVertexAttrib4Nub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLubyte'),
        Argument(name='y', type='GLubyte'),
        Argument(name='z', type='GLubyte'),
        Argument(name='w', type='GLubyte'),
    ],
)

Function(name='glVertexAttrib4NubARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nub')

Function(name='glVertexAttrib4Nubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4NubvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nubv')

Function(name='glVertexAttrib4Nuiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4NuivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nuiv')

Function(name='glVertexAttrib4Nusv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4NusvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nusv')

Function(name='glVertexAttrib4bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4bvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4bv')

Function(name='glVertexAttrib4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glVertexAttrib4dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4d')

Function(name='glVertexAttrib4dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4d')

Function(name='glVertexAttrib4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4dv')

Function(name='glVertexAttrib4dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4dv')

Function(name='glVertexAttrib4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glVertexAttrib4fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4f')

Function(name='glVertexAttrib4fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4f')

Function(name='glVertexAttrib4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4fv')

Function(name='glVertexAttrib4fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4fv')

Function(name='glVertexAttrib4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
        Argument(name='w', type='GLhalfNV'),
    ],
)

Function(name='glVertexAttrib4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4iv')

Function(name='glVertexAttrib4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

Function(name='glVertexAttrib4sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4s')

Function(name='glVertexAttrib4sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4s')

Function(name='glVertexAttrib4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4sv')

Function(name='glVertexAttrib4svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4sv')

Function(name='glVertexAttrib4ubNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLubyte'),
        Argument(name='y', type='GLubyte'),
        Argument(name='z', type='GLubyte'),
        Argument(name='w', type='GLubyte'),
    ],
)

Function(name='glVertexAttrib4ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4ubvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4ubv')

Function(name='glVertexAttrib4ubvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4ubv')

Function(name='glVertexAttrib4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4uivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4uiv')

Function(name='glVertexAttrib4usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttrib4usvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4usv')

Function(name='glVertexAttribArrayObjectATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLuint'),
    ],
)

Function(name='glVertexAttribBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
    ],
)

Function(name='glVertexAttribDivisor', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

Function(name='glVertexAttribDivisorANGLE', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribDivisor', run_wrap=False)

Function(name='glVertexAttribDivisorARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribDivisor', run_wrap=False)

Function(name='glVertexAttribDivisorEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glVertexAttribDivisor', run_wrap=False)

Function(name='glVertexAttribDivisorNV', enabled=False, function_type=FuncType.NONE, inherit_from='glVertexAttribDivisor', run_wrap=False)

Function(name='glVertexAttribFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexAttribFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glVertexAttribI1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
    ],
)

Function(name='glVertexAttribI1iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1i')

Function(name='glVertexAttribI1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttribI1ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1iv')

Function(name='glVertexAttribI1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
    ],
)

Function(name='glVertexAttribI1uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1ui')

Function(name='glVertexAttribI1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttribI1uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1uiv')

Function(name='glVertexAttribI2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

Function(name='glVertexAttribI2iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2i')

Function(name='glVertexAttribI2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttribI2ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2iv')

Function(name='glVertexAttribI2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
    ],
)

Function(name='glVertexAttribI2uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2ui')

Function(name='glVertexAttribI2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttribI2uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2uiv')

Function(name='glVertexAttribI3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

Function(name='glVertexAttribI3iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3i')

Function(name='glVertexAttribI3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttribI3ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3iv')

Function(name='glVertexAttribI3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
    ],
)

Function(name='glVertexAttribI3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3ui')

Function(name='glVertexAttribI3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttribI3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3uiv')

Function(name='glVertexAttribI4bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribI4bvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4bv')

Function(name='glVertexAttribI4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glVertexAttribI4iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4i')

Function(name='glVertexAttribI4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribI4ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4iv')

Function(name='glVertexAttribI4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribI4svEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4sv')

Function(name='glVertexAttribI4ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribI4ubvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4ubv')

Function(name='glVertexAttribI4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
        Argument(name='w', type='GLuint'),
    ],
)

Function(name='glVertexAttribI4uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4ui')

Function(name='glVertexAttribI4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribI4uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4uiv')

Function(name='glVertexAttribI4usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribI4usvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4usv')

Function(name='glVertexAttribIFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexAttribIFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glVertexAttribIPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glVertexAttribIPointerEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribIPointer')

Function(name='glVertexAttribL1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
    ],
)

Function(name='glVertexAttribL1dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL1d')

Function(name='glVertexAttribL1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttribL1dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL1dv')

Function(name='glVertexAttribL1i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
    ],
)

Function(name='glVertexAttribL1i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttribL1ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

Function(name='glVertexAttribL1ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

Function(name='glVertexAttribL1ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*'),
    ],
)

Function(name='glVertexAttribL1ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='1, v'),
    ],
)

Function(name='glVertexAttribL2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glVertexAttribL2dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL2d')

Function(name='glVertexAttribL2dv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttribL2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL2dv')

Function(name='glVertexAttribL2i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
    ],
)

Function(name='glVertexAttribL2i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttribL2ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
    ],
)

Function(name='glVertexAttribL2ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='2, v'),
    ],
)

Function(name='glVertexAttribL3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glVertexAttribL3dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL3d')

Function(name='glVertexAttribL3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttribL3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL3dv')

Function(name='glVertexAttribL3i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
    ],
)

Function(name='glVertexAttribL3i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttribL3ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
    ],
)

Function(name='glVertexAttribL3ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='3, v'),
    ],
)

Function(name='glVertexAttribL4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glVertexAttribL4dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL4d')

Function(name='glVertexAttribL4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribL4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL4dv')

Function(name='glVertexAttribL4i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
        Argument(name='w', type='GLint64EXT'),
    ],
)

Function(name='glVertexAttribL4i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribL4ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
        Argument(name='w', type='GLuint64EXT'),
    ],
)

Function(name='glVertexAttribL4ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='4, v'),
    ],
)

Function(name='glVertexAttribLFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

Function(name='glVertexAttribLFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glVertexAttribLPointer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glVertexAttribLPointerEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glVertexAttribLPointer')

Function(name='glVertexAttribP1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexAttribP1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

Function(name='glVertexAttribP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexAttribP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='2, value'),
    ],
)

Function(name='glVertexAttribP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexAttribP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='3, value'),
    ],
)

Function(name='glVertexAttribP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexAttribP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='4, value'),
    ],
)

Function(name='glVertexAttribParameteriAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glVertexAttribPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glVertexAttribPointerARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribPointer')

Function(name='glVertexAttribPointerNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='fsize', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glVertexAttribs1dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='count, v'),
    ],
)

Function(name='glVertexAttribs1fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='count, v'),
    ],
)

Function(name='glVertexAttribs1hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='n, v'),
    ],
)

Function(name='glVertexAttribs1svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='count, v'),
    ],
)

Function(name='glVertexAttribs2dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='2 * count, v'),
    ],
)

Function(name='glVertexAttribs2fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='2 * count, v'),
    ],
)

Function(name='glVertexAttribs2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='2 * n, v'),
    ],
)

Function(name='glVertexAttribs2svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='2 * count, v'),
    ],
)

Function(name='glVertexAttribs3dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='3 * count, v'),
    ],
)

Function(name='glVertexAttribs3fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='3 * count, v'),
    ],
)

Function(name='glVertexAttribs3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='3 * n, v'),
    ],
)

Function(name='glVertexAttribs3svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='3 * count, v'),
    ],
)

Function(name='glVertexAttribs4dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='4 * count, v'),
    ],
)

Function(name='glVertexAttribs4fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='4 * count, v'),
    ],
)

Function(name='glVertexAttribs4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='4 * n, v'),
    ],
)

Function(name='glVertexAttribs4svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='4 * count, v'),
    ],
)

Function(name='glVertexAttribs4ubvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLubyte*', wrap_params='4 * count, v'),
    ],
)

Function(name='glVertexBindingDivisor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

Function(name='glVertexBlendARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLint'),
    ],
)

Function(name='glVertexBlendEnvfATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

Function(name='glVertexBlendEnviATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

Function(name='glVertexFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

Function(name='glVertexP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

Function(name='glVertexP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

Function(name='glVertexP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

Function(name='glVertexP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

Function(name='glVertexPointSizefAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLfloat'),
    ],
)

Function(name='glVertexPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glVertexPointerBounds', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glVertexPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

Function(name='glVertexPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

Function(name='glVertexPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

Function(name='glVertexStream1dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
    ],
)

Function(name='glVertexStream1dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='1, coords'),
    ],
)

Function(name='glVertexStream1fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
    ],
)

Function(name='glVertexStream1fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='1, coords'),
    ],
)

Function(name='glVertexStream1iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
    ],
)

Function(name='glVertexStream1ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='1, coords'),
    ],
)

Function(name='glVertexStream1sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
    ],
)

Function(name='glVertexStream1svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='1, coords'),
    ],
)

Function(name='glVertexStream2dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glVertexStream2dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='2, coords'),
    ],
)

Function(name='glVertexStream2fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glVertexStream2fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='2, coords'),
    ],
)

Function(name='glVertexStream2iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

Function(name='glVertexStream2ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='2, coords'),
    ],
)

Function(name='glVertexStream2sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

Function(name='glVertexStream2svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='2, coords'),
    ],
)

Function(name='glVertexStream3dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glVertexStream3dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='3, coords'),
    ],
)

Function(name='glVertexStream3fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glVertexStream3fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='3, coords'),
    ],
)

Function(name='glVertexStream3iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

Function(name='glVertexStream3ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='3, coords'),
    ],
)

Function(name='glVertexStream3sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

Function(name='glVertexStream3svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='3, coords'),
    ],
)

Function(name='glVertexStream4dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glVertexStream4dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='4, coords'),
    ],
)

Function(name='glVertexStream4fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glVertexStream4fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='4, coords'),
    ],
)

Function(name='glVertexStream4iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glVertexStream4ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='4, coords'),
    ],
)

Function(name='glVertexStream4sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

Function(name='glVertexStream4svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='4, coords'),
    ],
)

Function(name='glVertexWeightPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glVertexWeightfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='GLfloat'),
    ],
)

Function(name='glVertexWeightfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='const GLfloat*', wrap_params='1, weight'),
    ],
)

Function(name='glVertexWeighthNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='GLhalfNV'),
    ],
)

Function(name='glVertexWeighthvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='const GLhalfNV*', wrap_params='1, weight'),
    ],
)

Function(name='glVideoCaptureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='sequence_num', type='GLuint*'),
        Argument(name='capture_time', type='GLuint64EXT*'),
    ],
)

Function(name='glVideoCaptureStreamParameterdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLdouble*'),
    ],
)

Function(name='glVideoCaptureStreamParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

Function(name='glVideoCaptureStreamParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

Function(name='glViewport', enabled=True, function_type=FuncType.PARAM, run_wrap=True, ccode_wrap=True, pre_token='CgitsViewportSettings(x, y, width, height)', recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

Function(name='glViewportArrayv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='4 * count, v'),
    ],
)

Function(name='glViewportArrayvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportArrayv')

Function(name='glViewportArrayvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportArrayv')

Function(name='glViewportIndexedf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
        Argument(name='h', type='GLfloat'),
    ],
)

Function(name='glViewportIndexedfNV', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedf')

Function(name='glViewportIndexedfOES', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedf')

Function(name='glViewportIndexedfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glViewportIndexedfvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedfv')

Function(name='glViewportIndexedfvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedfv')

Function(name='glViewportPositionWScaleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='xcoeff', type='GLfloat'),
        Argument(name='ycoeff', type='GLfloat'),
    ],
)

Function(name='glViewportSwizzleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='swizzlex', type='GLenum'),
        Argument(name='swizzley', type='GLenum'),
        Argument(name='swizzlez', type='GLenum'),
        Argument(name='swizzlew', type='GLenum'),
    ],
)

Function(name='glWaitSemaphoreEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='numBufferBarriers', type='GLuint'),
        Argument(name='buffers', type='const GLuint*'),
        Argument(name='numTextureBarriers', type='GLuint'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='srcLayouts', type='const GLenum*'),
    ],
)

Function(name='glWaitSync', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync'),
        Argument(name='flags', type='GLbitfield'),
        Argument(name='timeout', type='GLuint64'),
    ],
)

Function(name='glWaitSyncAPPLE', enabled=False, function_type=FuncType.PARAM, inherit_from='glWaitSync')

Function(name='glWaitVkSemaphoreNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkSemaphore', type='GLuint64'),
    ],
)

Function(name='glWeightPathsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='paths', type='const GLuint*', wrap_params='1, paths'),
        Argument(name='weights', type='const GLfloat*', wrap_params='1, weights'),
    ],
)

Function(name='glWeightPointerARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glWeightPointerOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

Function(name='glWeightPointerOESBounds', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
        Argument(name='count', type='GLsizei'),
    ],
)

Function(name='glWeightbvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLbyte*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightdvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLdouble*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightfvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLfloat*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightivARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLint*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightsvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLshort*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightubvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLubyte*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightuivARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLuint*', wrap_params='size, weights'),
    ],
)

Function(name='glWeightusvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLushort*', wrap_params='size, weights'),
    ],
)

Function(name='glWindowPos2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

Function(name='glWindowPos2dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2d')

Function(name='glWindowPos2dMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2d')

Function(name='glWindowPos2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

Function(name='glWindowPos2dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2dv')

Function(name='glWindowPos2dvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2dv')

Function(name='glWindowPos2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

Function(name='glWindowPos2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2f')

Function(name='glWindowPos2fMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2f')

Function(name='glWindowPos2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

Function(name='glWindowPos2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2fv')

Function(name='glWindowPos2fvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2fv')

Function(name='glWindowPos2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

Function(name='glWindowPos2iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2i')

Function(name='glWindowPos2iMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2i')

Function(name='glWindowPos2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

Function(name='glWindowPos2ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2iv')

Function(name='glWindowPos2ivMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2iv')

Function(name='glWindowPos2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

Function(name='glWindowPos2sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2s')

Function(name='glWindowPos2sMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2s')

Function(name='glWindowPos2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

Function(name='glWindowPos2svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2sv')

Function(name='glWindowPos2svMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2sv')

Function(name='glWindowPos3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

Function(name='glWindowPos3dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3d')

Function(name='glWindowPos3dMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3d')

Function(name='glWindowPos3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

Function(name='glWindowPos3dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3dv')

Function(name='glWindowPos3dvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3dv')

Function(name='glWindowPos3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

Function(name='glWindowPos3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3f')

Function(name='glWindowPos3fMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3f')

Function(name='glWindowPos3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

Function(name='glWindowPos3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3fv')

Function(name='glWindowPos3fvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3fv')

Function(name='glWindowPos3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

Function(name='glWindowPos3iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3i')

Function(name='glWindowPos3iMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3i')

Function(name='glWindowPos3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

Function(name='glWindowPos3ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3iv')

Function(name='glWindowPos3ivMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3iv')

Function(name='glWindowPos3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

Function(name='glWindowPos3sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3s')

Function(name='glWindowPos3sMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3s')

Function(name='glWindowPos3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

Function(name='glWindowPos3svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3sv')

Function(name='glWindowPos3svMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3sv')

Function(name='glWindowPos4dMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

Function(name='glWindowPos4dvMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

Function(name='glWindowPos4fMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

Function(name='glWindowPos4fvMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

Function(name='glWindowPos4iMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

Function(name='glWindowPos4ivMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

Function(name='glWindowPos4sMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

Function(name='glWindowPos4svMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

Function(name='glWindowRectanglesEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='box', type='const GLint*'),
    ],
)

Function(name='glWriteMaskEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='res', type='GLuint'),
        Argument(name='in', type='GLuint'),
        Argument(name='outX', type='GLenum'),
        Argument(name='outY', type='GLenum'),
        Argument(name='outZ', type='GLenum'),
        Argument(name='outW', type='GLenum'),
    ],
)
